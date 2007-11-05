/*
Copyright (C) 2001-2007, The Perl Foundation.
$Id$

=head1 NAME

src/io/io.c - Generic IO

=head1 DESCRIPTION

The Parrot IO subsystem uses a per-interpreter stack to provide a layer-based
approach to IO. Each layer implements a subset of the C<ParrotIOLayerAPI>
vtable. To find an IO function, Parrot searches the layer stack downwards until
it finds a non-C<NULL> function pointer for that particular slot.

This file implements the generic functionality. Specific layers are in
separate files: F<src/io/io_buf.c>, F<src/io/io_stdio.c>, F<src/io/io_unix.c>,
F<src/io/io_win32.c>, and F<src/io/io_layers.c>.

The C<ParrotIO> PMC provides the class-based interface that is used in
Parrot ops. The C<ParrotIO struct> is defined in F<src/io/io_private.h>.

=head2 Resource Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "io_private.h"

#include <stdarg.h>

/* HEADERIZER HFILE: include/parrot/io.h */

/* This is list of valid layers */
ParrotIOLayer **pio_registered_layers = NULL;

/* This is the default stack used for IO. Copy this to each new interp */
/*
ParrotIOLayer   * pio_default_stack;
*/

/*
        The standard streams are:

                interp->piodata->table[PIO_STD*_FILENO].
*/


PIOOFF_T piooffsetzero;

/*

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
new_io_pmc(PARROT_INTERP, NULLOK(ParrotIO *io))>

Creates and returns a new C<ParrotIO> PMC.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
new_io_pmc(PARROT_INTERP, NULLOK(ParrotIO *io))
{
    PMC * const new_pmc = pmc_new(interp, enum_class_ParrotIO);
    PMC_data(new_pmc) = io;
    PMC_struct_val(new_pmc) = io ? io->stack : NULL;
    return new_pmc;
}

/*

=item C<PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING *
PIO_make_io_string(PARROT_INTERP, NOTNULL(STRING **buf), size_t len)>

Creates a STRING* suitable for returning results from IO read functions.
The passed in C<buf> parameter can:

=over 4

=item 1

Point to a NULL STRING

=item 2

Point to a real STRING

=item 3

Point to a fake STRING with (strstart, bufused) holding the *buffer
information.

=back

In the third case, the buffer or STRING must be able to hold the required
amount of data. For cases 1 and 2, a NULL C<strstart> tells this function to
allocate the STRING memory.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING *
PIO_make_io_string(PARROT_INTERP, NOTNULL(STRING **buf), size_t len)
{
    STRING *s;
    /*
     * when we get a NULL string, we read a default len
     */
    if (*buf == NULL) {
        *buf = string_make_empty(interp, enum_stringrep_one, len);
        return *buf;
    }
    s = *buf;
    if (s->bufused < len)
        Parrot_allocate_string(interp, s, len);
    return s;
}


/*

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
ParrotIO *
PIO_new(PARROT_INTERP, SHIM(INTVAL iotype), INTVAL flags, INTVAL mode)>

Creates a new IO stream.

The values of C<flags> and C<mode> are set in the returned C<ParrotIO>.

Currently C<iotype> is unused.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
ParrotIO *
PIO_new(PARROT_INTERP, SHIM(INTVAL iotype), INTVAL flags, INTVAL mode)
{
    ParrotIO * const new_io = (ParrotIO *)mem_sys_allocate(sizeof (ParrotIO));

    new_io->fpos = new_io->lpos = piooffsetzero;
    new_io->flags = flags;
    new_io->mode = mode;
    new_io->stack = interp->piodata->default_stack;
    new_io->b.flags = 0;
    new_io->b.size = 0;
    new_io->b.startb = NULL;
    new_io->b.endb = NULL;
    new_io->b.next = NULL;
    return new_io;
}

/*

=item C<PARROT_API
void
PIO_destroy(SHIM_INTERP, NOTNULL(PMC *pmc))>

Destroys the IO stream.  At the moment, this only frees the memory and removes
the pointers from the PMC.

=cut

*/

PARROT_API
void
PIO_destroy(SHIM_INTERP, NOTNULL(PMC *pmc))
{
    ParrotIO * const io = (ParrotIO *)PMC_data0(pmc);

    if (!io)
        return;
    if (io->b.startb && (io->b.flags & PIO_BF_MALLOC)) {
        mem_sys_free(io->b.startb);
        io->b.startb = 0;
    }
#if 0
    /*
     * PIO_destroy is called by PIO_close
     * XXX can't munmap now
     */
    if (io->b.startb && (io->b.flags & PIO_BF_MMAP)) {
#  ifdef PARROT_HAS_HEADER_SYSMMAN
        munmap((void*)io->b.startb, io->b.size);
#  endif
        io->b.startb = io->b.endb = NULL;
        io->b.size = 0;
    }
#endif
    if ((io->stack->flags & PIO_L_LAYER_COPIED)) {
        ParrotIOLayer *p;
        for (p = io->stack; p;) {
            ParrotIOLayer *down;
            /* if top got copied, all have to be malloced */
            PARROT_ASSERT(p->flags & PIO_L_LAYER_COPIED);
            down = p->down;
            if (p->api->Delete)
                (*p->api->Delete) (p);
            p = down;
        }
    }
    mem_sys_free(io);
    PMC_data(pmc) = NULL;
    PMC_struct_val(pmc) = NULL;
}

