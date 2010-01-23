/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 NAME

src/embed.c - The Parrot embedding interface

=head1 DESCRIPTION

This file implements the Parrot embedding interface.

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "parrot/embed.h"
#include "parrot/oplib/ops.h"
#include "pmc/pmc_sub.h"
#include "pmc/pmc_callcontext.h"
#include "parrot/runcore_api.h"

#include "../compilers/imcc/imc.h"

/* HEADERIZER HFILE: none */ /* The visible types are different than what we use in here */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_CANNOT_RETURN_NULL
PARROT_OBSERVER
static const char * op_name(PARROT_INTERP, int k)
        __attribute__nonnull__(1);

static void print_constant_table(PARROT_INTERP)
        __attribute__nonnull__(1);

static void print_debug(PARROT_INTERP, SHIM(int status), SHIM(void *p))
        __attribute__nonnull__(1);

PARROT_CANNOT_RETURN_NULL
static PMC* set_current_sub(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_CANNOT_RETURN_NULL
static PMC* setup_argv(PARROT_INTERP, int argc, ARGIN(char **argv))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

#define ASSERT_ARGS_op_name __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_print_constant_table __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_print_debug __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_set_current_sub __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_setup_argv __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(argv))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

extern int Parrot_exec_run;

/*

=item C<Parrot_Interp Parrot_new(Parrot_Interp parent)>

Returns a new Parrot interpreter.

The first created interpreter (C<parent> is C<NULL>) is the last one
to get destroyed.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
Parrot_Interp
Parrot_new(ARGIN_NULLOK(Parrot_Interp parent))
{
    /* inter_create.c:make_interpreter builds a new Parrot_Interp. */
    return make_interpreter(parent, PARROT_NO_FLAGS);
}


/*

=item C<void Parrot_init_stacktop(PARROT_INTERP, void *stack_top)>

Initializes the new interpreter when it hasn't been initialized before.

Additionally sets the stack top, so that Parrot objects created
in inner stack frames will be visible during GC stack walking code.
B<stack_top> should be the address of an automatic variable in the caller's
stack frame. All unanchored Parrot objects (PMCs) must live in inner stack
frames so that they are not destroyed during GC runs.

Use this function when you call into Parrot before entering a run loop.

=cut

*/

PARROT_EXPORT
void
Parrot_init_stacktop(PARROT_INTERP, void *stack_top)
{
    interp->lo_var_ptr = stack_top;
    init_world_once(interp);
}


/*

=item C<void Parrot_set_flag(PARROT_INTERP, INTVAL flag)>

Sets on any of the following flags, specified by C<flag>, in the interpreter:

Flag                    Effect
C<PARROT_BOUNDS_FLAG>   enable bounds checking
C<PARROT_PROFILE_FLAG>  enable profiling,
C<PARROT_THR_TYPE_1>    disable variable sharing and thread communication
C<PARROT_THR_TYPE_2>    disable variable sharing but enable thread communication
C<PARROT_THR_TYPE_3>    enable variable sharing.

=cut

*/

PARROT_EXPORT
void
Parrot_set_flag(PARROT_INTERP, INTVAL flag)
{
    /* These two macros (from interpreter.h) do exactly what they look like. */

    Interp_flags_SET(interp, flag);
    switch (flag) {
      case PARROT_BOUNDS_FLAG:
      case PARROT_PROFILE_FLAG:
        Parrot_runcore_switch(interp, Parrot_str_new_constant(interp, "slow"));
        break;
      default:
        break;
    }
}


/*

=item C<void Parrot_set_debug(PARROT_INTERP, UINTVAL flag)>

Set a debug flag: C<PARROT_DEBUG_FLAG>.

=cut

*/

PARROT_EXPORT
void
Parrot_set_debug(PARROT_INTERP, UINTVAL flag)
{
    interp->debug_flags |= flag;
}


