/*
Copyright (C) 2001-2007, The Perl Foundation.
$Id$

=head1 NAME

src/cpu_dep.c - CPU-dependent functions

=head1 DESCRIPTION

These functions are called while stackwalking during dead object
destruction. They implement conditional CPU-specific behaviour related
to register windowing.

Register windowing is a technique which avoids having to empty registers
by moving a virtual window up/down the register stack restricting the
number of registers which are visible.

Remember you read something about it in F<docs/infant.dev>?

=head2 Functions

*/

#include "parrot/parrot.h"

/* HEADER: include/parrot/dod.h */

#ifdef __ia64__

#  include <ucontext.h>
extern void *flush_reg_store(void);
#  define BACKING_STORE_BASE 0x80000fff80000000

#endif

static void trace_system_stack(Interp *interp);

/*

FUNCDOC: trace_system_areas

Traces the system stack and any additional CPU-specific areas.

*/

void
trace_system_areas(Interp *interp /*NN*/)
{
#if defined(__sparc) /* Flush register windows */
    static union {
        unsigned int insns[4];
        double align_hack[2];
    } u = { {
#  ifdef __sparcv9
                            0x81580000, /* flushw */
#  else
                            0x91d02003, /* ta ST_FLUSH_WINDOWS */
#  endif
                            0x81c3e008, /* retl */
                            0x01000000  /* nop */
    } };

    static void (*fn_ptr)(void) = (void (*)(void))&u.align_hack[0];
    fn_ptr();

#elif defined(__ia64__)

    struct ucontext ucp;
    void *current_regstore_top;

    getcontext(&ucp);
    current_regstore_top = flush_reg_store();

    trace_mem_block(interp, 0x80000fff80000000,
            (size_t)current_regstore_top);
#else

#  ifdef PARROT_HAS_HEADER_SETJMP
    Parrot_jump_buff env;

    /* Zero the Parrot_jump_buff, otherwise you will trace stale objects */
    memset(&env, 0, sizeof (env));

    /* this should put registers in env, which then get marked in
     * trace_system_stack below
     */
    setjmp(env);
#  endif

#endif


    trace_system_stack(interp);
}

/*

FUNCDOC: trace_system_stack

Traces the memory block starting at C<< interp->lo_var_ptr >>.

*/

static void
trace_system_stack(Interp *interp)
{
    size_t lo_var_ptr = (size_t)interp->lo_var_ptr;

    trace_mem_block(interp, (size_t)lo_var_ptr,
            (size_t)&lo_var_ptr);
}

/*

=head1 SEE ALSO

F<src/dod.c>, F<include/parrot/dod.h> and F<docs/infant.dev>.

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