/*

=item C<PARROT_API
void
PIO_init(PARROT_INTERP)>

Sets up the interpreter's layer stack and creates the C<STD*> handles.

Called when creating an interpreter.

=cut

*/

PARROT_API
void
PIO_init(PARROT_INTERP)
{
    /* Has interp been initialized already? */
    if (interp->piodata) {
        /* memsub system is up and running: */
        /* Init IO stacks and handles for interp instance.  */
        if (PIO_init_stacks(interp) != 0) {
            real_exception(interp, NULL, PIO_ERROR, "PIO init stacks failed.");
        }

        /*
         * see also #36677
         *
         */
        if (!_PIO_STDIN(interp)) {
            _PIO_STDIN(interp) = pmc_new(interp, enum_class_Undef);
        }
        if (!_PIO_STDOUT(interp)) {
            _PIO_STDOUT(interp) = pmc_new(interp, enum_class_Undef);
        }
        if (!_PIO_STDERR(interp)) {
            _PIO_STDERR(interp) = pmc_new(interp, enum_class_Undef);
        }

        if (Interp_debug_TEST(interp, PARROT_START_DEBUG_FLAG)) {
            PIO_eprintf(NULL, "PIO: IO system initialized.\n");
        }

        return;
    }


    interp->piodata = mem_allocate_typed(ParrotIOData);
    if (interp->piodata == NULL)
        real_exception(interp, NULL, PIO_ERROR, "PIO alloc piodata failure.");
    interp->piodata->default_stack = NULL;
    interp->piodata->table = (PMC **)mem_sys_allocate_zeroed(PIO_NR_OPEN * sizeof (ParrotIO *));
    if (interp->piodata->table == NULL)
        real_exception(interp, NULL, PIO_ERROR, "PIO alloc table failure.");

}

/*

=item C<PARROT_API
void
PIO_finish(PARROT_INTERP)>

Closes the interpreter's IO resourses.  Called during its interpreter
destruction.

=cut

*/

PARROT_API
void
PIO_finish(PARROT_INTERP)
{
    ParrotIOLayer *layer;
#if 0
    PMC *pmc;
    ParrotIO *io;
    int i;

    /* When here, the PMC arena is already destroyed, these PMCs
     * aren't usable any more.
     * But ParrotIO::destroy should have flushed/closed all PIOs
     * already. If that's not quite true, we have to separate IO subsys
     * destruction into two parts (again).
     */
    for (i = 0 ; i < PIO_NR_OPEN; i++) {
        if ((pmc = interp->piodata->table[i])) {
            layer = PMC_struct_val(pmc);
            io = PMC_data(pmc);
            PIO_close_down(interp, layer, io);
        }
    }
#endif
    /*
     * TODO free IO of std-handles
     */
    for (layer = interp->piodata->default_stack; layer;) {
        ParrotIOLayer * const down = layer->down;
        if (layer->api->Delete)
            (*layer->api->Delete) (layer);
        layer = down;
    }
    mem_sys_free(interp->piodata->table);
    interp->piodata->table = NULL;
    mem_sys_free(interp->piodata);
    interp->piodata = NULL;

}

/*

=item C<PARROT_API
void
PIO_internal_shutdown(SHIM_INTERP)>

IO system destructor, called on destruction of the last interpreter.

=cut

*/

PARROT_API
void
PIO_internal_shutdown(SHIM_INTERP)
{
    mem_sys_free(pio_registered_layers);
    pio_registered_layers = NULL;
}

/*

=item C<PARROT_API
INTVAL
PIO_init_stacks(PARROT_INTERP)>

Initializes the interpreter's default IO stack by pushing on the IO
layers (OS-specific first).

=cut

*/

