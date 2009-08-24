/*
Copyright (C) 2004-2009, Parrot Foundation.
$Id$

=head1 NAME

src/pic.c - Polymorphic Inline Cache

=head1 DESCRIPTION

The PIC supports inline caching for MMD and object method lookups in
prederefed run cores. Additionally opcodes that do some kind of lookup
like C<new_p_sc> are changed to faster variants.

For non-prederefed run-cores there's a less efficient variant which
is basically:

 * the bytecode segment has an index per cached opcode (code->pic_index)
 * this index points into pic_store
 * TODO use the cache in opcodes

=head1 OPERATION SCHEME

Given this bytecode:

    0               1              2    3    4                5
  +--------------+---------------+----+----+-----------------+----------+
  | infix_ic_p_p | .MMD_SUBTRACT | P5 | P6 | callmethodcc_sc | "method" |
  +--------------+---------------+----+----+-----------------+----------+

In init_prederef the opcodes are replaced with prederef__, operands
are replaced with their addresses (&) in the const table or offsets
(+) in the register frame:

    0               1              2    3    4                5
  +--------------+---------------+----+----+-----------------+----------+
  | prederef__   | .MMD_SUBTRACT | +P5| +P6| prederef__      |&"method" |
  +--------------+---------------+----+----+-----------------+----------+

we have code->pic_index with an index into pic_store - the pic_index is
half the size of the bytecode and addressed with pc_offset/2:

    0   1   2
  +---+---+---+
  | 1 |   | 2 |
  +---+---+---+

During predereferencing the opcode gets rewritten to the PIC variant,
the constant infix operation number is replaced with a pointer to the MIC
in the pic_store at the index pic_index:

    0                    1     2    3
  +--------------------+-----+----+----+-----------------------+-----+
  | pic_infix___ic_p_p | MIC1|+P5 |+P6 | pic_callmethodcc___sc | MIC2|
  +--------------------+-----+----+----+-----------------------+-----+

This can be further optimized due to static inlining:

    0                    1     2    3
  +--------------------+-----+----+----+-----------------------+-----+
  | pic_inline_sub_p_p | MIC1|+P5 |+P6 | pic_callmethodcc___sc | MIC2|
  +--------------------+-----+----+----+-----------------------+-----+

The opcode is an opcode number for the switched core or the actual code address
for the direct-threaded CGP core. With a little help of the JIT system we could
also dynamicall create inlined code.

Runcores with r/o (mmapped) bytecode can't be rewritten in this way, the
lookup of the cache has to be done in the opcode itself.

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "parrot/oplib/ops.h"
#include "pmc/pmc_fixedintegerarray.h"
#include "pmc/pmc_continuation.h"
#ifdef HAVE_COMPUTED_GOTO
#  include "parrot/oplib/core_ops_cgp.h"
#endif

#ifdef HAS_JIT
#  include "parrot/exec.h"
#  include "jit.h"
#endif

/* needs a Makefile dependency */
/* #include "pmc/pmc_integer.h" */

/* XXX Define this in a header file */
extern void Parrot_Integer_i_subtract_Integer(Interp* , PMC* pmc, PMC* value);

/*
 * hack to turn on inlining - just sub_p_p for mops done
 */

#define ENABLE_INLINING 0

