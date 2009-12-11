/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 NAME

src/runcore/main.c - main functions for Parrot runcores

=head1 DESCRIPTION

The runcore API handles running the operations.

The predereferenced code chunk is pre-initialized with the opcode
function pointers, addresses, or opnumbers of the C<prederef__>
opcode. This opcode then calls the C<do_prederef()> function, which then
fills in the real function, address or op number.

Because the C<prederef__> opcode returns the same C<pc_prederef> it was
passed, the runops loop will re-execute the same location, which will
then have the pointer to the real C<prederef> opfunc and C<prederef>
args.

Pointer arithmetic is used to determine the index into the bytecode
corresponding to the currect opcode. The bytecode and prederef arrays
have the same number of elements because there is a one-to-one mapping.

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "parrot/runcore_api.h"
#include "parrot/runcore_profiling.h"
#include "parrot/oplib/core_ops.h"
#include "parrot/oplib/core_ops_switch.h"
#include "parrot/oplib/ops.h"
#include "main.str"

#ifdef HAVE_COMPUTED_GOTO
#  include "parrot/oplib/core_ops_cg.h"
#  include "parrot/oplib/core_ops_cgp.h"
#endif
#include "parrot/dynext.h"
#include "pmc/pmc_parrotlibrary.h"
#include "pmc/pmc_callcontext.h"


/* HEADERIZER HFILE: include/parrot/runcore_api.h */
/* XXX Needs to get done at the same time as the other interpreter files */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static void dynop_register_switch(size_t n_old, size_t n_new);
static void dynop_register_xx(PARROT_INTERP,
    size_t n_old,
    size_t n_new,
    oplib_init_f init_func)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static oplib_init_f get_dynamic_op_lib_init(SHIM_INTERP,
    ARGIN(const PMC *lib))
        __attribute__nonnull__(2);

static void notify_func_table(PARROT_INTERP,
    ARGIN(op_func_t *table),
    int on)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void prederef_args(
    ARGMOD(void **pc_prederef),
    PARROT_INTERP,
    ARGIN(const opcode_t *pc),
    ARGIN(const op_info_t *opinfo))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*pc_prederef);

static void stop_prederef(PARROT_INTERP)
        __attribute__nonnull__(1);

static void turn_ev_check(PARROT_INTERP, int on)
        __attribute__nonnull__(1);

#define ASSERT_ARGS_dynop_register_switch __attribute__unused__ int _ASSERT_ARGS_CHECK = (0)
#define ASSERT_ARGS_dynop_register_xx __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_get_dynamic_op_lib_init __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(lib))
#define ASSERT_ARGS_notify_func_table __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(table))
#define ASSERT_ARGS_prederef_args __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(pc_prederef) \
    , PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pc) \
    , PARROT_ASSERT_ARG(opinfo))
#define ASSERT_ARGS_stop_prederef __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_turn_ev_check __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

/*

=item C<void Parrot_runcore_init(PARROT_INTERP)>

Initializes the runcores.

=cut

*/

void
Parrot_runcore_init(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_runcore_init)
    STRING * const default_core = CONST_STRING(interp, "fast");

    interp->cores        = NULL;
    interp->num_cores    = 0;

    Parrot_runcore_slow_init(interp);
    Parrot_runcore_fast_init(interp);
    Parrot_runcore_switch_init(interp);

    Parrot_runcore_exec_init(interp);
    Parrot_runcore_gc_debug_init(interp);
    Parrot_runcore_debugger_init(interp);

    Parrot_runcore_profiling_init(interp);

    /* set the default runcore */
    Parrot_runcore_switch(interp, default_core);

#ifdef HAVE_COMPUTED_GOTO
    Parrot_runcore_cgp_init(interp);
    Parrot_runcore_cgoto_init(interp);
#endif
}


/*

=item C<INTVAL Parrot_runcore_register(PARROT_INTERP, Parrot_runcore_t
*coredata)>

Registers a new runcore with Parrot.  Returns 1 on success, 0 on failure.

=cut

*/