PARROT_API
INTVAL
PIO_init_stacks(PARROT_INTERP)
{
    ParrotIOLayer *p, *bottom = NULL;
    int fill, n, i;

    /* First push the platform specific OS layer */
    /* Optimize this to keep a default stack and just
     * call copy stack.
     */
#ifdef PIO_OS_UNIX
    PIO_push_layer(interp, PMCNULL, PIO_base_new_layer(&pio_unix_layer));
#endif
#ifdef PIO_OS_WIN32
    PIO_push_layer(interp, PMCNULL, PIO_base_new_layer(&pio_win32_layer));
#endif
#ifdef PIO_OS_STDIO
    PIO_push_layer(interp, PMCNULL, PIO_base_new_layer(&pio_stdio_layer));
#endif
    PIO_push_layer(interp, PMCNULL, PIO_base_new_layer(&pio_buf_layer));

    fill = 0;
    if (!pio_registered_layers) {
        n = 5;  /* 2 default layers for now + utf8, mmap, string */
        pio_registered_layers = (ParrotIOLayer **)mem_sys_allocate(
                sizeof (ParrotIOLayer *) * (n + 1));
        fill = 1;
    }

    /* Note: All layer pushes should be done before init calls */
    for (i = 0, p = interp->piodata->default_stack; p; p = p->down) {
        bottom = p;
        if (fill) {
            PARROT_ASSERT(i < n); /* XXX n can be undefined at this point. */
            pio_registered_layers[i++] = p;
            pio_registered_layers[i] = NULL;
        }
    }
    /*
     * Init calls where done top down, which seem quite wrong  - lower
     * levels need first to open e.g. STD*. Then the buffered layer can
     * set linebuffering or such
     */
    for (p = bottom; p; p = p->up) {
        if (p->api->Init) {
            if ((*p->api->Init) (interp, p) != 0) {
                /* ignore err
                 * see also #36677
                char buf[1024];
                sprintf(buf, "Parrot IO: Failed init layer(%s).\n", p->name);
                real_exception(interp, NULL, PIO_ERROR, buf);
                */
                ;
            }
        }
    }
    if (fill) {
        PARROT_ASSERT(i == 2);
        PARROT_ASSERT(pio_registered_layers[2] == NULL);
        pio_registered_layers[2] = PIO_utf8_register_layer();
        pio_registered_layers[3] = PIO_mmap_register_layer();
        pio_registered_layers[4] = PIO_string_register_layer();
        pio_registered_layers[5] = NULL;
    }

    return 0;
}

/*

=item C<PARROT_API
INTVAL
PIO_base_init(SHIM_INTERP, SHIM(ParrotIOLayer *l))>

Init routine called once for each layer at interpreter creation time.
This is similar to a Perl module C<INIT {}> block.

This default implementation does nothing and returns C<0>.

=cut

*/

PARROT_API
INTVAL
PIO_base_init(SHIM_INTERP, SHIM(ParrotIOLayer *l))
{
    return 0;
}


/*

=back

=head2 Generic top-level C<ParrotIO> interface

=over 4

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL
PIO_parse_open_flags(NULLOK(const char *flagstr))>

Parses C<*flagstr> for Perl-style file open mode flags (C<< < >>, C<< > >>,
C<<< >> >>>, C<< +< >>, C<< +> >>) and returns the combined generic bit flags.

The low level OS layers may then interpret the generic bits differently
depending on platform.

XXX BD Should this be static?

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL
PIO_parse_open_flags(NULLOK(const char *flagstr))
{
    INTVAL flags;
    const char *s;

    if (!flagstr || !(*flagstr))
        return 0;
    flags = 0;
    s = flagstr;
    /* Set mode flags - <, >, >>, +<, +> */
    switch (*s++) {
        case '+':
            flags |= (PIO_F_WRITE | PIO_F_READ);
            switch (*s++) {
                case '<':
                    break;
                case '>':
                    flags |= PIO_F_TRUNC;
                    break;
                default:
                    return 0;
            }
            break;
        case '<':
            flags |= PIO_F_READ;
            break;
        case '>':
            flags |= PIO_F_WRITE;
            if (*s == '>') {
                flags |= PIO_F_APPEND;
                s++;
            }
            else {
                flags |= PIO_F_TRUNC;
            }
            break;
        case '-':       /* -| read from pipe */
            if (*s == '|') {
                flags |= PIO_F_PIPE | PIO_F_READ;
                s++;
            }
            break;
        case '|':       /* |- write to pipe */
            if (*s == '-') {
                flags |= PIO_F_PIPE | PIO_F_WRITE;
                s++;
            }
            break;
        default:
            return 0;
    }

    /* TODO: add ? and ! for block/non-block */
    switch (*s++) {
        case '\0':
            /* No extra arguments */
            break;
        default:
            return 0;
    }

    return flags;
}

/*

=item C<PARROT_API
INTVAL
PIO_peek(PARROT_INTERP, NOTNULL(PMC *pmc), NOTNULL(STRING **buffer))>

Iterates down the stack to the first layer implementing "Peek" API.

=cut

*/

PARROT_API
INTVAL
PIO_peek(PARROT_INTERP, NOTNULL(PMC *pmc), NOTNULL(STRING **buffer))
{
    ParrotIOLayer * const l = (ParrotIOLayer *)PMC_struct_val(pmc);
    ParrotIO * const io = (ParrotIO *)PMC_data0(pmc);
    if (!io)
        return -1;
    return PIO_peek_down(interp, l, io, buffer);
}



/*

=item C<PARROT_API
INTVAL
PIO_pioctl(PARROT_INTERP, NOTNULL(PMC *pmc), INTVAL cmd, INTVAL arg)>

General purpose interface for manipulating IO objects and layer attributes.

Refer to the C<PIOCTL*> values in F<include/parrot/io.h>.

All C<set> operations return C<0> on success and a negative value on error.
C<get> operations use the return value as the value requested, but should
always be C<< >= 0 >>. A negative value indicates an error. This may be too
limited, but we will see. --Melvin

=cut

*/