/* HEADERIZER HFILE: include/parrot/pic.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static int is_pic_func(PARROT_INTERP,
    ARGIN(void **pc),
    ARGOUT(Parrot_MIC *mic),
    int core_type)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*mic);

static int is_pic_param(PARROT_INTERP,
    ARGIN(void **pc),
    ARGOUT(Parrot_MIC *mic),
    opcode_t op)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*mic);

static int pass_int(PARROT_INTERP,
    ARGIN(PMC *sig),
    ARGIN(const char *src_base),
    ARGIN(const void **src),
    ARGOUT(char *dest_base),
    ARGIN(void * const *dest))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        __attribute__nonnull__(6)
        FUNC_MODIFIES(*dest_base);

static int pass_mixed(PARROT_INTERP,
    ARGIN(PMC *sig),
    ARGIN(const char *src_base),
    ARGIN(void * const *src),
    ARGOUT(char *dest_base),
    ARGIN(void * const *dest))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        __attribute__nonnull__(6)
        FUNC_MODIFIES(*dest_base);

static int pass_num(PARROT_INTERP,
    ARGIN(PMC *sig),
    ARGIN(const char *src_base),
    ARGIN(const void **src),
    ARGOUT(char *dest_base),
    ARGIN(void * const *dest))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        __attribute__nonnull__(6)
        FUNC_MODIFIES(*dest_base);

static int pass_pmc(PARROT_INTERP,
    ARGIN(PMC *sig),
    ARGIN(const char *src_base),
    ARGIN(const void **src),
    ARGOUT(char *dest_base),
    ARGIN(void * const *dest))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        __attribute__nonnull__(6)
        FUNC_MODIFIES(*dest_base);

static int pass_str(PARROT_INTERP,
    ARGIN(PMC *sig),
    ARGIN(const char *src_base),
    ARGIN(const void **src),
    ARGOUT(char *dest_base),
    ARGIN(void * const *dest))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        __attribute__nonnull__(6)
        FUNC_MODIFIES(*dest_base);

#define ASSERT_ARGS_is_pic_func __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pc) \
    || PARROT_ASSERT_ARG(mic)
#define ASSERT_ARGS_is_pic_param __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pc) \
    || PARROT_ASSERT_ARG(mic)
#define ASSERT_ARGS_pass_int __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(sig) \
    || PARROT_ASSERT_ARG(src_base) \
    || PARROT_ASSERT_ARG(src) \
    || PARROT_ASSERT_ARG(dest_base) \
    || PARROT_ASSERT_ARG(dest)
#define ASSERT_ARGS_pass_mixed __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(sig) \
    || PARROT_ASSERT_ARG(src_base) \
    || PARROT_ASSERT_ARG(src) \
    || PARROT_ASSERT_ARG(dest_base) \
    || PARROT_ASSERT_ARG(dest)
#define ASSERT_ARGS_pass_num __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(sig) \
    || PARROT_ASSERT_ARG(src_base) \
    || PARROT_ASSERT_ARG(src) \
    || PARROT_ASSERT_ARG(dest_base) \
    || PARROT_ASSERT_ARG(dest)
#define ASSERT_ARGS_pass_pmc __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(sig) \
    || PARROT_ASSERT_ARG(src_base) \
    || PARROT_ASSERT_ARG(src) \
    || PARROT_ASSERT_ARG(dest_base) \
    || PARROT_ASSERT_ARG(dest)
#define ASSERT_ARGS_pass_str __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(sig) \
    || PARROT_ASSERT_ARG(src_base) \
    || PARROT_ASSERT_ARG(src) \
    || PARROT_ASSERT_ARG(dest_base) \
    || PARROT_ASSERT_ARG(dest)
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */


/*

=item C<void parrot_PIC_alloc_store(PackFile_ByteCode *cs, size_t n)>

Initialize the PIC storage for the given code segment with the capacitiy of
holding at least C<n> MIC entries. The PIC_store itself, room for C<n> MICs and
some space for PICs is allocated as one piece. MICs are returned from the start
of usable memory, PICs from the rear.

=cut

*/

void
parrot_PIC_alloc_store(ARGOUT(PackFile_ByteCode *cs), size_t n)
{
    ASSERT_ARGS(parrot_PIC_alloc_store)
    Parrot_PIC_store *store;
    size_t size;

    /*
     * estimated 95% of calls are monomorphic, 5% are polymorphic
     * we need therefore:
     */
#define POLYMORPHIC 0.05
    size_t poly = (size_t)(n * POLYMORPHIC) * sizeof (Parrot_PIC);

    if (!poly)
        poly = 2 * sizeof (Parrot_PIC);

    size = n * sizeof (Parrot_MIC) + poly + sizeof (Parrot_PIC_store);

    store         = (Parrot_PIC_store *)mem_sys_allocate_zeroed(size);
    store->prev   = NULL;
    cs->pic_store = store;
    store->pic    = (Parrot_PIC *)((char *)store + size);
    store->usable = poly;
    store->mic    = (Parrot_MIC *)((char*)store + sizeof (Parrot_PIC_store));
    store->n_mics = n;
}

