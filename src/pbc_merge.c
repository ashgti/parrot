/*
Copyright (C) 2005, The Perl Foundation.
$Id$

=head1 NAME

pbc_merge - Merge multiple Parrot bytecode (PBC) files into
                  a single PBC file.

=head1 SYNOPSIS

 pbc_merge -o out.pbc input1.pbc input2.pbc ...

=head1 DESCRIPTION

This program takes two or more PBC files and produces a single
merged output PBC file with a single fix-up table and constants
table.

=head2 Command-Line Options

=over 4

=item C<-o out.pbc>

The name of the PBC file to produce, containing the merged
segments from the input PBC files.

=back

=cut

*/


#include "parrot/parrot.h"
#include "parrot/embed.h"
#include "parrot/oplib/ops.h"


/* This struct describes an input file. */
typedef struct pbc_merge_input
{
    char    *filename;      /* Filename of the input file. */
    PackFile *pf;  /* The loaded packfile. */
    opcode_t code_start;    /* Where the bytecode is located in the merged
                             bytecode. */
    opcode_t const_start;   /* Where the const table is located in the merged
                             one. */
} pbc_merge_input;

/*

static void help(Interp *)

Print out the user help info.

*/
static void
help(Interp *interp)
{
    printf("pbc_merge - merge multiple parrot bytecode files into one\n");
    printf("Usage:\n");
    printf("   pbc_merge -o out.pbc file1.pbc file2.pbc ...\n\n");
    Parrot_exit(interp, 0);
}


/*

static PackFile*
pbc_merge_loadpbc(Interp *interp, char *filename)

This function loads a PBC file and unpacks it. We can't
use Parrot_readbc because that is specified to also
fixup the segments, which we don't want.

*/
static PackFile*
pbc_merge_loadpbc(Interp *interp, char *fullname)
{
    INTVAL program_size, wanted;
    char *program_code;
    PackFile *pf;
    FILE * io = NULL;
    INTVAL is_mapped = 0;
    size_t chunk_size;
    char *cursor;
    INTVAL read_result;

    /* Check the file exists. */
    STRING *fs;
    fs = string_make(interp, fullname,
            strlen(fullname), NULL, 0);
    if (!Parrot_stat_info_intval(interp, fs, STAT_EXISTS)) {
        PIO_eprintf(interp, "PBC Merge: Can't stat %s, code %i.\n",
                fullname, errno);
        Parrot_exit(interp, 1);
    }

    /* Get program size. */
    program_size = Parrot_stat_info_intval(interp, fs, STAT_FILESIZE);

    /* Attempt to open file and handle any errors. */
    io = fopen(fullname, "rb");
    if (!io) {
        PIO_eprintf(interp, "PBC Merge: Can't open %s, code %i.\n",
                fullname, errno);
        Parrot_exit(interp, 1);
    }

    /* Read in program. Nabbed from Parrot_readpbc. */
    chunk_size   = program_size > 0 ? program_size : 1024;
    program_code = (char *)mem_sys_allocate(chunk_size);
    wanted       = program_size;
    program_size = 0;
    cursor       = (char *)program_code;

    while ((read_result = fread(cursor, 1, chunk_size, io)) > 0) {
        program_size += read_result;
        if (program_size == wanted)
            break;
        chunk_size   = 1024;
        program_code =
            (char *)mem_sys_realloc(program_code, program_size + chunk_size);

        if (!program_code) {
            PIO_eprintf(interp,
                "PBC Merge: Could not reallocate buffer");
            Parrot_exit(interp, 1);
        }

        cursor = (char *)program_code + program_size;
    }

    if (read_result < 0) {
        PIO_eprintf(interp,
                "PBC Merge: Problem reading packfile from PIO.\n");
        Parrot_exit(interp, 1);
    }
    fclose(io);

    /* Now that we have the bytecode, let's unpack it. */
    pf = PackFile_new(interp, is_mapped);
    if (!PackFile_unpack(interp,
                pf, (opcode_t *)program_code, program_size)) {
        PIO_eprintf(interp, "PBC Merge: Can't unpack packfile %s.\n",
                fullname);
        Parrot_exit(interp, 1);
    }

    /* Return the packfile. */
    return pf;
}