PARROT_API
INTVAL
PIO_pioctl(PARROT_INTERP, NOTNULL(PMC *pmc), INTVAL cmd, INTVAL arg)
{

    ParrotIO * const io = (ParrotIO *)PMC_data0(pmc);
    ParrotIOBuf * b;
    if (!io)
        return -1;
    b = &io->b;

    switch (cmd) {
       case PIOCTL_CMDSETRECSEP: io->recsep = arg;

       case PIOCTL_CMDGETRECSEP: return io->recsep;

       case PIOCTL_CMDSETBUFTYPE:
            if (arg == PIOCTL_NONBUF)
               return PIO_setbuf(interp, pmc, 0);
            else if (arg == PIOCTL_LINEBUF)
               return PIO_setlinebuf(interp, pmc);
            else if (arg == PIOCTL_BLKBUF)
               return PIO_setbuf(interp, pmc, PIO_UNBOUND);
            else return -1;

       case PIOCTL_CMDGETBUFTYPE:
            if (io->flags & PIO_F_LINEBUF) return PIOCTL_LINEBUF;
            if (io->flags & PIO_F_BLKBUF) return PIOCTL_BLKBUF;
            return PIOCTL_NONBUF;

       case PIOCTL_CMDSETBUFSIZE:
            return PIO_setbuf(interp, pmc, arg);

       case PIOCTL_CMDGETBUFSIZE:
             if (b) return b->size;
             else return -1;
       default: return -1;
    }
}

/*

=item C<PARROT_API
INTVAL
PIO_setbuf(PARROT_INTERP, NOTNULL(PMC *pmc), size_t bufsize)>

Sets the buffer size for C<*pmc> to C<bufsize>. Returns C<0> if the
buffering was enabled.

=cut

*/

PARROT_API
INTVAL
PIO_setbuf(PARROT_INTERP, NOTNULL(PMC *pmc), size_t bufsize)
{
    ParrotIOLayer * const layer = (ParrotIOLayer *)PMC_struct_val(pmc);
    ParrotIO * const io = (ParrotIO *)PMC_data0(pmc);
    if (!io)
        return -1;
    PIO_flush(interp, pmc);
    return PIO_setbuf_down(interp, layer, io, bufsize);
}

/*

=item C<PARROT_API
INTVAL
PIO_setlinebuf(PARROT_INTERP, NOTNULL(PMC *pmc))>

Enables line buffering for C<*pmc>. Returns C<0> if line buffering was
successfully set, or already enabled.

=cut

*/

PARROT_API
INTVAL
PIO_setlinebuf(PARROT_INTERP, NOTNULL(PMC *pmc))
{
    ParrotIOLayer * const l = (ParrotIOLayer *)PMC_struct_val(pmc);
    ParrotIO * const io = (ParrotIO *)PMC_data0(pmc);
    if (!io)
        return -1;

    return PIO_setlinebuf_down(interp, l, io);
}

/*

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
PIO_open(PARROT_INTERP, NULLOK(ParrotIOLayer *layer), NOTNULL(const char *spath),
         NOTNULL(const char *sflags))>

Creates and returns a C<ParrotIO> PMC for C<*spath>.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
PIO_open(PARROT_INTERP, NULLOK(ParrotIOLayer *layer), NOTNULL(const char *spath),
         NOTNULL(const char *sflags))
{
    ParrotIO *io;
    const INTVAL flags = PIO_parse_open_flags(sflags);

    if (!layer) {
        layer = interp->piodata->default_stack;
    }

    io = PIO_open_down(interp, layer, spath, flags);
    /* io could be null here but we still have to
     * to create a PMC for the caller, no PMCNULL here
     * as that would cause an exception upon access.
     */
    if (io) {
        io->stack = layer;
    }

    return new_io_pmc(interp, io);
}

/*

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
PIO_fdopen(PARROT_INTERP, NULLOK(ParrotIOLayer *layer), PIOHANDLE fd, NOTNULL(const char *sflags))>

Creates and returns a C<ParrotIO> PMC for C<*spath> on an existing, open
file descriptor.

This is used particularly to initialize the C<STD*> IO handles onto the
OS IO handles (0,1,2).

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
PIO_fdopen(PARROT_INTERP, NULLOK(ParrotIOLayer *layer), PIOHANDLE fd, NOTNULL(const char *sflags))
{
    ParrotIO *io;
    INTVAL flags;

    if (!layer) {
        layer = interp->piodata->default_stack;
    }

    flags = PIO_parse_open_flags(sflags);
    if (!flags)
        return NULL;

    io = PIO_fdopen_down(interp, layer, fd, flags);
    /* io could be null here but we still have to
     * to create a PMC for the caller, no PMCNULL here
     * as that would cause an exception upon access.
     */
    if (io) {
        io->stack = layer;
    }

    return new_io_pmc(interp, io);
}