/*

=item C<void parrot_PIC_destroy(PackFile_ByteCode *cs)>

Free memory for the PIC storage.

=cut

*/

void
parrot_PIC_destroy(ARGMOD(PackFile_ByteCode *cs))
{
    ASSERT_ARGS(parrot_PIC_destroy)
    Parrot_PIC_store *store = cs->pic_store;

    while (store) {
        Parrot_PIC_store * const prev = store->prev;
        mem_sys_free(store);
        store = prev;
    }

    cs->pic_store = NULL;
}

/*

=item C<int parrot_PIC_op_is_cached(int op_code)>

Return true, if the opcode needs a PIC slot.

=cut

*/

PARROT_CONST_FUNCTION
int
parrot_PIC_op_is_cached(int op_code)
{
    ASSERT_ARGS(parrot_PIC_op_is_cached)
    switch (op_code) {
        case PARROT_OP_get_params_pc:
        case PARROT_OP_set_returns_pc:
        case PARROT_OP_set_args_pc:
            return 1;
        default:
            return 0;
    }
}

/*

=item C<Parrot_MIC* parrot_PIC_alloc_mic(const PARROT_INTERP, size_t n)>

Allocate a new MIC structure for the C<n>th cached opcode in this
bytecode segement.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
Parrot_MIC*
parrot_PIC_alloc_mic(const PARROT_INTERP, size_t n)
{
    ASSERT_ARGS(parrot_PIC_alloc_mic)
    Parrot_PIC_store * const store = interp->code->pic_store;
    PARROT_ASSERT(n < store->n_mics);
    return store->mic + n;
}

/*

=item C<Parrot_PIC* parrot_PIC_alloc_pic(PARROT_INTERP)>

Allocate a new PIC structure for the C<n>th cached opcode in this
bytecode segement.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
Parrot_PIC*
parrot_PIC_alloc_pic(PARROT_INTERP)
{
    ASSERT_ARGS(parrot_PIC_alloc_pic)
    Parrot_PIC_store *store = interp->code->pic_store;
    Parrot_PIC_store *new_store;

    if (store->usable < sizeof (Parrot_PIC)) {
        size_t size =
            (size_t)(store->n_mics * POLYMORPHIC) * sizeof (Parrot_PIC);

        if (size == 0)
            size = 2 * sizeof (Parrot_PIC);

        new_store               = (Parrot_PIC_store *)
                    mem_sys_allocate_zeroed(size + sizeof (Parrot_PIC_store));
        new_store->prev         = store;
        interp->code->pic_store = new_store;

        new_store->pic    = (Parrot_PIC *)((char *)new_store + size
                          + sizeof (Parrot_PIC_store));
        new_store->usable = size;

        /* the addon store has only poly-morphic slots
         * point the monomorphic to the old store */
        new_store->mic    = store->mic;
        new_store->n_mics = store->n_mics;
        store             = new_store;
    }
    store->usable -= sizeof (Parrot_PIC);
    return --store->pic;
}

/*

=item C<void * parrot_pic_opcode(PARROT_INTERP, INTVAL op)>

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
void *
parrot_pic_opcode(PARROT_INTERP, INTVAL op)
{
    ASSERT_ARGS(parrot_pic_opcode)
#ifdef HAVE_COMPUTED_GOTO
    op_lib_t *cg_lib;
#endif
    const int core = interp->run_core;

    if (core == PARROT_SWITCH_CORE || core == PARROT_SWITCH_JIT_CORE)
        return (void *)op;
#ifdef HAVE_COMPUTED_GOTO
    cg_lib = PARROT_CORE_CGP_OPLIB_INIT(1);
    return ((void **)cg_lib->op_func_table)[op];
#else
    return NULL;
#endif
}

/*

=item C<static int pass_int(PARROT_INTERP, PMC *sig, const char *src_base, const
void **src, char *dest_base, void * const *dest)>

=cut

*/