/*

static PackFile_ByteCode*
pbc_merge_bytecode(Interp *interp, pbc_merge_input **inputs,
                   int num_inputs, PackFile *pf)

This function merges the bytecode from the input packfiles, storing the
offsets that each bit of bytecode now exists at.

*/
static PackFile_ByteCode*
pbc_merge_bytecode(Interp *interp, pbc_merge_input **inputs,
                   int num_inputs, PackFile *pf)
{
    PackFile_ByteCode *bc_seg;
    opcode_t *bc    = mem_allocate_typed(opcode_t);
    opcode_t cursor = 0;
    int i;

    /* Add a bytecode segment. */
    bc_seg = (PackFile_ByteCode *)PackFile_Segment_new_seg(interp,
              &pf->directory, PF_BYTEC_SEG, BYTE_CODE_SEGMENT_NAME, 1);
    if (bc_seg == NULL)
    {
        PIO_eprintf(interp, "PBC Merge: Error creating bytecode segment.");
        Parrot_exit(interp, 1);
    }

    /* Loop over input files. */
    for (i = 0; i < num_inputs; i++)
    {
        /* Get the bytecode segment from the input file. */
        PackFile_ByteCode *in_seg = inputs[i]->pf->cur_cs;
        if (in_seg == NULL)
        {
            PIO_eprintf(interp,
                "PBC Merge: Cannot locate bytecode segment in %s",
                inputs[i]->filename);
            Parrot_exit(interp, 1);
        }

        /* Re-allocate the current buffer. */
        bc = (opcode_t *)mem_sys_realloc(bc,
            (cursor + in_seg->base.size) * sizeof (opcode_t));
        if (bc == NULL)
        {
            PIO_eprintf(interp, "PBC Merge: Cannot reallocate memory\n");
            Parrot_exit(interp, 1);
        }

        /* Copy data and store cursor. */
        memcpy(bc + cursor, in_seg->base.data,
            in_seg->base.size * sizeof (opcode_t));
        inputs[i]->code_start = cursor;

        /* Update cursor. */
        cursor += in_seg->base.size;
    }

    /* Stash produced bytecode. */
    bc_seg->base.data = bc;
    bc_seg->base.size = cursor;
    bc_seg->base.name = strdup("MERGED");
    return bc_seg;
}


/*

static PackFile_ConstTable*
pbc_merge_constants(Interp *interp, pbc_merge_input **inputs,
                    int num_inputs, PackFile *pf, PackFile_ByteCode *bc)

This function merges the constants tables from the input PBC files.

*/
static PackFile_ConstTable*
pbc_merge_constants(Interp *interp, pbc_merge_input **inputs,
                    int num_inputs, PackFile *pf, PackFile_ByteCode *bc)
{
    PackFile_ConstTable *const_seg;
    PackFile_Constant   **constants = mem_allocate_typed(PackFile_Constant *);
    opcode_t cursor = 0;
    int i, j;

    /* Add a constant table segment. */
    const_seg = (PackFile_ConstTable*)PackFile_Segment_new_seg(
        interp, &pf->directory, PF_CONST_SEG, CONSTANT_SEGMENT_NAME, 1);
    if (const_seg == NULL)
    {
        PIO_eprintf(interp,
            "PBC Merge: Error creating constant table segment.");
        Parrot_exit(interp, 1);
    }

    /* Loop over input files. */
    for (i = 0; i < num_inputs; i++)
    {
        /* Get the constant table segment from the input file. */
        PackFile_ConstTable *in_seg = inputs[i]->pf->cur_cs->const_table;
        if (in_seg == NULL)
        {
            PIO_eprintf(interp,
                "PBC Merge: Cannot locate constant table segment in %s\n",
                inputs[i]->filename);
            Parrot_exit(interp, 1);
        }

        /* Store cursor as position where constant table starts. */
        inputs[i]->const_start = cursor;

        /* Allocate space for the constant list, provided we have some. */
        if (in_seg->const_count > 0)
        {
            constants = (PackFile_Constant **)mem_sys_realloc(constants,
                (cursor + in_seg->const_count) * sizeof (Parrot_Pointer));
            if (constants == NULL)
            {
                PIO_eprintf(interp, "PBC Merge: Out of memory");
                Parrot_exit(interp, 1);
            }
        }

        /* Loop over the constants and copy them to the output PBC. */
        for (j = 0; j < in_seg->const_count; j++)
        {
            /* Get the entry and allocate space for copy. */
            PackFile_Constant *cur_entry = in_seg->constants[j];
            PackFile_Constant *copy      = mem_allocate_typed(
                PackFile_Constant);
            if (copy == NULL)
            {
                PIO_eprintf(interp, "PBC Merge: Out of memory");
                Parrot_exit(interp, 1);
            }

            /* Copy. */
            memcpy(copy, cur_entry, sizeof (PackFile_Constant));

            /* If it's a sub PMC, need to deal with offsets. */
            if (copy->type == PFC_PMC)
            {
                Parrot_sub *sub;
                switch (copy->u.key->vtable->base_type) {
                    case enum_class_Sub:
                    case enum_class_Closure:
                    case enum_class_Coroutine:
                        sub = PMC_sub(copy->u.key);
                        sub->start_offs += inputs[i]->code_start;
                        sub->end_offs += inputs[i]->code_start;
                        break;
                }
            }

            /* Slot it into the list. */
            constants[cursor] = copy;
            cursor++;
        }
    }

    /* Stash merged constants table and count and return the new segment. */
    const_seg->constants   = constants;
    const_seg->const_count = cursor;
    const_seg->code        = bc;
    bc->const_table        = const_seg;
    return const_seg;
}