/*

=item C<void Parrot_set_executable_name(PARROT_INTERP, Parrot_String name)>

Sets the name of the executable launching Parrot (see C<pbc_to_exe> and the
C<parrot> binary).

=cut

*/

PARROT_EXPORT
void
Parrot_set_executable_name(PARROT_INTERP, Parrot_String name)
{
    PMC * const name_pmc = pmc_new(interp, enum_class_String);
    VTABLE_set_string_native(interp, name_pmc, name);
    VTABLE_set_pmc_keyed_int(interp, interp->iglobals, IGLOBALS_EXECUTABLE,
        name_pmc);
}


/*

=item C<void Parrot_set_trace(PARROT_INTERP, UINTVAL flag)>

Set a trace flag: C<PARROT_TRACE_FLAG>

=cut

*/

PARROT_EXPORT
void
Parrot_set_trace(PARROT_INTERP, UINTVAL flag)
{
    Parrot_pcc_trace_flags_on(interp, interp->ctx, flag);
    Parrot_runcore_switch(interp, Parrot_str_new_constant(interp, "slow"));
}


/*

=item C<void Parrot_clear_flag(PARROT_INTERP, INTVAL flag)>

Clears a flag in the interpreter.

=cut

*/

PARROT_EXPORT
void
Parrot_clear_flag(PARROT_INTERP, INTVAL flag)
{
    Interp_flags_CLEAR(interp, flag);
}


/*

=item C<void Parrot_clear_debug(PARROT_INTERP, UINTVAL flag)>

Clears a flag in the interpreter.

=cut

*/

PARROT_EXPORT
void
Parrot_clear_debug(PARROT_INTERP, UINTVAL flag)
{
    interp->debug_flags &= ~flag;
}


/*

=item C<void Parrot_clear_trace(PARROT_INTERP, UINTVAL flag)>

Clears a flag in the interpreter.

=cut

*/

PARROT_EXPORT
void
Parrot_clear_trace(PARROT_INTERP, UINTVAL flag)
{
    Parrot_pcc_trace_flags_off(interp, interp->ctx, flag);
}


/*

=item C<Parrot_Int Parrot_test_flag(PARROT_INTERP, INTVAL flag)>

Test the interpreter flags specified in C<flag>.

=cut

*/

PARROT_EXPORT
Parrot_Int
Parrot_test_flag(PARROT_INTERP, INTVAL flag)
{
    return Interp_flags_TEST(interp, flag);
}


/*

=item C<UINTVAL Parrot_test_debug(PARROT_INTERP, UINTVAL flag)>

Test the interpreter flags specified in C<flag>.

=cut

*/

PARROT_EXPORT
UINTVAL
Parrot_test_debug(PARROT_INTERP, UINTVAL flag)
{
    return interp->debug_flags & flag;
}


/*

=item C<UINTVAL Parrot_test_trace(PARROT_INTERP, UINTVAL flag)>

Test the interpreter flags specified in C<flag>.

=cut

*/

PARROT_EXPORT
UINTVAL
Parrot_test_trace(PARROT_INTERP, UINTVAL flag)
{
    return Parrot_pcc_trace_flags_test(interp, interp->ctx, flag);
}


/*

=item C<void Parrot_set_run_core(PARROT_INTERP, Parrot_Run_core_t core)>

Sets the specified run core.

=cut

*/