static int
pass_int(PARROT_INTERP, ARGIN(PMC *sig), ARGIN(const char *src_base),
        ARGIN(const void **src), ARGOUT(char *dest_base), ARGIN(void * const *dest))
{
    ASSERT_ARGS(pass_int)
    int i;
    int n = VTABLE_elements(interp, sig);

    for (i = 2; n; ++i, --n) {
        const INTVAL arg = *(const INTVAL *)(src_base + ((const opcode_t*)src)[i]);
        *(INTVAL *)(dest_base + ((const opcode_t*)dest)[i]) = arg;
    }
    return i;
}

/*

=item C<static int pass_num(PARROT_INTERP, PMC *sig, const char *src_base, const
void **src, char *dest_base, void * const *dest)>

=cut

*/

static int
pass_num(PARROT_INTERP, ARGIN(PMC *sig), ARGIN(const char *src_base),
        ARGIN(const void **src), ARGOUT(char *dest_base), ARGIN(void * const *dest))
{
    ASSERT_ARGS(pass_num)
    int i;
    int n = VTABLE_elements(interp, sig);

    for (i = 2; n; ++i, --n) {
        const FLOATVAL arg = *(const FLOATVAL *)(src_base + ((const opcode_t*)src)[i]);
        *(FLOATVAL *)(dest_base + ((const opcode_t*)dest)[i]) = arg;
    }
    return i;
}

/*

=item C<static int pass_str(PARROT_INTERP, PMC *sig, const char *src_base, const
void **src, char *dest_base, void * const *dest)>

=cut

*/

static int
pass_str(PARROT_INTERP, ARGIN(PMC *sig), ARGIN(const char *src_base),
        ARGIN(const void **src), ARGOUT(char *dest_base), ARGIN(void * const *dest))
{
    ASSERT_ARGS(pass_str)
    int i;
    int n = VTABLE_elements(interp, sig);

    for (i = 2; n; ++i, --n) {
        STRING * const arg = *(STRING* const *)(src_base + ((const opcode_t*)src)[i]);
        *(STRING* *)(dest_base + ((const opcode_t*)dest)[i]) = arg;
    }

    return i;
}

/*

=item C<static int pass_pmc(PARROT_INTERP, PMC *sig, const char *src_base, const
void **src, char *dest_base, void * const *dest)>

=cut

*/

static int
pass_pmc(PARROT_INTERP, ARGIN(PMC *sig), ARGIN(const char *src_base),
        ARGIN(const void **src), ARGOUT(char *dest_base), ARGIN(void * const *dest))
{
    ASSERT_ARGS(pass_pmc)
    int i;
    int n = VTABLE_elements(interp, sig);

    for (i = 2; n; ++i, --n) {
        PMC * const arg = *(PMC* const *)(src_base + ((const opcode_t*)src)[i]);
        *(PMC* *)(dest_base + ((const opcode_t*)dest)[i])= arg;
    }
    return i;
}

/*

=item C<static int pass_mixed(PARROT_INTERP, PMC *sig, const char *src_base,
void * const *src, char *dest_base, void * const *dest)>

=cut

*/

