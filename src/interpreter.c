/* interpreter.c
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  CVS Info
 *     $Id$
 *  Overview:
 *     The interpreter api handles running the operations
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#include <assert.h>
#include "parrot/parrot.h"
#include "parrot/interp_guts.h"
#include "parrot/oplib/core_ops.h"
#include "parrot/oplib/core_ops_prederef.h"
#include "parrot/oplib/core_ops_switch.h"
#include "parrot/runops_cores.h"
#if JIT_CAPABLE
#  include "parrot/exec.h"
#  include "parrot/jit.h"
#endif
#ifdef HAVE_COMPUTED_GOTO
#  include "parrot/oplib/core_ops_cg.h"
#  include "parrot/oplib/core_ops_cgp.h"
#endif
#include "parrot/method_util.h"
#include "parrot/dynext.h"

#define ATEXIT_DESTROY

#if EXEC_CAPABLE
struct Parrot_Interp interpre;
#endif

static void setup_default_compreg(Parrot_Interp interpreter);
static void setup_event_func_ptrs(Parrot_Interp interpreter);

/*=for api interpreter prederef
 *
 * Predereference the current opcode.
 *
 * The prederef code chunk is pre-initialized with opcode function
 * pointers (or addresses or opnumbers) of the prederef__ opcode.
 * This opcode call the do_prederef function, which fills in the
 * real function (address/op number)
 *
 * Since the prederef__ opcode returns the
 * same pc_prederef it was passed, the runops loop will re-execute
 * the same location, which will then have the pointer to the real
 * prederef opfunc and prederef args.
 *
 * The initial few lines of pointer arithmetic are used to determine
 * the index into the bytecode corresponding to the currect pc_prederef.
 * The bytecode and prederef arrays have the same number of elements
 * since there is a one-to-one mapping.
 */

static void
prederef_args(void **pc_prederef, struct Parrot_Interp *interpreter,
        opcode_t *pc)
{
    op_info_t *opinfo;
    struct PackFile_ConstTable * const_table = interpreter->code->const_table;
    int i;

    opinfo = &interpreter->op_info_table[*pc];
    for (i = 1; i < opinfo->arg_count; i++) {
        opcode_t arg = pc[i];

        switch (opinfo->types[i]) {

        case PARROT_ARG_KI:
        case PARROT_ARG_I:
            if (arg < 0 || arg >= NUM_REGISTERS)
                internal_exception(INTERP_ERROR, "Illegal register number");
            pc_prederef[i] = (void *)&interpreter->int_reg.registers[arg];
            break;

        case PARROT_ARG_N:
            if (arg < 0 || arg >= NUM_REGISTERS)
                internal_exception(INTERP_ERROR, "Illegal register number");
            pc_prederef[i] = (void *)&interpreter->num_reg.registers[arg];
            break;

        case PARROT_ARG_K:
        case PARROT_ARG_P:
            if (arg < 0 || arg >= NUM_REGISTERS)
                internal_exception(INTERP_ERROR, "Illegal register number");
            pc_prederef[i] = (void *)&interpreter->pmc_reg.registers[arg];
            break;

        case PARROT_ARG_S:
            if (arg < 0 || arg >= NUM_REGISTERS)
                internal_exception(INTERP_ERROR, "Illegal register number");
            pc_prederef[i] = (void *)&interpreter->string_reg.registers[arg];
            break;

        case PARROT_ARG_KIC:
        case PARROT_ARG_IC:
            pc_prederef[i] = (void *)&pc[i];
            break;

        case PARROT_ARG_NC:
            if (arg < 0 || arg >= const_table->const_count)
                internal_exception(INTERP_ERROR, "Illegal constant number");
            pc_prederef[i] = (void *) &const_table->constants[arg]->u.number;
            break;

        case PARROT_ARG_PC:
            if (arg < 0 || arg >= const_table->const_count)
                internal_exception(INTERP_ERROR, "Illegal constant number");
/*        pc_prederef[i] = (void *)
                 &const_table->constants[arg]->pmc; */
            internal_exception(ARG_OP_NOT_HANDLED,
                               "PMC constants not yet supported!\n");
            break;

        case PARROT_ARG_SC:
            if (arg < 0 || arg >= const_table->const_count)
                internal_exception(INTERP_ERROR, "Illegal constant number");
            pc_prederef[i] = (void *)
                &const_table->constants[arg]->u.string;
            break;

        case PARROT_ARG_KC:
            if (arg < 0 || arg >= const_table->const_count)
                internal_exception(INTERP_ERROR, "Illegal constant number");
            pc_prederef[i] = (void *)
                &const_table->constants[arg]->u.key;
            break;
        default:
            internal_exception(ARG_OP_NOT_HANDLED,
                               "Unhandled argtype %d\n",opinfo->types[i]);
            break;
        }

        if (pc_prederef[i] == 0) {
            internal_exception(INTERP_ERROR,
                    "Prederef generated a NULL pointer for arg of type %d!\n",
                    opinfo->types[i]);
        }
    }
}

/*
 * this is called from inside the run cores
 */