PARROT_EXPORT
void
Parrot_set_run_core(PARROT_INTERP, Parrot_Run_core_t core)
{
    switch (core) {
      case PARROT_SLOW_CORE:
        Parrot_runcore_switch(interp, Parrot_str_new_constant(interp, "slow"));
        break;
      case PARROT_FAST_CORE:
        Parrot_runcore_switch(interp, Parrot_str_new_constant(interp, "fast"));
        break;
      case PARROT_SWITCH_CORE:
        Parrot_runcore_switch(interp, Parrot_str_new_constant(interp, "switch"));
        break;
      case PARROT_CGP_CORE:
        Parrot_runcore_switch(interp, Parrot_str_new_constant(interp, "cgp"));
        break;
      case PARROT_CGOTO_CORE:
        Parrot_runcore_switch(interp, Parrot_str_new_constant(interp, "cgoto"));
        break;
      case PARROT_EXEC_CORE:
        Parrot_runcore_switch(interp, Parrot_str_new_constant(interp, "exec"));
        break;
      case PARROT_GC_DEBUG_CORE:
        Parrot_runcore_switch(interp, Parrot_str_new_constant(interp, "gc_debug"));
        break;
      case PARROT_DEBUGGER_CORE:
        Parrot_runcore_switch(interp, Parrot_str_new_constant(interp, "debugger"));
        break;
      case PARROT_PROFILING_CORE:
        Parrot_runcore_switch(interp, Parrot_str_new_constant(interp, "profiling"));
        break;
      default:
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_UNIMPLEMENTED,
                "Invalid runcore requested\n");
    }
}


/*

=item C<void Parrot_setwarnings(PARROT_INTERP, Parrot_warnclass wc)>

Activates the given warnings.

=cut

*/

PARROT_EXPORT
void
Parrot_setwarnings(PARROT_INTERP, Parrot_warnclass wc)
{
    /* Activates the given warnings.  (Macro from warnings.h.) */
    PARROT_WARNINGS_on(interp, wc);
}


