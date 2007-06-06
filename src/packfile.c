/*
Copyright (C) 2001-2006, The Perl Foundation.
This program is free software. It is subject to the same license as
Parrot itself.
$Id$

=head1 NAME

src/packfile.c - Parrot PackFile API

=head1 DESCRIPTION

=head2 PackFile Manipulation Functions

This file contains all the functions required for the processing of the
structure of a PackFile. It is not intended to understand the byte code
stream itself, but merely to dissect and reconstruct data from the
various segments. See F<docs/parrotbyte.pod> for information about the
structure of the frozen bytecode.

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "parrot/embed.h"
#include "parrot/packfile.h"

#include <assert.h>

#define TRACE_PACKFILE 0
#define TRACE_PACKFILE_PMC 0

/*
** Static functions
*/
static void segment_init(Interp*, PackFile_Segment *self, PackFile *pf,
                          const char *name);

static void default_destroy(Interp*, PackFile_Segment *self);
static size_t default_packed_size(Interp*, const PackFile_Segment *self);
static opcode_t * default_pack(Interp*, const PackFile_Segment *self,
                                opcode_t *dest);
static opcode_t * default_unpack(Interp *, PackFile_Segment *self,
                                  opcode_t *dest);
static void default_dump(Interp *, PackFile_Segment *self);

static PackFile_Segment *directory_new(Interp*, PackFile *, const char *, int);
static void directory_destroy(Interp*, PackFile_Segment *self);
static size_t directory_packed_size(Interp*, PackFile_Segment *self);
static opcode_t * directory_pack(Interp*, PackFile_Segment *, opcode_t *dest);
static opcode_t * directory_unpack(Interp *, PackFile_Segment *,
                                    opcode_t *cursor);
static void directory_dump(Interp *, PackFile_Segment *);

static PackFile_Segment *fixup_new(Interp*, PackFile *, const char *, int);
static size_t fixup_packed_size(Interp*, PackFile_Segment *self);
static opcode_t * fixup_pack(Interp*, PackFile_Segment *self, opcode_t *dest);
static opcode_t * fixup_unpack(Interp *, PackFile_Segment *, opcode_t *cursor);
static void fixup_destroy(Interp*, PackFile_Segment *self);

static PackFile_Segment *const_new(Interp*, PackFile *, const char *, int);
static void const_destroy(Interp*, PackFile_Segment *self);

static PackFile_Segment *byte_code_new(Interp*, PackFile *pf,
        const char *, int);
static void byte_code_destroy(Interp*, PackFile_Segment *self);
static INTVAL pf_register_standard_funcs(Interp*, PackFile *pf);

static PackFile_Segment * pf_debug_new(Interp*, PackFile *, const char *, int);
static size_t pf_debug_packed_size(Interp*, PackFile_Segment *self);
static opcode_t * pf_debug_pack(Interp*, PackFile_Segment *self, opcode_t *);
static void pf_debug_dump(Interp *, PackFile_Segment *);
static opcode_t * pf_debug_unpack(Interp *, PackFile_Segment *self, opcode_t *);
static void pf_debug_destroy(Interp*, PackFile_Segment *self);

static PackFile_Constant **find_constants(Interp *, PackFile_ConstTable *);

#define ROUND_16(val) (((val) & 0xf) ? 16 - ((val) & 0xf) : 0)
#define ALIGN_16(st, cursor) \
    (cursor) += ROUND_16((char *)(cursor) - (char *)(st))/sizeof (opcode_t)

/*

=item C<void
PackFile_destroy(PackFile *pf)>

Delete a C<PackFile>.

=cut

*/

void
PackFile_destroy(Interp *interp, PackFile *pf)
{
    if (!pf) {
        PIO_eprintf(NULL, "PackFile_destroy: pf == NULL!\n");
        return;
    }
#ifdef PARROT_HAS_HEADER_SYSMMAN
    if (pf->is_mmap_ped)
        munmap((void*)pf->src, pf->size);
#endif
    mem_sys_free(pf->header);
    pf->header = NULL;
    mem_sys_free(pf->dirp);
    pf->dirp = NULL;
    PackFile_Segment_destroy(interp, &pf->directory.base);
    return;
}


/*

=item C<static void
make_code_pointers(PackFile_Segment *seg)>

Make compat/shorthand pointers.

The first segments read are the default segments.

=cut

*/

static void
make_code_pointers(PackFile_Segment *seg)
{
    PackFile * const pf = seg->pf;

    switch (seg->type) {
        case PF_BYTEC_SEG:
            if (!pf->cur_cs) {
                pf->cur_cs = (PackFile_ByteCode *)seg;
            }
            break;
        case PF_FIXUP_SEG:
            if (!pf->cur_cs->fixups) {
                pf->cur_cs->fixups = (PackFile_FixupTable *)seg;
                pf->cur_cs->fixups->code = pf->cur_cs;
            }
            break;
        case PF_CONST_SEG:
            if (!pf->cur_cs->const_table) {
                pf->cur_cs->const_table = (PackFile_ConstTable *)seg;
                pf->cur_cs->const_table->code = pf->cur_cs;
            }
        case PF_UNKNOWN_SEG:
            if (memcmp(seg->name, "PIC_idx", 7) == 0)
                pf->cur_cs->pic_index = seg;
            break;
        case PF_DEBUG_SEG:
            pf->cur_cs->debugs = (PackFile_Debug *)seg;
            pf->cur_cs->debugs->code = pf->cur_cs;
            break;
        default:
            break;
    }
}


/*

=item C<static int
sub_pragma(Parrot_Interp interp,
        int action, PMC *sub_pmc)>

Handle :load, :main ... pragmas for B<sub_pmc>

=cut

*/

static int
sub_pragma(Parrot_Interp interp, int action, PMC *sub_pmc)
{
    int pragmas = PObj_get_FLAGS(sub_pmc) & SUB_FLAG_PF_MASK;
    int todo = 0;

    pragmas &= ~SUB_FLAG_IS_OUTER;
    if (!pragmas && !Sub_comp_INIT_TEST(sub_pmc))
        return 0;
    switch (action) {
        case PBC_PBC:
        case PBC_MAIN:
            if (interp->resume_flag & RESUME_INITIAL) {
                /*
                 * denote MAIN entry in first loaded PASM
                 */
                todo = 1;
            }
            /* :init functions need to be called at MAIN time, so return 1 */
            if (Sub_comp_INIT_TEST(sub_pmc)) /* symreg.h:P_INIT */
                todo = 1;
            break;
        case PBC_LOADED:
            if (pragmas & SUB_FLAG_PF_LOAD) /* symreg.h:P_LOAD */
                todo = 1;
            break;
    }
    if (pragmas & (SUB_FLAG_PF_IMMEDIATE | SUB_FLAG_PF_POSTCOMP))
        todo = 1;
    return todo;
}

/*

=item C<static PMC* run_sub(Parrot_Interp interp, PMC *sub_pmc)>

Run the B<sub_pmc> due its B<:load>, B<:immediate>, ... pragma

=cut

*/

static PMC*
run_sub(Parrot_Interp interp, PMC *sub_pmc)
{
    const INTVAL old = interp->run_core;
    PMC *retval;

    /*
     * turn off JIT and prederef - both would act on the whole
     * PackFile which isn't worth the effort - probably
     */
    if (interp->run_core != PARROT_CGOTO_CORE  &&
        interp->run_core != PARROT_SLOW_CORE  &&
        interp->run_core != PARROT_FAST_CORE)
            interp->run_core = PARROT_FAST_CORE;

    CONTEXT(interp->ctx)->constants = interp->code->const_table->constants;

    retval           = (PMC *)Parrot_runops_fromc_args(interp, sub_pmc, "P");
    interp->run_core = old;

    return retval;
}

/*

=item <static PMC*
do_1_sub_pragma(Parrot_Interp interp, PackFile *self, int action)>

Run autoloaded or immediate bytecode, mark MAIN subroutine entry

=cut

*/

static PMC*
do_1_sub_pragma(Parrot_Interp interp, PMC *sub_pmc /*NN*/, int action)
{

    size_t start_offs;
    Parrot_sub const * sub = PMC_sub(sub_pmc);
    PMC *result;

    switch (action) {
        case PBC_IMMEDIATE:
            /*
             * run IMMEDIATE sub
             */
            if (PObj_get_FLAGS(sub_pmc) & SUB_FLAG_PF_IMMEDIATE) {
                void *lo_var_ptr;

                PObj_get_FLAGS(sub_pmc) &= ~SUB_FLAG_PF_IMMEDIATE;
                lo_var_ptr = interp->lo_var_ptr;
                result = run_sub(interp, sub_pmc);
                /*
                 * reset initial flag so MAIN detection works
                 * and reset lo_var_ptr to prev
                 */
                interp->resume_flag = RESUME_INITIAL;
                interp->lo_var_ptr = lo_var_ptr;
                return result;
            }
            break;
        case PBC_POSTCOMP:
            /*
             * run POSTCOMP sub
             */
            if (PObj_get_FLAGS(sub_pmc) & SUB_FLAG_PF_POSTCOMP) {
                PObj_get_FLAGS(sub_pmc) &= ~SUB_FLAG_PF_POSTCOMP;
                run_sub(interp, sub_pmc);
                /*
                 * reset initial flag so MAIN detection works
                 */
                interp->resume_flag = RESUME_INITIAL;
                return NULL;
            }
            break;

        case PBC_LOADED:
            if (PObj_get_FLAGS(sub_pmc) & SUB_FLAG_PF_LOAD) {
                PObj_get_FLAGS(sub_pmc) &= ~SUB_FLAG_PF_LOAD;
                Sub_comp_INIT_CLEAR(sub_pmc); /* if loaded no need for init */
                run_sub(interp, sub_pmc);
            }
            break;
        default:
            if (PObj_get_FLAGS(sub_pmc) & SUB_FLAG_PF_MAIN) {
                if ((interp->resume_flag & RESUME_INITIAL) &&
                        interp->resume_offset == 0) {
                    const ptrdiff_t code = (ptrdiff_t) sub->seg->base.data;

                    start_offs =
                        ((ptrdiff_t) VTABLE_get_pointer(interp, sub_pmc)
                         - code) / sizeof (opcode_t*);
                    interp->resume_offset = start_offs;
                    PObj_get_FLAGS(sub_pmc) &= ~SUB_FLAG_PF_MAIN;
                    CONTEXT(interp->ctx)->current_sub = sub_pmc;
                }
                else {
                    /* XXX which warn_class */
                    Parrot_warn(interp, PARROT_WARNINGS_ALL_FLAG,
                                ":main sub not allowed\n");
                }
            }

            /* run :init tagged functions */
            if (action == PBC_MAIN && (Sub_comp_INIT_TEST(sub_pmc))) {
                Sub_comp_INIT_CLEAR(sub_pmc); /* if loaded no need for init */
                PObj_get_FLAGS(sub_pmc) &= ~SUB_FLAG_PF_LOAD; /* if inited no need for load */
                run_sub(interp, sub_pmc);
                interp->resume_flag = RESUME_INITIAL;
            }
            break;
    }
    return NULL;
}

/*
 * while the PMCs should be constant, there possible contents like
 * a property isn't constructed const so we have to mark them
 */
static void
mark_1_seg(Parrot_Interp interp, PackFile_ConstTable *ct)
{
    opcode_t i;
    PackFile_Constant **constants;

    constants = find_constants(interp, ct);
    for (i = 0; i < ct->const_count; i++) {
        switch (constants[i]->type) {
            case PFC_PMC:
                {
                PMC * const pmc = constants[i]->u.key;
                if (pmc)
                    pobject_lives(interp, (PObj *)pmc);
                }
        }
    }
}

static INTVAL
find_const_iter(Interp *interp, PackFile_Segment *seg, void *user_data)
{
    if (seg->type == PF_DIR_SEG) {
        PackFile_map_segments(interp, (PackFile_Directory *)seg,
                find_const_iter, user_data);
    }
    else if (seg->type == PF_CONST_SEG) {
        mark_1_seg(interp, (PackFile_ConstTable *)seg);
    }
    return 0;
}