PARROT_EXPORT
INTVAL
Parrot_runcore_register(PARROT_INTERP, ARGIN(Parrot_runcore_t *coredata))
{
    ASSERT_ARGS(Parrot_runcore_register)
    size_t num_cores = ++interp->num_cores;

    mem_realloc_n_typed(interp->cores, num_cores, Parrot_runcore_t *);

    interp->cores[num_cores - 1] = coredata;

    return 1;
}


/*

=item C<void Parrot_runcore_switch(PARROT_INTERP, STRING *name)>

Switches to a named runcore.  Throws an exception on an unknown runcore.

=cut

*/

PARROT_EXPORT
void
Parrot_runcore_switch(PARROT_INTERP, ARGIN(STRING *name))
{
    ASSERT_ARGS(Parrot_runcore_switch)

    size_t num_cores = interp->num_cores;
    size_t i;

    if (interp->run_core
    &&  Parrot_str_equal(interp, name, interp->run_core->name))
        return;

    for (i = 0; i < num_cores; ++i) {
        if (Parrot_str_equal(interp, name, interp->cores[i]->name)) {
            interp->run_core = interp->cores[i];
            return;
        }
    }

    Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_UNIMPLEMENTED,
        "Invalid runcore %Ss requested\n", name);
}


/*

=item C<static void prederef_args(void **pc_prederef, PARROT_INTERP, const
opcode_t *pc, const op_info_t *opinfo)>

Called from C<do_prederef()> to deal with any arguments.

C<pc_prederef> is the current opcode.

=cut

*/

static void
prederef_args(ARGMOD(void **pc_prederef), PARROT_INTERP,
        ARGIN(const opcode_t *pc), ARGIN(const op_info_t *opinfo))
{
    ASSERT_ARGS(prederef_args)
    const PackFile_ConstTable * const const_table = interp->code->const_table;

    const int regs_n = Parrot_pcc_get_regs_used(interp, CURRENT_CONTEXT(interp), REGNO_NUM);
    const int regs_i = Parrot_pcc_get_regs_used(interp, CURRENT_CONTEXT(interp), REGNO_INT);
    const int regs_p = Parrot_pcc_get_regs_used(interp, CURRENT_CONTEXT(interp), REGNO_PMC);
    const int regs_s = Parrot_pcc_get_regs_used(interp, CURRENT_CONTEXT(interp), REGNO_STR);

    /* prederef var part too */
    const int m = opinfo->op_count;
    int       n = opinfo->op_count;
    int       i;

    ADD_OP_VAR_PART(interp, interp->code, pc, n);
    for (i = 1; i < n; i++) {
        const opcode_t arg = pc[i];
        int type;
        if (i >= m) {
            PMC * const sig = (PMC*) pc_prederef[1];
            type = VTABLE_get_integer_keyed_int(interp, sig, i - m);
            type &= (PARROT_ARG_TYPE_MASK | PARROT_ARG_CONSTANT);
        }
        else
            type = opinfo->types[i - 1];

        switch (type) {

          case PARROT_ARG_KI:
          case PARROT_ARG_I:
            if (arg < 0 || arg >= regs_i)
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INTERP_ERROR,
                    "Illegal register number");

            pc_prederef[i] = (void *)REG_OFFS_INT(arg);
            break;

          case PARROT_ARG_N:
            if (arg < 0 || arg >= regs_n)
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INTERP_ERROR,
                    "Illegal register number");

            pc_prederef[i] = (void *)REG_OFFS_NUM(arg);
            break;

          case PARROT_ARG_K:
          case PARROT_ARG_P:
            if (arg < 0 || arg >= regs_p)
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INTERP_ERROR,
                    "Illegal register number");

            pc_prederef[i] = (void *)REG_OFFS_PMC(arg);
            break;

          case PARROT_ARG_S:
            if (arg < 0 || arg >= regs_s)
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INTERP_ERROR,
                    "Illegal register number");

            pc_prederef[i] = (void *)REG_OFFS_STR(arg);
            break;

          case PARROT_ARG_KIC:
          case PARROT_ARG_IC:
            pc_prederef[i] = (void *)pc[i];
            break;

          case PARROT_ARG_NC:
            if (arg < 0 || arg >= const_table->const_count)
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INTERP_ERROR,
                    "Illegal constant number");

            pc_prederef[i] = (void *)&const_table->constants[arg]->u.number;
            break;

          case PARROT_ARG_SC:
            if (arg < 0 || arg >= const_table->const_count)
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INTERP_ERROR,
                    "Illegal constant number");

            pc_prederef[i] = (void *)const_table->constants[arg]->u.string;
            break;

          case PARROT_ARG_PC:
          case PARROT_ARG_KC:
            if (arg < 0 || arg >= const_table->const_count)
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INTERP_ERROR,
                    "Illegal constant number");

            pc_prederef[i] = (void *)const_table->constants[arg]->u.key;
            break;
          default:
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_ARG_OP_NOT_HANDLED,
                "Unhandled argtype 0x%x\n", type);
            break;
        }
    }
}