/*

=item C<PARROT_API
INTVAL
PIO_close(PARROT_INTERP, NULLOK(PMC *pmc))>

Flushes, closes, and destroys the C<ParrotIO> PMC C<*pmc>.

=cut

*/

PARROT_API
INTVAL
PIO_close(PARROT_INTERP, NULLOK(PMC *pmc))
{
    INTVAL res;
    ParrotIOLayer * const l = (ParrotIOLayer *)PMC_struct_val(pmc);
    ParrotIO * const io = (ParrotIO *)PMC_data0(pmc);
    if (!io)
        return -1;
    PIO_flush(interp, pmc); /* XXX boe: is this neccessary here? */
    res =  PIO_close_down(interp, l, io);
    PIO_destroy(interp, pmc);

    return res;
}

/*

=item C<PARROT_API
void
PIO_flush(PARROT_INTERP, NOTNULL(PMC *pmc))>

Flushes the C<ParrotIO> PMC C<*pmc>.

=cut

*/

PARROT_API
void
PIO_flush(PARROT_INTERP, NOTNULL(PMC *pmc))
{
    ParrotIOLayer * const l  = (ParrotIOLayer *)PMC_struct_val(pmc);
    ParrotIO      * const io = (ParrotIO *)PMC_data0(pmc);
    INTVAL                ignored;

    if (!io)
        return;

    ignored = PIO_flush_down(interp, l, io);
    UNUSED(ignored);
}

/*

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING *
PIO_reads(PARROT_INTERP, NOTNULL(PMC *pmc), size_t len)>

Return a new C<STRING*> holding up to C<len> bytes.

=cut

*/


PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING *
PIO_reads(PARROT_INTERP, NOTNULL(PMC *pmc), size_t len)
{
    STRING               *res = NULL;
    ParrotIOLayer * const l   = (ParrotIOLayer *)PMC_struct_val(pmc);
    ParrotIO      * const io  = (ParrotIO *)PMC_data0(pmc);
    INTVAL                ignored;

    if (!io)
        return new_string_header(interp, 0);

    if (io->b.flags & PIO_BF_MMAP) {
        res           = new_string_header(interp, 0);
        res->charset  = Parrot_iso_8859_1_charset_ptr;   /* XXX binary */
        res->encoding = Parrot_fixed_8_encoding_ptr;
    }
    else
        res = PIO_make_io_string(interp, &res, len);

    res->bufused = len;
    ignored      = PIO_read_down(interp, l, io, &res);
    UNUSED(ignored);

    return res;
}

/*

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL
PIO_read(PARROT_INTERP, NOTNULL(PMC *pmc), NOTNULL(char *buffer), size_t len)>

Reads up to C<len> bytes from C<*pmc> and copies them into C<*buffer>.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL
PIO_read(PARROT_INTERP, NOTNULL(PMC *pmc), NOTNULL(char *buffer), size_t len)
{
    ParrotIOLayer * const l = (ParrotIOLayer *)PMC_struct_val(pmc);
    ParrotIO * const io = (ParrotIO *)PMC_data0(pmc);
    STRING *res = new_string_header(interp, 0);
    if (!io)
        return -1;

    res->strstart = buffer;
    res->bufused = len;
    return PIO_read_down(interp, l, io, &res);
}

/*

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL
PIO_write(PARROT_INTERP, NOTNULL(PMC *pmc), NOTNULL(const void *buffer), size_t len)>

Writes C<len> bytes from C<*buffer> to C<*pmc>.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL
PIO_write(PARROT_INTERP, NOTNULL(PMC *pmc), NOTNULL(const void *buffer), size_t len)
{
    ParrotIOLayer * const l = (ParrotIOLayer *)PMC_struct_val(pmc);
    ParrotIO * const io = (ParrotIO *)PMC_data0(pmc);
    DECL_CONST_CAST;

    if (!io)
        return -1;

    if (io->flags & PIO_F_WRITE) {
        STRING fake;
        /* TODO skip utf8 translation layers if any */
        fake.strstart = (char *)const_cast(buffer);
        fake.strlen = fake.bufused = len;
        fake.charset = Parrot_default_charset_ptr;
        fake.encoding = Parrot_default_encoding_ptr;
        return PIO_write_down(interp, l, io, &fake);
    }
    else
        return 0;
}