/*

static void
pbc_merge_fixups(Interp *interp, pbc_merge_input **inputs,
                 int num_inputs, PackFile *pf, PackFile_ByteCode *bc)

This function merges the fixups tables from the input PBC files.

*/
static void
pbc_merge_fixups(Interp *interp, pbc_merge_input **inputs,
                 int num_inputs, PackFile *pf, PackFile_ByteCode *bc)
{
    PackFile_FixupTable *fixup_seg;
    PackFile_FixupEntry **fixups = mem_allocate_typed(PackFile_FixupEntry *);
    opcode_t cursor = 0;
    int i, j;

    /* Add a fixup table segment. */
    fixup_seg = (PackFile_FixupTable*)PackFile_Segment_new_seg(
        interp, &pf->directory, PF_FIXUP_SEG, FIXUP_TABLE_SEGMENT_NAME, 1);
    if (fixup_seg == NULL)
    {
        PIO_eprintf(interp,
            "PBC Merge: Error creating fixup table segment.");
        Parrot_exit(interp, 1);
    }

    /* Loop over input files. */
    for (i = 0; i < num_inputs; i++)
    {
        /* Get the fixup segment from the input file. */
        PackFile_FixupTable *in_seg = inputs[i]->pf->cur_cs->fixups;
        if (in_seg == NULL)
        {
            PIO_eprintf(interp,
                "PBC Merge: Cannot locate fixup segment in %s",
                inputs[i]->filename);
            Parrot_exit(interp, 1);
        }

        /* Allocate space for these fixups, provided we have some. */
        if (in_seg->fixup_count > 0)
        {
            fixups = (PackFile_FixupEntry **)mem_sys_realloc(fixups,
                (cursor + in_seg->fixup_count) * sizeof (Parrot_Pointer));
            if (fixups == NULL)
            {
                PIO_eprintf(interp, "PBC Merge: Out of memory");
                Parrot_exit(interp, 1);
            }
        }

        /* Loop over the fixups and copy them to the output PBC, correcting
           the offsets into the bytecode. */
        for (j = 0; j < in_seg->fixup_count; j++)
        {
            /* Get the entry and allocate space for copies. */
            PackFile_FixupEntry *cur_entry = in_seg->fixups[j];
            PackFile_FixupEntry *copy      = mem_allocate_typed(
                PackFile_FixupEntry);
            char *name_copy = (char *)mem_sys_allocate(
                strlen(cur_entry->name) + 1);
            if (copy == NULL || name_copy == NULL)
            {
                PIO_eprintf(interp, "PBC Merge: Out of memory");
                Parrot_exit(interp, 1);
            }

            /* Copy type and name. */
            copy->type = cur_entry->type;
            strcpy(name_copy, cur_entry->name);
            copy->name = name_copy;

            /* Set new offset and bytecode pointer. */
            switch (copy->type)
            {
                case enum_fixup_label:
                    copy->offset = cur_entry->offset + inputs[i]->code_start;
                    break;
                case enum_fixup_sub:
                    copy->offset = cur_entry->offset + inputs[i]->const_start;
                    break;
                default:
                    PIO_eprintf(interp, "PBC Merge: Unknown fixup type");
                    Parrot_exit(interp, 1);
            }

            copy->seg = bc;

            /* Slot it into the list. */
            fixups[cursor] = copy;
            cursor++;
        }
    }

    /* Stash merged fixup table and count. */
    fixup_seg->fixups      = fixups;
    fixup_seg->fixup_count = cursor;
}