/*

=item C<void do_prederef(void **pc_prederef, PARROT_INTERP, Parrot_runcore_t
*runcore)>

This is called from within the run cores to predereference the current
opcode.

C<pc_prederef> is the current opcode, and C<type> is the run core type.

=cut

*/

void
do_prederef(ARGIN(void **pc_prederef), PARROT_INTERP, ARGIN(Parrot_runcore_t *runcore))
{
    ASSERT_ARGS(do_prederef)
    const size_t     offset = pc_prederef - interp->code->prederef.code;
    opcode_t * const pc     = ((opcode_t *)interp->code->base.data) + offset;
    const op_info_t *opinfo;
    size_t           n;

    if (*pc < 0 || *pc >= (opcode_t)interp->op_count)
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INTERP_ERROR,
            "Illegal opcode");

    opinfo = &interp->op_info_table[*pc];

    prederef_args(pc_prederef, interp, pc, opinfo);

    if (PARROT_RUNCORE_PREDEREF_OPS_TEST(runcore)) {
        *pc_prederef = PARROT_RUNCORE_CGOTO_OPS_TEST(runcore)
            ? ((void **)interp->op_lib->op_func_table)[*pc]
            : (void**)*pc;
    }
    else
        Parrot_ex_throw_from_c_args(interp, NULL, 1,
            "Tried to prederef wrong core");

    /* now remember backward branches, invoke and similar opcodes */
    n = opinfo->op_count;

    if (((opinfo->jump & PARROT_JUMP_RELATIVE)
    &&    opinfo->types[n - 2] == PARROT_ARG_IC
    &&    pc[n - 1] < 0)                          /* relative backward branch */
    ||   (opinfo->jump & PARROT_JUMP_ADDRESS)) {
        Prederef * const pi = &interp->code->prederef;

        /* first time prederef.branches == NULL:
         * estimate size to 1/16th of opcodes */
        if (!pi->branches) {
            size_t nb = interp->code->base.size / 16;
            if (nb < 8)
                nb = (size_t)8;

            pi->branches    = mem_allocate_n_typed(nb, Prederef_branch);
            pi->n_allocated = nb;
            pi->n_branches  = 0;
        }
        else if (pi->n_branches >= pi->n_allocated) {
            pi->n_allocated = (size_t) (pi->n_allocated * 1.5);
            mem_realloc_n_typed(pi->branches, pi->n_allocated, Prederef_branch);
        }

        pi->branches[pi->n_branches].offs = offset;
        pi->branches[pi->n_branches].op   = *pc_prederef;

        ++pi->n_branches;
    }
}


/*

=item C<static void turn_ev_check(PARROT_INTERP, int on)>

Turn on or off event checking for prederefed cores.

Fills in the C<event_checker> opcode, or restores original ops in all
branch locations of the opcode stream.

Note that when C<on> is true, this is being called from the event
handler thread.

=cut

*/

static void
turn_ev_check(PARROT_INTERP, int on)
{
    ASSERT_ARGS(turn_ev_check)
    const Prederef * const pi = &interp->code->prederef;
    size_t i;

    if (!pi->branches)
        return;

    for (i = 0; i < pi->n_branches; ++i) {
        const size_t offs = pi->branches[i].offs;
        if (on)
            interp->code->prederef.code[offs] =
                ((void **)interp->op_lib->op_func_table)
                            [CORE_OPS_check_events__];
        else
            interp->code->prederef.code[offs] = pi->branches[i].op;
    }
}


