/*
Copyright (C) 2001-2006, The Perl Foundation.
$Id$

=head1 NAME

src/interpreter.c - Parrot Interpreter

=head1 DESCRIPTION

The interpreter API handles running the operations.

The predereferenced code chunk is pre-initialized with the opcode
function pointers, addresses, or opnumbers of the C<prederef__>
opcode. This opcode then calls the C<do_prederef()> function, which then
fills in the real function, address or op number.

Since the C<prederef__> opcode returns the same C<pc_prederef> it was
passed, the runops loop will re-execute the same location, which will
then have the pointer to the real C<prederef> opfunc and C<prederef>
args.

Pointer arithmetic is used to determine the index into the bytecode
corresponding to the currect opcode. The bytecode and prederef arrays
have the same number of elements since there is a one-to-one mapping.

=head2 Functions

=over 4

=cut

*/

#include <assert.h>
#include "parrot/parrot.h"
#include "interp_guts.h"
#include "parrot/oplib/core_ops.h"
#include "parrot/oplib/core_ops_switch.h"
#include "parrot/oplib/ops.h"
#include "runops_cores.h"
#if JIT_CAPABLE
#  include "parrot/exec.h"
#  include "parrot/jit.h"
#endif
#ifdef HAVE_COMPUTED_GOTO
#  include "parrot/oplib/core_ops_cg.h"
#  include "parrot/oplib/core_ops_cgp.h"
#endif
#include "parrot/dynext.h"


static void setup_event_func_ptrs(Parrot_Interp interpreter);

/*

=item C<static void
prederef_args(void **pc_prederef, Interp *interpreter,
        opcode_t *pc, op_info_t *opinfo)>

Called from C<do_prederef()> to deal with any arguments.

C<pc_prederef> is the current opcode.

=cut

*/

static void
prederef_args(void **pc_prederef, Interp *interpreter,
        opcode_t *pc, op_info_t *opinfo)
{
    struct PackFile_ConstTable * const_table
        = interpreter->code->const_table;
    int i, n, m, regs_n, regs_i, regs_p, regs_s;
    PMC *sig = NULL;

    regs_n = CONTEXT(interpreter->ctx)->n_regs_used[REGNO_NUM];
    regs_i = CONTEXT(interpreter->ctx)->n_regs_used[REGNO_INT];
    regs_p = CONTEXT(interpreter->ctx)->n_regs_used[REGNO_PMC];
    regs_s = CONTEXT(interpreter->ctx)->n_regs_used[REGNO_STR];
    /* prederef var part too */
    n = m = opinfo->op_count; 
    ADD_OP_VAR_PART(interpreter, interpreter->code, pc, n);
    for (i = 1; i < n; i++) {
        opcode_t arg = pc[i];
        int type;
        if (i >= m) {
	    sig = (PMC*) pc_prederef[1]; 
	    type = SIG_ITEM(sig, i - m);
            type &= (PARROT_ARG_TYPE_MASK | PARROT_ARG_CONSTANT);       
        }
	else
	    type = opinfo->types[i - 1];

        switch (type) {

        case PARROT_ARG_KI:
        case PARROT_ARG_I:
            if (arg < 0 || arg >= regs_i)
                internal_exception(INTERP_ERROR, "Illegal register number");
            pc_prederef[i] = (void *)REG_OFFS_INT(arg);
            break;

        case PARROT_ARG_N:
            if (arg < 0 || arg >= regs_n)
                internal_exception(INTERP_ERROR, "Illegal register number");
            pc_prederef[i] = (void *)REG_OFFS_NUM(arg);
            break;

        case PARROT_ARG_K:
        case PARROT_ARG_P:
            if (arg < 0 || arg >= regs_p)
                internal_exception(INTERP_ERROR, "Illegal register number");
            pc_prederef[i] = (void *)REG_OFFS_PMC(arg);
            break;

        case PARROT_ARG_S:
            if (arg < 0 || arg >= regs_s)
                internal_exception(INTERP_ERROR, "Illegal register number");
            pc_prederef[i] = (void *)REG_OFFS_STR(arg);
            break;

        case PARROT_ARG_KIC:
        case PARROT_ARG_IC:
            pc_prederef[i] = (void *)pc[i];
            break;

        case PARROT_ARG_NC:
            if (arg < 0 || arg >= const_table->const_count)
                internal_exception(INTERP_ERROR, "Illegal constant number");
            pc_prederef[i] = (void *) &const_table->constants[arg]->u.number;
            break;

        case PARROT_ARG_SC:
            if (arg < 0 || arg >= const_table->const_count)
                internal_exception(INTERP_ERROR, "Illegal constant number");
            pc_prederef[i] = (void *)const_table->constants[arg]->u.string;
            break;

        case PARROT_ARG_PC:
        case PARROT_ARG_KC:
            if (arg < 0 || arg >= const_table->const_count)
                internal_exception(INTERP_ERROR, "Illegal constant number");
            pc_prederef[i] = (void *)const_table->constants[arg]->u.key;
            break;
        default:
            internal_exception(ARG_OP_NOT_HANDLED,
                               "Unhandled argtype 0x%x\n", type);
            break;
        }
    }
}