/*

=item C<PackFile * Parrot_pbc_read(PARROT_INTERP, const char *fullname, const
int debug)>

Read in a bytecode, unpack it into a C<PackFile> structure, and do fixups.

=cut

*/

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
PackFile *
Parrot_pbc_read(PARROT_INTERP, ARGIN_NULLOK(const char *fullname), const int debug)
{
    PackFile *pf;
    char     *program_code;
    FILE     *io        = NULL;
    INTVAL    is_mapped = 0;
    INTVAL    program_size;

#ifdef PARROT_HAS_HEADER_SYSMMAN
    int       fd        = -1;
#endif

    if (!fullname || STREQ(fullname, "-")) {
        /* read from STDIN */
        io = stdin;

        /* read 1k at a time */
        program_size = 0;
    }
    else {
        STRING * const fs = string_make(interp, fullname, strlen(fullname),
            NULL, 0);

        /* can't read a file that doesn't exist */
        if (!Parrot_stat_info_intval(interp, fs, STAT_EXISTS)) {
            Parrot_io_eprintf(interp, "Parrot VM: Can't stat %s, code %i.\n",
                    fullname, errno);
            return NULL;
        }

        /* we may need to relax this if we want to read bytecode from pipes */
        if (!Parrot_stat_info_intval(interp, fs, STAT_ISREG)) {
            Parrot_io_eprintf(interp,
                "Parrot VM: '%s', is not a regular file %i.\n",
                fullname, errno);
            return NULL;
        }

        program_size = Parrot_stat_info_intval(interp, fs, STAT_FILESIZE);

#ifndef PARROT_HAS_HEADER_SYSMMAN
        io = fopen(fullname, "rb");

        if (!io) {
            Parrot_io_eprintf(interp, "Parrot VM: Can't open %s, code %i.\n",
                    fullname, errno);
            return NULL;
        }
#endif  /* PARROT_HAS_HEADER_SYSMMAN */

    }
#ifdef PARROT_HAS_HEADER_SYSMMAN
again:
#endif
    /* if we've opened a file (or stdin) with PIO, read it in */
    if (io) {
        char  *cursor;
        size_t chunk_size = program_size > 0 ? program_size : 1024;
        INTVAL wanted     = program_size;
        size_t read_result;

        program_code = mem_allocate_n_typed(chunk_size, char);
        cursor       = program_code;
        program_size = 0;

        while ((read_result = fread(cursor, 1, chunk_size, io)) > 0) {
            program_size += read_result;

            if (program_size == wanted)
                break;

            chunk_size   = 1024;
            mem_realloc_n_typed(program_code, program_size + chunk_size, char);

            if (!program_code) {
                Parrot_io_eprintf(interp,
                            "Parrot VM: Could not reallocate buffer "
                            "while reading packfile from PIO.\n");
                fclose(io);
                return NULL;
            }

            cursor = (char *)(program_code + program_size);
        }

        if (ferror(io)) {
            Parrot_io_eprintf(interp,
             "Parrot VM: Problem reading packfile from PIO:  code %d.\n",
                        ferror(io));
            fclose(io);
            mem_sys_free(program_code);
            return NULL;
        }

        fclose(io);
    }
    else {
        /* if we've gotten here, we opted not to use PIO to read the file.
         * use mmap */

#ifdef PARROT_HAS_HEADER_SYSMMAN

        /* check that fullname isn't NULL, just in case */
        if (!fullname)
            Parrot_ex_throw_from_c_args(interp, NULL, 1,
                "Trying to open a NULL filename");

        fd = open(fullname, O_RDONLY | O_BINARY);

        if (!fd) {
            Parrot_io_eprintf(interp, "Parrot VM: Can't open %s, code %i.\n",
                    fullname, errno);
            return NULL;
        }

        program_code = (char *)mmap(0, (size_t)program_size,
                        PROT_READ, MAP_SHARED, fd, (off_t)0);

        if (program_code == (void *)MAP_FAILED) {
            Parrot_warn(interp, PARROT_WARNINGS_IO_FLAG,
                    "Parrot VM: Can't mmap file %s, code %i.\n",
                    fullname, errno);

            /* try again, now with IO reading the file */
            io = fopen(fullname, "rb");
            if (!io) {
                Parrot_io_eprintf(interp,
                    "Parrot VM: Can't open %s, code %i.\n", fullname, errno);
                return NULL;
            }
            goto again;
        }

        is_mapped = 1;

#else   /* PARROT_HAS_HEADER_SYSMMAN */

        Parrot_io_eprintf(interp, "Parrot VM: uncaught error occurred reading "
                    "file or mmap not available.\n");
        return NULL;

#endif  /* PARROT_HAS_HEADER_SYSMMAN */

    }

    /* Now that we have the bytecode, let's unpack it. */

    pf = PackFile_new(interp, is_mapped);

    /* Make the cmdline option available to the unpackers */
    pf->options = debug;

    if (!PackFile_unpack(interp, pf, (opcode_t *)program_code,
            (size_t)program_size)) {
        Parrot_io_eprintf(interp, "Parrot VM: Can't unpack packfile %s.\n",
                fullname);
        return NULL;
    }

    /* Set :main routine */
    if (!(pf->options & PFOPT_HEADERONLY))
        do_sub_pragmas(interp, pf->cur_cs, PBC_PBC, NULL);

    /* Prederefing the sub/the bytecode is done in switch_to_cs before
     * actual usage of the segment */

#ifdef PARROT_HAS_HEADER_SYSMMAN
    /* the man page states that it's ok to close a mmaped file */
    if (fd >= 0)
        close(fd);
#endif

    return pf;
}


/*

=item C<void Parrot_pbc_load(PARROT_INTERP, PackFile *pf)>

Loads the C<PackFile> returned by C<Parrot_pbc_read()>.

=cut

*/

PARROT_EXPORT
void
Parrot_pbc_load(PARROT_INTERP, NOTNULL(PackFile *pf))
{
    if (!pf) {
        Parrot_io_eprintf(interp, "Invalid packfile\n");
        return;
    }

    interp->initial_pf = pf;
    interp->code       = pf->cur_cs;
}


/*

=item C<void Parrot_pbc_fixup_loaded(PARROT_INTERP)>

Fixups after pbc loading

=cut

*/

PARROT_EXPORT
void
Parrot_pbc_fixup_loaded(PARROT_INTERP)
{
    PackFile_fixup_subs(interp, PBC_LOADED, NULL);
}


/*

=item C<static PMC* setup_argv(PARROT_INTERP, int argc, char **argv)>

Creates and returns C<ARGS> array PMC.

=cut

*/