/*

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
PIOOFF_T
PIO_seek(PARROT_INTERP, NOTNULL(PMC *pmc), PIOOFF_T offset, INTVAL w)>

Moves the read/write position of C<*pmc> to offset C<bytes> from the
position indicated by C<w>. Typically C<w> will be C<0> for the start of
the file, C<1> for the current position, and C<2> for the end.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PIOOFF_T
PIO_seek(PARROT_INTERP, NOTNULL(PMC *pmc), PIOOFF_T offset, INTVAL w)
{
    ParrotIOLayer * const l = (ParrotIOLayer *)PMC_struct_val(pmc);
    ParrotIO * const io = (ParrotIO *)PMC_data0(pmc);
    if (!io)
        return -1;

    return PIO_seek_down(interp, l, io, offset, w);
}

/*

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
PIOOFF_T
PIO_tell(PARROT_INTERP, NOTNULL(PMC *pmc))>

Returns the current read/write position of C<*pmc>.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PIOOFF_T
PIO_tell(PARROT_INTERP, NOTNULL(PMC *pmc))
{
    ParrotIOLayer * const l = (ParrotIOLayer *)PMC_struct_val(pmc);
    ParrotIO * const io = (ParrotIO *)PMC_data0(pmc);
    if (!io)
        return -1;

    return PIO_tell_down(interp, l, io);
}

/*

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL
PIO_eof(SHIM_INTERP, NOTNULL(PMC *pmc))>

Returns a boolean value indication whether C<*pmc>'s current read/write
position is C<EOF>.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL
PIO_eof(SHIM_INTERP, NOTNULL(PMC *pmc))
{
    ParrotIO * const io = (ParrotIO *)PMC_data0(pmc);

    /* io could be null here, but rather than return a negative error
     * we just fake EOF since eof test is usually in a boolean context.
     */
    if (io) {
        return (io->flags & (PIO_F_EOF)) ? 1 : 0;
    }
    return 1;
}

/*

=item C<PARROT_API
INTVAL
PIO_puts(PARROT_INTERP, NOTNULL(PMC *pmc), NOTNULL(const char *s))>

Writes C<*s> tp C<*pmc>. C string version.

=cut

*/

PARROT_API
INTVAL
PIO_puts(PARROT_INTERP, NOTNULL(PMC *pmc), NOTNULL(const char *s))
{
    return PIO_write(interp, pmc, s, strlen(s));
}

/*

=item C<PARROT_API
INTVAL
PIO_putps(PARROT_INTERP, NOTNULL(PMC *pmc), NULLOK(STRING *s))>

Writes C<*s> to C<*pmc>. Parrot string version.

=cut

*/

PARROT_API
INTVAL
PIO_putps(PARROT_INTERP, NOTNULL(PMC *pmc), NULLOK(STRING *s))
{
    ParrotIOLayer * const l = (ParrotIOLayer *)PMC_struct_val(pmc);
    ParrotIO * const io = (ParrotIO *)PMC_data0(pmc);
    PARROT_ASSERT((unsigned long)l != 0xdeadbeefUL);
    PARROT_ASSERT(io);

    if (!s)
        return 0;
#if ! DISABLE_GC_DEBUG
    /* trigger GC for debug - but not during tests */
    if (0 && GC_DEBUG(interp))
        Parrot_do_dod_run(interp, DOD_trace_stack_FLAG);
#endif
    return PIO_write_down(interp, l, io, s);
}

/*

=item C<PARROT_API
INTVAL
PIO_fprintf(PARROT_INTERP, NOTNULL(PMC *pmc), NOTNULL(const char *s), ...)>

Writes a C string format with varargs to C<*pmc>.

=cut

*/

PARROT_API
INTVAL
PIO_fprintf(PARROT_INTERP, NOTNULL(PMC *pmc), NOTNULL(const char *s), ...)
{
    va_list args;
    INTVAL ret;

    va_start(args, s);

    ret=PIO_putps(interp, pmc, Parrot_vsprintf_c(interp, s, args));

    va_end(args);

    return ret;
}

/*

=item C<PARROT_API
INTVAL
PIO_printf(PARROT_INTERP, NOTNULL(const char *s), ...)>

Writes a C string format with varargs to C<stdout>.

=cut

*/

PARROT_API
INTVAL
PIO_printf(PARROT_INTERP, NOTNULL(const char *s), ...)
{
    va_list args;
    INTVAL ret;

    va_start(args, s);

    if (interp) {
        STRING * const str = Parrot_vsprintf_c(interp, s, args);
        ret=PIO_putps(interp, _PIO_STDOUT(interp), str);
    }
    else {
        /* Be nice about this...
         **   XXX BD Should this use the default PIO_STDOUT or something?
         */
        ret=vfprintf(stdout, s, args);
    }

    va_end(args);

    return ret;
}

/*

=item C<PARROT_API
INTVAL
PIO_eprintf(NULLOK(PARROT_INTERP), NOTNULL(const char *s), ...)>

Writes a C string format with varargs to C<stderr>.

=cut

*/

PARROT_API
INTVAL
PIO_eprintf(NULLOK(PARROT_INTERP), NOTNULL(const char *s), ...)
{
    va_list args;
    INTVAL ret;

    va_start(args, s);

    if (interp) {
        STRING * const str = Parrot_vsprintf_c(interp, s, args);

        ret=PIO_putps(interp, _PIO_STDERR(interp), str);
    }
    else {
        /* Be nice about this...
        **   XXX BD Should this use the default PIO_STDERR or something?
        */
        ret=vfprintf(stderr, s, args);
    }

    va_end(args);

    return ret;
}