/*

=item C<void
do_prederef(void **pc_prederef, Parrot_Interp interpreter, int type)>

This is called from within the run cores to predereference the current
opcode.

C<pc_prederef> is the current opcode, and C<type> is the run core type.

=cut

*/

void
do_prederef(void **pc_prederef, Parrot_Interp interpreter, int type)
{
    size_t offset = pc_prederef - interpreter->code->prederef.code;
    opcode_t *pc = ((opcode_t *)interpreter->code->base.data) + offset;
    op_func_t *prederef_op_func = interpreter->op_lib->op_func_table;
    op_info_t *opinfo;
    size_t n;

    if (*pc < 0 || *pc >= (opcode_t)interpreter->op_count)
        internal_exception(INTERP_ERROR, "Illegal opcode");
    opinfo = &interpreter->op_info_table[*pc];
    /* first arguments - PIC needs it */
    prederef_args(pc_prederef, interpreter, pc, opinfo);
    switch (type) {
        case PARROT_SWITCH_CORE:
        case PARROT_SWITCH_JIT_CORE:
        case PARROT_CGP_CORE:
        case PARROT_CGP_JIT_CORE:
            parrot_PIC_prederef(interpreter, *pc, pc_prederef, type);
            break;
        default:
            internal_exception(1, "Tried to prederef wrong core");
            break;
    }
    /*
     * now remember backward branches, invoke and similar opcodes
     */
    n = opinfo->op_count;
    if (((opinfo->jump & PARROT_JUMP_RELATIVE) &&
            opinfo->types[n - 2] == PARROT_ARG_IC &&
            pc[n - 1] < 0) ||   /* relative backward branch */
            (opinfo->jump & PARROT_JUMP_ADDRESS)) {
        Prederef *pi = &interpreter->code->prederef;
        /*
         * first time prederef.branches == NULL:
         * estimate size to 1/16th of opcodes
         */
        if (!pi->branches) {
            size_t nb = interpreter->code->base.size / 16;
            if (nb < 8)
                nb = (size_t)8;
            pi->branches = mem_sys_allocate( sizeof(Prederef_branch) * nb);
            pi->n_allocated = nb;
            pi->n_branches = 0;
        }
        else if (pi->n_branches >= pi->n_allocated) {
            pi->n_allocated = (size_t) (pi->n_allocated * 1.5);
            pi->branches = mem_sys_realloc( pi->branches,
                    sizeof(Prederef_branch) * pi->n_allocated);
        }
        pi->branches[pi->n_branches].offs = offset;
        pi->branches[pi->n_branches].op = *pc_prederef;
        ++pi->n_branches;
    }
}

/*

=item C<static void
turn_ev_check(Parrot_Interp interpreter, int on)>

Turn on or off event checking for prederefed cores.

Fills in the C<event_checker> opcode, or restores original op in all
branch locations of the opcode stream.

Note that when C<on> is true, this is being called from the event
handler thread.

=cut

*/

static void
turn_ev_check(Parrot_Interp interpreter, int on)
{
    Prederef *pi = &interpreter->code->prederef;
    size_t i, offs;

    if (!pi->branches)
        return;
    for (i = 0; i < pi->n_branches; ++i) {
        offs = pi->branches[i].offs;
        if (on) {
            interpreter->code->prederef.code[offs] =
                ((void **)interpreter->op_lib->op_func_table)
                            [CORE_OPS_check_events__];
        }
        else
            interpreter->code->prederef.code[offs] = pi->branches[i].op;
    }
}