PARROT_CANNOT_RETURN_NULL
static PMC*
setup_argv(PARROT_INTERP, int argc, ARGIN(char **argv))
{
    ASSERT_ARGS(setup_argv)
    PMC   *userargv = pmc_new(interp, enum_class_ResizableStringArray);
    INTVAL i;

    if (Interp_debug_TEST(interp, PARROT_START_DEBUG_FLAG)) {
        Parrot_io_eprintf(interp,
            "*** Parrot VM: Setting up ARGV array.  Current argc: %d ***\n",
            argc);
    }

    /* immediately anchor pmc to root set */
    VTABLE_set_pmc_keyed_int(interp, interp->iglobals,
            (INTVAL)IGLOBALS_ARGV_LIST, userargv);

    for (i = 0; i < argc; i++) {
        /* Run through argv, adding everything to @ARGS. */
        STRING * const arg =
            string_make(interp, argv[i], strlen(argv[i]), "unicode",
                PObj_external_FLAG);

        if (Interp_debug_TEST(interp, PARROT_START_DEBUG_FLAG))
            Parrot_io_eprintf(interp, "\t%vd: %s\n", i, argv[i]);

        VTABLE_push_string(interp, userargv, arg);
    }

    return userargv;
}


/*

=item C<static const char * op_name(PARROT_INTERP, int k)>

Returns the name of the opcode.

=cut

*/

PARROT_CANNOT_RETURN_NULL
PARROT_OBSERVER
static const char *
op_name(PARROT_INTERP, int k)
{
    ASSERT_ARGS(op_name)
    return interp->op_info_table[k].full_name;
}


/*

=item C<static void print_debug(PARROT_INTERP, int status, void *p)>

Prints GC info.

=cut

*/

static void
print_debug(PARROT_INTERP, SHIM(int status), SHIM(void *p))
{
    ASSERT_ARGS(print_debug)
    if (Interp_debug_TEST(interp, PARROT_MEM_STAT_DEBUG_FLAG)) {
        /* Give souls brave enough to activate debugging an earful about GC. */

        Parrot_io_eprintf(interp, "*** Parrot VM: Dumping GC info ***\n");
        PDB_info(interp);
    }
}


/*

=item C<static PMC* set_current_sub(PARROT_INTERP)>

Search the fixup table for a PMC matching the argument.  On a match,
set up the appropriate context.

If no match, set up a dummy PMC entry.  In either case, return a
pointer to the PMC.

=cut

*/

PARROT_CANNOT_RETURN_NULL
static PMC*
set_current_sub(PARROT_INTERP)
{
    ASSERT_ARGS(set_current_sub)
    Parrot_Sub_attributes *sub_pmc_sub;
    PMC        *sub_pmc;

    PackFile_ByteCode   * const cur_cs = interp->code;
    PackFile_FixupTable * const ft     = cur_cs->fixups;
    PackFile_ConstTable * const ct     = cur_cs->const_table;

    opcode_t    i;

    /*
     * Walk the fixup table.  The first Sub-like entry should be our
     * entry point with the address at our resume_offset.
     */

    for (i = 0; i < ft->fixup_count; i++) {
        if (ft->fixups[i]->type == enum_fixup_sub) {
            const opcode_t ci      = ft->fixups[i]->offset;
            PMC           *sub_pmc = ct->constants[ci]->u.key;
            Parrot_Sub_attributes *sub;

            PMC_get_sub(interp, sub_pmc, sub);
            if (sub->seg == cur_cs) {
                const size_t offs = sub->start_offs;

                if (offs == interp->resume_offset) {
                    Parrot_pcc_set_sub(interp, CURRENT_CONTEXT(interp), sub_pmc);
                    Parrot_pcc_set_HLL(interp, CURRENT_CONTEXT(interp), sub->HLL_id);
                    return sub_pmc;
                }

                break;
            }
        }
    }

    /* if we didn't find anything, put a dummy PMC into current_sub */

    sub_pmc                      = pmc_new(interp, enum_class_Sub);
    PMC_get_sub(interp, sub_pmc, sub_pmc_sub);
    sub_pmc_sub->start_offs      = 0;
    Parrot_pcc_set_sub(interp, CURRENT_CONTEXT(interp), sub_pmc);

    return sub_pmc;
}