void
mark_const_subs(Parrot_Interp interp)
{
    PackFile_Directory *dir;

    PackFile * const self = interp->initial_pf;
    if (!self)
        return;
    /*
     * locate top level dir
     */
    dir = &self->directory;
    /*
     * iterate over all dir/segs
     */
    PackFile_map_segments(interp, dir, find_const_iter, NULL);
}

/*

=item C<static void
do_sub_pragmas(Interp *interp, PackFile_Bytecode *self, int action,
    PMC *eval_pmc)>

B<action> is one of
B<PBC_PBC>, B<PBC_LOADED>, B<PBC_INIT>, or B<PBC_MAIN>. Also store the C<eval_pmc>
in the sub structure, so that the eval PMC is kept alive be living subs.

=cut

*/

void
do_sub_pragmas(Interp *interp, PackFile_ByteCode *self,
        int action, PMC *eval_pmc)
{
    opcode_t i;
    PMC *sub_pmc, *result;
    PackFile_FixupTable *ft = self->fixups;
    PackFile_ConstTable *ct = self->const_table;

#if TRACE_PACKFILE
    PIO_eprintf(NULL, "PackFile: do_sub_pragmas (action=%d)\n", action);
#endif

    for (i = 0; i < ft->fixup_count; i++) {
        switch (ft->fixups[i]->type) {
            case enum_fixup_sub:
                {
                /*
                 * offset is an index into the const_table holding
                 * the Sub PMC
                 */
                const opcode_t ci = ft->fixups[i]->offset;
                if (ci < 0 || ci >= ct->const_count)
                    internal_exception(1,
                            "Illegal fixup offset (%d) in enum_fixup_sub");
                sub_pmc = ct->constants[ci]->u.key;
                PMC_sub(sub_pmc)->eval_pmc = eval_pmc;
                if (((PObj_get_FLAGS(sub_pmc) & SUB_FLAG_PF_MASK)
                        || (Sub_comp_get_FLAGS(sub_pmc) & SUB_COMP_FLAG_MASK))
                        && sub_pragma(interp, action, sub_pmc)) {
                    result = do_1_sub_pragma(interp,
                            sub_pmc, action);
                    /*
                     * replace the Sub PMC with the result of the
                     * computation
                     */
                    if (action == PBC_IMMEDIATE &&
                            !PMC_IS_NULL(result)) {
                        ft->fixups[i]->type = enum_fixup_none;
                        ct->constants[ci]->u.key = result;
                    }
                }
                }
                break;
            case enum_fixup_label:
                /* fill in current bytecode seg */
                ft->fixups[i]->seg = self;
                break;
        }
    }
}

/*

=item C<opcode_t
PackFile_unpack(Interp *interp, PackFile *self, opcode_t *packed,
    size_t packed_size)>

Unpack a C<PackFile> from a block of memory. The format is:

  byte     wordsize
  byte     byteorder
  byte     major
  byte     minor
  byte     intvalsize
  byte     floattype
  byte     pad[10] = fingerprint

  opcode_t magic
  opcode_t language type

  opcode_t dir_format
  opcode_t padding

  directory segment
    * segment
    ...

All segments have this common header:

  - op_count    ... total segment size incl. this count
  - itype       ... internal type of data
  - id          ... id of data e.g. byte code nr.
  - size        ... size of data oparray
  - data[size]  ... data array e.g. bytecode
  segment specific data follows here ...

Checks to see if the magic matches the Parrot magic number for
Parrot C<PackFiles>.

Returns size of unpacked if everything is OK, else zero (0).

=cut

*/

opcode_t
PackFile_unpack(Interp *interp, PackFile *self, opcode_t *packed,
    size_t packed_size)
{
    PackFile_Header *header = self->header;
    opcode_t *cursor;

    if (!self) {
        PIO_eprintf(NULL, "PackFile_unpack: self == NULL!\n");
        return 0;
    }
    self->src = packed;
    self->size = packed_size;

    /*
     * Map the header on top of the buffer later when we are sure
     * we have alignment done right.
     */
    cursor = packed + PACKFILE_HEADER_BYTES/sizeof (opcode_t);
    memcpy(header, packed, PACKFILE_HEADER_BYTES);

    if (header->wordsize != 4 && header->wordsize != 8) {
        PIO_eprintf(NULL, "PackFile_unpack: Invalid wordsize %d\n",
                    header->wordsize);
        return 0;
    }
    if (header->floattype != 0 && header->floattype != 1) {
        PIO_eprintf(NULL, "PackFile_unpack: Invalid floattype %d\n",
                    header->floattype);
        return 0;
    }

    PackFile_assign_transforms(self);

#if TRACE_PACKFILE
    PIO_eprintf(NULL, "PackFile_unpack: Wordsize %d.\n", header->wordsize);
    PIO_eprintf(NULL, "PackFile_unpack: Floattype %d (%s).\n",
                header->floattype,
                header->floattype ?
                  "x86 little endian 12 byte long double" :
                  "IEEE-754 8 byte double");
    PIO_eprintf(NULL, "PackFile_unpack: Byteorder %d (%sendian).\n",
                header->byteorder, header->byteorder ? "big " : "little-");
#endif

    if (header->major != PARROT_MAJOR_VERSION ||
            header->minor != PARROT_MINOR_VERSION) {
        PIO_eprintf(NULL, "PackFile_unpack: Bytecode not valid for this "
                "interpreter: version mismatch\n");
        return 0;
    }

    /* check the fingerprint */
    if (!PackFile_check_fingerprint(header->pad)) {
        PIO_eprintf(NULL, "PackFile_unpack: Bytecode not valid for this "
                    "interpreter: fingerprint mismatch\n");
        return 0;
    }
    /*
     * Unpack and verify the magic which is stored byteorder of the file:
     */
    header->magic = PF_fetch_opcode(self, &cursor);

    /*
     * The magic and opcodetype fields are in native byteorder.
     */
    if (header->magic != PARROT_MAGIC) {
        PIO_eprintf(NULL, "PackFile_unpack: Not a Parrot PackFile!\n");
        PIO_eprintf(NULL, "Magic number was 0x%08x not 0x%08x\n",
                    header->magic, PARROT_MAGIC);
        return 0;
    }

#if TRACE_PACKFILE
    PIO_eprintf(NULL, "PackFile_unpack: Magic 0x%08x.\n",
                header->magic);
#endif

    header->opcodetype = PF_fetch_opcode(self, &cursor);

#if TRACE_PACKFILE
    PIO_eprintf(NULL, "PackFile_unpack: Opcodetype 0x%x.\n",
                header->opcodetype);
#endif

    /*
     * Unpack the dir_format
     */

#if TRACE_PACKFILE
    PIO_eprintf(NULL, "PackFile_unpack: Directory, offset %d.\n",
                (INTVAL)cursor - (INTVAL)packed);
#endif
    header->dir_format = PF_fetch_opcode(self, &cursor);

    /* dir_format 1 use directory */
    if (header->dir_format != PF_DIR_FORMAT) {
        PIO_eprintf(NULL,
                "PackFile_unpack: Dir format was %d not %d\n",
                    header->dir_format, PF_DIR_FORMAT);
        return 0;
    }
#if TRACE_PACKFILE
    PIO_eprintf(NULL, "PackFile_unpack: Dirformat %d.\n", header->dir_format);
#endif

    (void)PF_fetch_opcode(self, &cursor); /* padding */
#if TRACE_PACKFILE
    PIO_eprintf(NULL, "PackFile_unpack: Directory read, offset %d.\n",
                (INTVAL)cursor - (INTVAL)packed);
#endif

    self->directory.base.file_offset = (INTVAL)cursor - (INTVAL)self->src;
    /*
     * now unpack dir, which unpacks its contents ...
     */
    Parrot_block_DOD(interp);
    cursor = PackFile_Segment_unpack(interp,
                                     &self->directory.base, cursor);
    Parrot_unblock_DOD(interp);

#ifdef PARROT_HAS_HEADER_SYSMMAN
    if (self->is_mmap_ped && (
                self->need_endianize || self->need_wordsize)) {
        munmap((void *)self->src, self->size);
        self->is_mmap_ped = 0;
    }
#endif

#if TRACE_PACKFILE
    PIO_eprintf(NULL, "PackFile_unpack: Unpack done.\n");
#endif

    return cursor - packed;
}

/*

=item C<INTVAL
PackFile_map_segments(Interp*, PackFile_Directory *dir,
                       PackFile_map_segments_func_t callback,
                       void *user_data)>

For each segment in the directory C<dir> the callback function
C<callback> is called. The pointer C<user_data> is append to each call.

If a callback returns non-zero the processing of segments is stopped,
and this value is returned.

=cut

*/

INTVAL
PackFile_map_segments(Interp *interp, PackFile_Directory *dir,
                       PackFile_map_segments_func_t callback,
                       void *user_data)
{
    size_t i;

    for (i = 0; i < dir->num_segments; i++) {
        const INTVAL ret = callback(interp, dir->segments[i], user_data);
        if (ret)
            return ret;
    }

    return 0;
}

/*

=item C<INTVAL
PackFile_add_segment(PackFile_Directory *dir, PackFile_Segment *seg)>

Adds the Segment C<seg> to the directory C<dir> The PackFile becomes the
owner of the segment; that means its getting destroyed, when the
packfile gets destroyed.

=cut

*/

INTVAL
PackFile_add_segment(Interp *interp, PackFile_Directory *dir,
        PackFile_Segment *seg)
{
    if (dir->segments) {
        dir->segments =
            (PackFile_Segment **)mem_sys_realloc(dir->segments,
                    sizeof (PackFile_Segment *) *
                    (dir->num_segments+1));
    }
    else {
        dir->segments = (PackFile_Segment **)mem_sys_allocate(
                sizeof (PackFile_Segment *) * (dir->num_segments+1));
    }
    dir->segments[dir->num_segments] = seg;
    dir->num_segments++;
    seg->dir = dir;

    return 0;
}

/*

=item C<PackFile_Segment *
PackFile_find_segment(Interp *, PackFile_Directory *dir, const char *name,
                      int sub_dir)>

Finds the segment with the name C<name> in the C<PackFile_Directory> if
C<sub_dir> is true, directories are searched recursively The segment is
returned, but its still owned by the C<PackFile>.

=cut

*/

PackFile_Segment *
PackFile_find_segment(Interp *interp, PackFile_Directory *dir,
    const char *name, int sub_dir)
{
    size_t i;

    if (!dir)
        return NULL;
    for (i=0; i < dir->num_segments; i++) {
        PackFile_Segment *seg = dir->segments[i];
        if (seg && strcmp(seg->name, name) == 0) {
            return seg;
        }
        if (sub_dir && seg->type == PF_DIR_SEG) {
            seg = PackFile_find_segment(interp,
                    (PackFile_Directory *)seg, name, sub_dir);
            if (seg)
                return seg;
        }
    }

    return NULL;
}

/*

=item C<PackFile_Segment *
PackFile_remove_segment_by_name(Interp *, PackFile_Directory *dir,
                                 const char *name)>

Finds and removes the segment with name C<name> in the
C<PackFile_Directory>. The segment is returned and must be destroyed by
the user.

=cut

*/

PackFile_Segment *
PackFile_remove_segment_by_name(Interp *interp, PackFile_Directory *dir,
                                const char *name)
{
    size_t i;

    for (i=0; i < dir->num_segments; i++) {
        PackFile_Segment * const seg = dir->segments[i];
        if (strcmp(seg->name, name) == 0) {
            dir->num_segments--;
            if (i != dir->num_segments) {
                /* We're not the last segment, so we need to move things */
                memmove(&dir->segments[i], &dir->segments[i+1],
                       (dir->num_segments - i) *
                       sizeof (PackFile_Segment *));
            }
            return seg;
        }
    }

    return NULL;
}

/*

=back

=head2 PackFile Structure Functions

=over 4

=item C<static void
PackFile_set_header(PackFile *self)>

Fill a C<PackFile> header with system specific data.

=cut

*/

static void
PackFile_set_header(PackFile *self)
{
    self->header->wordsize = sizeof (opcode_t);
    self->header->byteorder = PARROT_BIGENDIAN;
    self->header->major = PARROT_MAJOR_VERSION;
    self->header->minor = PARROT_MINOR_VERSION;
    self->header->intvalsize = sizeof (INTVAL);
    if (NUMVAL_SIZE == 8)
        self->header->floattype = 0;
    else /* if XXX */
        self->header->floattype = 1;
    /* write the fingerprint */
    PackFile_write_fingerprint(self->header->pad);
}