/*

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
PIOHANDLE
PIO_getfd(SHIM_INTERP, NOTNULL(PMC *pmc))>

Returns C<*pmc>'s file descriptor, or C<0> if it is not defined.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PIOHANDLE
PIO_getfd(SHIM_INTERP, NOTNULL(PMC *pmc))
{
    ParrotIO * const io = (ParrotIO *)PMC_data0(pmc);

    if (io)
        return io->fd;

    return (PIOHANDLE)0;        /* XXX that's plain wrong --leo */
}

/*

=back

=head2 C<PIO_STD*> Functions

=over 4

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
PIO_STDIN(PARROT_INTERP)>

Returns the C<ParrotIO> PMC for C<stdin>.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
PIO_STDIN(PARROT_INTERP)
{
    return _PIO_STDIN(interp);
}

/*

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
PIO_STDOUT(PARROT_INTERP)>

Returns the C<ParrotIO> PMC for C<stdout>.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
PIO_STDOUT(PARROT_INTERP)
{
    return _PIO_STDOUT(interp);
}

/*

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
PIO_STDERR(PARROT_INTERP)>

Returns the C<ParrotIO> PMC for C<stderr>.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
PIO_STDERR(PARROT_INTERP)
{
    return _PIO_STDERR(interp);
}

/*

=back

=head2 DOD-related Functions

=over 4

=item C<PARROT_API
void
Parrot_IOData_mark(PARROT_INTERP, NOTNULL(ParrotIOData *piodata))>

Called from C<trace_active_PMCs()> to mark the IO data live.

=cut

*/

PARROT_API
void
Parrot_IOData_mark(PARROT_INTERP, NOTNULL(ParrotIOData *piodata))
{
    INTVAL i;
    ParrotIOTable table = piodata->table;

    /* this was i < PIO_NR_OPEN, but only standard handles 0..2 need
     * to be kept alive AFAIK -leo
     */
    for (i = 0; i < 3; i++) {
        if (table[i]) {
            pobject_lives(interp, (PObj *)table[i]);
        }
    }
}

/*

=back

=head2 Offset Functions

These are used to create offsets for the C<seek> op.

=over 4

=item C<PIOOFF_T
PIO_make_offset(INTVAL offset)>

Returns C<offset>.

=cut

*/

PIOOFF_T
PIO_make_offset(INTVAL offset)
{
    return offset;
}

/*

=item C<PIOOFF_T
PIO_make_offset32(INTVAL hi, INTVAL lo)>

C<hi> is shifted 32 bytes to the left and C<or>ed together with C<lo>.
This allows 64-bit seeks with only 32-bit C<INTVALS>.

=cut

*/

PIOOFF_T
PIO_make_offset32(INTVAL hi, INTVAL lo)
{
    return ((PIOOFF_T)hi << 31) | lo;
}

/*

=item C<PIOOFF_T
PIO_make_offset_pmc(PARROT_INTERP, NOTNULL(PMC *pmc))>

Returns the return value of the C<get_integer> vtable method on C<*pmc>.

=cut

*/

PIOOFF_T
PIO_make_offset_pmc(PARROT_INTERP, NOTNULL(PMC *pmc))
{
    /* XXX: Maybe use bignums here */
    return VTABLE_get_integer(interp, pmc);
}

/*

=back

=head2 Networking Functions

=over 4

=item C<PARROT_API
INTVAL
PIO_poll(PARROT_INTERP, NOTNULL(PMC *pmc), INTVAL which, INTVAL sec, INTVAL usec)>

Polls C<*pmc> for the events in C<which> every C<sec> seconds + C<usec>
microseconds.

=cut

*/

PARROT_API
INTVAL
PIO_poll(PARROT_INTERP, NOTNULL(PMC *pmc), INTVAL which, INTVAL sec, INTVAL usec)
{
    ParrotIOLayer *l;
    ParrotIO *io;

    if (PMC_IS_NULL(pmc)) {
        real_exception(interp, NULL, E_ValueError, "Can't poll NULL pmc");
    }

    l = (ParrotIOLayer *)PMC_struct_val(pmc);
    io = (ParrotIO *)PMC_data0(pmc);
    return PIO_poll_down(interp, l, io, which, sec, usec);
}

/*

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
PIO_socket(PARROT_INTERP, INTVAL fam, INTVAL type, INTVAL proto)>

Creates and returns a socket using the specified address family, socket type,
and protocol number. Check the returned PMC with a boolean test to see whether
the socket was successfully created.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
PIO_socket(PARROT_INTERP, INTVAL fam, INTVAL type, INTVAL proto)
{
    ParrotIOLayer * const l = interp->piodata->default_stack;
    ParrotIO * const io = PIO_socket_down(interp, l, fam, type, proto);
    /* We have to create a PMC here even if the IO handle
     * didn't create because caller has to be able to
     * check with a bool test. Can't use a NULL PMC in a bool
     * test as that will cause an exception.
     */
    return new_io_pmc(interp, io);
}