/*

=item C<static oplib_init_f get_dynamic_op_lib_init(PARROT_INTERP, const PMC
*lib)>

Returns an dynamic oplib's opcode's library C<op_lib> init function.

C<lib> will be a C<ParrotLibrary> PMC.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static oplib_init_f
get_dynamic_op_lib_init(SHIM_INTERP, ARGIN(const PMC *lib))
{
    ASSERT_ARGS(get_dynamic_op_lib_init)
    return (oplib_init_f)D2FPTR(
            ((Parrot_ParrotLibrary_attributes *)PMC_data(lib))->oplib_init);
}


/*

=item C<static void stop_prederef(PARROT_INTERP)>

Restore the interpreter's op function tables to their initial state.
Also recreate the event function pointers. This is only necessary
for run-core changes, but we don't know the old run core.

=cut

*/

static void
stop_prederef(PARROT_INTERP)
{
    ASSERT_ARGS(stop_prederef)
    interp->op_func_table = PARROT_CORE_OPLIB_INIT(1)->op_func_table;

    if (interp->evc_func_table) {
        mem_sys_free(interp->evc_func_table);
        interp->evc_func_table = NULL;
    }

    Parrot_setup_event_func_ptrs(interp);
}


/*

=item C<void prepare_for_run(PARROT_INTERP)>

Prepares to run the interpreter's run core.

=cut

*/

void
prepare_for_run(PARROT_INTERP)
{
    ASSERT_ARGS(prepare_for_run)
    const runcore_prepare_fn_type prepare_run = interp->run_core->prepare_run;

    if (prepare_run)
        (*prepare_run)(interp, interp->run_core);
}


/*

=item C<void runops_int(PARROT_INTERP, size_t offset)>

Run Parrot operations of loaded code segment until an end opcode is
reached.  Run core is selected depending on the C<Interp_flags>.  When a
C<restart> opcode is encountered, a different core may be selected and
evaluation of opcode continues.

=cut

*/

void
runops_int(PARROT_INTERP, size_t offset)
{
    ASSERT_ARGS(runops_int)

    /* setup event function ptrs */
    if (!interp->save_func_table)
        Parrot_setup_event_func_ptrs(interp);

    interp->resume_offset = offset;
    interp->resume_flag  |= RESUME_RESTART;

    while (interp->resume_flag & RESUME_RESTART) {
        opcode_t * const pc = (opcode_t *)
            interp->code->base.data + interp->resume_offset;
        const runcore_runops_fn_type core = interp->run_core->runops;

        interp->resume_offset = 0;
        interp->resume_flag  &= ~(RESUME_RESTART | RESUME_INITIAL);

        (*core)(interp, interp->run_core, pc);

        /* if we have fallen out with resume and we were running CGOTO, set
         * the stacktop again to a sane value, so that restarting the runloop
         * is ok. */
        if (interp->resume_flag & RESUME_RESTART) {
            if ((int)interp->resume_offset < 0)
                Parrot_ex_throw_from_c_args(interp, NULL, 1,
                    "branch_cs: illegal resume offset");
            stop_prederef(interp);
        }
    }
}


/*

=item C<void Parrot_setup_event_func_ptrs(PARROT_INTERP)>

Setup a C<func_table> containing pointers (or addresses) of the
C<check_event__> opcode.

TODO: Free it at destroy. Handle run-core changes.

=cut

*/

void
Parrot_setup_event_func_ptrs(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_setup_event_func_ptrs)
    const size_t       n         = interp->op_count;
    const oplib_init_f init_func = get_core_op_lib_init(interp, interp->run_core);
    op_lib_t * const   lib       = init_func(1);

    /* remember op_func_table */
    interp->save_func_table      = lib->op_func_table;

    if (!lib->op_func_table)
        return;

    /* function or CG core - prepare func_table */
    if (!interp->evc_func_table) {
        size_t i;

        interp->evc_func_table = mem_allocate_n_typed(n, op_func_t);

        for (i = 0; i < n; ++i)
            interp->evc_func_table[i] = (op_func_t)
                D2FPTR(((void**)lib->op_func_table)[CORE_OPS_check_events__]);
    }
}