/*

static void
pbc_merge_debugs(Interp *interp, pbc_merge_input **inputs,
                 int num_inputs, PackFile *pf, PackFile_ByteCode *bc)

This function merges the debug segments from the input PBC files.

*/
static void
pbc_merge_debugs(Interp *interp, pbc_merge_input **inputs,
                 int num_inputs, PackFile *pf, PackFile_ByteCode *bc)
{
    PackFile_Debug *debug_seg;
    opcode_t *lines                  = mem_allocate_typed(opcode_t);
    PackFile_DebugMapping **mappings =
        mem_allocate_typed(PackFile_DebugMapping *);
    opcode_t num_mappings = 0;
    opcode_t num_lines    = 0;
    int i, j;

    /* We need to merge both the mappings and the list of line numbers.
       The line numbers can just be concatenated. The mappings must have
       their offsets fixed up. */
    for (i = 0; i < num_inputs; i++)
    {
        PackFile_Debug *in_seg = inputs[i]->pf->cur_cs->debugs;

        /* Concatenate line numbers. */
        lines = (opcode_t *)mem_sys_realloc(lines,
                (num_lines + in_seg->base.size) * sizeof (opcode_t));
        if (lines == NULL)
        {
            PIO_eprintf(interp, "PBC Merge: Cannot reallocate memory\n");
            Parrot_exit(interp, 1);
        }
        memcpy(lines + num_lines, in_seg->base.data,
            in_seg->base.size * sizeof (opcode_t));

        /* Concatenate mappings. */
        mappings = (PackFile_DebugMapping **)mem_sys_realloc(mappings,
                   (num_mappings + in_seg->num_mappings) *
                   sizeof (Parrot_Pointer));
        for (j = 0; j < in_seg->num_mappings; j++)
        {
            PackFile_DebugMapping *mapping = mem_allocate_typed(
                PackFile_DebugMapping);
            memcpy(mapping, in_seg->mappings[j],
                sizeof (PackFile_DebugMapping));
            mapping->offset += num_lines;
            if (mapping->mapping_type == PF_DEBUGMAPPINGTYPE_FILENAME)
                mapping->u.filename += inputs[i]->const_start;
            mappings[num_mappings + j] = mapping;
        }

        /* Update counts. */
        num_lines    += in_seg->base.size;
        num_mappings += in_seg->num_mappings;
    }

    /* Create merged debug segment. Replace created data and mappings
       with merged ones we have created. */
    debug_seg = Parrot_new_debug_seg(interp, bc, num_lines);
    PackFile_add_segment(interp, &pf->directory, (PackFile_Segment*)debug_seg);
    free(debug_seg->base.data);
    debug_seg->base.data    = lines;
    free(debug_seg->mappings);

    debug_seg->mappings     = mappings;
    debug_seg->num_mappings = num_mappings;
}

/*

static void
pbc_merge_pic_index(Interp *interp, pbc_merge_input **inputs,
                 int num_inputs, PackFile *pf, PackFile_ByteCode *bc)

This function merges the pic_index segments from the input PBC files.

*/