/*

=item C<PARROT_API
INTVAL
PIO_recv(PARROT_INTERP, NOTNULL(PMC *pmc), NOTNULL(STRING **buf))>

Receives a message from the connected socket C<*pmc> in C<*buf>.  Returns C<-1>
if it fails.

=cut

*/

PARROT_API
INTVAL
PIO_recv(PARROT_INTERP, NOTNULL(PMC *pmc), NOTNULL(STRING **buf))
{
    ParrotIOLayer * const l = (ParrotIOLayer *)PMC_struct_val(pmc);
    ParrotIO * const io = (ParrotIO *)PMC_data(pmc);
    if (!io)
        return -1;

    return PIO_recv_down(interp, l, io, buf);
}

/*

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL
PIO_send(PARROT_INTERP, NOTNULL(PMC *pmc), NOTNULL(STRING *buf))>

Sends the message C<*buf> to the connected socket C<*pmc>.  Returns
C<-1> if it cannot send the message.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL
PIO_send(PARROT_INTERP, NOTNULL(PMC *pmc), NOTNULL(STRING *buf))
{
    ParrotIOLayer * const l = (ParrotIOLayer *)PMC_struct_val(pmc);
    ParrotIO * const io = (ParrotIO *)PMC_data(pmc);
    if (!io)
        return -1;

    return PIO_send_down(interp, l, io, buf);
}

/*

=item C<PARROT_API
INTVAL
PIO_connect(PARROT_INTERP, NOTNULL(PMC *pmc), NOTNULL(STRING *address))>

Connects C<*pmc> to C<*address>.  Returns C<-1> on failure.

=cut

*/

PARROT_API
INTVAL
PIO_connect(PARROT_INTERP, NOTNULL(PMC *pmc), NOTNULL(STRING *address))
{
    ParrotIOLayer * const l = (ParrotIOLayer *)PMC_struct_val(pmc);
    ParrotIO * const io = (ParrotIO *)PMC_data(pmc);
    if (!io)
        return -1;

    return PIO_connect_down(interp, l, io, address);
}

/*

=item C<PARROT_API
INTVAL
PIO_bind(PARROT_INTERP, NOTNULL(PMC *pmc), NOTNULL(STRING *address))>

Binds C<*pmc>'s socket to the local address and port specified by
C<*address>.  Returns C<-1> on failure.

=cut

*/

PARROT_API
INTVAL
PIO_bind(PARROT_INTERP, NOTNULL(PMC *pmc), NOTNULL(STRING *address))
{
    ParrotIOLayer * const l = (ParrotIOLayer *)PMC_struct_val(pmc);
    ParrotIO * const io = (ParrotIO *)PMC_data(pmc);
    if (!io)
        return -1;

    return PIO_bind_down(interp, l, io, address);
}

/*

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL
PIO_listen(PARROT_INTERP, NOTNULL(PMC *pmc), INTVAL backlog)>

Listens for new connections on socket C<*pmc>.  Returns C<-1> on failure.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL
PIO_listen(PARROT_INTERP, NOTNULL(PMC *pmc), INTVAL backlog)
{
    ParrotIOLayer * const l = (ParrotIOLayer *)PMC_struct_val(pmc);
    ParrotIO * const io = (ParrotIO *)PMC_data(pmc);
    if (!io)
        return -1;

    return PIO_listen_down(interp, l, io, backlog);
}

/*

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC *
PIO_accept(PARROT_INTERP, NOTNULL(PMC *pmc))>

Accepts a new connection and returns a newly created C<ParrotIO> socket.
Returns C<NULL> on failure.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC *
PIO_accept(PARROT_INTERP, NOTNULL(PMC *pmc))
{
    ParrotIO *io2;
    ParrotIOLayer * const l = (ParrotIOLayer *)PMC_struct_val(pmc);
    ParrotIO * const io = (ParrotIO *)PMC_data(pmc);

    /* XXX - return NULL or -1 -- c (02 July 2006) */
    if (!io)
        return NULL;

    io2 = PIO_accept_down(interp, l, io);
    return new_io_pmc(interp, io2);
}

/*

=item C<PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL
PIO_isatty(SHIM_INTERP, NOTNULL(PMC *pmc))>

Returns a boolean value indicating whether C<*pmc> is a console/tty.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL
PIO_isatty(SHIM_INTERP, NOTNULL(PMC *pmc))
{
    ParrotIO * const io = (ParrotIO *)PMC_data(pmc);

    if (!io)
        return 0;

    return (io->flags & PIO_F_CONSOLE) ? 1 : 0;
}

/*

=back

=head1 SEE ALSO

F<io/io_buf.c>,
F<io/io_passdown.c>,
F<io/io_stdio.c>,
F<io/io_unix.c>,
F<io/io_win32.c>,
F<io/io_private.h>.

=head1 HISTORY

Initially written by Melvin Smith.

Some ideas and goals from Perl 5.7 and Nick Ing-Simmons' work.

=head1 TODO

Rework to use copy-on-write IO stacks rather than creating a new stack for each
IO stream.

Add support for loadable layers in Parrot bytecode.

=cut

*/



/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