/*

=item C<void Parrot_runcore_destroy(PARROT_INTERP)>

Shuts down the runcores and deallocates any dynops memory.

=cut

*/

void
Parrot_runcore_destroy(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_runcore_destroy)
    op_lib_t         *cg_lib;
    size_t            num_cores = interp->num_cores;
    size_t            i;

    for (i = 0; i < num_cores; ++i) {
        Parrot_runcore_t        *core    = interp->cores[i];
        runcore_destroy_fn_type  destroy = core->destroy;

        if (destroy)
            (*destroy)(interp, core);

        mem_sys_free(core);
    }

    if (interp->cores)
        mem_sys_free(interp->cores);

    interp->cores    = NULL;
    interp->run_core = NULL;

    /* dynop libs */
    if (interp->n_libs <= 0)
        return;

#ifdef HAVE_COMPUTED_GOTO
    cg_lib = PARROT_CORE_CGP_OPLIB_INIT(1);

    if (cg_lib->op_func_table)
        mem_sys_free(cg_lib->op_func_table);
    cg_lib->op_func_table = NULL;

    cg_lib = PARROT_CORE_CG_OPLIB_INIT(1);
    if (cg_lib->op_func_table)
        mem_sys_free(cg_lib->op_func_table);
    cg_lib->op_func_table = NULL;
#endif

    mem_sys_free(interp->op_info_table);
    mem_sys_free(interp->op_func_table);
    interp->op_info_table = NULL;
    interp->op_func_table = NULL;
}


/*

=back

=head2 Dynamic Loading Functions

=over 4

=item C<void dynop_register(PARROT_INTERP, PMC *lib_pmc)>

Register a dynamic oplib.

=cut

*/

void
dynop_register(PARROT_INTERP, ARGIN(PMC *lib_pmc))
{
    ASSERT_ARGS(dynop_register)
    op_lib_t *lib, *core;
    oplib_init_f init_func;
    op_func_t *new_func_table, *new_evc_func_table;
    op_info_t *new_info_table;
    size_t i, n_old, n_new, n_tot;

    if (n_interpreters > 1) {
        /* This is not supported because oplibs are always shared.
         * If we mem_sys_reallocate() the op_func_table while another
         * interpreter is running using that exact op_func_table,
         * this will cause problems
         * Also, the mapping from op name to op number is global even for
         * dynops (!). The mapping is done by get_op in core_ops.c (even for
         * dynops) and uses a global hash as a cache and relies on modifications
         * to the static-scoped core_op_lib data structure to see dynops.
         */
        Parrot_ex_throw_from_c_args(interp, NULL, 1, "loading a new dynoplib while "
            "more than one thread is running is not supported.");
    }

    if (!interp->all_op_libs)
        interp->all_op_libs = (op_lib_t **)mem_sys_allocate(
                sizeof (op_lib_t *) * (interp->n_libs + 1));
    else
        mem_realloc_n_typed(interp->all_op_libs, interp->n_libs + 1,
                op_lib_t *);

    init_func = get_dynamic_op_lib_init(interp, lib_pmc);
    lib       = init_func(1);

    interp->all_op_libs[interp->n_libs++] = lib;

    /* if we are registering an op_lib variant, called from below the base
     * names of this lib and the previous one are the same */
    if (interp->n_libs >= 2
    && (STREQ(interp->all_op_libs[interp->n_libs-2]->name, lib->name))) {
        /* registering is handled below */
        return;
    }

    /* when called from yyparse, we have to set up the evc_func_table */
    Parrot_setup_event_func_ptrs(interp);

    n_old = interp->op_count;
    n_new = lib->op_count;
    n_tot = n_old + n_new;
    core  = PARROT_CORE_OPLIB_INIT(1);

    PARROT_ASSERT(interp->op_count == core->op_count);

    new_evc_func_table = (op_func_t *)mem_sys_realloc(interp->evc_func_table,
            sizeof (op_func_t) * n_tot);
    if (core->flags & OP_FUNC_IS_ALLOCATED) {
        new_func_table = (op_func_t *)mem_sys_realloc(core->op_func_table,
                sizeof (op_func_t) * n_tot);
        new_info_table = (op_info_t *)mem_sys_realloc(core->op_info_table,
                sizeof (op_info_t) * n_tot);
    }
    else {
        /* allocate new op_func and info tables */
        new_func_table = mem_allocate_n_typed(n_tot, op_func_t);
        new_info_table = mem_allocate_n_typed(n_tot, op_info_t);

        /* copy old */
        for (i = 0; i < n_old; ++i) {
            new_func_table[i] = interp->op_func_table[i];
            new_info_table[i] = interp->op_info_table[i];
        }
    }

    /* add new */
    for (i = n_old; i < n_tot; ++i) {
        new_func_table[i] = ((op_func_t*)lib->op_func_table)[i - n_old];
        new_info_table[i] = lib->op_info_table[i - n_old];

        /*
         * fill new ops of event checker func table
         * if we are running a different core, entries are
         * changed below
         */
        new_evc_func_table[i] = new_func_table[CORE_OPS_check_events__];
    }

    interp->evc_func_table  = new_evc_func_table;
    interp->save_func_table = new_func_table;

    /* deinit core, so that it gets rehashed */
    (void) PARROT_CORE_OPLIB_INIT(0);

    /* set table */
    core->op_func_table = interp->op_func_table = new_func_table;
    core->op_info_table = interp->op_info_table = new_info_table;
    core->op_count      = interp->op_count      = n_tot;
    core->flags         = OP_FUNC_IS_ALLOCATED | OP_INFO_IS_ALLOCATED;

    /* done for plain core */
#ifdef HAVE_COMPUTED_GOTO
    dynop_register_xx(interp, n_old, n_new, PARROT_CORE_CGP_OPLIB_INIT);
    dynop_register_xx(interp, n_old, n_new, PARROT_CORE_CG_OPLIB_INIT);
#endif

    dynop_register_switch(n_old, n_new);
}


