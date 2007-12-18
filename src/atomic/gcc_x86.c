/* atomic/gcc_x86.c
 *  Copyright (C) 2006, The Perl Foundation.
 *  SVN Info
 *     $Id$
 */

#include "parrot/parrot.h"
#include "parrot/atomic/gcc_x86.h"

/* HEADERIZER HFILE: include/parrot/atomic/gcc_x86.h */

/*

=head1 NAME

src/atomic/gcc_x86.c

=head1 DESCRIPTION

An implementation of atomic operations on x86 platforms with GCC-style inline
assembly suppport.

=head2 Functions

=over 4

=cut

*/

/*
 * if both I386 and X86_64 cmpxchg are defined, we are on x86_64 -
 * reuse existing code
 */
/*

=item C<PARROT_INLINE
PARROT_CANNOT_RETURN_NULL
void *
parrot_i386_cmpxchg(void *volatile *ptr, void *expect,
                                        void *update)>

RT#48260: Not yet documented!!!

=cut

*/


PARROT_API
PARROT_INLINE
PARROT_CANNOT_RETURN_NULL
void *
parrot_i386_cmpxchg(void *volatile *ptr, void *expect,
                                        void *update)
{
#if defined(PARROT_HAS_X86_64_GCC_CMPXCHG)
    __asm__ __volatile__("lock\n"
                         "cmpxchgq %1,%2":"=a"(expect):"q"(update), "m"(*ptr),
                         "0"(expect)
                         :"memory");
#else
    __asm__ __volatile__("lock\n"
                         "cmpxchgl %1,%2":"=a"(expect):"q"(update), "m"(*ptr),
                         "0"(expect)
                         :"memory");
#endif
    return expect;
}

/*

=item C<PARROT_INLINE
long
parrot_i386_xadd(volatile long *l, long amount)>

RT#48260: Not yet documented!!!

=cut

*/


PARROT_API
PARROT_INLINE
long
parrot_i386_xadd(volatile long *l, long amount)
{
    long result = amount;
#if defined(PARROT_HAS_X86_64_GCC_CMPXCHG)
    __asm__ __volatile__("lock\n" "xaddq %0, %1" : "=r"(result), "=m"(*l) :
            "0"(result), "m"(*l));
#else
    __asm__ __volatile__("lock\n" "xaddl %0, %1" : "=r"(result), "=m"(*l) :
            "0"(result), "m"(*l));
#endif
    return result + amount;
}

/*

=back

=cut

*/

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