/*

=item C<PackFile *
PackFile_new(Interp*, INTVAL is_mapped)>

Allocate a new empty C<PackFile> and setup the directory.

Directory segment:

  +----------+----------+----------+----------+
  |    Segment Header                         |
  |    ..............                         |
  +----------+----------+----------+----------+

  +----------+----------+----------+----------+
  |    number of directory items              |
  +----------+----------+----------+----------+

followed by a sequence of items

  +----------+----------+----------+----------+
  |    Segment type                           |
  +----------+----------+----------+----------+
  |    "name"                                 |
  |    ...     '\0'       padding bytes       |
  +----------+----------+----------+----------+
  |    Offset in the file                     |
  +----------+----------+----------+----------+
  |    Size of the segment                    |
  +----------+----------+----------+----------+

"name" is a NUL-terminated c-string encoded in plain ASCII.

Segment types are defined in F<include/parrot/packfile.h>.

Offset and size are in C<opcode_t>.

A Segment Header has these entries:

 - op_count     total ops of segment incl. this count
 - itype        internal type of segment
 - id           internal id e.g code seg nr
 - size         size of following op array, 0 if none
 * data         possibly empty data, or e.g. byte code

=cut

*/

PackFile *
PackFile_new(Interp *interp, INTVAL is_mapped)
{
    PackFile * const pf = mem_allocate_zeroed_typed(PackFile);

    if (!pf) {
        PIO_eprintf(NULL, "PackFile_new: Unable to allocate!\n");
        return NULL;
    }
    pf->is_mmap_ped = is_mapped;

    pf->header = mem_allocate_zeroed_typed(PackFile_Header);
    if (!pf->header) {
        PIO_eprintf(NULL, "PackFile_new: Unable to allocate header!\n");
        PackFile_destroy(interp, pf);
        return NULL;
    }
    /*
     * fill header with system specific data
     */
    PackFile_set_header(pf);

    /* Other fields empty for now */
    pf->cur_cs = NULL;
    pf_register_standard_funcs(interp, pf);
    /* create the master directory, all subirs go there */
    pf->directory.base.pf = pf;
    pf->dirp = (PackFile_Directory *)
        PackFile_Segment_new_seg(interp, &pf->directory,
            PF_DIR_SEG, DIRECTORY_SEGMENT_NAME, 0);
    pf->directory = *pf->dirp;
    pf->fetch_op = (opcode_t (*)(unsigned char*)) NULLfunc;
    pf->fetch_iv = (INTVAL (*)(unsigned char*)) NULLfunc;
    pf->fetch_nv = (void (*)(unsigned char *, unsigned char *)) NULLfunc;
    return pf;
}

/*

=item C<PackFile * PackFile_new_dummy(Interp*, const char *name)>

Create a new (initial) dummy PackFile. This is needed, if the interpreter
doesn't load any bytecode, but is using Parrot_compile_string.

=cut

*/

PackFile *
PackFile_new_dummy(Interp *interp, const char *name)
{
    PackFile *pf;

    pf = PackFile_new(interp, 0);
    interp->initial_pf = pf;
    interp->code =
        pf->cur_cs = PF_create_default_segs(interp, name, 1);
    return pf;
}

/*

=item C<INTVAL PackFile_funcs_register(Interp*, PackFile *pf, UINTVAL type,
                                       PackFile_funcs funcs)>

Register the C<pack>/C<unpack>/... functions for a packfile type.

=cut

*/

INTVAL
PackFile_funcs_register(Interp *interp, PackFile *pf, UINTVAL type,
                        PackFile_funcs funcs)
{
    /* TODO dynamic registering */
    pf->PackFuncs[type] = funcs;
    return 1;
}

/*

=item C<static opcode_t * default_unpack(Interp *interp, PackFile_Segment *self,
                                         opcode_t *cursor)>

The default unpack function.

=cut

*/

static opcode_t *
default_unpack(Interp *interp, PackFile_Segment *self, opcode_t *cursor)
{
    if (self->pf->header->dir_format) {
        self->op_count = PF_fetch_opcode(self->pf, &cursor);
        self->itype = PF_fetch_opcode(self->pf, &cursor);
        self->id = PF_fetch_opcode(self->pf, &cursor);
        self->size = PF_fetch_opcode(self->pf, &cursor);
    }
    if (self->size == 0)
        return cursor;
    /* if the packfile is mmap()ed just point to it if we don't
     * need any fetch transforms
     */
    if (self->pf->is_mmap_ped &&
            !self->pf->need_endianize && !self->pf->need_wordsize) {
        self->data = cursor;
        cursor += self->size;
        return cursor;
    }
    /* else allocate mem */
    self->data = (opcode_t *)mem_sys_allocate(self->size * sizeof (opcode_t));

    if (!self->data) {
        PIO_eprintf(NULL,
                "PackFile_unpack: Unable to allocate data memory!\n");
        self->size = 0;
        return 0;
    }

    if (!self->pf->need_endianize && !self->pf->need_wordsize) {
        mem_sys_memcopy(self->data, cursor, self->size * sizeof (opcode_t));
        cursor += self->size;
    }
    else {
        int i;
        for (i = 0; i < (int)self->size ; i++) {
            self->data[i] = PF_fetch_opcode(self->pf, &cursor);
#if TRACE_PACKFILE
            PIO_eprintf(NULL, "op[#%d] %u\n", i, self->data[i]);
#endif
        }
    }

    return cursor;
}

/*

=item C<void
default_dump_header(Parrot_Interp interp, PackFile_Segment *self)>

The default dump header function.

=cut

*/

void
default_dump_header(Parrot_Interp interp, PackFile_Segment *self)
{
    PIO_printf(interp, "%s => [ # offs 0x%x(%d)",
            self->name, (int)self->file_offset, (int)self->file_offset);
    PIO_printf(interp, " = op_count %d, itype %d, id %d, size %d, ...",
            (int)self->op_count, (int)self->itype,
            (int)self->id, (int)self->size);
}

/*

=item C<static void
default_dump(Parrot_Interp interp, PackFile_Segment *self)>

The default dump function.

=cut

*/

static void
default_dump(Parrot_Interp interp, PackFile_Segment *self)
{
    size_t i;

    default_dump_header(interp, self);
    i = self->data ? 0: self->file_offset + 4;
    if (i % 8)
        PIO_printf(interp, "\n %04x:  ", (int) i);

    for (; i < (self->data ? self->size :
            self->file_offset + self->op_count); i++) {
        if (i % 8 == 0) {
            PIO_printf(interp, "\n %04x:  ", (int) i);
        }
        PIO_printf(interp, "%08lx ", (unsigned long)
                self->data ? self->data[i] : self->pf->src[i]);
    }
    PIO_printf(interp, "\n]\n");
}

/*

=item C<static INTVAL
pf_register_standard_funcs(Interp*, PackFile *pf)>

Called from within C<PackFile_new()> register the standard functions.

=cut

*/

static INTVAL
pf_register_standard_funcs(Interp *interp, PackFile *pf)
{
    PackFile_funcs dirf = {
        directory_new,
        directory_destroy,
        directory_packed_size,
        directory_pack,
        directory_unpack,
        directory_dump
    };
    PackFile_funcs defaultf = {
        PackFile_Segment_new,
        (PackFile_Segment_destroy_func_t) NULLfunc,
        (PackFile_Segment_packed_size_func_t) NULLfunc,
        (PackFile_Segment_pack_func_t) NULLfunc,
        (PackFile_Segment_unpack_func_t) NULLfunc,
        default_dump
    };
    PackFile_funcs fixupf = {
        fixup_new,
        fixup_destroy,
        fixup_packed_size,
        fixup_pack,
        fixup_unpack,
        default_dump
    };
    PackFile_funcs constf = {
        const_new,
        const_destroy,
        PackFile_ConstTable_pack_size,
        PackFile_ConstTable_pack,
        PackFile_ConstTable_unpack,
        default_dump
    };
    PackFile_funcs bytef = {
        byte_code_new,
        byte_code_destroy,
        (PackFile_Segment_packed_size_func_t) NULLfunc,
        (PackFile_Segment_pack_func_t) NULLfunc,
        (PackFile_Segment_unpack_func_t) NULLfunc,
        default_dump
    };
    PackFile_funcs debugf = {
        pf_debug_new,
        pf_debug_destroy,
        pf_debug_packed_size,
        pf_debug_pack,
        pf_debug_unpack,
        pf_debug_dump
    };
    PackFile_funcs_register(interp, pf, PF_DIR_SEG, dirf);
    PackFile_funcs_register(interp, pf, PF_UNKNOWN_SEG, defaultf);
    PackFile_funcs_register(interp, pf, PF_FIXUP_SEG, fixupf);
    PackFile_funcs_register(interp, pf, PF_CONST_SEG, constf);
    PackFile_funcs_register(interp, pf, PF_BYTEC_SEG, bytef);
    PackFile_funcs_register(interp, pf, PF_DEBUG_SEG, debugf);
    return 1;
}

/*

=item C<PackFile_Segment *
PackFile_Segment_new_seg(Interp*, PackFile_Directory *dir, UINTVAL type,
        const char *name, int add)>

Create a new segment.

=cut

*/

PackFile_Segment *
PackFile_Segment_new_seg(Interp *interp, PackFile_Directory *dir, UINTVAL type,
                         const char *name, int add)
{
    PackFile * const pf = dir->base.pf;
    PackFile_Segment_new_func_t f = pf->PackFuncs[type].new_seg;
    PackFile_Segment * const seg = (f)(interp, pf, name, add);
    segment_init (interp, seg, pf, name);
    seg->type = type;
    if (add)
        PackFile_add_segment(interp, dir, seg);
    return seg;
}

static PackFile_Segment *
create_seg(Interp *interp, PackFile_Directory *dir, pack_file_types t,
           const char *name, const char *file_name, int add)
{
    PackFile_Segment *seg;

    const size_t len = strlen(name) + strlen(file_name) + 2;
    char * const buf = (char *)malloc(len);

    sprintf(buf, "%s_%s", name, file_name);
    seg = PackFile_Segment_new_seg(interp, dir, t, buf, add);
    free(buf);
    return seg;
}

/*

=item C<PackFile_ByteCode *
PF_create_default_segs(Interp*, const char *file_name, int add)>

Create bytecode, constant, and fixup segment for C<file_nam>. If C<add>
is true, the current packfile becomes the owner of these segments by
adding the segments to the directory.

=cut

*/

PackFile_ByteCode *
PF_create_default_segs(Interp *interp, const char *file_name, int add)
{
    PackFile * const pf   = interp->initial_pf;
    PackFile_Segment *seg =
        create_seg(interp, &pf->directory,
            PF_BYTEC_SEG, BYTE_CODE_SEGMENT_NAME, file_name, add);
    PackFile_ByteCode * const cur_cs = (PackFile_ByteCode *)seg;

    seg = create_seg(interp, &pf->directory,
            PF_FIXUP_SEG, FIXUP_TABLE_SEGMENT_NAME, file_name, add);
    cur_cs->fixups = (PackFile_FixupTable *)seg;
    cur_cs->fixups->code = cur_cs;

    seg = create_seg(interp, &pf->directory,
            PF_CONST_SEG, CONSTANT_SEGMENT_NAME, file_name, add);
    cur_cs->const_table = (PackFile_ConstTable *) seg;
    cur_cs->const_table->code = cur_cs;

    seg = create_seg(interp, &pf->directory,
            PF_UNKNOWN_SEG, "PIC_idx", file_name, add);
    cur_cs->pic_index = seg;

    return cur_cs;
}
/*

=item C<void
PackFile_Segment_destroy(Interp *, PackFile_Segment *self)>

=cut

*/

void
PackFile_Segment_destroy(Interp *interp, PackFile_Segment *self)
{
    PackFile_Segment_destroy_func_t f =
        self->pf->PackFuncs[self->type].destroy;
    if (f)
        (f)(interp, self);
    default_destroy(interp, self);    /* destroy self after specific */
}