void
do_prederef(void **pc_prederef, Parrot_Interp interpreter, int type)
{
    size_t offset = pc_prederef - interpreter->prederef_code;
    opcode_t *pc = ((opcode_t *)interpreter->code->byte_code) + offset;
    op_func_t *prederef_op_func = interpreter->op_lib->op_func_table;

    if (*pc < 0 || *pc >= (opcode_t)interpreter->op_count)
        internal_exception(INTERP_ERROR, "Illegal opcode");
    switch (type) {
        case PARROT_SWITCH_CORE:
            *pc_prederef = (void**) *pc;
            break;
        case PARROT_PREDEREF_CORE:
        case PARROT_CGP_CORE:
            *pc_prederef = ((void**)(prederef_op_func)) [*pc];
            break;
        default:
            internal_exception(1, "Tried to prederef wrong core");
            break;
    }
    /* and arguments */
    prederef_args(pc_prederef, interpreter, pc);
}

/*=for api interpreter get_op_lib_init
 *
 * return an  opcode's library op_lib init func
 */

static oplib_init_f
get_op_lib_init(int core_op, int which, PMC *lib)
{
    oplib_init_f init_func = (oplib_init_f)NULL;
    if (core_op) {
        switch (which) {
            case PARROT_SWITCH_CORE:
                init_func = PARROT_CORE_SWITCH_OPLIB_INIT;
                break;
            case PARROT_PREDEREF_CORE:
                init_func = PARROT_CORE_PREDEREF_OPLIB_INIT;
                break;
#ifdef HAVE_COMPUTED_GOTO
            case PARROT_CGP_CORE:
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
    return (oplib_init_f) D2FPTR(lib->cache.struct_val);
}

/*=for api interpreter load_prederef
 *
 * interpreter->op_lib = prederefed oplib
 *
 */
static void
load_prederef(struct Parrot_Interp *interpreter, int which)
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

/*=for api interpreter init_prederef
 *
 * initialize: load prederef func_table, file prederef_code
 *
 */
static void
init_prederef(struct Parrot_Interp *interpreter, int which)
{
    load_prederef(interpreter, which);
    if (!interpreter->prederef_code) {
        size_t N = interpreter->code->cur_cs->base.size;
        size_t i;
        void *pred_func;
/* Parrot_memalign_if_possible in OpenBSD allocates 256 if you ask for 312 */
#if 1
        void **temp = (void **)mem_sys_allocate_zeroed(N * sizeof(void *));
#else
        void **temp = (void **)Parrot_memalign_if_possible(256,
                N * sizeof(void *));
#endif

        /* fill with the prederef__ opcode function */
        if (which == PARROT_SWITCH_CORE)
            pred_func = (void*) CORE_OPS_prederef__;
        else
            pred_func = ((void **)
                    interpreter->op_lib->op_func_table)[CORE_OPS_prederef__];
        for (i = 0; i < N; i++) {
            temp[i] = pred_func;
        }

        interpreter->prederef_code = temp;
        interpreter->code->cur_cs->prederef_code = temp;

    }
}

/*=for api interpreter stop_prederef
 *
 * Unload the prederef oplib.
 */

static void
stop_prederef(struct Parrot_Interp *interpreter)
{
    (void) PARROT_CORE_PREDEREF_OPLIB_INIT(0);
}

#if EXEC_CAPABLE
/*=for api interpreter exec_init_prederef
 *
 * interpreter->op_lib = prederefed oplib
 *
 * the "normal" op_lib has a copy in the interpreter structure
 * - but get the op_code lookup function from standard core
 *   prederef has no op_info_table
 */
void
exec_init_prederef(struct Parrot_Interp *interpreter, void *prederef_arena)
{
    load_prederef(interpreter, PARROT_CGP_CORE);

    if (!interpreter->prederef_code) {
        size_t N = interpreter->code->cur_cs->base.size;
        size_t i;
        size_t n;
        void **temp = prederef_arena;
        opcode_t *pc = interpreter->code->cur_cs->base.data;

        interpreter->prederef_code = temp;
        interpreter->code->cur_cs->prederef_code = temp;
        /* TODO */
    }
}
#endif

void *
init_jit(struct Parrot_Interp *interpreter, opcode_t *pc)
{
#if JIT_CAPABLE
    opcode_t *code_start;
    UINTVAL code_size;          /* in opcodes */
    opcode_t *code_end;
    jit_f jit_code;
    if (interpreter->jit_info)
        return ((Parrot_jit_info_t *)interpreter->jit_info)->arena.start;

    code_start = interpreter->code->byte_code;
    code_size = interpreter->code->cur_cs->base.size;
    code_end = interpreter->code->byte_code + code_size;
#  if defined HAVE_COMPUTED_GOTO && defined USE_CGP
#    ifdef __GNUC__
#      ifdef PARROT_I386
    init_prederef(interpreter, PARROT_CGP_CORE);
#      endif
#    endif
#  endif

    jit_code = build_asm(interpreter, pc, code_start, code_end, NULL);
    interpreter->code->cur_cs->jit_info = interpreter->jit_info;
    return F2DPTR(jit_code);
#else
    return NULL;
#endif
}

void
prepare_for_run(Parrot_Interp interpreter)
{
    switch (interpreter->run_core) {
        case PARROT_JIT_CORE:
            (void) init_jit(interpreter, interpreter->code->byte_code);
            break;
        case PARROT_PREDEREF_CORE:
        case PARROT_SWITCH_CORE:
        case PARROT_CGP_CORE:
            init_prederef(interpreter, interpreter->run_core);
            break;
        default:
            break;
    }
}

static opcode_t *
runops_jit(struct Parrot_Interp *interpreter, opcode_t *pc)
{
#if JIT_CAPABLE
    jit_f jit_code = (jit_f) D2FPTR(init_jit(interpreter, pc));
    (jit_code) (interpreter, pc);
#endif
    return NULL;
}

static opcode_t *
runops_exec(struct Parrot_Interp *interpreter, opcode_t *pc)
{
#if EXEC_CAPABLE
    opcode_t *code_start;
    UINTVAL code_size;          /* in opcodes */
    opcode_t *code_end;
    extern int Parrot_exec_run;

    code_start = interpreter->code->byte_code;
    code_size = interpreter->code->cur_cs->base.size;
    code_end = interpreter->code->byte_code + code_size;
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


/*=for api interpreter runops_prederef
 *
 * This runops core is used when we are in prederef mode. It works
 * just like the basic fast core, except it uses pc_prederef instead
 * of pc, and calls prederef opfuncs instead of regular opfuncs.
 *
 * There is code after the main while loop to resynchronize pc with
 * pc_prederef in case we have exited the loop under restart
 * conditions (such as with interpreter flag changing ops).
 *
 * TODO: The calls to init_prederef() and stop_prederef() would be
 * best placed elsewhere, since we would re-pay the costs of loading
 * the prederef oplib every time we dropped out of and back into
 * this core. For now, however, this implementation should do fine.
 * Since dropping out of and back into cores is expected to be rare
 * (at the time of implementation that only occurs for interpreter
 * flag changing ops).
 */
static opcode_t *
runops_prederef(struct Parrot_Interp *interpreter, opcode_t *pc)
{
    opcode_t *code_start = (opcode_t *)interpreter->code->byte_code;
    void **pc_prederef;

    init_prederef(interpreter, PARROT_PREDEREF_CORE);
    pc_prederef = interpreter->prederef_code + (pc - code_start);

    while (pc_prederef) {
        pc_prederef =
            ((op_func_prederef_t)(ptrcast_t)*pc_prederef) (pc_prederef,
                                                           interpreter);
    }

    stop_prederef(interpreter);
    return 0;
}

static opcode_t *
runops_cgp(struct Parrot_Interp *interpreter, opcode_t *pc)
{
#ifdef HAVE_COMPUTED_GOTO
    opcode_t *code_start = (opcode_t *)interpreter->code->byte_code;
    void **pc_prederef;
    init_prederef(interpreter, PARROT_CGP_CORE);
    pc_prederef = interpreter->prederef_code + (pc - code_start);
    pc = cgp_core((opcode_t*)pc_prederef, interpreter);
    return pc;
#else
    PIO_eprintf(interpreter,
            "Computed goto unavailable in this configuration.\n");
    Parrot_exit(1);
    return NULL;
#endif
}

static opcode_t *
runops_switch(struct Parrot_Interp *interpreter, opcode_t *pc)
{
    opcode_t *code_start = (opcode_t *)interpreter->code->byte_code;
    void **pc_prederef;
    init_prederef(interpreter, PARROT_SWITCH_CORE);
    pc_prederef = interpreter->prederef_code + (pc - code_start);
    pc = switch_core((opcode_t*)pc_prederef, interpreter);
    return pc;
}

/*=for api interpreter runops_int
 * run parrot operations of loaded code segment until an end opcode is reached
 * run core is selected depending on the Interp_flags
 * when a restart opcode is encountered a different core my be selected
 * and evaluation of opcode continues
 */
void
runops_int(struct Parrot_Interp *interpreter, size_t offset)
{
    int lo_var_ptr;
    void *old_lo_var_ptr;
    opcode_t *(*core) (struct Parrot_Interp *, opcode_t *) =
        (opcode_t *(*) (struct Parrot_Interp *, opcode_t *)) 0;

    /*
     * setup event function ptrs and set the stack limit
     */
    setup_event_func_ptrs(interpreter);
    interpreter->lo_var_ptr = (void *)&lo_var_ptr;

    interpreter->resume_offset = offset;
    interpreter->resume_flag |= RESUME_RESTART;

    while (interpreter->resume_flag & RESUME_RESTART) {
        opcode_t *pc = (opcode_t *)
            interpreter->code->byte_code + interpreter->resume_offset;

        old_lo_var_ptr = interpreter->lo_var_ptr;
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
                /* clear stacktop, it gets set in runops_cgoto_core beyond the
                 * opfunc table again, if the compiler supports nested funcs
                 * - but only, if we are the top running loop
                 */
                /* #ifdef HAVE_NESTED_FUNC */
#  ifdef __GNUC__
                if (old_lo_var_ptr == interpreter->lo_var_ptr)
                    interpreter->lo_var_ptr = 0;
#  endif
                core = runops_cgoto_core;
#else
                internal_exception(1, "Error: PARROT_CGOTO_CORE not available");
#endif
                break;
            case PARROT_CGP_CORE:
#ifdef HAVE_COMPUTED_GOTO
                core = runops_cgp;
#else
                internal_exception(1, "Error: PARROT_CGP_CORE not available");
#endif
                break;
            case PARROT_SWITCH_CORE:
                core = runops_switch;
                break;
            case PARROT_PREDEREF_CORE:
                core = runops_prederef;
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
        }


        /* run it finally */
        core(interpreter, pc);
        /* if we have fallen out with resume and we were running CGOTO, set
         * the stacktop again to a sane value, so that restarting the runloop
         * is ok.
         */
        interpreter->lo_var_ptr = old_lo_var_ptr;
        if ((interpreter->resume_flag & RESUME_RESTART) &&
                (int)interpreter->resume_offset < 0)
                internal_exception(1, "branch_cs: illegal resume offset");
    }
}

/*=for api interpreter runops_ex
 * handles intersegment jumps from eval'ed code
 */
static void
runops_ex(struct Parrot_Interp *interpreter, size_t offset)
{
    interpreter->resume_flag |= RESUME_ISJ;

    while (interpreter->resume_flag & RESUME_ISJ) {
        interpreter->resume_flag &= ~RESUME_ISJ;
        runops_int(interpreter, offset);

        if (interpreter->resume_flag & RESUME_ISJ) {
            /* inter segment jump
             * resume_offset = entry of name in current const_table
             */
            struct PackFile_Constant * c;
            struct PackFile_FixupEntry *fe;
            struct PackFile *pf = interpreter->code;
            char *s;

            if ((int)interpreter->resume_offset >= PF_NCONST(pf) ||
                (int)interpreter->resume_offset < 0)
                internal_exception(1, "branch_cs: illegal resume offset");
            c = PF_CONST(pf, interpreter->resume_offset);
            if (c->type != PFC_STRING)
                internal_exception(1, "branch_cs: not a string\n");
            s = c->u.string->strstart;
            fe = PackFile_find_fixup_entry(interpreter, enum_fixup_label, s);
            if (!fe)
                internal_exception(1, "branch_cs: fixup not found\n");
            offset = fe->offset;
            Parrot_switch_to_cs(interpreter, fe->seg);
            if (Interp_flags_TEST(interpreter, PARROT_TRACE_FLAG)) {
                PIO_eprintf(interpreter, "*** Resume at seg %s ofs %d\n",
                        fe->seg->base.name, (int)offset);
            }
        }
    }
}

/*=for api interpreter runops
 * run parrot ops
 * set exception handler and/or resume after exception
 */
extern Parrot_exception the_exception;
void
runops(struct Parrot_Interp *interpreter, size_t offset)
{
    if (setjmp(the_exception.destination)) {
        /* an exception was thrown */
        offset = handle_exception(interpreter);
    }
    if (interpreter->profile &&
            Interp_flags_TEST(interpreter, PARROT_PROFILE_FLAG)) {
        RunProfile *profile = interpreter->profile;
        if (profile->cur_op == PARROT_PROF_EXCEPTION) {
            profile->data[PARROT_PROF_EXCEPTION].time +=
                Parrot_floatval_time() - profile->starttime;
        }
    }
    runops_ex(interpreter, offset);
    /*
     * not yet - this needs classifying of exceptions and handlers
     * so that only an exit handler does catch this exception
     */
#if 0
    do_exception(EXCEPT_exit, 0);
#endif
}

/*=for api interpreter Parrot_runops_fromc
 * run parrot ops, called from c code
 * function arguments are already setup according to PCC
 * the sub argument is an invocable Sub PMC
 */

void
Parrot_runops_fromc(Parrot_Interp interpreter, PMC *sub)
{
    PMC *ret_c = NULL;
    opcode_t offset, *dest;

    /* we need one return continuation with a NULL offset */
    ret_c = pmc_new(interpreter, enum_class_RetContinuation);
    REG_PMC(1) = ret_c;
    /* invoke the sub, which places the context of the sub in the
     * interpreter, and switches code segments if needed
     */
    dest = VTABLE_invoke(interpreter, sub, NULL);
    if (!dest) {
        /* code was run inside invoke - probably - e.g. for NCI */
        return;
    }

    offset = dest - interpreter->code->byte_code;
    runops(interpreter, offset);
}

/*
 * duplicated from delegates.pmc
 */
static struct regsave {
    struct IReg int_reg;
    struct NReg num_reg;
    struct SReg string_reg;
    struct PReg pmc_reg;
} regsave;

PARROT_INLINE static struct regsave *
save_regs(Parrot_Interp interp) {
    struct regsave *save;
    save = mem_sys_allocate(sizeof(struct regsave));
    if (!save) {
	internal_exception(ALLOCATION_ERROR, "No memory for save struct");
    }
    mem_sys_memcopy(save, interp, sizeof(struct regsave));
    return save;
}

PARROT_INLINE static void
restore_regs(Parrot_Interp interp, struct regsave *data) {
    mem_sys_memcopy(interp, data, sizeof(struct regsave));
    mem_sys_free(data);
}
/*=for api interpreter Parrot_runops_fromc_save
 * like above but preserve registers
 */
void
Parrot_runops_fromc_save(Parrot_Interp interpreter, PMC *sub)
{
    struct regsave *data = save_regs(interpreter);
    Parrot_runops_fromc(interpreter, sub);
    restore_regs(interpreter, data);
}

/*=for api interpreter Parrot_runops_fromc_args
 * run parrot ops, called from c code
 * function arguments are passed as va_args according to signature
 * the sub argument is an invocable Sub PMC
 *
 * signatures are similar to NCI:
 * v ... void return
 * I ... INTVAL (not Interpreter)
 * N ... NUMVAL
 * S ... STRING*
 * P ... PMC*
 *
 * return value, if any is passed as (void*)ptr or (void*)&val
 */

void *
Parrot_runops_fromc_args(Parrot_Interp interpreter, PMC *sub,
        const char *sig, ...)
{
    va_list ap;
    /* *sig is retval like in NCI */
    int ret;
    int next[4];
    int i;
    void *retval;

    for (i = 0; i < 4; i++)
        next[i] = 5;

    REG_INT(0) = 1;     /* kind of a prototyped call */
    REG_INT(1) = 0;     /* # of I params */
    REG_INT(2) = 0;     /* # of S params */
    REG_INT(3) = 0;     /* # of P params */
    REG_INT(4) = 0;     /* # of N params */

    ret = *sig++;
    va_start(ap, sig);
    while (*sig) {
        switch (*sig++) {
            case 'v':       /* void func, no params */
                break;
            case 'I':       /* REG_INT */
                REG_INT(next[0]++) = va_arg(ap, INTVAL);
                ++REG_INT(1);
                break;
            case 'S':       /* REG_STR */
                REG_STR(next[1]++) = va_arg(ap, STRING*);
                ++REG_INT(2);
                break;
            case 'P':       /* REG_PMC */
                REG_PMC(next[2]++) = va_arg(ap, PMC*);
                ++REG_INT(3);
                break;
            case 'N':       /* REG_NUM */
                REG_NUM(next[3]++) = va_arg(ap, FLOATVAL);
                ++REG_INT(4);
                break;
            default:
                internal_exception(1,
                        "unhandle signature '%c' in Parrot_runops_fromc_args",
                        sig[-1]);
        }
    }
    va_end(ap);

    Parrot_runops_fromc(interpreter, sub);
    /*
     * TODO check, if return is prototyped, for now, assume yes
     */
    retval = NULL;
    /*
     * XXX should we trust the signature or the registers set
     *     by the subroutine or both if possible, i.e. extract
     *     e.g. an INTVAL from a returned PMC?
     */
    switch (ret) {
        case 'v': break;
        case 'I': retval = (void* )&REG_INT(5); break;
        case 'S': retval = (void* ) REG_STR(5); break;
        case 'P': retval = (void* ) REG_PMC(5); break;
        case 'N': retval = (void* )&REG_NUM(5); break;
        default:
            internal_exception(1,
                    "unhandle signature '%c' in Parrot_runops_fromc_args",
                    ret);
    }
    return retval;
}

static int
is_env_var_set(const char* var)
{
    int free_it, retval;
    char* value = Parrot_getenv(var, &free_it);
    if (value == NULL)
        retval = 0;
    else if (*value == '\0')
        retval = 0;
    else
        retval = ! (strcmp(value, "0") == 0);
    if (free_it)
        mem_sys_free(value);
    return retval;
}

/*
 * setup a func_table containing ptrs (or addresses) of the
 * check_event__ opcode
 * TODO free it at destroy
 * TODO handle run-core changes
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
            interpreter->evc_func_table[i] =
                ((void**)lib->op_func_table)[CORE_OPS_check_events__];
    }
}

void Parrot_really_destroy(int exit_code, void *interpreter);

/*=for api interpreter make_interpreter
 *  Create the Parrot interpreter.  Allocate memory and clear the registers.
 */

Parrot_Interp
make_interpreter(Parrot_Interp parent, Interp_flags flags)
{
    struct Parrot_Interp *interpreter;
#if EXEC_CAPABLE
    extern int Parrot_exec_run;
#endif

    /* Get an empty interpreter from system memory */
#if EXEC_CAPABLE
    if (Parrot_exec_run)
        interpreter = &interpre;
    else
#endif
        interpreter = mem_sys_allocate_zeroed(sizeof(struct Parrot_Interp));

    /*
     * the last interpreter (w/o) parent has to cleanup globals
     * so remember parent if any
     */
    if (parent) {
        interpreter->parent_interpreter = parent;
        interpreter->lo_var_ptr = parent->lo_var_ptr;
    }
    else {
        interpreter->resume_flag = RESUME_INITIAL;
        SET_NULL(interpreter->parent_interpreter);
    }

    interpreter->DOD_block_level = 1;
    interpreter->GC_block_level = 1;

    /* Must initialize flags here so the GC_DEBUG stuff is available before
     * mem_setup_allocator() is called. */
    interpreter->flags = flags;

    /* PANIC will fail until this is done */
    SET_NULL(interpreter->piodata);
    PIO_init(interpreter);

    if (is_env_var_set("PARROT_GC_DEBUG")) {
#if ! DISABLE_GC_DEBUG
        Interp_flags_SET(interpreter, PARROT_GC_DEBUG_FLAG);
#else
        fprintf(stderr, "PARROT_GC_DEBUG is set but the binary was compiled "
                "with DISABLE_GC_DEBUG.\n");
#endif
    }

    /* Set up the memory allocation system */
    mem_setup_allocator(interpreter);

    /* Set up the MMD struct */
    interpreter->binop_mmd_funcs =
        mem_sys_allocate_zeroed(sizeof(struct MMD_table));

    /* Go and init the MMD tables */
    register_fallback_methods(interpreter);

    /* initialize classes - this needs mmd func table */
    Parrot_init(interpreter);

#if PARROT_CATCH_NULL
    /* Initialize once only. For now all interpreters share the NULL PMC.
     * Adding to interpreter-local storage will add a deref overhead.
     * init_null will return the NULL PMC, but ignore for now since it is global.
     */
    pmc_init_null(interpreter);
#endif

    /* Need an empty stash */
    interpreter->globals = mem_sys_allocate(sizeof(struct Stash));
    interpreter->globals->stash_hash =
        pmc_new(interpreter, enum_class_PerlHash);
    interpreter->globals->parent_stash = NULL;

    /* context data */
    /* Initialize interpreter's flags */
    interpreter->ctx.warns = new_buffer_header(interpreter);
    Parrot_allocate(interpreter, interpreter->ctx.warns,
        sizeof(struct warnings_t));
    PARROT_WARNINGS_off(interpreter, PARROT_WARNINGS_ALL_FLAG);

    /* Set up the initial register chunks */
    interpreter->ctx.int_reg_top =
        mem_sys_allocate_zeroed(sizeof(struct IRegChunk));
    interpreter->ctx.num_reg_top =
        mem_sys_allocate_zeroed(sizeof(struct NRegChunk));
    interpreter->ctx.string_reg_top =
        mem_sys_allocate_zeroed(sizeof(struct SRegChunk));
    interpreter->ctx.pmc_reg_top =
        mem_sys_allocate_zeroed(sizeof(struct PRegChunk));

    /* the SET_NULL macros are only for systems where a NULL pointer
     * isn't represented by zeroes, so don't use these for resetting
     * non-null pointers
     */
    SET_NULL(interpreter->ctx.int_reg_top->next);
    SET_NULL(interpreter->ctx.int_reg_top->prev);
    SET_NULL(interpreter->ctx.num_reg_top->next);
    SET_NULL(interpreter->ctx.num_reg_top->prev);
    SET_NULL(interpreter->ctx.string_reg_top->next);
    SET_NULL(interpreter->ctx.string_reg_top->prev);
    SET_NULL(interpreter->ctx.pmc_reg_top->next);
    SET_NULL(interpreter->ctx.pmc_reg_top->prev);

    Parrot_clear_s(interpreter);
    Parrot_clear_p(interpreter);

    stack_system_init(interpreter);
    /* Stack for lexical pads */
    interpreter->ctx.pad_stack = new_stack(interpreter, "Pad");

    /* Need a user stack */
    interpreter->ctx.user_stack = new_stack(interpreter, "User");

    /* And a control stack */
    interpreter->ctx.control_stack = new_stack(interpreter, "Control");

    /* A regex stack would be nice too. */
    interpreter->ctx.intstack = intstack_new(interpreter);

    /* Load the core op func and info tables */
    interpreter->op_lib = get_op_lib_init(1, 0, NULL)(1);
    interpreter->op_count = interpreter->op_lib->op_count;
    interpreter->op_func_table = interpreter->op_lib->op_func_table;
    interpreter->op_info_table = interpreter->op_lib->op_info_table;
    SET_NULL_P(interpreter->all_op_libs, op_lib_t **);
    SET_NULL_P(interpreter->evc_func_table, op_func_t *);
    SET_NULL_P(interpreter->save_func_table, op_func_t *);

    /* Set up defaults for line/package/file */
    interpreter->current_file =
        string_make(interpreter, "(unknown file)", 14, NULL, 0, NULL);
    interpreter->current_package =
        string_make(interpreter, "(unknown package)", 18, NULL, 0, NULL);;

    SET_NULL_P(interpreter->code, struct PackFile *);
    SET_NULL_P(interpreter->profile, ProfData *);

    /* next two are pointers to the real thing in the current code seg */
    SET_NULL_P(interpreter->prederef_code, void **);
    SET_NULL(interpreter->jit_info);

    /* register assembler/compilers */
    setup_default_compreg(interpreter);

    /* setup stdio PMCs */
    PIO_init(interpreter);
    /* Done. Return and be done with it */

    /* Add in the class hash. Bit of a hack, probably, as there's
       altogether too much overlap with the PMC classes */
    interpreter->class_hash = pmc_new(interpreter, enum_class_PerlHash);

    /* Okay, we've finished doing anything that might trigger GC.
     * Actually, we could enable DOD/GC earlier, but here all setup is
     * done
     */
    Parrot_unblock_DOD(interpreter);
    Parrot_unblock_GC(interpreter);

    /* all sys running, init the event and signal stuff
     * the first or "master" interpreter is handling events and signals
     */
    SET_NULL_P(interpreter->task_queue, QUEUE*);
    SET_NULL_P(interpreter->thread_data, _Thread_data *);

    Parrot_init_events(interpreter);

#ifdef ATEXIT_DESTROY
    Parrot_on_exit(Parrot_really_destroy, (void*)interpreter);
#endif

    return interpreter;
}

void
Parrot_destroy(struct Parrot_Interp *interpreter)
{
#ifdef ATEXIT_DESTROY
    UNUSED(interpreter);
#else
    Parrot_really_destroy(0, (void*) interpreter);
#endif
}

void
Parrot_really_destroy(int exit_code, void *vinterp)
{
    int i;
    Interp *interpreter = (Interp*) vinterp;
    struct Stash *stash, *next_stash;

    UNUSED(exit_code);

    /*
     * wait for threads to complete if needed
     */
    pt_join_threads(interpreter);
    /* if something needs destruction (e.g. closing PIOs)
     * we must destroy it now:
     * no DOD run, so everything is considered dead
     */

    free_unused_pobjects(interpreter, interpreter->arena_base->pmc_pool);

    /* Now the PIOData gets also cleared */
    PIO_finish(interpreter);

    /* we destroy all child interpreters and the last one too,
     * if the --leak-test commandline was given
     */
    if (! (interpreter->parent_interpreter ||
                Interp_flags_TEST(interpreter, PARROT_DESTROY_FLAG)))
        return;

    /* buffer headers, PMCs */
    Parrot_destroy_header_pools(interpreter);
    /* memory pools in resources */
    Parrot_destroy_memory_pools(interpreter);
    /* mem subsystem is dead now */
    mem_sys_free(interpreter->arena_base);
    /* packfile */

    if (!Interp_flags_TEST(interpreter, PARROT_EXTERN_CODE_FLAG))  {
        struct PackFile *pf = interpreter->code;
        if (pf)
            PackFile_destroy(pf);
    }

    /* walk and free the stash, pmc's are already dead */
    stash = interpreter->globals;
    while (stash) {
        next_stash = stash->parent_stash;
        mem_sys_free(stash);
        stash = next_stash;
    }
    /* free vtables */
    if (!interpreter->parent_interpreter)
        for (i = 1; i < (int)enum_class_max; i++)
            Parrot_destroy_vtable(interpreter, Parrot_base_vtables[i]);
    mmd_destroy(interpreter);

    if (interpreter->profile) {
        mem_sys_free(interpreter->profile->data);
        mem_sys_free(interpreter->profile);
    }

    /* deinit op_lib */
    (void) PARROT_CORE_OPLIB_INIT(0);

    /* XXX move this to register.c */
    {
        struct IRegChunk *stacks[4];
        struct IRegChunk *top, *next;
        stacks[0] = interpreter->ctx.int_reg_top;
        stacks[1] = (struct IRegChunk*) interpreter->ctx.num_reg_top;
        stacks[2] = (struct IRegChunk*) interpreter->ctx.string_reg_top;
        stacks[3] = (struct IRegChunk*) interpreter->ctx.pmc_reg_top;
        for (i = 0; i< 4; i++) {
            top = stacks[i];
            for (; top ; ) {
                next = top->next;
                mem_sys_free(top);
                top = next;
            }
        }
    }

    stack_destroy(interpreter->ctx.pad_stack);
    stack_destroy(interpreter->ctx.user_stack);
    stack_destroy(interpreter->ctx.control_stack);
    /* intstack */
    intstack_free(interpreter, interpreter->ctx.intstack);

    /* chartype, encodings */
    if (!interpreter->parent_interpreter) {
        chartype_destroy();
        encoding_destroy();
    }

    mem_sys_free(interpreter);
}

#ifdef GC_IS_MALLOC
#  if 0
struct mallinfo {
    int arena;                  /* non-mmapped space allocated from system */
    int ordblks;                /* number of free chunks */
    int smblks;                 /* number of fastbin blocks */
    int hblks;                  /* number of mmapped regions */
    int hblkhd;                 /* space in mmapped regions */
    int usmblks;                /* maximum total allocated space */
    int fsmblks;                /* space available in freed fastbin blocks */
    int uordblks;               /* total allocated space */
    int fordblks;               /* total free space */
    int keepcost;               /* top-most, releasable (via malloc_trim)
                                 * space */
};
#  endif
extern struct mallinfo mallinfo(void);
#endif /* GC_IS_MALLOC */
INTVAL
interpinfo(struct Parrot_Interp *interpreter, INTVAL what)
{
    INTVAL ret = 0;
    struct Small_Object_Pool *header_pool;
    int j;

    switch (what) {
        case TOTAL_MEM_ALLOC:
#ifdef GC_IS_MALLOC
#if 0
            interpreter->memory_allocated = mallinfo().uordblks;
#endif
#endif
            ret = interpreter->memory_allocated;
            break;
        case DOD_RUNS:
            ret = interpreter->dod_runs;
            break;
        case COLLECT_RUNS:
            ret = interpreter->collect_runs;
            break;
        case ACTIVE_PMCS:
            ret = interpreter->arena_base->pmc_pool->total_objects -
                interpreter->arena_base->pmc_pool->num_free_objects;
            break;
        case ACTIVE_BUFFERS:
            ret = 0;
            for (j = 0; j < (INTVAL)interpreter->arena_base->num_sized; j++) {
                header_pool = interpreter->arena_base->sized_header_pools[j];
                if (header_pool)
                    ret += header_pool->total_objects -
                        header_pool->num_free_objects;
            }
            break;
        case TOTAL_PMCS:
            ret = interpreter->arena_base->pmc_pool->total_objects;
            break;
        case TOTAL_BUFFERS:
            ret = 0;
            for (j = 0; j < (INTVAL)interpreter->arena_base->num_sized; j++) {
                header_pool = interpreter->arena_base->sized_header_pools[j];
                if (header_pool)
                    ret += header_pool->total_objects;
            }
            break;
        case HEADER_ALLOCS_SINCE_COLLECT:
            ret = interpreter->header_allocs_since_last_collect;
            break;
        case MEM_ALLOCS_SINCE_COLLECT:
            ret = interpreter->mem_allocs_since_last_collect;
            break;
        case TOTAL_COPIED:
            ret = interpreter->memory_collected;
            break;
    }
    return ret;
}

/*=for api interpreter Parrot_compreg
 * register a parser/compiler function
 */

void Parrot_compreg(Parrot_Interp interpreter, STRING *type, PMC *func)
{
    PMC *hash, *nci;
    PMC* iglobals = interpreter->iglobals;
    hash = VTABLE_get_pmc_keyed_int(interpreter, interpreter->iglobals,
            IGLOBALS_COMPREG_HASH);
    if (!hash) {
        hash = pmc_new_noinit(interpreter, enum_class_PerlHash);
        VTABLE_init(interpreter, hash);
        VTABLE_set_pmc_keyed_int(interpreter, iglobals,
                (INTVAL)IGLOBALS_COMPREG_HASH, hash);
    }
    nci = pmc_new(interpreter, enum_class_Compiler);
    VTABLE_set_pmc_keyed_str(interpreter, hash, type, nci);
    /* build native call interface fir the C sub in "func" */
    VTABLE_set_string_keyed(interpreter, nci, func,
            string_from_cstring(interpreter, "pIt", 0));
}


/*
 * setup default compiler for PASM
 */
static void setup_default_compreg(Parrot_Interp interpreter)
{
    STRING *pasm1 = string_from_cstring(interpreter, "PASM1", 0);

    Parrot_csub_t p = (Parrot_csub_t) PDB_compile;
    /* register the nci ccompiler object */
    Parrot_compreg(interpreter, pasm1, (PMC*)F2DPTR(p));
}

INTVAL
sysinfo_i(Parrot_Interp interpreter, INTVAL info_wanted)
{
    switch (info_wanted) {
    case PARROT_INTSIZE:
        return sizeof(INTVAL);
    case PARROT_FLOATSIZE:
        return sizeof(FLOATVAL);
    case PARROT_POINTERSIZE:
        return sizeof(void *);
    default:
        return -1;
    }
}

STRING *
sysinfo_s(Parrot_Interp interpreter, INTVAL info_wanted)
{
    switch (info_wanted) {
    case PARROT_OS:
        return string_from_cstring(interpreter, BUILD_OS_NAME, 0);
    case PARROT_OS_VERSION:
    case PARROT_OS_VERSION_NUMBER:
    case CPU_ARCH:
    case CPU_TYPE:
    default:
        return string_from_cstring(interpreter, "", 0);
    }
}

/*
 * dynamic loading stuff
 */
static void dynop_register_xx(Parrot_Interp, PMC*, size_t, size_t,
        oplib_init_f init_func);
static void dynop_register_switch(Parrot_Interp, PMC*, size_t, size_t);


/*=for api interpreter dynop_register
 *
 * register a dynamic oplib
 */

void
dynop_register(Parrot_Interp interpreter, PMC* lib_pmc)
{
    op_lib_t *lib, *core;
    oplib_init_f init_func;
    op_func_t *new_func_table, *new_evc_func_table;
    op_info_t *new_info_table;
    size_t i, n_old, n_new, n_tot;

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
    new_evc_func_table = mem_sys_realloc(interpreter->evc_func_table,
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
    dynop_register_xx(interpreter, lib_pmc, n_old, n_new,
            PARROT_CORE_PREDEREF_OPLIB_INIT);
    dynop_register_switch(interpreter, lib_pmc, n_old, n_new);
}

/*
 * register op_lib with other cores
 */
static void
dynop_register_xx(Parrot_Interp interpreter, PMC* lib_pmc,
        size_t n_old, size_t n_new, oplib_init_f init_func)
{
    op_lib_t *cg_lib, *new_lib;
    void **ops_addr = NULL;
    size_t i, n_tot;
    STRING *op_variant;
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
    /* check if the lib_pmc exists with a _xx flavor */
    new_init_func = get_op_lib_init(0, 0, lib_pmc);
    new_lib = new_init_func(1);
    op_variant = Parrot_sprintf_c(interpreter, "%s_ops%s",
            new_lib->name, cg_lib->suffix);
    lib_variant = Parrot_load_lib(interpreter, op_variant, NULL);
    if (lib_variant) {
        new_init_func = get_op_lib_init(0, 0, lib_variant);
        new_lib = new_init_func(1);
        for (i = n_old; i < n_tot; ++i)
            ops_addr[i] = ((void **)new_lib->op_func_table)[i - n_old];
        new_lib->op_func_table = (void *) ops_addr;
        new_lib->op_count = n_tot;
        new_init_func((int) ops_addr);
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
            interpreter->evc_func_table[i] = ops_addr[CORE_OPS_check_events__];
        interpreter->save_func_table = (void *) ops_addr;
    }
    /*
     * tell the cg_core about the new jump table
     */
    cg_lib->op_func_table = (void *) ops_addr;
    cg_lib->op_count = n_tot;
    init_func((int) ops_addr);
}

static void
dynop_register_switch(Parrot_Interp interpreter, PMC* lib_pmc,
        size_t n_old, size_t n_new)
{
    op_lib_t *lib = PARROT_CORE_SWITCH_OPLIB_INIT(1);
    lib->op_count = n_old + n_new;
}

/*
 * tell running core about new func table
 */
static void
notify_func_table(Parrot_Interp interpreter, void* table)
{
    oplib_init_f init_func = get_op_lib_init(1, interpreter->run_core, NULL);
    op_lib_t *lib = init_func(1);
    init_func((int) table);
    switch (interpreter->run_core) {
        case PARROT_SLOW_CORE:      /* normal func core */
        case PARROT_FAST_CORE:      /* normal func core */
            interpreter->op_func_table = table;
            break;
        default:
            break;
    }
}

/*
 * restore old func table
 * XXX function core only by now
 */
void
disable_event_checking(Parrot_Interp interpreter)
{
    /*
     * restore func table
     */
    notify_func_table(interpreter, interpreter->save_func_table);
}

/*
 * replace func table with one that does event checking for all
 * opcodes
 * XXX plain core only
 */
void
enable_event_checking(Parrot_Interp interpreter)
{
    /*
     * put table in place
     */
    notify_func_table(interpreter, interpreter->evc_func_table);
}

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