/*

=item C<static void dynop_register_xx(PARROT_INTERP, size_t n_old, size_t n_new,
oplib_init_f init_func)>

Register C<op_lib> with other cores.

=cut

*/

static void
dynop_register_xx(PARROT_INTERP,
        size_t n_old, size_t n_new, oplib_init_f init_func)
{
    ASSERT_ARGS(dynop_register_xx)
    const size_t n_tot    = n_old + n_new;
    op_func_t   *ops_addr = NULL;
    op_lib_t    *cg_lib   = init_func(1);
    op_lib_t    *new_lib;

#if 0
    /* related to CG and CGP ops issue below */
    STRING *op_variant;
#endif

    oplib_init_f new_init_func;
    PMC *lib_variant;

    if (cg_lib->flags & OP_FUNC_IS_ALLOCATED) {
        ops_addr = (op_func_t *)mem_sys_realloc(cg_lib->op_func_table,
                n_tot * sizeof (op_func_t));
    }
    else {
        size_t i;

        ops_addr      = mem_allocate_n_typed(n_tot, op_func_t);
        cg_lib->flags = OP_FUNC_IS_ALLOCATED;

        for (i = 0; i < n_old; ++i)
            ops_addr[i] = cg_lib->op_func_table[i];
    }

    /*
     * XXX running CG and CGP ops currently works only via the wrapper
     *
     * the problem is:
     *  The actual runcores cg_core and cgp_core are very big functions.
     *  The C compiler usually addresses "spilled" registers in the C stack.
     *  The loaded opcode lib is another possibly big function, but with
     *  a likely different stack layout. Directly jumping around between
     *  code locations in these two opcode functions works, but access
     *  to stack-ed (or spilled) variables fails badly.
     *
     *  We would need to prepare the assembly source of the opcode
     *  lib so that all variable access on the stack has the same
     *  layout and compile the prepared assembly to ops_cgp?.o
     *
     *  The switched core is different anyway, as we can't extend the
     *  compiled big switch statement with the new cases. We have
     *  always to use the wrapper__ opcode called from the default case.
     */
#if 0
    /* check if the lib_pmc exists with a _xx flavor */
    new_init_func = get_op_lib_init(0, 0, lib_pmc);
    new_lib       = new_init_func(1);
    op_variant    = Parrot_sprintf_c(interp, "%s_ops%s",
                        new_lib->name, cg_lib->suffix);
    lib_variant   = Parrot_load_lib(interp, op_variant, NULL);

    /* XXX running CG and CGP ops currently works only via the wrapper */
    if (0 /*lib_variant */) {
        size_t i;

        new_init_func = get_dynamic_op_lib_init(interp, lib_variant);
        new_lib       = new_init_func(1);

        for (i = n_old; i < n_tot; ++i)
            ops_addr[i] = (new_lib->op_func_table)[i - n_old];

        new_lib->op_func_table = ops_addr;
        new_lib->op_count      = n_tot;

        new_init_func((long) ops_addr);
    }
    else
#endif
    {
        size_t i;

        /* if not install wrappers */
        /* fill new entries with the wrapper op */
        for (i = n_old; i < n_tot; ++i)
            ops_addr[i] = ops_addr[CORE_OPS_wrapper__];
    }

    /* if we are running this core, update event check ops */
    if (interp->run_core->id == cg_lib->core_type) {
        size_t i;

        for (i = n_old; i < n_tot; ++i)
            interp->evc_func_table[i] =
                (op_func_t)ops_addr[CORE_OPS_check_events__];
        interp->save_func_table = ops_addr;
    }

    /* tell the cg_core about the new jump table */
    cg_lib->op_func_table = ops_addr;
    cg_lib->op_count      = n_tot;
    init_func((long) ops_addr);
}