/*

=item C<size_t
PackFile_Segment_packed_size(Interp*, PackFile_Segment *self)>

=cut

*/

size_t
PackFile_Segment_packed_size(Interp *interp, PackFile_Segment *self)
{
    size_t size = default_packed_size(interp, self);
    PackFile_Segment_packed_size_func_t f =
        self->pf->PackFuncs[self->type].packed_size;
    const size_t align = 16/sizeof (opcode_t);
    if (f)
        size += (f)(interp, self);
    if (align && size % align)
        size += (align - size % align);   /* pad/align it */
    return size;
}

/*

=item C<opcode_t *
PackFile_Segment_pack(Interp*, PackFile_Segment *self, opcode_t *cursor)>

=cut

*/

opcode_t *
PackFile_Segment_pack(Interp *interp, PackFile_Segment *self, opcode_t *cursor)
{
    PackFile_Segment_pack_func_t f =
        self->pf->PackFuncs[self->type].pack;
    const size_t align = 16/sizeof (opcode_t);

    cursor = default_pack(interp, self, cursor);
    if (!cursor)
        return 0;
    if (f)
        cursor = (f)(interp, self, cursor);
    if (align && (cursor - self->pf->src) % align)
        cursor += align - (cursor - self->pf->src) % align;
    return cursor;
}

/*

=item C<opcode_t *
PackFile_Segment_unpack(Interp *interp, PackFile_Segment *self,
                        opcode_t *cursor)>

All all these functions call the related C<default_*> function.

If a special is defined this gets called after.

=cut

*/

opcode_t *
PackFile_Segment_unpack(Interp *interp, PackFile_Segment *self,
                        opcode_t *cursor)
{
    PackFile_Segment_unpack_func_t f =
        self->pf->PackFuncs[self->type].unpack;

    cursor = default_unpack(interp, self, cursor);
    if (!cursor)
        return 0;
    if (f) {
        cursor = (f)(interp, self, cursor);
        if (!cursor)
            return 0;
    }
    ALIGN_16(self->pf->src, cursor);
    return cursor;
}

/*

=item C<void
PackFile_Segment_dump(Interp *interp, PackFile_Segment *self)>

Dumps the segment C<self>.

=cut

*/

void
PackFile_Segment_dump(Interp *interp, PackFile_Segment *self)
{
    self->pf->PackFuncs[self->type].dump(interp, self);
}

/*

=back

=head2 Standard Directory Functions

=over 4

=item C<static PackFile_Segment *
directory_new(Interp*, PackFile *pf, const char *name, int add)>

Returns a new C<PackFile_Directory> cast as a C<PackFile_Segment>.

=cut

*/

static PackFile_Segment *
directory_new(Interp *interp, PackFile *pf, const char *name, int add)
{
    PackFile_Directory * const dir = mem_allocate_typed(PackFile_Directory);

    dir->num_segments = 0;
    dir->segments = NULL;

    return (PackFile_Segment *)dir;
}

/*

=item C<static void
directory_dump(Interp *interp, PackFile_Segment *self)>

Dumps the directory C<self>.

=cut

*/

static void
directory_dump(Interp *interp, PackFile_Segment *self)
{
    PackFile_Directory * const dir = (PackFile_Directory *) self;
    size_t i;

    default_dump_header(interp, self);
    PIO_printf(interp, "\n\t# %d segments\n", dir->num_segments);
    for (i=0; i < dir->num_segments; i++) {
        PackFile_Segment *seg = dir->segments[i];
        PIO_printf(interp,
                "\ttype %d\t%s\t", (int)seg->type, seg->name);
        PIO_printf(interp,
                " offs 0x%x(0x%x)\top_count %d\n",
                (int)seg->file_offset,
                (int)seg->file_offset * sizeof (opcode_t),
                (int)seg->op_count);
    }
    PIO_printf(interp, "]\n");
    for (i=0; i < dir->num_segments; i++) {
        PackFile_Segment * const seg = dir->segments[i];
        PackFile_Segment_dump(interp, seg);
    }
}

/*

=item C<static opcode_t *
directory_unpack(Interp *interp, PackFile_Segment *segp, opcode_t *cursor)>

Unpacks the directory.

=cut

*/

static opcode_t *
directory_unpack(Interp *interp, PackFile_Segment *segp, opcode_t *cursor)
{
    size_t i;
    PackFile_Directory * const dir = (PackFile_Directory *) segp;
    PackFile * const pf = dir->base.pf;
    opcode_t *pos;

    dir->num_segments = PF_fetch_opcode(pf, &cursor);
    if (dir->segments) {
        dir->segments = (PackFile_Segment **)mem_sys_realloc(dir->segments,
                             sizeof (PackFile_Segment *) * dir->num_segments);
    }
    else {
        dir->segments = (PackFile_Segment **)mem_sys_allocate(
                sizeof (PackFile_Segment *) * dir->num_segments);
    }

    for (i=0; i < dir->num_segments; i++) {
        PackFile_Segment *seg;
        size_t tmp;
        UINTVAL type;
        char *name;

        /* get type */
        type = PF_fetch_opcode(pf, &cursor);
        if (type >= PF_MAX_SEG)
            type = PF_UNKNOWN_SEG;
#if TRACE_PACKFILE
        PIO_eprintf(NULL, "Segment type %d.\n", type);
#endif
        /* get name */
        name = PF_fetch_cstring(pf, &cursor);
#if TRACE_PACKFILE
        PIO_eprintf(NULL, "Segment name \"%s\".\n", name);
#endif

        /* create it */
        seg = PackFile_Segment_new_seg(interp, dir, type, name, 0);
        mem_sys_free(name);

        seg->file_offset = PF_fetch_opcode(pf, &cursor);
        seg->op_count = PF_fetch_opcode(pf, &cursor);

        if (pf->need_wordsize) {
#if OPCODE_T_SIZE == 8
            if (pf->header->wordsize == 4)
                pos = pf->src + seg->file_offset / 2;
#else
            if (pf->header->wordsize == 8)
                pos = pf->src + seg->file_offset * 2;
#endif
        } else
            pos = pf->src + seg->file_offset;
        tmp = PF_fetch_opcode(pf, &pos);
        if (seg->op_count != tmp) {
            fprintf(stderr,
                     "%s: Size in directory %d doesn't match size %d "
                     "at offset 0x%x\n", seg->name, (int)seg->op_count,
                     (int)tmp, (int)seg->file_offset);
        }
        if (i) {
            PackFile_Segment *last = dir->segments[i-1];
            if (last->file_offset + last->op_count != seg->file_offset) {
                fprintf(stderr, "%s: sections are not back to back\n",
                        "section");
            }
        }
        make_code_pointers(seg);

        /* store the segment */
        dir->segments[i] = seg;
        seg->dir = dir;
    }

    ALIGN_16(pf->src, cursor);
    /* and now unpack contents of dir */
    for (i = 0; cursor && i < dir->num_segments; i++) {
        opcode_t *csave = cursor;
        size_t tmp = PF_fetch_opcode(pf, &cursor); /* check len again */
        size_t delta = 0;       /* keep gcc -O silent */

        cursor = csave;
        pos = PackFile_Segment_unpack(interp, dir->segments[i],
                                       cursor);
        if (!pos) {
            fprintf(stderr, "PackFile_unpack segment '%s' failed\n",
                    dir->segments[i]->name);
            return 0;
        }
        if (pf->need_wordsize) {
#if OPCODE_T_SIZE == 8
            if (pf->header->wordsize == 4)
                delta = (pos - cursor) * 2;
#else
            if (pf->header->wordsize == 8)
                delta = (pos - cursor) / 2;
#endif
        } else
            delta = pos - cursor;
        if ((size_t)delta != tmp || dir->segments[i]->op_count != tmp)
            fprintf(stderr, "PackFile_unpack segment '%s' directory length %d "
                    "length in file %d needed %d for unpack\n",
                    dir->segments[i]->name,
                    (int)dir->segments[i]->op_count, (int)tmp,
                    (int)delta);
        cursor = pos;
    }
    return cursor;
}

/*

=item C<static void
directory_destroy(Interp*, PackFile_Segment *self)>

Destroys the directory.

=cut

*/

static void
directory_destroy(Interp *interp, PackFile_Segment *self)
{
    PackFile_Directory *dir = (PackFile_Directory *)self;
    size_t i;

    for (i = 0; i < dir->num_segments; i++)
        PackFile_Segment_destroy(interp, dir->segments[i]);

    if (dir->segments) {
        mem_sys_free(dir->segments);
        dir->segments = NULL;
    }
}

/*

=item C<static void
sort_segs(Interp*, PackFile_Directory *dir)>

Sorts the segments in C<dir>.

=cut

*/

static void
sort_segs(Interp *interp, PackFile_Directory *dir)
{
    const size_t num_segs = dir->num_segments;

    PackFile_Segment *seg = dir->segments[0];
    if (seg->type != PF_BYTEC_SEG) {
        size_t i;
        for (i = 1; i < num_segs; i++) {
            PackFile_Segment * const s2 = dir->segments[i];
            if (s2->type == PF_BYTEC_SEG) {
                dir->segments[0] = s2;
                dir->segments[i] = seg;
                break;
            }
        }
    }
    seg = dir->segments[1];
    if (seg->type != PF_FIXUP_SEG) {
        size_t i;
        for (i = 2; i < num_segs; i++) {
            PackFile_Segment * const s2 = dir->segments[i];
            if (s2->type == PF_FIXUP_SEG) {
                dir->segments[1] = s2;
                dir->segments[i] = seg;
                break;
            }
        }
    }
}

/*

=item C<static size_t
directory_packed_size(Interp*, PackFile_Segment *self)>

Returns the size of the directory minus the value returned by
C<default_packed_size()>.

=cut

*/

static size_t
directory_packed_size(Interp *interp, PackFile_Segment *self)
{
    PackFile_Directory * const dir = (PackFile_Directory *)self;
    const size_t align = 16/sizeof (opcode_t);
    size_t size, i, seg_size;

    /* need bytecode, fixup, other segs ... */
    sort_segs(interp, dir);
    /* number of segments + default, we need it for the offsets */
    size = 1 + default_packed_size(interp, self);
    for (i = 0; i < dir->num_segments; i++) {
        size += 3;        /* type, offset, size */
        size += PF_size_cstring(dir->segments[i]->name);
    }
    if (align && size % align)
        size += (align - size % align);   /* pad/align it */
    for (i=0; i < dir->num_segments; i++) {
        dir->segments[i]->file_offset = size + self->file_offset;
        seg_size = PackFile_Segment_packed_size(interp, dir->segments[i]);
        dir->segments[i]->op_count = seg_size;
        size += seg_size;
    }
    self->op_count = size;
    /* subtract default, it is added in PackFile_Segment_packed_size */
    return size - default_packed_size(interp, self);
}

/*

=item C<static opcode_t *
directory_pack(Interp*, PackFile_Segment *self, opcode_t *cursor)>

Packs the directory C<self>.

=cut

*/

static opcode_t *
directory_pack(Interp *interp, PackFile_Segment *self, opcode_t *cursor)
{
    PackFile_Directory *dir = (PackFile_Directory *)self;
    size_t i;
    size_t align;
    const size_t num_segs = dir->num_segments;

    *cursor++ = num_segs;

    for (i = 0; i < num_segs; i++) {
        const PackFile_Segment * const seg = dir->segments[i];
        *cursor++ = seg->type;
        cursor = PF_store_cstring(cursor, seg->name);
        *cursor++ = seg->file_offset;
        *cursor++ = seg->op_count;
    }
    align = 16/sizeof (opcode_t);
    if (align && (cursor - self->pf->src) % align)
        cursor += align - (cursor - self->pf->src) % align;
    /* now pack all segments into new format */
    for (i = 0; i < dir->num_segments; i++) {
        PackFile_Segment * const seg = dir->segments[i];
        const size_t size = seg->op_count;

        PackFile_Segment_pack(interp, seg, cursor);
        /*
         * XXX somehow it's smelling fishy here:
         * - either cursor is unaligned
         * - or the return result of _pack doesn't match
         *   expected size
         *
         * likely in combination with pbc_merge
         *
         * the relevant code with size check is visible in:
         *
         * svn diff -r15516:15517
         */
        cursor += size;
    }

    return cursor;
}