/*

=item C<static oplib_init_f
get_op_lib_init(int core_op, int which, PMC *lib)>

Returns an opcode's library C<op_lib> init function.

C<core_op> indicates whether the opcode represents a core Parrot operation.

C<which> is the run core type.

For dynamic oplibs C<core_op> will be 0 and C<lib> will be a
C<ParrotLibrary> PMC.

=cut

*/

static oplib_init_f
get_op_lib_init(int core_op, int which, PMC *lib)
{
    oplib_init_f init_func = (oplib_init_f)NULL;
    if (core_op) {
        switch (which) {
            case PARROT_SWITCH_CORE:
            case PARROT_SWITCH_JIT_CORE:
                init_func = PARROT_CORE_SWITCH_OPLIB_INIT;
                break;
#ifdef HAVE_COMPUTED_GOTO
            case PARROT_CGP_CORE:
            case PARROT_CGP_JIT_CORE:
                init_func = PARROT_CORE_CGP_OPLIB_INIT;
                break;
            case PARROT_CGOTO_CORE:
                init_func = PARROT_CORE_CG_OPLIB_INIT;
                break;
#endif
            case PARROT_EXEC_CORE:      /* normal func core */
            case PARROT_JIT_CORE:      /* normal func core */
            case PARROT_SLOW_CORE:      /* normal func core */
            case PARROT_FAST_CORE:      /* normal func core */
                init_func = PARROT_CORE_OPLIB_INIT;
                break;
        }
        if (!init_func)
            internal_exception(1, "Couldn't find init_func for core %d", which);
        return init_func;
    }
    return (oplib_init_f) D2FPTR(PMC_struct_val(lib));
}

/*

=item C<static void
load_prederef(Interp *interpreter, int which)>

C<< interpreter->op_lib >> = prederefed oplib.

=cut

*/

static void
load_prederef(Interp *interpreter, int which)
{
    oplib_init_f init_func = get_op_lib_init(1, which, NULL);
    int (*get_op)(const char * name, int full);

    get_op = interpreter->op_lib->op_code;
    interpreter->op_lib = init_func(1);
    /* preserve the get_op function */
    interpreter->op_lib->op_code = get_op;
    if (interpreter->op_lib->op_count != interpreter->op_count)
        internal_exception(PREDEREF_LOAD_ERROR,
                "Illegal op count (%d) in prederef oplib\n",
                (int)interpreter->op_lib->op_count);
}

/*

=item C<static void
init_prederef(Interp *interpreter, int which)>

Initialize: load prederef C<func_table>, file prederef.code.

=cut

*/

static void
init_prederef(Interp *interpreter, int which)
{
    load_prederef(interpreter, which);
    if (!interpreter->code->prederef.code) {
        size_t N = interpreter->code->base.size;
        opcode_t *pc = interpreter->code->base.data;
        size_t i, n, n_pics;
        void *pred_func;
        op_info_t *opinfo;
/* Parrot_memalign_if_possible in OpenBSD allocates 256 if you ask for 312
   -- Need to verify this, it may have been a bug elsewhere. If it works now,
   we can remove the mem_sys_allocate_zeroed line below. */
#if 0
        void **temp = (void **)mem_sys_allocate_zeroed(N * sizeof(void *));
#else
        void **temp = (void **)Parrot_memalign_if_possible(256,
                N * sizeof(void *));
#endif
	/*
	 * calc and remember pred_offset
	 */
	CONTEXT(interpreter->ctx)->pred_offset = pc - (opcode_t*)temp;

        /* fill with the prederef__ opcode function */
        if (which == PARROT_SWITCH_CORE || which == PARROT_SWITCH_JIT_CORE )
            pred_func = (void*) CORE_OPS_prederef__;
        else
            pred_func = ((void **)
                    interpreter->op_lib->op_func_table)[CORE_OPS_prederef__];
        for (i = n_pics = 0; i < N; ) {
            opinfo = &interpreter->op_info_table[*pc];
            temp[i] = pred_func;
            n = opinfo->op_count;
	    ADD_OP_VAR_PART(interpreter, interpreter->code, pc, n);
            /* count ops that need a PIC */
            if (parrot_PIC_op_is_cached(interpreter, *pc))
                n_pics++;
            pc += n;
            i += n;
        }

        interpreter->code->prederef.code = temp;
        /* allocate pic store */
        if (n_pics) {
            /* pic_index is starting from 1 */
            parrot_PIC_alloc_store(interpreter, interpreter->code, n_pics + 1);
        }
    }
}