/*

=item C<static void dynop_register_switch(size_t n_old, size_t n_new)>

Used only at the end of dynop_register.  Sums the old and new op_counts
storing the result into the operations count field of the interpreter
object.

=cut

*/

static void
dynop_register_switch(size_t n_old, size_t n_new)
{
    ASSERT_ARGS(dynop_register_switch)
    op_lib_t * const lib = PARROT_CORE_SWITCH_OPLIB_INIT(1);
    lib->op_count        = n_old + n_new;
}


/*

=item C<static void notify_func_table(PARROT_INTERP, op_func_t *table, int on)>

Tell the interpreter's running core about the new function table.

=cut

*/

static void
notify_func_table(PARROT_INTERP, ARGIN(op_func_t *table), int on)
{
    ASSERT_ARGS(notify_func_table)
    const oplib_init_f init_func = get_core_op_lib_init(interp, interp->run_core);

    init_func((long) table);

    if (PARROT_RUNCORE_FUNC_TABLE_TEST(interp->run_core)) {
        PARROT_ASSERT(table);
        interp->op_func_table = table;
    }

    if (PARROT_RUNCORE_EVENT_CHECK_TEST(interp->run_core))
        turn_ev_check(interp, on);
}


/*

=item C<void disable_event_checking(PARROT_INTERP)>

Restore old function table.

XXX This is only implemented for the function core at present.

=cut

*/

PARROT_EXPORT
void
disable_event_checking(PARROT_INTERP)
{
    ASSERT_ARGS(disable_event_checking)
    /* restore func table */
    PARROT_ASSERT(interp->save_func_table);
    notify_func_table(interp, interp->save_func_table, 0);
}


/*

=item C<void enable_event_checking(PARROT_INTERP)>

Replace func table with one that does event checking for all opcodes.

NOTE: C<enable_event_checking()> is called async by the event handler
thread. All action done from here has to be async safe.

XXX This is only implemented for the function core at present.

=cut

*/

PARROT_EXPORT
void
enable_event_checking(PARROT_INTERP)
{
    ASSERT_ARGS(enable_event_checking)
    /* put table in place */
    notify_func_table(interp, interp->evc_func_table, 1);
}


/*

=back

=head1 SEE ALSO

F<include/parrot/interpreter.h>, F<src/interp/inter_cb.c>,
F<src/interp/inter_create.c>, F<src/interp/inter_misc.c>, F<src/call/ops.c>.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