/*

=item C<void Parrot_runcode(PARROT_INTERP, int argc, char **argv)>

Sets up C<ARGV> and runs the ops.

=cut

*/

PARROT_EXPORT
void
Parrot_runcode(PARROT_INTERP, int argc, ARGIN(char **argv))
{
    PMC *userargv, *main_sub;

    /* Debugging mode nonsense. */
    if (Interp_debug_TEST(interp, PARROT_START_DEBUG_FLAG)) {
        if (Interp_flags_TEST(interp, PARROT_BOUNDS_FLAG)) {
            Parrot_io_eprintf(interp,
                    "*** Parrot VM: Bounds checking enabled. ***\n");
        }

        if (Interp_trace_TEST(interp, PARROT_TRACE_OPS_FLAG))
            Parrot_io_eprintf(interp, "*** Parrot VM: Tracing enabled. ***\n");

        Parrot_io_eprintf(interp, "*** Parrot VM: %Ss core ***\n",
                interp->run_core->name);
    }

    /* Set up @ARGS (or whatever this language calls it) in userargv. */
    userargv = setup_argv(interp, argc, argv);

    /*
     * If any profile information was gathered, print it out
     * before exiting, then print debug infos if turned on.
     */
    Parrot_on_exit(interp, print_debug,   NULL);

    /* Let's kick the tires and light the fires--call interpreter.c:runops. */
    main_sub = Parrot_pcc_get_sub(interp, CURRENT_CONTEXT(interp));

    /* if no sub was marked being :main, we create a dummy sub with offset 0 */

    if (!main_sub)
        main_sub = set_current_sub(interp);

    Parrot_pcc_set_sub(interp, CURRENT_CONTEXT(interp), NULL);
    Parrot_pcc_set_constants(interp, interp->ctx, interp->code->const_table->constants);

    Parrot_pcc_invoke_sub_from_c_args(interp, main_sub, "P->", userargv);
}


/*

=item C<opcode_t * Parrot_debug(PARROT_INTERP, Parrot_Interp debugger, opcode_t
* pc)>

Runs the interpreter's bytecode in debugging mode.

=cut

*/

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
opcode_t *
Parrot_debug(PARROT_INTERP, NOTNULL(Parrot_Interp debugger), opcode_t * pc)
{
    const char *command;
    PDB_t      * const pdb = debugger->pdb;

    pdb->cur_opcode        = pc;

    PDB_init(debugger, NULL);

    /* disassemble needs this for now */
    /*
    interp               = pdb->debugee;
    interp->pdb          = pdb;
    */
    debugger->lo_var_ptr = interp->lo_var_ptr;

    PDB_disassemble(interp, NULL);

    while (!(pdb->state & PDB_EXIT)) {
        PDB_get_command(debugger);
        command = pdb->cur_command;
        PDB_run_command(debugger, command);
    }

    return NULL;
}