static int
pass_mixed(PARROT_INTERP, ARGIN(PMC *sig), ARGIN(const char *src_base),
        ARGIN(void * const *src), ARGOUT(char *dest_base), ARGIN(void * const *dest))
{
    ASSERT_ARGS(pass_mixed)
    int i;
    INTVAL *bitp;
    int n = VTABLE_elements(interp, sig);

    ASSERT_SIG_PMC(sig);
    GETATTR_FixedIntegerArray_int_array(interp, sig, bitp);

    for (i = 2; n; ++i, --n) {
        const INTVAL bits = *bitp++;
        switch (bits) {
            case PARROT_ARG_INTVAL:
                {
                const INTVAL argI = *(const INTVAL *)(src_base + ((const opcode_t*)src)[i]);
                *(INTVAL *)(dest_base + ((const opcode_t*)dest)[i])= argI;
                }
                break;
            case PARROT_ARG_INTVAL|PARROT_ARG_CONSTANT:
                *(INTVAL *)(dest_base + ((const opcode_t*)dest)[i]) = (INTVAL)(src)[i];
                break;
            case PARROT_ARG_FLOATVAL:
                {
                const FLOATVAL argN = *(const FLOATVAL *)(src_base + ((const opcode_t*)src)[i]);
                *(FLOATVAL *)(dest_base + ((const opcode_t*)dest)[i])= argN;
                }
                break;
            case PARROT_ARG_FLOATVAL|PARROT_ARG_CONSTANT:
                {
                const FLOATVAL argN = *(const FLOATVAL*)(src)[i];
                *(FLOATVAL *)(dest_base + ((const opcode_t*)dest)[i])= argN;
                }
                break;
            case PARROT_ARG_STRING:
                {
                STRING *argS = *(STRING * const *)(src_base + ((const opcode_t *)src)[i]);

                if (argS && PObj_constant_TEST(argS))
                    argS = Parrot_str_new_COW(interp, argS);

                *(STRING **)(dest_base + ((const opcode_t*)dest)[i]) = argS;
                }
                break;
            case PARROT_ARG_STRING|PARROT_ARG_CONSTANT:
                {
                STRING *argS = (STRING *)(src)[i];
                if (argS && PObj_constant_TEST(argS))
                    argS = Parrot_str_new_COW(interp, argS);
                *(STRING **)(dest_base + ((const opcode_t *)dest)[i]) = argS;
                }
                break;
            case PARROT_ARG_PMC:
                {
                PMC* const argP = *(PMC * const *)(src_base + ((const opcode_t*)src)[i]);
                *(PMC* *)(dest_base + ((const opcode_t*)dest)[i])= argP;
                }
                break;
            case PARROT_ARG_PMC|PARROT_ARG_CONSTANT:
                {
                PMC* const argP = (PMC*)(src)[i];
                *(PMC* *)(dest_base + ((const opcode_t*)dest)[i])= argP;
                }
                break;
            default:
                Parrot_ex_throw_from_c_args(interp, NULL, 1,
                    "bogus signature 0x%x", bits);
                break;
        }
    }
    return i;
}

/*

=item C<int parrot_pic_check_sig(PARROT_INTERP, PMC *sig1, PMC *sig2, int
*type)>

return argument count and type of the signature or -1 if not pic-able
the type PARROT_ARG_CONSTANT stands for mixed types or constants

=cut

*/

PARROT_WARN_UNUSED_RESULT
int
parrot_pic_check_sig(PARROT_INTERP, ARGIN(PMC *sig1), ARGIN(PMC *sig2),
        ARGOUT(int *type))
{
    ASSERT_ARGS(parrot_pic_check_sig)
    int i, n, t0;

    ASSERT_SIG_PMC(sig1);
    ASSERT_SIG_PMC(sig2);

    n = VTABLE_elements(interp, sig1);

    if (n != VTABLE_elements(interp, sig2))
        return -1;

    if (!n) {
        *type = 0;
        return 0;
    }

    for (i = 0; i < n; ++i) {
        int t1 = VTABLE_get_integer_keyed_int(interp, sig1, i);
        int t2 = VTABLE_get_integer_keyed_int(interp, sig2, i);

        if (i) {
            t0 = 0;
        }
        else {
            t0 = t1 & PARROT_ARG_TYPE_MASK;
            *type = t0;
        }

        if (t1 & PARROT_ARG_CONSTANT) {
            *type =  PARROT_ARG_CONSTANT;
            t1   &= ~PARROT_ARG_CONSTANT;
        }

        if (t1 & ~PARROT_ARG_TYPE_MASK)
            return -1;

        if (t2 & PARROT_ARG_CONSTANT) {
            *type =  PARROT_ARG_CONSTANT;
            t2   &= ~PARROT_ARG_CONSTANT;
        }

        if (t2 & ~PARROT_ARG_TYPE_MASK)
            return -1;

        if (t2 != t1)
            return -1;

        if (t1 != t0)
            *type = PARROT_ARG_CONSTANT;
    }

    return n;
}