/*

=back

=head2 C<PackFile_Segment> Functions

=over 4

=item C<static void
segment_init(Interp*, PackFile_Segment *self, PackFile *pf, const char *name)>

Initializes the segment C<self>.

=cut

*/

static void
segment_init(Interp *interp, PackFile_Segment *self, PackFile *pf,
             const char *name)
{
    self->pf          = pf;
    self->type        = PF_UNKNOWN_SEG;
    self->file_offset = 0;
    self->op_count    = 0;
    self->itype       = 0;
    self->size        = 0;
    self->data        = NULL;
    self->id          = 0;
    self->name        = (char *)mem_sys_allocate(strlen(name) + 1);

    strcpy(self->name, name);
}

/*

=item C<PackFile_Segment *
PackFile_Segment_new(Interp*, PackFile *pf, const char *name, int add)>

Create a new default section.

=cut

*/

PackFile_Segment *
PackFile_Segment_new(Interp *interp, PackFile *pf, const char *name, int add)
{
    PackFile_Segment * const seg = mem_allocate_typed(PackFile_Segment);

    return seg;
}

/*

=back

=head2 Default Function Implementations

The default functions are called before the segment specific functions
and can read a block of C<opcode_t> data.

=over 4

=item C<static void
default_destroy(Interp*, PackFile_Segment *self)>

The default destroy function.

=cut

*/

static void
default_destroy(Interp *interp, PackFile_Segment *self)
{
    if (!self->pf->is_mmap_ped && self->data) {
        mem_sys_free(self->data);
        self->data = NULL;
    }
    if (self->name) {
        mem_sys_free(self->name);
        self->name = NULL;
    }
    mem_sys_free(self);
}

/*

=item C<static size_t
default_packed_size(Interp*, PackFile_Segment *self)>

Returns the default size of the segment C<self>.

=cut

*/

static size_t
default_packed_size(Interp *interp, const PackFile_Segment *self)
{
    /* op_count, itype, id, size */
    /* XXX There should be a constant defining this 4, and why */
    /* This is the 2nd place in the file that has this */
    return 4 + self->size;
}

/*

=item C<static opcode_t *
default_pack(Interp*, PackFile_Segment *self, opcode_t *dest)>

Performs the default pack.

=cut

*/

static opcode_t *
default_pack(Interp *interp, const PackFile_Segment *self,
             opcode_t *dest)
{
    *dest++ = self->op_count;
    *dest++ = self->itype;
    *dest++ = self->id;
    *dest++ = self->size;
    if (self->size)
        memcpy(dest, self->data, self->size * sizeof (opcode_t));
    return dest + self->size;
}

/* XXX Should be declared elsewhere */
extern void Parrot_destroy_jit(void *ptr);

/*

=back

=head2 ByteCode

=over 4

=item C<static void
byte_code_destroy(Interp*, PackFile_Segment *self)>

Destroys the C<PackFile_ByteCode> segment C<self>.

=cut

*/

static void
byte_code_destroy(Interp *interp, PackFile_Segment *self)
{
    PackFile_ByteCode * const byte_code = (PackFile_ByteCode *)self;

#ifdef HAS_JIT
    Parrot_destroy_jit(byte_code->jit_info);
#endif
    parrot_PIC_destroy(interp, byte_code);
    if (byte_code->prederef.code) {
        Parrot_free_memalign(byte_code->prederef.code);
        byte_code->prederef.code = NULL;
        if (byte_code->prederef.branches) {
            mem_sys_free(byte_code->prederef.branches);
            byte_code->prederef.branches = NULL;
        }
    }
    byte_code->fixups      = NULL;
    byte_code->const_table = NULL;
    byte_code->pic_index   = NULL;
    byte_code->debugs      = NULL;
}

/*

=item C<static PackFile_Segment *
byte_code_new(Interp*, PackFile *pf, const char *name, int add)>

New C<PackFile_ByteCode> segment.

C<pf> and C<add> are ignored.

=cut

*/

static PackFile_Segment *
byte_code_new(Interp *interp, PackFile *pf, const char *name, int add)
{
    PackFile_ByteCode *byte_code    = mem_allocate_typed(PackFile_ByteCode);

    byte_code->base.dir             = NULL;
    byte_code->prederef.code        = NULL;
    byte_code->prederef.branches    = NULL;
    byte_code->prederef.n_allocated = 0;
    byte_code->jit_info             = NULL;
    byte_code->debugs               = NULL;
    byte_code->const_table          = NULL;
    byte_code->fixups               = NULL;
    byte_code->pic_index            = NULL;
    byte_code->pic_store            = NULL;

    return (PackFile_Segment *) byte_code;
}

/*

=back

=head2 Debug Info

=over 4

=item C<static void
pf_debug_destroy(Interp*, PackFile_Segment *self)>

Destroys the C<PackFile_Debug> segment C<self>.

=cut

*/

static void
pf_debug_destroy(Interp *interp, PackFile_Segment *self)
{
    PackFile_Debug * const debug = (PackFile_Debug *) self;
    int i;

    /* Free each mapping. */
    for (i = 0; i < debug->num_mappings; i++)
        mem_sys_free(debug->mappings[i]);

    /* Free mappings pointer array. */
    mem_sys_free(debug->mappings);
    debug->mappings     = NULL;
    debug->num_mappings = 0;
}

/*

=item C<static PackFile_Segment *
pf_debug_new(Interp *, PackFile *pf, const char *name, int add)>

Returns a new C<PackFile_Debug> segment.

C<pf> and C<add> ignored.

=cut

*/

static PackFile_Segment *
pf_debug_new(Interp *interp, PackFile *pf, const char *name, int add)
{
    PackFile_Debug * const debug = mem_allocate_typed(PackFile_Debug);

    debug->code                  = NULL;
    debug->mappings              = mem_allocate_typed(PackFile_DebugMapping *);
    debug->mappings[0]           = NULL;
    debug->num_mappings          = 0;

    return (PackFile_Segment *)debug;
}

/*

=item C<static size_t
pf_debug_packed_size (Interp*, PackFile_Segment *self)>

Returns the size of the C<PackFile_Debug> segment's filename in
C<opcode_t> units.

=cut

*/

static size_t
pf_debug_packed_size(Interp *interp, PackFile_Segment *self)
{
    PackFile_Debug * const debug = (PackFile_Debug *)self;
    int size = 0;
    int i;

    /* Size of mappings count. */
    size += 1;

    /* Size of entries in mappings list. */
    for (i = 0; i < debug->num_mappings; i++) {
        /* Bytecode offset and mapping type */
        size += 2;

        /* Mapping specific stuff. */
        switch (debug->mappings[i]->mapping_type) {
            case PF_DEBUGMAPPINGTYPE_NONE:
                break;
            case PF_DEBUGMAPPINGTYPE_FILENAME:
            case PF_DEBUGMAPPINGTYPE_SOURCESEG:
                size += 1;
                break;
        }
    }

    return size;
}

/*

=item C<static opcode_t *
pf_debug_pack(Interp*, PackFile_Segment *self, opcode_t *cursor)>

Pack the debug segment.

=cut

*/

static opcode_t *
pf_debug_pack(Interp *interp, PackFile_Segment *self, opcode_t *cursor)
{
    PackFile_Debug * const debug = (PackFile_Debug *)self;
    int i;

    /* Store number of mappings. */
    *cursor++ = debug->num_mappings;

    /* Now store each mapping. */
    for (i = 0; i < debug->num_mappings; i++) {
        /* Bytecode offset and mapping type */
        *cursor++ = debug->mappings[i]->offset;
        *cursor++ = debug->mappings[i]->mapping_type;

        /* Mapping specific stuff. */
        switch (debug->mappings[i]->mapping_type) {
            case PF_DEBUGMAPPINGTYPE_NONE:
                break;
            case PF_DEBUGMAPPINGTYPE_FILENAME:
                *cursor++ = debug->mappings[i]->u.filename;
                break;
            case PF_DEBUGMAPPINGTYPE_SOURCESEG:
                *cursor++ = debug->mappings[i]->u.source_seg;
                break;
        }
    }

    return cursor;
}

/*

=item C<static opcode_t *
pf_debug_unpack(Interp *interp, PackFile_Segment *self, opcode_t *cursor)>

Unpack a debug segment into a PackFile_Debug structure.

=cut

*/

static opcode_t *
pf_debug_unpack(Interp *interp, PackFile_Segment *self, opcode_t *cursor)
{
    PackFile_Debug * const debug = (PackFile_Debug *)self;
    PackFile_ByteCode *code;
    int i;

    /* For some reason, we store the source file name in the segment
       name. So we can't find the bytecode seg without knowing the filename.
       But with the new scheme we can have many file names. For now, just
       base this on the name of the debug segment. */
    char *code_name = NULL;
    size_t str_len;

    /* Number of mappings. */
    debug->num_mappings = PF_fetch_opcode(self->pf, &cursor);

    /* Allocate space for mappings vector. */
    debug->mappings = (PackFile_DebugMapping **)mem_sys_allocate(
        sizeof (PackFile_DebugMapping *) * (debug->num_mappings + 1));

    /* Read in each mapping. */
    for (i = 0; i < debug->num_mappings; i++) {
        /* Allocate struct and get offset and mapping type. */
        debug->mappings[i] = mem_allocate_typed(PackFile_DebugMapping);
        debug->mappings[i]->offset = PF_fetch_opcode(self->pf, &cursor);
        debug->mappings[i]->mapping_type = PF_fetch_opcode(self->pf, &cursor);

        /* Read mapping specific stuff. */
        switch (debug->mappings[i]->mapping_type) {
            case PF_DEBUGMAPPINGTYPE_NONE:
                break;
            case PF_DEBUGMAPPINGTYPE_FILENAME:
                debug->mappings[i]->u.filename =
                    PF_fetch_opcode(self->pf, &cursor);
                break;
            case PF_DEBUGMAPPINGTYPE_SOURCESEG:
                debug->mappings[i]->u.source_seg =
                    PF_fetch_opcode(self->pf, &cursor);
                break;
        }
    }

    /*
     * find seg e.g. CODE_DB => CODE
     * and attach it
     */
    code_name = strdup(debug->base.name);
    str_len = strlen(code_name);
    code_name[str_len - 3] = 0;
    code = (PackFile_ByteCode *)PackFile_find_segment(interp,
            self->dir, code_name, 0);
    if (!code || code->base.type != PF_BYTEC_SEG)
        internal_exception(1, "Code '%s' not found for debug segment '%s'\n",
                code_name, self->name);
    code->debugs = debug;
    debug->code = code;
    free(code_name);
    return cursor;
}


/*

=item C<static void
pf_debug_dump(Interp *interp, PackFile_Segment *self)>

Dumps a debug segment to a human readable form.

=cut

*/

static void
pf_debug_dump(Parrot_Interp interp, PackFile_Segment *self)
{
    opcode_t i;
    size_t   j;
    PackFile_Debug * const debug = (PackFile_Debug *)self;
    char *filename;

    default_dump_header(interp, self);

    PIO_printf(interp, "\n  mappings => [\n");
    for (i = 0; i < debug->num_mappings; i++) {
        PIO_printf(interp, "    #%d\n    [\n", i);
        PIO_printf(interp, "        OFFSET => %d,\n",
                   debug->mappings[i]->offset);
        switch (debug->mappings[i]->mapping_type) {
            case PF_DEBUGMAPPINGTYPE_NONE:
                PIO_printf(interp, "        MAPPINGTYPE => NONE\n");
                break;
            case PF_DEBUGMAPPINGTYPE_FILENAME:
                PIO_printf(interp, "        MAPPINGTYPE => FILENAME,\n");
                filename = string_to_cstring(interp, PF_CONST(debug->code,
                           debug->mappings[i]->u.filename)->u.string);
                PIO_printf(interp, "        FILENAME => %s\n", filename);
                string_cstring_free(filename);
                break;
            case PF_DEBUGMAPPINGTYPE_SOURCESEG:
                PIO_printf(interp, "        MAPPINGTYPE => SOURCESEG,\n");
                PIO_printf(interp, "        SOURCESEG => %d\n",
                           debug->mappings[i]->u.source_seg);
                break;
        }
        PIO_printf(interp, "    ],\n");
    }
    PIO_printf(interp, "  ]\n");

    j = self->data ? 0: self->file_offset + 4;
    if (j % 8)
        PIO_printf(interp, "\n %04x:  ", (int) j);

    for (; j < (self->data ? self->size :
            self->file_offset + self->op_count); j++) {
        if (j % 8 == 0) {
            PIO_printf(interp, "\n %04x:  ", (int) j);
        }
        PIO_printf(interp, "%08lx ", (unsigned long)
                self->data ? self->data[j] : self->pf->src[j]);
    }
    PIO_printf(interp, "\n]\n");
}