static void
pbc_merge_pic_index(Interp *interp, pbc_merge_input **inputs,
                 int num_inputs, PackFile *pf, PackFile_ByteCode *bc)
{
    int i;
    PackFile_Segment * pic_index, *in_seg;
    size_t size, j;
    opcode_t k, cursor = 0, start = 0;
    opcode_t last      = 0; /* avoid unitialized warning */

    /* calc needed size */
    for (i = 0, size = 0; i < num_inputs; i++) {
        in_seg  = inputs[i]->pf->cur_cs->pic_index;
        size   += in_seg->size;
    }
    pic_index = PackFile_Segment_new_seg(interp,
              &pf->directory, PF_UNKNOWN_SEG, "PIC_idx_MERGED", 1);
    pic_index->data
        = (opcode_t *)mem_sys_allocate_zeroed(size * sizeof (opcode_t));
    pic_index->size = size;

    for (i = 0, size = 0; i < num_inputs; i++) {
        in_seg = inputs[i]->pf->cur_cs->pic_index;
        /*
         * pic_index is 0 or an ever increasing (by 1) number
         */
        for (j = 0; j < in_seg->size; j++) {
            k = in_seg->data[j];
            if (k) {
                pic_index->data[cursor] = k + start;
                last = k;
            }
            cursor++;
        }
        start = last;
    }
    bc->pic_index = pic_index;
}

/*

static void
pbc_merge_ctpointers(Interp *interp, pbc_merge_input **inputs,
                     int num_inputs, PackFile *pf, PackFile_ByteCode *bc,
                     PackFile_ConstTable *ct)

This function corrects the pointers into the constants table found in the
bytecode.

*/
static void
pbc_merge_ctpointers(Interp *interp, pbc_merge_input **inputs,
                     int num_inputs, PackFile *pf, PackFile_ByteCode *bc,
                     PackFile_ConstTable *ct)
{
    op_info_t *op;
    opcode_t   op_num;
    opcode_t  *op_ptr;
    opcode_t  *ops       = bc->base.data;
    opcode_t   cur_op    = 0;
    int        cur_input = 0;
    int        cur_arg;

    /* Loop over the ops in the merged bytecode. */
    while (cur_op < (opcode_t)bc->base.size)
    {
        /* Keep track of the current input file. */
        if (cur_input + 1 < num_inputs &&
            cur_op >= inputs[cur_input + 1]->code_start)
            cur_input++;

        /* Get info about this op and jump over it. */
        op_num = ops[cur_op];
        op     = &interp->op_info_table[op_num];
        op_ptr = ops + cur_op;
        cur_op++;

        /* Loop over the arguments. */
        for (cur_arg = 1; cur_arg < op->op_count; cur_arg++)
        {
            /* Pick out any indexes into the constant table and correct them. */
            switch (op->types[cur_arg - 1])
            {
                case PARROT_ARG_NC:
                case PARROT_ARG_PC:
                case PARROT_ARG_SC:
                case PARROT_ARG_KC:
                    ops[cur_op] += inputs[cur_input]->const_start;
                    break;
            }

            /* Move along the bytecode array. */
            cur_op++;
        }

        /* Handle special case variable argument opcodes. */
        if (op_num == PARROT_OP_set_args_pc    ||
            op_num == PARROT_OP_get_results_pc ||
            op_num == PARROT_OP_get_params_pc  ||
            op_num == PARROT_OP_set_returns_pc)
        {
            /* Get the signature. */
            PMC *sig = bc->const_table->constants[op_ptr[1]]->u.key;

            /* Loop over the arguments to locate any that need a fixup. */
            int sig_items = SIG_ELEMS(sig);
            for (cur_arg = 0; cur_arg < sig_items; cur_arg++)
            {
                switch (SIG_ITEM(sig, cur_arg))
                {
                    case PARROT_ARG_NC:
                    case PARROT_ARG_PC:
                    case PARROT_ARG_SC:
                    case PARROT_ARG_KC:
                        ops[cur_op] += inputs[cur_input]->const_start;
                        break;
                }
                cur_op++;
            }
        }
    }
}


/*

static PackFile*
pbc_merge_begin(Interp *interp, pbc_merge_input **inputs,
                int num_inputs)

This is the function that drives PBC merging process.

*/
static PackFile*
pbc_merge_begin(Interp *interp, pbc_merge_input **inputs,
                int num_inputs)
{
    PackFile            *merged;
    PackFile_ByteCode   *bc;
    PackFile_ConstTable *ct;

    /* Create a new empty packfile. */
    merged = PackFile_new(interp, 0);
    if (merged == NULL)
    {
        PIO_eprintf(interp, "PBC Merge: Error creating new packfile.\n");
        Parrot_exit(interp, 1);
    }

    /* Merge the various stuff. */
    bc = pbc_merge_bytecode(interp, inputs, num_inputs, merged);
    ct = pbc_merge_constants(interp, inputs, num_inputs, merged, bc);

    pbc_merge_fixups(interp, inputs, num_inputs, merged, bc);
    pbc_merge_debugs(interp, inputs, num_inputs, merged, bc);
    pbc_merge_pic_index(interp, inputs, num_inputs, merged, bc);

    /* Walk bytecode and fix ops that reference the constants table. */
    pbc_merge_ctpointers(interp, inputs, num_inputs, merged, bc, ct);

    /* Return merged result. */
    return merged;
}