/*

=item C<static int is_pic_param(PARROT_INTERP, void **pc, Parrot_MIC *mic,
opcode_t op)>

=cut

*/

static int
is_pic_param(PARROT_INTERP, ARGIN(void **pc), ARGOUT(Parrot_MIC *mic), opcode_t op)
{
    ASSERT_ARGS(is_pic_param)
    PMC                *sig2;
    PMC                *caller_ctx;
    opcode_t           *args;
    PMC         * const sig1 = (PMC *)(pc[1]);
    PMC                *ctx  = CONTEXT(interp);
    int                 type = 0;

    /* check params */

    if (op == PARROT_OP_set_returns_pc) {
        PMC * const ccont = CONTEXT_FIELD(interp, ctx, current_cont);
        if (!PMC_cont(ccont)->address)
            return 0;
        caller_ctx = PMC_cont(ccont)->to_ctx;
        args       = CONTEXT_FIELD(interp, caller_ctx, current_results);
    }
    else {
        caller_ctx = CONTEXT_FIELD(interp, ctx, caller_ctx);
        args       = interp->current_args;
    }

    if (args) {
        const INTVAL const_nr = args[1];
        int          n;

        /* check current_args signature */
        sig2 = CONTEXT_FIELD(interp, caller_ctx, constants[const_nr])->u.key;
        n    = parrot_pic_check_sig(interp, sig1, sig2, &type);

        if (n == -1)
            return 0;
    }
    else {
        if (VTABLE_elements(interp, sig1) == 0) {
            sig2 = NULL;
            type = 0;
        }
        else
            return 0;
    }

    switch (type) {
        case PARROT_ARG_INTVAL:
            mic->lru.f.real_function = (funcptr_t)pass_int;
            break;
        case PARROT_ARG_FLOATVAL:
            mic->lru.f.real_function = (funcptr_t)pass_num;
            break;
        case PARROT_ARG_STRING:
            mic->lru.f.real_function = (funcptr_t)pass_str;
            break;
        case PARROT_ARG_PMC:
            mic->lru.f.real_function = (funcptr_t)pass_pmc;
            break;
        case PARROT_ARG_CONSTANT:
            mic->lru.f.real_function = (funcptr_t)pass_mixed;
            break;
        default:
            return 0;
    }

    mic->m.sig = sig1;

    /* remember this sig2 - it has to match the other end at call time */
    mic->lru.u.signature = sig2;

    return 1;
}


/*

=item C<static int is_pic_func(PARROT_INTERP, void **pc, Parrot_MIC *mic, int
core_type)>

=cut

*/

static int
is_pic_func(PARROT_INTERP, ARGIN(void **pc), ARGOUT(Parrot_MIC *mic), int core_type)
{
    ASSERT_ARGS(is_pic_func)
    /*
     * if we have these opcodes
     *
     *   set_args '..' ...
     *   set_p_pc Px, PFunx
     *   get_results '..' ...
     *   invokecc_p Px
     *
     * and all args are matching the called sub and we don't have
     * too many args, and only INTVAL or FLOATVAL, the
     * whole sequence is replaced by the C<callr> pic opcode.
     *
     * Oh, I forgot to mention - the to-be-called C function is of
     * course compiled on-the-fly by the JIT compiler ;)
     *
     * pc is at set_args
     */

    PMC *sub, *sig_results;
    opcode_t *op, n;
    int flags;

    PMC * const ctx      = CONTEXT(interp);
    PMC * const sig_args = (PMC *)(pc[1]);

    ASSERT_SIG_PMC(sig_args);
    n                    = VTABLE_elements(interp, sig_args);
    interp->current_args = (opcode_t*)pc + CONTEXT_FIELD(interp, ctx, pred_offset);
    pc                  += 2 + n;
    op                   = (opcode_t*)pc + CONTEXT_FIELD(interp, ctx, pred_offset);

    if (*op != PARROT_OP_set_p_pc)
        return 0;

    do_prederef(pc, interp, core_type);
    sub = (PMC *)(pc[2]);

    PARROT_ASSERT(PObj_is_PMC_TEST(sub));

    if (sub->vtable->base_type != enum_class_Sub)
        return 0;

    pc += 3;    /* results */
    op  = (opcode_t *)pc + CONTEXT_FIELD(interp, ctx, pred_offset);

    if (*op != PARROT_OP_get_results_pc)
        return 0;

    do_prederef(pc, interp, core_type);
    sig_results = (PMC *)(pc[1]);
    ASSERT_SIG_PMC(sig_results);

    CONTEXT_FIELD(interp, ctx, current_results) = (opcode_t *)pc + CONTEXT_FIELD(interp, ctx, pred_offset);
    if (!parrot_pic_is_safe_to_jit(interp, sub, sig_args, sig_results, &flags))
        return 0;

    mic->lru.f.real_function = parrot_pic_JIT_sub(interp, sub, flags);
    mic->m.sig               = sig_args;

    return 1;
}