/*

=item C<static void print_constant_table(PARROT_INTERP)>

Prints the contents of the constants table.

=cut

*/
static void
print_constant_table(PARROT_INTERP)
{
    ASSERT_ARGS(print_constant_table)
    const INTVAL numconstants = interp->code->const_table->const_count;
    INTVAL i;

    /* TODO: would be nice to print the name of the file as well */
    Parrot_io_printf(interp, "=head1 Constant-table\n\n");

    for (i = 0; i < numconstants; ++i) {
        const PackFile_Constant * const c = interp->code->const_table->constants[i];

        switch (c->type) {
          case PFC_NUMBER:
            Parrot_io_printf(interp, "PMC_CONST(%d): %f\n", i, c->u.number);
            break;
          case PFC_STRING:
            Parrot_io_printf(interp, "PMC_CONST(%d): %S\n", i, c->u.string);
            break;
          case PFC_KEY:
            Parrot_io_printf(interp, "PMC_CONST(%d): ", i);
            /* XXX */
            /* Parrot_print_p(interp, c->u.key); */
            Parrot_io_printf(interp, "(PMC constant)");
            Parrot_io_printf(interp, "\n");
            break;
          case PFC_PMC:
            {
                Parrot_io_printf(interp, "PMC_CONST(%d): ", i);

                switch (c->u.key->vtable->base_type) {
                    /* each PBC file has a ParrotInterpreter, but it can't
                     * stringify by itself */
                  case enum_class_ParrotInterpreter:
                    Parrot_io_printf(interp, "'ParrotInterpreter'");
                    break;

                    /* FixedIntegerArrays used for signatures, handy to print */
                  case enum_class_FixedIntegerArray:
                    {
                        INTVAL n = VTABLE_elements(interp, c->u.key);
                        INTVAL i;
                        Parrot_io_printf(interp, "[");

                        for (i = 0; i < n; ++i) {
                            INTVAL val = VTABLE_get_integer_keyed_int(interp, c->u.key, i);
                            Parrot_io_printf(interp, "%d", val);
                            if (i < n - 1)
                                Parrot_io_printf(interp, ",");
                        }
                        Parrot_io_printf(interp, "]");
                        break;
                    }
                  case enum_class_NameSpace:
                  case enum_class_String:
                  case enum_class_Key:
                  case enum_class_ResizableStringArray:
                    {
                        /*Parrot_print_p(interp, c->u.key);*/
                        STRING * const s = VTABLE_get_string(interp, c->u.key);
                        if (s)
                            Parrot_io_printf(interp, "%Ss", s);
                        break;
                    }
                  case enum_class_Sub:
                    Parrot_io_printf(interp, "%S", VTABLE_get_string(interp, c->u.key));
                    break;
                  default:
                    Parrot_io_printf(interp, "(PMC constant)");
                    break;
                }

                Parrot_io_printf(interp, "\n");
                break;
            }
          default:
            Parrot_io_printf(interp, "wrong constant type in constant table!\n");
            /* XXX throw an exception? Is it worth the trouble? */
            break;
        }
    }

    Parrot_io_printf(interp, "\n=cut\n\n");
}


/*

=item C<void Parrot_disassemble(PARROT_INTERP, const char *outfile,
Parrot_disassemble_options options)>

Disassembles and prints out the interpreter's bytecode.

This is used by the Parrot disassembler.

=cut

*/