/*

=item C<PackFile_Debug *
Parrot_new_debug_seg(Interp *interp, PackFile_ByteCode *cs, size_t size)>

Create and append (or resize) a new debug seg for a code segment.

=cut

*/

PackFile_Debug *
Parrot_new_debug_seg(Interp *interp, PackFile_ByteCode *cs, size_t size)
{
    PackFile_Debug *debug;

    if (cs->debugs) {    /* it exists already, resize it */
        debug = cs->debugs;
        debug->base.data = (opcode_t *)mem_sys_realloc(debug->base.data, size *
                sizeof (opcode_t));
    }
    else {              /* create one */
        const size_t len = strlen(cs->base.name) + 4;
        char * const name = (char *)mem_sys_allocate(len);

        sprintf(name, "%s_DB", cs->base.name);
        if (interp->code && interp->code->base.dir) {
            debug = (PackFile_Debug *)
                PackFile_Segment_new_seg(interp,
                interp->code->base.dir, PF_DEBUG_SEG, name, 1);
        }
        else {
            /* used by eval - don't register the segment */
            debug = (PackFile_Debug *)
                PackFile_Segment_new_seg(interp,
                cs->base.dir ?  cs->base.dir :
                    &interp->initial_pf->directory,
                PF_DEBUG_SEG, name, 0);
        }
        mem_sys_free(name);

        debug->base.data    = (opcode_t *)mem_sys_allocate(size *
            sizeof (opcode_t));
        debug->code = cs;
        cs->debugs = debug;
    }
    debug->base.size = size;
    return debug;
}

/*

=item c<void
Parrot_debug_add_mapping(Interp *interp, PackFile_Debug *debug,
                         opcode_t offset, int mapping_type,
                         const char *filename, int source_seg)>

Add a bytecode offset to filename/source segment mapping. mapping_type may be
one of PF_DEBUGMAPPINGTYPE_NONE (in which case the last two parameters are
ignored), PF_DEBUGMAPPINGTYPE_FILENAME (in which case filename must be given)
or PF_DEBUGMAPPINGTYPE_SOURCESEG (in which case source_seg should contains the
number of the source segment in question).

=cut

*/
void
Parrot_debug_add_mapping(Interp *interp, PackFile_Debug *debug,
                         opcode_t offset, int mapping_type,
                         const char *filename, int source_seg)
{
    PackFile_DebugMapping *mapping;
    PackFile_ConstTable * const ct = debug->code->const_table;
    PackFile_Constant *fnconst;
    int insert_pos = 0;

    /* Allocate space for the extra entry. */
    debug->mappings = (PackFile_DebugMapping **)mem_sys_realloc(debug->mappings,
        sizeof (PackFile_DebugMapping *) * (debug->num_mappings + 1));

    /* Can it just go on the end? */
    if (debug->num_mappings == 0 ||
        offset >= debug->mappings[debug->num_mappings - 1]->offset)
    {
        insert_pos = debug->num_mappings;
    }
    else {
        /* Find the right place and shift stuff that's after it. */
        int i;
        for (i = 0; i < debug->num_mappings; i++) {
            if (debug->mappings[i]->offset > offset) {
                insert_pos = i;
                memmove(debug->mappings + i + 1, debug->mappings + i,
                    debug->num_mappings - i);
                break;
            }
        }
    }

    /* Set up new entry and insert it. */
    mapping               = mem_allocate_typed(PackFile_DebugMapping);
    mapping->offset       = offset;
    mapping->mapping_type = mapping_type;

    switch (mapping_type) {
        case PF_DEBUGMAPPINGTYPE_NONE:
            break;
        case PF_DEBUGMAPPINGTYPE_FILENAME:
            /* Need to put filename in constants table. */
            ct->const_count = ct->const_count + 1;
            if (ct->constants)
                ct->constants = (PackFile_Constant **)mem_sys_realloc(
                    ct->constants, ct->const_count *
                        sizeof (PackFile_Constant *));
            else
                ct->constants = (PackFile_Constant **)mem_sys_allocate(
                    ct->const_count * sizeof (PackFile_Constant *));
            fnconst = PackFile_Constant_new(interp);
            fnconst->type = PFC_STRING;
            fnconst->u.string = string_make_direct(interp, filename,
                strlen(filename), PARROT_DEFAULT_ENCODING,
                PARROT_DEFAULT_CHARSET, PObj_constant_FLAG);
            ct->constants[ct->const_count - 1] = fnconst;
            mapping->u.filename = ct->const_count - 1;
            break;
        case PF_DEBUGMAPPINGTYPE_SOURCESEG:
            mapping->u.source_seg = source_seg;
            break;
    }
    debug->mappings[insert_pos] = mapping;
    debug->num_mappings = debug->num_mappings + 1;
}

/*
=item C<STRING*
Parrot_debug_pc_to_filename(Interp *interp, PackFile_Debug *debug, opcode_t pc)>

Take a position in the bytecode and return the filename of the source for
that position.

=cut

*/

STRING *
Parrot_debug_pc_to_filename(Interp *interp, PackFile_Debug *debug, opcode_t pc)
{
    /* Look through mappings until we find one that maps the passed
       bytecode offset. */
    int i;
    for (i = 0; i < debug->num_mappings; i++) {
        /* If this is the last mapping or the current position is
           between this mapping and the next one, return a filename. */
        if (i + 1 == debug->num_mappings ||
            (debug->mappings[i]->offset <= pc &&
             debug->mappings[i+1]->offset > pc))
        {
            switch (debug->mappings[i]->mapping_type) {
                case PF_DEBUGMAPPINGTYPE_NONE:
                    return string_from_const_cstring(interp,
                        "(unknown file)", 0);
                case PF_DEBUGMAPPINGTYPE_FILENAME:
                    return PF_CONST(debug->code,
                        debug->mappings[i]->u.filename)->u.string;
                case PF_DEBUGMAPPINGTYPE_SOURCESEG:
                    return string_from_const_cstring(interp,
                        "(unknown file)", 0);
            }
        }
    }

    /* Otherwise, no mappings = no filename. */
    return string_from_const_cstring(interp, "(unknown file)", 0);
}

/*

=item C<void
Parrot_switch_to_cs_by_nr(Interp *interp, opcode_t seg)>

Switch to byte code segment number C<seg>.

=cut

*/

void
Parrot_switch_to_cs_by_nr(Interp *interp, opcode_t seg)
{
    PackFile_Directory * const dir = interp->code->base.dir;
    const size_t num_segs = dir->num_segments;
    size_t i;
    opcode_t n;

    /* TODO make an index of code segments for faster look up */
    for (i = n = 0; i < num_segs; i++) {
        if (dir->segments[i]->type == PF_BYTEC_SEG) {
            if (n == seg) {
                Parrot_switch_to_cs(interp, (PackFile_ByteCode *)
                        dir->segments[i], 1);
                return;
            }
            n++;
        }
    }
    internal_exception(1, "Segment number %d not found\n", (int) seg);
}

/*

=item C<PackFile_ByteCode *
Parrot_switch_to_cs(Interp *interp, PackFile_ByteCode *new_cs, int really)>

Switch to a byte code segment C<new_cs>, returning the old segment.

=cut

*/

PackFile_ByteCode *
Parrot_switch_to_cs(Interp *interp, PackFile_ByteCode *new_cs, int really)
{
    PackFile_ByteCode * const cur_cs = interp->code;

    if (!new_cs) {
        internal_exception(NO_PREV_CS, "No code segment to switch to\n");
    }
    /* compiling source code uses this function too,
     * which gives misleading trace messages
     */
    if (really && Interp_trace_TEST(interp, PARROT_TRACE_SUB_CALL_FLAG)) {
        Interp *tracer = interp->debugger ?
            interp->debugger : interp;
        PIO_eprintf(tracer, "*** switching to %s\n",
                new_cs->base.name);
    }
    interp->code = new_cs;
    CONTEXT(interp->ctx)->constants =
        really ? find_constants(interp, new_cs->const_table) :
        new_cs->const_table->constants;
            /* new_cs->const_table->constants; */
    CONTEXT(interp->ctx)->pred_offset =
        new_cs->base.data - (opcode_t*) new_cs->prederef.code;
    if (really)
        prepare_for_run(interp);
    return cur_cs;
}

/*

=item C<static PackFile_Constant **
find_constants(Interp *interp, PackFile_ConstTable *ct)>

Find the constant table associated with a thread. For now, we need to copy
constant tables because some entries aren't really constant; e.g.
subroutines need to reference namespace pointers.

=cut

*/

static PackFile_Constant *
clone_constant(Interp *interp, PackFile_Constant *old_const) {
    STRING * const _sub = interp->vtables[enum_class_Sub]->whoami;

    if (old_const->type == PFC_PMC
            && VTABLE_isa(interp, old_const->u.key, _sub)) {
        PackFile_Constant *ret;
        PMC *old_sub;
        PMC *new_sub;
        ret = mem_allocate_typed(PackFile_Constant);

        ret->type = old_const->type;

        old_sub = old_const->u.key;
        new_sub = Parrot_thaw_constants(interp,
            Parrot_freeze(interp, old_sub));

        PMC_sub(new_sub)->seg = PMC_sub(old_sub)->seg;
        Parrot_store_sub_in_namespace(interp, new_sub);

        ret->u.key = new_sub;

        return ret;
    }
    else {
        return old_const;
    }
}

static PackFile_Constant **
find_constants(Interp *interp, PackFile_ConstTable *ct) {
    if (!n_interpreters || !interp->thread_data ||
            interp->thread_data->tid == 0) {
        return ct->constants;
    }
    else {
        Hash *tables;
        PackFile_Constant **new_consts;

        assert(interp->thread_data);

        if (!interp->thread_data->const_tables) {
            interp->thread_data->const_tables = mem_allocate_typed(Hash);
            parrot_new_pointer_hash(interp,
                                    &interp->thread_data->const_tables);
        }

        tables     = interp->thread_data->const_tables;
        new_consts = (PackFile_Constant **)parrot_hash_get(interp, tables, ct);

        if (!new_consts) {
            /* need to construct it */
            PackFile_Constant **old_consts;
            INTVAL i;
            INTVAL const num_consts = ct->const_count;

            old_consts = ct->constants;
            new_consts = (PackFile_Constant **)mem_sys_allocate(
                sizeof (PackFile_Constant *) * num_consts);

            for (i = 0; i < num_consts; ++i) {
                new_consts[i] = clone_constant(interp, old_consts[i]);
            }

            parrot_hash_put(interp, tables, ct, new_consts);
        }

        return new_consts;
    }
}

void
Parrot_destroy_constants(Interp *interp) {
    UINTVAL i;
    Hash *hash;
    if (!interp->thread_data) {
        return;
    }

    hash = interp->thread_data->const_tables;

    if (!hash) {
        return;
    }

    for (i = 0; i <= hash->mask; ++i) {
        HashBucket *bucket = hash->bi[i];
        while (bucket) {
            PackFile_ConstTable * const table      =
                (PackFile_ConstTable *)bucket->key;
            PackFile_Constant ** const orig_consts = table->constants;
            PackFile_Constant ** const consts      =
                (PackFile_Constant **) bucket->value;
            INTVAL const const_count               = table->const_count;
            INTVAL i;

            for (i = 0; i < const_count; ++i) {
                if (consts[i] != orig_consts[i]) {
                    mem_sys_free(consts[i]);
                }
            }
            mem_sys_free(consts);
            bucket = bucket->next;
        }
    }

    parrot_hash_destroy(interp, hash);
}