/*

=item C<void parrot_PIC_prederef(PARROT_INTERP, opcode_t op, void **pc_pred, int
core)>

Define either the normal prederef function or the PIC stub, if PIC for
this opcode function is available. Called from C<do_prederef>.

=cut

*/

void
parrot_PIC_prederef(PARROT_INTERP, opcode_t op, ARGOUT(void **pc_pred), int core)
{
    ASSERT_ARGS(parrot_PIC_prederef)
    op_func_t * const prederef_op_func = interp->op_lib->op_func_table;
    opcode_t  * const cur_opcode       = (opcode_t *)pc_pred;
    Parrot_MIC       *mic              = NULL;

    if (parrot_PIC_op_is_cached(op)) {
        const PackFile_ByteCode * const cs = interp->code;
        size_t                          n  = cur_opcode
                                           - (opcode_t *)cs->prederef.code;

        /*
         * pic_index is half the size of the code
         * XXX if it's there - pbc_merge needs updates
         */
        PARROT_ASSERT(cs->pic_index);
        n   = cs->pic_index->data[n / 2];
        mic = parrot_PIC_alloc_mic(interp, n);
    }

    switch (op) {
        case PARROT_OP_get_params_pc:
            if (is_pic_param(interp, pc_pred, mic, op)) {
                pc_pred[1] = (void *)mic;
                op         = PARROT_OP_pic_get_params___pc;
            }
            break;
        case PARROT_OP_set_returns_pc:
            if (is_pic_param(interp, pc_pred, mic, op)) {
                pc_pred[1] = (void *)mic;
                op         = PARROT_OP_pic_set_returns___pc;
            }
            break;
        case PARROT_OP_set_args_pc:
            if (is_pic_func(interp, pc_pred, mic, core)) {
                pc_pred[1] = (void *)mic;
                op         = PARROT_OP_pic_callr___pc;
            }
            break;
        default:
            break;
    }

    /* rewrite opcode */
    if (core == PARROT_SWITCH_CORE || core == PARROT_SWITCH_JIT_CORE)
        *pc_pred = (void **)op;
    else
        *pc_pred = ((void **)prederef_op_func)[op];
}

/*

=item C<void parrot_pic_find_infix_v_pp(PARROT_INTERP, PMC *left, PMC *right,
Parrot_MIC *mic, opcode_t *cur_opcode)>

=cut

*/

void
parrot_pic_find_infix_v_pp(PARROT_INTERP, ARGIN(PMC *left), ARGIN(PMC *right),
                ARGOUT(Parrot_MIC *mic), ARGOUT(opcode_t *cur_opcode))
{
    ASSERT_ARGS(parrot_pic_find_infix_v_pp)
    /* unused; deprecated */
}

/*

=back

=head1 AUTHOR

Leopold Toetsch with many hints from Ken Fox.

=head1 SEE ALSO

L<src/multidispatch.c>, L<src/object.c>, L<src/interp/interpreter.c>,
L<ops/core_ops_cgp.c>, L<include/parrot/pic.h>, L<ops/pic.ops>

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