PARROT_EXPORT
void
Parrot_disassemble(PARROT_INTERP, SHIM(const char *outfile), Parrot_disassemble_options options)
{
    PDB_line_t *line;
    PDB_t * const pdb   = mem_allocate_zeroed_typed(PDB_t);
    int num_mappings    = 0;
    int curr_mapping    = 0;
    int op_code_seq_num = 0;
    int debugs;

    interp->pdb     = pdb;
    pdb->cur_opcode = interp->code->base.data;

    PDB_disassemble(interp, NULL);

    line   = pdb->file->line;
    debugs = (interp->code->debugs != NULL);

    print_constant_table(interp);
    if (options & enum_DIS_HEADER)
        return;

    if (!(options & enum_DIS_BARE))
        Parrot_io_printf(interp, "# %12s-%12s", "Seq_Op_Num", "Relative-PC");

    if (debugs) {
        if (!(options & enum_DIS_BARE))
            Parrot_io_printf(interp, " %6s:\n", "SrcLn#");
        num_mappings = interp->code->debugs->num_mappings;
    }
    else {
        Parrot_io_printf(interp, "\n");
    }

    while (line->next) {
        const char *c;

        /* Parrot_io_printf(interp, "%i < %i %i == %i \n", curr_mapping,
         * num_mappings, op_code_seq_num,
         * interp->code->debugs->mappings[curr_mapping]->offset); */

        if (debugs && curr_mapping < num_mappings) {
            if (op_code_seq_num == interp->code->debugs->mappings[curr_mapping]->offset) {
                const int filename_const_offset =
                    interp->code->debugs->mappings[curr_mapping]->filename;
                Parrot_io_printf(interp, "# Current Source Filename '%Ss'\n",
                        interp->code->const_table->constants[filename_const_offset]->u.string);
                curr_mapping++;
            }
        }

        if (!(options & enum_DIS_BARE))
            Parrot_io_printf(interp, "%012i-%012i",
                             op_code_seq_num, line->opcode - interp->code->base.data);

        if (debugs && !(options & enum_DIS_BARE))
            Parrot_io_printf(interp, " %06i: ",
                    interp->code->debugs->base.data[op_code_seq_num]);

        /* If it has a label print it */
        if (line->label)
            Parrot_io_printf(interp, "L%li:\t", line->label->number);
        else
            Parrot_io_printf(interp, "\t");

        c = pdb->file->source + line->source_offset;

        while (c && *c != '\n')
            Parrot_io_printf(interp, "%c", *(c++));

        Parrot_io_printf(interp, "\n");
        line = line->next;
        op_code_seq_num++;
    }

    return;
}


/*

=item C<void Parrot_run_native(PARROT_INTERP, native_func_t func)>

Runs the C function C<func> through the program C<[enternative, end]>.  This
ensures that the function runs with the same setup as in other run loops.

This function is used in some of the source tests in F<t/src> which use
the interpreter outside a runloop.

=cut

*/

PARROT_EXPORT
void
Parrot_run_native(PARROT_INTERP, native_func_t func)
{
    PackFile * const pf = PackFile_new(interp, 0);
    static opcode_t program_code[2];

    program_code[0] = interp->op_lib->op_code("enternative", 0);
    program_code[1] = 0; /* end */

    pf->cur_cs = (PackFile_ByteCode *)
        (pf->PackFuncs[PF_BYTEC_SEG].new_seg)(interp, pf,
                Parrot_str_new_constant(interp, "code"), 1);
    pf->cur_cs->base.data = program_code;
    pf->cur_cs->base.size = 2;

    Parrot_pbc_load(interp, pf);

    run_native = func;

    if (interp->code && interp->code->const_table)
        Parrot_pcc_set_constants(interp, interp->ctx, interp->code->const_table->constants);

    runops(interp, interp->resume_offset);
}


/*

=item C<Parrot_PMC Parrot_compile_string(PARROT_INTERP, Parrot_String type,
const char *code, Parrot_String *error)>

Compiles a code string.

=cut

*/

PARROT_EXPORT
Parrot_PMC
Parrot_compile_string(PARROT_INTERP, Parrot_String type,
        const char *code, Parrot_String *error)
{
    /* For the benefit of embedders that do not load any pbc
     * before compiling a string */

    if (!interp->initial_pf) {
        PackFile * const pf = PackFile_new_dummy(interp,
                Parrot_str_new_constant(interp, "compile_string"));
        /* Assumption: there is no valid reason to fail to create it.
         * If the assumption changes, replace the assertion with a
         * runtime check */
        PARROT_ASSERT(interp->initial_pf);
    }

    if (Parrot_str_compare(interp, Parrot_str_new(interp, "PIR", 3), type) == 0)
        return IMCC_compile_pir_s(interp, code, error);

    if (Parrot_str_compare(interp, Parrot_str_new(interp, "PASM", 4), type) == 0)
        return IMCC_compile_pasm_s(interp, code, error);

    *error = Parrot_str_new(interp, "Invalid interpreter type", 0);
    return NULL;
}


/*

=back

=head1 SEE ALSO

F<include/parrot/embed.h> and F<docs/embed.pod>.

=head1 HISTORY

Initial version by Brent Dax on 2002.1.28.

=cut

*/

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