/*

=back

=head2 PackFile FixupTable Structure Functions

=over 4

=item C<void
PackFile_FixupTable_clear(Interp *, PackFile_FixupTable *self)>

Clear a PackFile FixupTable.

=cut

*/

void
PackFile_FixupTable_clear(Interp *interp, PackFile_FixupTable *self)
{
    opcode_t i;
    if (!self) {
        PIO_eprintf(NULL, "PackFile_FixupTable_clear: self == NULL!\n");
        return;
    }

    for (i = 0; i < self->fixup_count; i++) {
        mem_sys_free(self->fixups[i]->name);
        self->fixups[i]->name = NULL;
        mem_sys_free(self->fixups[i]);
        self->fixups[i] = NULL;
    }

    if (self->fixup_count) {
        mem_sys_free(self->fixups);
        self->fixups = NULL;
    }

    self->fixups = NULL;
    self->fixup_count = 0;

    return;
}

/*

=item C<static void
fixup_destroy(Interp*, PackFile_Segment *self)>

Just calls C<PackFile_FixupTable_clear()> with C<self>.

=cut

*/

static void
fixup_destroy(Interp *interp, PackFile_Segment *self)
{
    PackFile_FixupTable * const ft = (PackFile_FixupTable *)self;
    PackFile_FixupTable_clear(interp, ft);
}

/*

=item C<static size_t
fixup_packed_size(Interp*, PackFile_Segment *self)>

I<What does this do?>

=cut

*/

static size_t
fixup_packed_size(Interp *interp, PackFile_Segment *self)
{
    PackFile_FixupTable * const ft = (PackFile_FixupTable *)self;
    size_t size;
    opcode_t i;

    size = 1;    /* fixup_count */
    for (i = 0; i < ft->fixup_count; i++) {
        size++;  /* fixup_entry type */
        switch (ft->fixups[i]->type) {
            case enum_fixup_label:
            case enum_fixup_sub:
                size += PF_size_cstring(ft->fixups[i]->name);
                size ++; /* offset */
                break;
            case enum_fixup_none:
                break;
            default:
                internal_exception(1, "Unknown fixup type\n");
                return 0;
        }
    }
    return size;
}

/*

=item C<static opcode_t *
fixup_pack(Interp*, PackFile_Segment *self, opcode_t *cursor)>

I<What does this do?>

=cut

*/

static opcode_t *
fixup_pack(Interp *interp, PackFile_Segment *self, opcode_t *cursor)
{
    PackFile_FixupTable * const ft = (PackFile_FixupTable *)self;
    opcode_t i;

    *cursor++ = ft->fixup_count;
    for (i = 0; i < ft->fixup_count; i++) {
        *cursor++ = (opcode_t) ft->fixups[i]->type;
        switch (ft->fixups[i]->type) {
            case enum_fixup_label:
            case enum_fixup_sub:
                cursor = PF_store_cstring(cursor, ft->fixups[i]->name);
                *cursor++ = ft->fixups[i]->offset;
                break;
            case enum_fixup_none:
                break;
            default:
                internal_exception(1, "Unknown fixup type\n");
                return 0;
        }
    }
    return cursor;
}

/*

=item C<static PackFile_Segment *
fixup_new(Interp*, PackFile *pf, const char *name, int add)>

Returns a new C<PackFile_FixupTable> segment.

=cut

*/

static PackFile_Segment *
fixup_new(Interp *interp, PackFile *pf, const char *name, int add)
{
    PackFile_FixupTable * const fixup = mem_allocate_typed(PackFile_FixupTable);

    fixup->fixup_count                = 0;
    fixup->fixups                     = NULL;

    return (PackFile_Segment *) fixup;
}

/*

=item C<static opcode_t *
fixup_unpack(Interp *interp, PackFile_Segment *seg, opcode_t *cursor)>

Unpack a PackFile FixupTable from a block of memory.

Returns one (1) if everything is OK, else zero (0).

=cut

*/

static opcode_t *
fixup_unpack(Interp *interp, PackFile_Segment *seg, opcode_t *cursor)
{
    opcode_t i;
    PackFile *pf;
    PackFile_FixupTable * const self = (PackFile_FixupTable *)seg;

    if (!self) {
        PIO_eprintf(interp, "PackFile_FixupTable_unpack: self == NULL!\n");
        return 0;
    }

    PackFile_FixupTable_clear(interp, self);

    pf = self->base.pf;
    self->fixup_count = PF_fetch_opcode(pf, &cursor);

    if (self->fixup_count) {
        self->fixups = (PackFile_FixupEntry **)mem_sys_allocate_zeroed(
            self->fixup_count * sizeof (PackFile_FixupEntry *));

        if (!self->fixups) {
            PIO_eprintf(interp,
                    "PackFile_FixupTable_unpack: Could not allocate "
                    "memory for array!\n");
            self->fixup_count = 0;
            return 0;
        }
    }

    for (i = 0; i < self->fixup_count; i++) {
        PackFile_FixupEntry * const entry =
            self->fixups[i] =
            mem_allocate_typed(PackFile_FixupEntry);
        entry->type = PF_fetch_opcode(pf, &cursor);
        switch (entry->type) {
            case enum_fixup_label:
            case enum_fixup_sub:
                entry->name = PF_fetch_cstring(pf, &cursor);
                entry->offset = PF_fetch_opcode(pf, &cursor);
                break;
            case enum_fixup_none:
                break;
            default:
                PIO_eprintf(interp,
                        "PackFile_FixupTable_unpack: Unknown fixup type %d!\n",
                        entry->type);
                return 0;
        }
    }

    return cursor;
}

/*

=item C<void PackFile_FixupTable_new_entry(Interp *interp,
        char *label, enum_fixup_t type, opcode_t offs)>

I<What does this do?>

=cut

*/

void
PackFile_FixupTable_new_entry(Interp *interp,
        char *label, INTVAL type, opcode_t offs)
{
    PackFile_FixupTable *self = interp->code->fixups;
    opcode_t i;

    if (!self) {
        self = (PackFile_FixupTable *) PackFile_Segment_new_seg(
                interp,
                interp->code->base.dir, PF_FIXUP_SEG,
                FIXUP_TABLE_SEGMENT_NAME, 1);
        interp->code->fixups = self;
        self->code = interp->code;
    }
    i = self->fixup_count;
    self->fixup_count++;
    if (self->fixups) {
        self->fixups = (PackFile_FixupEntry **)mem_sys_realloc(
            self->fixups, self->fixup_count * sizeof (PackFile_FixupEntry *));
    }
    else
        self->fixups = mem_allocate_typed(PackFile_FixupEntry *);

    self->fixups[i]       = mem_allocate_typed(PackFile_FixupEntry);
    self->fixups[i]->type = type;
    self->fixups[i]->name = (char *)mem_sys_allocate(strlen(label) + 1);
    strcpy(self->fixups[i]->name, label);
    self->fixups[i]->offset = offs;
    self->fixups[i]->seg = self->code;
}

/*

=item C<static PackFile_FixupEntry *
find_fixup(PackFile_FixupTable *ft, INTVAL type, const char *name)>

Finds the fix-up entry for C<name> and returns it.

=cut

*/

static PackFile_FixupEntry *
find_fixup(PackFile_FixupTable *ft, INTVAL type, const char *name)
{
    opcode_t i;
    for (i = 0; i < ft->fixup_count; i++) {
        if ((INTVAL)((enum_fixup_t)ft->fixups[i]->type) == type &&
                !strcmp(ft->fixups[i]->name, name)) {
            ft->fixups[i]->seg = ft->code;
            return ft->fixups[i];
        }
    }
    return NULL;
}

/*

=item C<static INTVAL
find_fixup_iter(Interp*, PackFile_Segment *seg, void *user_data)>

I<What does this do?>

=cut

*/

static INTVAL
find_fixup_iter(Interp *interp, PackFile_Segment *seg, void *user_data)
{
    if (seg->type == PF_DIR_SEG) {
        if (PackFile_map_segments(interp, (PackFile_Directory *)seg,
                find_fixup_iter, user_data))
            return 1;
    }
    else if (seg->type == PF_FIXUP_SEG) {
        PackFile_FixupEntry ** const e = (PackFile_FixupEntry **)user_data;
        PackFile_FixupEntry * const fe = (PackFile_FixupEntry *)find_fixup(
                (PackFile_FixupTable *) seg, (*e)->type, (*e)->name);
        if (fe) {
            *e = fe;
            return 1;
        }
    }
    return 0;
}

/*

=item C<PackFile_FixupEntry *
PackFile_find_fixup_entry(Interp *interp, int type, char *name)>

I<What does this do?>

=cut

*/

PackFile_FixupEntry *
PackFile_find_fixup_entry(Interp *interp, INTVAL type, char *name)
{
    /* TODO make a hash of all fixups */
    PackFile_Directory *dir = interp->code->base.dir;
    PackFile_FixupEntry *ep, e;
    int found;

    e.type = type;
    e.name = name;
    ep = &e;
    found = PackFile_map_segments(interp, dir, find_fixup_iter,
            (void *) &ep);
    return found ? ep : NULL;
}

/*

=back

=head2 PackFile ConstTable Structure Functions

=over 4

=item C<void
PackFile_ConstTable_clear(Interp*, PackFile_ConstTable *self)>

Clear the C<PackFile_ConstTable> C<self>.

=cut

*/

void
PackFile_ConstTable_clear(Interp *interp, PackFile_ConstTable *self)
{
    opcode_t i;

    for (i = 0; i < self->const_count; i++) {
        PackFile_Constant_destroy(interp, self->constants[i]);
        self->constants[i] = NULL;
    }

    if (self->const_count) {
        mem_sys_free(self->constants);
    }

    self->constants = NULL;
    self->const_count = 0;

    return;
}

#if EXEC_CAPABLE
PackFile_Constant *exec_const_table;
#endif

/*

=item C<opcode_t *
PackFile_ConstTable_unpack(Interp *interp, PackFile_Segment *seg,
                           opcode_t *cursor)>

Unpack a PackFile ConstTable from a block of memory. The format is:

  opcode_t const_count
  *  constants

Returns cursor if everything is OK, else zero (0).

=cut

*/

opcode_t *
PackFile_ConstTable_unpack(Interp *interp, PackFile_Segment *seg,
                           opcode_t *cursor)
{
    opcode_t i;
    PackFile_ConstTable * const self = (PackFile_ConstTable *)seg;
    PackFile * const pf              = seg->pf;
#if EXEC_CAPABLE
    extern int Parrot_exec_run;
#endif

    PackFile_ConstTable_clear(interp, self);

    self->const_count = PF_fetch_opcode(pf, &cursor);

#if TRACE_PACKFILE
    PIO_eprintf(interp,
            "PackFile_ConstTable_unpack: Unpacking %ld constants\n",
            self->const_count);
#endif

    if (self->const_count == 0) {
        return cursor;
    }

    self->constants = (PackFile_Constant **)mem_sys_allocate_zeroed(
        self->const_count * sizeof (PackFile_Constant *));

    if (!self->constants) {
        PIO_eprintf(interp,
                "PackFile_ConstTable_unpack: Could not allocate "
                "memory for array!\n");
        self->const_count = 0;
        return 0;
    }

    for (i = 0; i < self->const_count; i++) {
#if TRACE_PACKFILE
        PIO_eprintf(interp,
                "PackFile_ConstTable_unpack(): Unpacking constant %ld\n", i);
#endif

#if EXEC_CAPABLE
        if (Parrot_exec_run)
            self->constants[i] = &exec_const_table[i];
        else
#endif
            self->constants[i] = PackFile_Constant_new(interp);

        cursor = PackFile_Constant_unpack(interp, self, self->constants[i],
                    cursor);
    }
    return cursor;
}

/*

=item C<static PackFile_Segment *
const_new(Interp*, PackFile *pf, const char *name, int add)>

Returns a new C<PackFile_ConstTable> segment.

=cut

*/

static PackFile_Segment *
const_new(Interp *interp, PackFile *pf, const char *name, int add)
{
    PackFile_ConstTable *const_table;

    const_table = mem_allocate_typed(PackFile_ConstTable);

    const_table->const_count = 0;
    const_table->constants = NULL;

    return (PackFile_Segment *)const_table;
}