/*

static void
pbc_merge_write(Interp *interp, PackFile *pf, const char *filename)

This functions writes out the merged packfile.

*/
static void
pbc_merge_write(Interp *interp, PackFile *pf, const char *filename)
{
    size_t    size;
    opcode_t *pack;
    FILE     *fp;

    /* Get size of packfile we'll write. */
    size = PackFile_pack_size(interp, pf) * sizeof (opcode_t);

    /* Allocate memory. */
    pack = (opcode_t*) mem_sys_allocate(size);
    if (pack == NULL) {
        PIO_eprintf(interp, "PBC Merge: Out of memory");
        Parrot_exit(interp, 1);
    }

    /* Write and clean up. */
    PackFile_pack(interp, pf, pack);
    if ((fp = fopen(filename, "wb")) == 0) {
        PIO_eprintf(interp, "PBC Merge: Couldn't open %s\n", filename);
        Parrot_exit(interp, 1);
    }
    if ((1 != fwrite(pack, size, 1, fp))) {
        PIO_eprintf(interp, "PBC Merge: Couldn't write %s\n", filename);
        Parrot_exit(interp, 1);
    }
    fclose(fp);
    mem_sys_free(pack);
}


/*

int
main(int argc, char **argv)

The main function that grabs console input, reads in the packfiles
provided they exist, hands them to another function that runs the
merge process and finally writes out the produced packfile.

*/

static struct longopt_opt_decl options[] = {
    { 'o', 'o', OPTION_required_FLAG, { "--output" } }
};

int
main(int argc, char **argv)
{
    Interp *interp;
    int status;
    pbc_merge_input** input_files;
    PackFile *merged;
    int i;
    const char *output_file     = NULL;
    struct longopt_opt_info opt = LONGOPT_OPT_INFO_INIT;

    /* Create a Parrot interpreter. */
    interp = Parrot_new(NULL);
    Parrot_block_DOD(interp);

    /* Get options, ensuring we have at least one input
       file and an output file. */
    if (argc < 4) {
        help(interp);
    }
    while ((status = longopt_get(interp, argc, argv, options, &opt)) > 0) {
        switch (opt.opt_id) {
            case 'o':
                if (output_file == NULL)
                    output_file = opt.opt_arg;
                else
                    help(interp);
                break;
            case '?':
                help(interp);
                break;
        }
    }
    if (status == -1 || !output_file) {
        help(interp);
    }
    argc -= opt.opt_index;    /* Now the number of input files. */
    argv += opt.opt_index;    /* Now at first input filename. */

    /* Load each packfile that we are to merge and set up an input
       structure for each of them. */
    input_files = (pbc_merge_input **)mem_sys_allocate(
        argc * sizeof (Parrot_Pointer));

    for (i = 0; i < argc; i++)
    {
        /* Allocate a struct. */
        input_files[i] = mem_allocate_typed(pbc_merge_input);

        /* Set filename */
        input_files[i]->filename = *argv;

        /* Load the packfile and unpack it. */
        input_files[i]->pf = pbc_merge_loadpbc(interp,
            input_files[i]->filename);
        if (input_files[i]->pf == NULL)
        {
            PIO_eprintf(interp,
                "PBC Merge: Unknown error while reading and unpacking %s\n",
                *argv);
            Parrot_exit(interp, 1);
        }

        /* Next file. */
        argv++;
    }

    /* Merge. */
    merged = pbc_merge_begin(interp, input_files, argc);
    if (merged == NULL)
    {
        PIO_eprintf(interp, "PBC Merge: Unknown error during merge\n");
        Parrot_exit(interp, 1);
    }

    /* Write merged packfile. */
    pbc_merge_write(interp, merged, output_file);

    /* Finally, we're done. */
    Parrot_exit(interp, 0);
    return 0;
}


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