/*

=item C<static void
stop_prederef(Interp *interpreter)>

Restore the interpreter's op function tables to their initial state.
Also the event function pointers are re-created. This is only necessary
for run-core changes, but we don't know the old run core.

=cut

*/

static void
stop_prederef(Interp *interpreter)
{
    interpreter->op_func_table = PARROT_CORE_OPLIB_INIT(1)->op_func_table;
    if (interpreter->evc_func_table) {
        mem_sys_free(interpreter->evc_func_table);
        interpreter->evc_func_table = NULL;
    }
    setup_event_func_ptrs(interpreter);
}

#if EXEC_CAPABLE

/*

=item C<void
exec_init_prederef(Interp *interpreter, void *prederef_arena)>

C<< interpreter->op_lib >> = prederefed oplib

The "normal" C<op_lib> has a copy in the interpreter structure - but get
the C<op_code> lookup function from standard core prederef has no
C<op_info_table>

=cut

*/

void
exec_init_prederef(Interp *interpreter, void *prederef_arena)
{
    load_prederef(interpreter, PARROT_CGP_CORE);

    if (!interpreter->code->prederef.code) {
        size_t N = interpreter->code->base.size;
        void **temp = prederef_arena;
        opcode_t *pc = interpreter->code->base.data;

        interpreter->code->prederef.code = temp;
        /* TODO */
    }
}
#endif

/*

=item C<void *
init_jit(Interp *interpreter, opcode_t *pc)>

Initializes JIT function for the specified opcode and returns it.

=cut

*/

void *
init_jit(Interp *interpreter, opcode_t *pc)
{
#if JIT_CAPABLE
    opcode_t *code_start;
    UINTVAL code_size;          /* in opcodes */
    opcode_t *code_end;
    Parrot_jit_info_t *jit_info;

    if (interpreter->code->jit_info)
        return ((Parrot_jit_info_t *)interpreter->code->jit_info)->arena.start;

    code_start = interpreter->code->base.data;
    code_size = interpreter->code->base.size;
    code_end = code_start + code_size;
#  if defined HAVE_COMPUTED_GOTO && PARROT_I386_JIT_CGP
#    ifdef __GNUC__
#      ifdef PARROT_I386
    init_prederef(interpreter, PARROT_CGP_CORE);
#      endif
#    endif
#  endif

    interpreter->code->jit_info = 
        jit_info = parrot_build_asm(interpreter, code_start, code_end, 
            NULL, JIT_CODE_FILE);
    return jit_info->arena.start;
#else
    return NULL;
#endif
}

/*

=item C<void
prepare_for_run(Parrot_Interp interpreter)>

Prepares to run the interpreter's run core.

=cut

*/

void
prepare_for_run(Parrot_Interp interpreter)
{
    switch (interpreter->run_core) {
        case PARROT_JIT_CORE:
            (void) init_jit(interpreter, interpreter->code->base.data);
            break;
        case PARROT_SWITCH_CORE:
        case PARROT_SWITCH_JIT_CORE:
        case PARROT_CGP_CORE:
        case PARROT_CGP_JIT_CORE:
            init_prederef(interpreter, interpreter->run_core);
            break;
        default:
            break;
    }
}

#ifdef PARROT_EXEC_OS_AIX
extern void* aix_get_toc( );
#endif

/*

=item C<static opcode_t *
runops_jit(Interp *interpreter, opcode_t *pc)>

Runs the JIT code for the specified opcode.

=cut

*/