/*

=item C<static void
const_destroy(Interp*, PackFile_Segment *self)>

Destroys the C<PackFile_ConstTable> C<self>.

=cut

*/

static void
const_destroy(Interp *interp, PackFile_Segment *self)
{
    PackFile_ConstTable * const ct = (PackFile_ConstTable *)self;

    PackFile_ConstTable_clear(interp, ct);
}

/*

=back

=head2 PackFile Constant Structure Functions

=over 4

=item C<PackFile_Constant *
PackFile_Constant_new(Interp*)>

Allocate a new empty PackFile Constant.

This is only here so we can make a new one and then do an unpack.

=cut

*/

PackFile_Constant *
PackFile_Constant_new(Interp *interp)
{
    PackFile_Constant * const self =
        mem_allocate_zeroed_typed(PackFile_Constant);

    self->type = PFC_NONE;

    return self;
}

/*

=item C<void
PackFile_Constant_destroy(Interp*, PackFile_Constant *self)>

Delete the C<PackFile_Constant> C<self>.

Don't delete C<PMC>s or C<STRING>s, they are destroyed via DOD/GC.

=cut

*/

void
PackFile_Constant_destroy(Interp *interp, PackFile_Constant *self)
{
    mem_sys_free(self);
}

/*

=item C<size_t
PackFile_Constant_pack_size(Interp*, PackFile_Constant *self)>

Determine the size of the buffer needed in order to pack the PackFile
Constant into a contiguous region of memory.

=cut

*/

size_t
PackFile_Constant_pack_size(Interp *interp, PackFile_Constant *self)
{
    size_t packed_size;
    PMC *component;
    STRING *image;

    switch (self->type) {

        case PFC_NUMBER:
            packed_size = PF_size_number();
            break;

        case PFC_STRING:
            packed_size = PF_size_string(self->u.string);
            break;

        case PFC_KEY:
            packed_size = 1;

            for (component = self->u.key; component;
                    component = (PMC *)PMC_data(component))
                packed_size += 2;
            break;

        case PFC_PMC:
            component = self->u.key; /* the pmc (Sub, ...) */

            /*
             * TODO create either
             * a) a frozen_size freeze entry or
             * b) change packout.c so that component size isn't needed
             */
            image = Parrot_freeze(interp, component);
            packed_size = PF_size_string(image);
            break;

        default:
            PIO_eprintf(NULL,
                    "Constant_packed_size: Unrecognized type '%c'!\n",
                    (char)self->type);
            return 0;
    }

    /* Tack on space for the initial type field */
    return packed_size + 1;
}

/*

=item C<opcode_t *
PackFile_Constant_unpack(Interp *interp,
                         PackFile_ConstTable *constt,
                         PackFile_Constant *self, opcode_t *cursor)>

Unpack a PackFile Constant from a block of memory. The format is:

  opcode_t type
  *  data

Returns cursor if everything is OK, else zero (0).

=cut

*/

opcode_t *
PackFile_Constant_unpack(Interp *interp,
                         PackFile_ConstTable *constt,
                         PackFile_Constant *self, opcode_t *cursor)
{
    PackFile * const pf = constt->base.pf;
    const opcode_t type = PF_fetch_opcode(pf, &cursor);

/* #define TRACE_PACKFILE 1 */
#if TRACE_PACKFILE
    PIO_eprintf(NULL, "PackFile_Constant_unpack(): Type is %ld ('%c')...\n",
            type, (char)type);
#endif

    switch (type) {
    case PFC_NUMBER:
        self->u.number = PF_fetch_number(pf, &cursor);
        self->type = PFC_NUMBER;
        break;

    case PFC_STRING:
        self->u.string = PF_fetch_string(interp, pf, &cursor);
        self->type = PFC_STRING;
        break;

    case PFC_KEY:
        cursor = PackFile_Constant_unpack_key(interp, constt,
                self, cursor);
        break;

    case PFC_PMC:
        cursor = PackFile_Constant_unpack_pmc(interp, constt,
                self, cursor);
        break;
    default:
        PIO_eprintf(NULL,
                "Constant_unpack: Unrecognized type '%c' during unpack!\n",
                (char)type);
        return 0;
    }
    return cursor;
}

/*

=item C<opcode_t *
PackFile_Constant_unpack_pmc(Interp *interp, PackFile_ConstTable *constt,
                         PackFile_Constant *self, opcode_t *cursor)>

Unpack a constant PMC.

=cut

*/

opcode_t *
PackFile_Constant_unpack_pmc(Interp *interp, PackFile_ConstTable *constt,
                             PackFile_Constant *self, opcode_t *cursor)
{
    PackFile * const pf = constt->base.pf;
    STRING *image, *_sub;
    PMC *pmc;

    /*
     * thawing the PMC needs the real packfile in place
     */
    PackFile_ByteCode * const cs_save = interp->code;
    interp->code = pf->cur_cs;

    image = PF_fetch_string(interp, pf, &cursor);
    /*
     * TODO use thaw_constants
     * current issue: a constant Sub with attached properties
     *                doesn't DOD mark the properties
     * for a constant PMC *all* contents have to be in the constant pools
     */
    pmc = Parrot_thaw(interp, image);
    /*
     * place item in const_table
     */
    self->type = PFC_PMC;
    self->u.key = pmc;

    _sub = const_string(interp, "Sub");    /* CONST_STRING */
    if (VTABLE_isa(interp, pmc, _sub)) {
        /*
         * finally place the sub into some namespace stash
         * XXX place this code in Sub.thaw ?
         */
        Parrot_store_sub_in_namespace(interp, pmc);
    }
    /*
     * restore code
     */
    interp->code = cs_save;
    return cursor;
}

/*

=item C<opcode_t *
PackFile_Constant_unpack_key(Interp *interp, PackFile_ConstTable *constt,
                             PackFile_Constant *self, opcode_t *cursor)>

Unpack a PackFile Constant from a block of memory. The format consists
of a sequence of key atoms, each with the following format:

  opcode_t type
  opcode_t value

Returns cursor if everything is OK, else zero (0).

=cut

*/

opcode_t *
PackFile_Constant_unpack_key(Interp *interp, PackFile_ConstTable *constt,
                             PackFile_Constant *self, opcode_t *cursor)
{
    PMC *head;
    PMC *tail;
    opcode_t type, op, slice_bits;
    PackFile * const pf = constt->base.pf;
    int pmc_enum = enum_class_Key;

    INTVAL components = (INTVAL)PF_fetch_opcode(pf, &cursor);
    head = tail = NULL;

    while (components-- > 0) {
        type = PF_fetch_opcode(pf, &cursor);
        slice_bits = type & PF_VT_SLICE_BITS;
        type &= ~PF_VT_SLICE_BITS;
        if (!head && slice_bits) {
            pmc_enum = enum_class_Slice;
        }
        if (tail) {
            PMC_data(tail)
                = constant_pmc_new_noinit(interp, pmc_enum);
            tail = (PMC *)PMC_data(tail);
        }
        else {
            head = tail = constant_pmc_new_noinit(interp, pmc_enum);
        }

        VTABLE_init(interp, tail);

        op = PF_fetch_opcode(pf, &cursor);
        switch (type) {
        case PARROT_ARG_IC:
            key_set_integer(interp, tail, op);
            break;
        case PARROT_ARG_NC:
            key_set_number(interp, tail, constt->constants[op]->u.number);
            break;
        case PARROT_ARG_SC:
            key_set_string(interp, tail, constt->constants[op]->u.string);
            break;
        case PARROT_ARG_I:
            key_set_register(interp, tail, op, KEY_integer_FLAG);
            break;
        case PARROT_ARG_N:
            key_set_register(interp, tail, op, KEY_number_FLAG);
            break;
        case PARROT_ARG_S:
            key_set_register(interp, tail, op, KEY_string_FLAG);
            break;
        case PARROT_ARG_P:
            key_set_register(interp, tail, op, KEY_pmc_FLAG);
            break;
        default:
            return 0;
        }
        if (slice_bits) {
            if (slice_bits & PF_VT_START_SLICE)
                PObj_get_FLAGS(tail) |= KEY_start_slice_FLAG;
            if (slice_bits & PF_VT_END_SLICE)
                PObj_get_FLAGS(tail) |= KEY_end_slice_FLAG;
            if (slice_bits & (PF_VT_START_ZERO | PF_VT_END_INF))
                PObj_get_FLAGS(tail) |= KEY_inf_slice_FLAG;
        }
    }

    self->type = PFC_KEY;
    self->u.key = head;

    return cursor;
}

/*

=item C<static PackFile *
PackFile_append_pbc(Interp *interp, const char *filename)>

Read a PBC and append it to the current directory
Fixup sub addresses in newly loaded bytecode and run :load subs.

=cut

*/

static PackFile *
PackFile_append_pbc(Interp *interp, const char *filename)
{
    PackFile * const pf = Parrot_readbc(interp, filename);
    if (!pf)
        return NULL;
    PackFile_add_segment(interp, &interp->initial_pf->directory,
            &pf->directory.base);
    do_sub_pragmas(interp, pf->cur_cs, PBC_LOADED, NULL);
    return pf;
}

/*

=item C<void
Parrot_load_bytecode(Interp *interp, STRING *filename)>

Load and append a bytecode, IMC or PASM file into interpreter.

Load some bytecode (PASM, PIR, PBC ...) and append it to the current
directory.

=cut

*/

/*
 * intermediate hook during changes
 */
/* XXX Declare this elsewhere */
void * IMCC_compile_file(Parrot_Interp interp, const char *s);

void
Parrot_load_bytecode(Interp *interp, STRING *file_str)
{
    char *filename;
    STRING *wo_ext, *ext, *pbc, *path;
    enum_runtime_ft file_type;
    PMC *is_loaded_hash;

    parrot_split_path_ext(interp, file_str, &wo_ext, &ext);
    /* check if wo_ext is loaded */
    is_loaded_hash = VTABLE_get_pmc_keyed_int(interp,
        interp->iglobals, IGLOBALS_PBC_LIBS);
    if (VTABLE_exists_keyed_str(interp, is_loaded_hash, wo_ext))
        return;
    pbc = const_string(interp, "pbc");
    if (string_equal(interp, ext, pbc) == 0)
        file_type = PARROT_RUNTIME_FT_PBC;
    else
        file_type = PARROT_RUNTIME_FT_SOURCE;

    path = Parrot_locate_runtime_file_str(interp, file_str, file_type);
    if (!path) {
        real_exception(interp, NULL, E_LibraryNotLoadedError,
                "\"load_bytecode\" couldn't find file '%Ss'", file_str);
        return;
    }
    /* remember wo_ext => full_path mapping */
    VTABLE_set_string_keyed_str(interp, is_loaded_hash,
            wo_ext, path);
    filename = string_to_cstring(interp, path);
    if (file_type == PARROT_RUNTIME_FT_PBC) {
        PackFile_append_pbc(interp, filename);
    }
    else {
        STRING *err;
        PackFile_ByteCode * const cs =
            (PackFile_ByteCode *)IMCC_compile_file_s(interp,
                filename, &err);
        if (cs) {
            do_sub_pragmas(interp, cs, PBC_LOADED, NULL);
        }
        else
            real_exception(interp, NULL, E_LibraryNotLoadedError,
                "compiler returned NULL ByteCode '%Ss' - %Ss", file_str, err);
    }
    string_cstring_free(filename);
}

/*

=item C<void
PackFile_fixup_subs(Interp *interp, pbc_action_enum_t what, PMC *eval)>

Run :load or :immediate subroutines for the current code segment.
If C<eval> is given, set this is the owner of the subroutines.

=cut

*/

void
PackFile_fixup_subs(Interp *interp, pbc_action_enum_t what, PMC *eval)
{
    do_sub_pragmas(interp, interp->code, what, eval);
}

/*

=back

=head1 HISTORY

Rework by Melvin; new bytecode format, make bytecode portable. (Do
endian conversion and wordsize transforms on the fly.)

leo applied and modified Juergen Boemmels packfile patch giving an
extensible packfile format with directory reworked again, with common
chunks (C<default_*>).

2003.11.21 leo: moved low level item fetch routines to new
F<pf/pf_items.c>

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