static opcode_t *
runops_jit(Interp *interpreter, opcode_t *pc)
{
#if JIT_CAPABLE
#  ifdef PARROT_EXEC_OS_AIX
    /* AIX calling convention requires that function-call-by-ptr be made
       through the following struct: */
    struct { jit_f functPtr; void *toc; void *env; } ptrgl_t;
    ptrgl_t.functPtr = (jit_f) D2FPTR(init_jit(interpreter, pc));
    ptrgl_t.env = NULL;

    /* r2 (TOC) needs to point back here so we can return from non-JIT
       functions */
    ptrgl_t.toc = aix_get_toc( );

    ((jit_f) D2FPTR(&ptrgl_t)) (interpreter, pc);
#  else
    jit_f jit_code = (jit_f) D2FPTR(init_jit(interpreter, pc));
    (jit_code) (interpreter, pc);
#  endif
#endif
    return NULL;
}

/*

=item C<static opcode_t *
runops_exec(Interp *interpreter, opcode_t *pc)>

Runs the native executable version of the specified opcode.

=cut

*/

static opcode_t *
runops_exec(Interp *interpreter, opcode_t *pc)
{
#if EXEC_CAPABLE
    opcode_t *code_start;
    UINTVAL code_size;          /* in opcodes */
    opcode_t *code_end;
    extern int Parrot_exec_run;

    code_start = interpreter->code->base.data;
    code_size = interpreter->code->base.size;
    code_end = code_start + code_size;
#  if defined HAVE_COMPUTED_GOTO && defined USE_CGP
#    ifdef __GNUC__
#      ifdef PARROT_I386
    init_prederef(interpreter, PARROT_CGP_CORE);
#      endif
#    endif
#  endif
    if (Parrot_exec_run == 2) {
        Parrot_exec_run = 0;
        Interp_core_SET(interpreter, PARROT_JIT_CORE);
        runops_jit(interpreter, pc);
        Interp_core_SET(interpreter, PARROT_EXEC_CORE);
    }
    else if (Parrot_exec_run == 1) {
        Parrot_exec(interpreter, pc, code_start, code_end);
    }
    else
        run_native(interpreter, pc, code_start);

#endif
    return NULL;
}


/*

=item C<static opcode_t *
runops_cgp(Interp *interpreter, opcode_t *pc)>

Runs the C C<goto>, predereferenced core.

=cut

*/

static opcode_t *
runops_cgp(Interp *interpreter, opcode_t *pc)
{
#ifdef HAVE_COMPUTED_GOTO
    opcode_t *code_start = (opcode_t *)interpreter->code->base.data;
    void **pc_prederef;
    init_prederef(interpreter, PARROT_CGP_CORE);
    pc_prederef = interpreter->code->prederef.code + (pc - code_start);
    pc = cgp_core((opcode_t*)pc_prederef, interpreter);
    return pc;
#else
    PIO_eprintf(interpreter,
            "Computed goto unavailable in this configuration.\n");
    Parrot_exit(1);
    return NULL;
#endif
}

/*

=item C<static opcode_t *
runops_switch(Interp *interpreter, opcode_t *pc)>

Runs the C<switch> core.

=cut

*/

static opcode_t *
runops_switch(Interp *interpreter, opcode_t *pc)
{
    opcode_t *code_start = (opcode_t *)interpreter->code->base.data;
    void **pc_prederef;
    init_prederef(interpreter, PARROT_SWITCH_CORE);
    pc_prederef = interpreter->code->prederef.code + (pc - code_start);
    pc = switch_core((opcode_t*)pc_prederef, interpreter);
    return pc;
}

/*

=item C<void
runops_int(Interp *interpreter, size_t offset)>

Run parrot operations of loaded code segment until an end opcode is
reached run core is selected depending on the C<Interp_flags> when a
C<restart> opcode is encountered a different core my be selected and
evaluation of opcode continues.

=cut

*/

void
runops_int(Interp *interpreter, size_t offset)
{
    int lo_var_ptr;
    opcode_t *(*core) (Interp *, opcode_t *) =
        (opcode_t *(*) (Interp *, opcode_t *)) 0;

    if (!interpreter->lo_var_ptr) {
        /*
         * if we are entering the run loop the first time
         */
        interpreter->lo_var_ptr = (void *)&lo_var_ptr;
    }

    /*
     * setup event function ptrs
     */
    if (!interpreter->save_func_table) {
        setup_event_func_ptrs(interpreter);
    }

    interpreter->resume_offset = offset;
    interpreter->resume_flag |= RESUME_RESTART;

    while (interpreter->resume_flag & RESUME_RESTART) {
        opcode_t *pc = (opcode_t *)
            interpreter->code->base.data + interpreter->resume_offset;

        interpreter->resume_offset = 0;
        interpreter->resume_flag &= ~(RESUME_RESTART | RESUME_INITIAL);
        switch (interpreter->run_core) {
            case PARROT_SLOW_CORE:

                core = runops_slow_core;

                if (Interp_flags_TEST(interpreter, PARROT_PROFILE_FLAG)) {
                    core = runops_profile_core;
                    if (interpreter->profile == NULL) {
                        interpreter->profile = (RunProfile *)
                            mem_sys_allocate_zeroed(sizeof(RunProfile));
                        interpreter->profile->data = (ProfData *)
                            mem_sys_allocate_zeroed((interpreter->op_count +
                                        PARROT_PROF_EXTRA) * sizeof(ProfData));
                    }
                }
                break;
            case PARROT_FAST_CORE:
                core = runops_fast_core;
                break;
            case PARROT_CGOTO_CORE:
#ifdef HAVE_COMPUTED_GOTO
                core = runops_cgoto_core;
#else
                internal_exception(1, "Error: PARROT_CGOTO_CORE not available");
#endif
                break;
            case PARROT_CGP_CORE:
            case PARROT_CGP_JIT_CORE:
#ifdef HAVE_COMPUTED_GOTO
                core = runops_cgp;
#else
                internal_exception(1, "Error: PARROT_CGP_CORE not available");
#endif
                break;
            case PARROT_SWITCH_CORE:
            case PARROT_SWITCH_JIT_CORE:
                core = runops_switch;
                break;
            case PARROT_JIT_CORE:
#if !JIT_CAPABLE
                internal_exception(JIT_UNAVAILABLE,
                        "Error: PARROT_JIT_FLAG is set, "
                        "but interpreter is not JIT_CAPABLE!\n");
#endif
                core = runops_jit;
                break;
            case PARROT_EXEC_CORE:
#if !EXEC_CAPABLE
                internal_exception(EXEC_UNAVAILABLE,
                        "Error: PARROT_EXEC_FLAG is set, "
                        "but interpreter is not EXEC_CAPABLE!\n");
#endif
                core = runops_exec;
                break;
            default:
                internal_exception(UNIMPLEMENTED,
                        "ambigious runcore switch used");
                break;
        }


        /* run it finally */
        core(interpreter, pc);
        /* if we have fallen out with resume and we were running CGOTO, set
         * the stacktop again to a sane value, so that restarting the runloop
         * is ok.
         */
        if (interpreter->resume_flag & RESUME_RESTART) {
            if ((int)interpreter->resume_offset < 0)
                internal_exception(1, "branch_cs: illegal resume offset");
            stop_prederef(interpreter);
        }
    }
}



/*

=item C<static void
setup_event_func_ptrs(Parrot_Interp interpreter)>

Setup a C<func_table> containing pointers (or addresses) of the
C<check_event__> opcode.

TODO: Free it at destroy. Handle run-core changes.

=cut

*/

static void
setup_event_func_ptrs(Parrot_Interp interpreter)
{
    size_t i, n = interpreter->op_count;
    oplib_init_f init_func = get_op_lib_init(1, interpreter->run_core, NULL);
    op_lib_t *lib = init_func(1);
    /*
     * remember op_func_table
     */
    interpreter->save_func_table = lib->op_func_table;
    if (!lib->op_func_table)
        return;
    /* function or CG core - prepare func_table */
    if (!interpreter->evc_func_table) {
        interpreter->evc_func_table = mem_sys_allocate(sizeof(void *) * n);
        for (i = 0; i < n; ++i)
            interpreter->evc_func_table[i] = (op_func_t)
                D2FPTR(((void**)lib->op_func_table)[CORE_OPS_check_events__]);
    }
}


/*

=back

=head2 Dynamic Loading Functions

=over 4

=cut

*/

static void dynop_register_xx(Parrot_Interp, PMC*, size_t, size_t,
        oplib_init_f init_func);
static void dynop_register_switch(Parrot_Interp, PMC*, size_t, size_t);

/*

=item C<void
dynop_register(Parrot_Interp interpreter, PMC* lib_pmc)>

Register a dynamic oplib.

=cut

*/

void
dynop_register(Parrot_Interp interpreter, PMC* lib_pmc)
{
    op_lib_t *lib, *core;
    oplib_init_f init_func;
    op_func_t *new_func_table, *new_evc_func_table;
    op_info_t *new_info_table;
    size_t i, n_old, n_new, n_tot;

    if (!interpreter->all_op_libs)
        interpreter->all_op_libs = mem_sys_allocate(
                sizeof(op_lib_t *) * (interpreter->n_libs + 1));
    else
        interpreter->all_op_libs = mem_sys_realloc(interpreter->all_op_libs,
                sizeof(op_lib_t *) * (interpreter->n_libs + 1));

    init_func = get_op_lib_init(0, 0, lib_pmc);
    lib = init_func(1);

    interpreter->all_op_libs[interpreter->n_libs++] = lib;
    /*
     * if we are registering an op_lib variant, called from below
     * the base names of this lib and the previous one are the same
     */
    if (interpreter->n_libs >= 2 &&
            !strcmp(interpreter->all_op_libs[interpreter->n_libs-2]->name,
                lib->name)) {
        /* registering is handled below */
        return;
    }
    /*
     * when called from yyparse, we have to set up the evc_func_table
     */
    setup_event_func_ptrs(interpreter);

    n_old = interpreter->op_count;
    n_new = lib->op_count;
    n_tot = n_old + n_new;
    core = PARROT_CORE_OPLIB_INIT(1);

    assert(interpreter->op_count == core->op_count);
    new_evc_func_table = mem__sys_realloc(interpreter->evc_func_table,
            sizeof (void *) * n_tot);
    if (core->flags & OP_FUNC_IS_ALLOCATED) {
        new_func_table = mem_sys_realloc(core->op_func_table,
                sizeof (void *) * n_tot);
        new_info_table = mem_sys_realloc(core->op_info_table,
                sizeof (op_info_t) * n_tot);
    }
    else {
        /*
         * allocate new op_func and info tables
         */
        new_func_table = mem_sys_allocate(sizeof (void *) * n_tot);
        new_info_table = mem_sys_allocate(sizeof (op_info_t) * n_tot);
        /* copy old */
        for (i = 0; i < n_old; ++i) {
            new_func_table[i] = interpreter->op_func_table[i];
            new_info_table[i] = interpreter->op_info_table[i];
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
        new_evc_func_table[i] =
            interpreter->op_func_table[CORE_OPS_check_events__];
    }
    interpreter->evc_func_table = new_evc_func_table;
    interpreter->save_func_table = new_func_table;
    /*
     * deinit core, so that it gets rehashed
     */
    (void) PARROT_CORE_OPLIB_INIT(0);
    /* set table */
    core->op_func_table = interpreter->op_func_table = new_func_table;
    core->op_info_table = interpreter->op_info_table = new_info_table;
    core->op_count = interpreter->op_count = n_tot;
    core->flags = OP_FUNC_IS_ALLOCATED | OP_INFO_IS_ALLOCATED;
    /* done for plain core */
#if defined HAVE_COMPUTED_GOTO
    dynop_register_xx(interpreter, lib_pmc, n_old, n_new,
            PARROT_CORE_CGP_OPLIB_INIT);
    dynop_register_xx(interpreter, lib_pmc, n_old, n_new,
            PARROT_CORE_CG_OPLIB_INIT);
#endif
    dynop_register_switch(interpreter, lib_pmc, n_old, n_new);
}

/*

=item C<static void
dynop_register_xx(Parrot_Interp interpreter, PMC* lib_pmc,
        size_t n_old, size_t n_new, oplib_init_f init_func)>

Register C<op_lib> with other cores.

=cut

*/

static void
dynop_register_xx(Parrot_Interp interpreter, PMC* lib_pmc,
        size_t n_old, size_t n_new, oplib_init_f init_func)
{
    op_lib_t *cg_lib, *new_lib;
    void **ops_addr = NULL;
    size_t i, n_tot;
#if 0
    /* related to CG and CGP ops issue below */
    STRING *op_variant;
#endif
    oplib_init_f new_init_func;
    PMC *lib_variant;

    n_tot = n_old + n_new;
    cg_lib = init_func(1);

    if (cg_lib->flags & OP_FUNC_IS_ALLOCATED) {
        ops_addr = mem_sys_realloc(cg_lib->op_func_table,
                n_tot * sizeof(void *));
    }
    else {
        ops_addr = mem_sys_allocate(n_tot * sizeof(void *));
        cg_lib->flags = OP_FUNC_IS_ALLOCATED;
        for (i = 0; i < n_old; ++i)
            ops_addr[i] = ((void **)cg_lib->op_func_table)[i];
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
    new_lib = new_init_func(1);
    op_variant = Parrot_sprintf_c(interpreter, "%s_ops%s",
            new_lib->name, cg_lib->suffix);
    lib_variant = Parrot_load_lib(interpreter, op_variant, NULL);
#endif
    /*
     * XXX running CG and CGP ops currently works only via the wrapper
     */
    if (0 /*lib_variant */) {
        new_init_func = get_op_lib_init(0, 0, lib_variant);
        new_lib = new_init_func(1);
        for (i = n_old; i < n_tot; ++i)
            ops_addr[i] = ((void **)new_lib->op_func_table)[i - n_old];
        new_lib->op_func_table = (void *) ops_addr;
        new_lib->op_count = n_tot;
        new_init_func((long) ops_addr);
    }
    else {
        /* if not install wrappers */
        /* fill new entries with the wrapper op */
        for (i = n_old; i < n_tot; ++i)
            ops_addr[i] = ((void **)cg_lib->op_func_table)[CORE_OPS_wrapper__];
    }
    /*
     * if we are running this core, update event check ops
     */
    if ((int)interpreter->run_core == cg_lib->core_type) {
        for (i = n_old; i < n_tot; ++i)
            interpreter->evc_func_table[i] =
                (op_func_t)D2FPTR(ops_addr[CORE_OPS_check_events__]);
        interpreter->save_func_table = (void *) ops_addr;
    }
    /*
     * tell the cg_core about the new jump table
     */
    cg_lib->op_func_table = (void *) ops_addr;
    cg_lib->op_count = n_tot;
    init_func((long) ops_addr);
}

/*

=item C<static void
dynop_register_switch(Parrot_Interp interpreter, PMC* lib_pmc,
        size_t n_old, size_t n_new)>

Description.

=cut

*/

static void
dynop_register_switch(Parrot_Interp interpreter, PMC* lib_pmc,
        size_t n_old, size_t n_new)
{
    op_lib_t *lib = PARROT_CORE_SWITCH_OPLIB_INIT(1);
    lib->op_count = n_old + n_new;
}

/*

=item C<static void
notify_func_table(Parrot_Interp interpreter, void* table, int on)>

Tell the interpreter's running core about the new function table.

=cut

*/

static void
notify_func_table(Parrot_Interp interpreter, void* table, int on)
{
    oplib_init_f init_func = get_op_lib_init(1, interpreter->run_core, NULL);
    op_lib_t *lib = init_func(1);
    init_func((long) table);
    switch (interpreter->run_core) {
        case PARROT_SLOW_CORE:      /* normal func core */
        case PARROT_FAST_CORE:      /* normal func core */
        case PARROT_CGOTO_CORE:      /* cgoto address list  */
            interpreter->op_func_table = table;
            break;
        case PARROT_CGP_CORE:
            turn_ev_check(interpreter, on);
            break;
        default:
            break;
    }
}

/*

=item C<void
disable_event_checking(Parrot_Interp interpreter)>

Restore old function table.

XXX This is only implemented for the function core at present.

=cut

*/

void
disable_event_checking(Parrot_Interp interpreter)
{
    /*
     * restore func table
     */
    notify_func_table(interpreter, interpreter->save_func_table, 0);
}

/*

=item C<void
enable_event_checking(Parrot_Interp interpreter)>

Replace func table with one that does event checking for all opcodes.

NOTE: C<enable_event_checking()> is called async by the event handler
thread. All action done from here has to be async safe.

XXX This is only implemented for the function core at present.

=cut

*/

void
enable_event_checking(Parrot_Interp interpreter)
{
    /*
     * put table in place
     */
    notify_func_table(interpreter, interpreter->evc_func_table, 1);
}


/*

=back

=head1 SEE ALSO

F<include/parrot/interpreter.h>, F<src/inter_cb.c>, F<src/inter_create.c>,
 F<src/inter_misc.c>, F<src/inter_run.c>.

=cut

*/

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
