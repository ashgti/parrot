/*
Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
$Id$

=head1 NAME

src/res_lea.c - Resource allocation using malloc

=head1 DESCRIPTION

This file provides alternative implementations of memory allocation
function found in F<src/resources.c>. It is used if F<Configure.pl> is
passed the C<--gc=malloc> or C<--gc=malloc-trace> options.

The "lea" in the file name refers to Doug Lea, who wrote malloc
implementation found in F<src/malloc.c>.

=head2 Functions

=over 4

=cut

*/

#include <assert.h>
#include "parrot/parrot.h"

/*

=item C<void
Parrot_go_collect(struct Parrot_Interp *interpreter)>

Does nothing other than increment the iterpreter's C<collect_runs>
count.

=cut

*/

void
Parrot_go_collect(struct Parrot_Interp *interpreter)
{
    if (interpreter->GC_block_level) {
        return;
    }
    interpreter->collect_runs++;        /* fake it */
}

/*

=item C<static PARROT_INLINE void*
xmalloc(size_t size)>

Obtains the memory from C<malloc()> and returns it. Panics if there is
no memory available.

=cut

*/

static PARROT_INLINE void*
xmalloc(size_t size)
{
    void *p = malloc(size);
    if (!p)
        do_panic(NULL, "malloc: out of mem", __FILE__, __LINE__);
    return p;
}

/*

=item C<static PARROT_INLINE void*
xcalloc(size_t n, size_t size)>

Obtains the memory from C<calloc()> and returns it. Panics if there is
no memory available.

=cut

*/

static PARROT_INLINE void*
xcalloc(size_t n, size_t size)
{
    void *p = calloc(n, size);
    if (!p)
        do_panic(NULL, "calloc: out of mem", __FILE__, __LINE__);
    return p;
}

/*

=item C<static PARROT_INLINE void*
xrealloc(void *p, size_t size)>

Reallocates the memory with C<realloc()> and returns it. Panics if there
is no memory available.

=cut

*/

static PARROT_INLINE void*
xrealloc(void *p, size_t size)
{
    void *n = realloc(p, size);
    if (!n)
        do_panic(NULL, "realloc: out of mem", __FILE__, __LINE__);
    return n;
}

/*

=item C<void *
Parrot_reallocate(struct Parrot_Interp *interpreter, void *from, size_t size)>

COWable objects (strings or Buffers) use an INTVAL at C<bufstart> for
refcounting in DOD. C<bufstart> is incremented by that C<INTVAL>.

=cut

*/

void *
Parrot_reallocate(struct Parrot_Interp *interpreter, void *from, size_t size)
{
    Buffer * buffer = from;
    void *p;
    size_t oldlen = PObj_buflen(buffer);
    if (!PObj_bufstart(buffer)) {
        PObj_bufstart(buffer) = xcalloc(1, size + sizeof(INTVAL));
        LVALUE_CAST(INTVAL *, PObj_bufstart(buffer))++;
    }
    else {
        if (!size) {    /* realloc(3) does free, if size == 0 here */
            return PObj_bufstart(buffer);    /* do nothing */
        }
        p = xrealloc((INTVAL*)PObj_bufstart(buffer) - 1, size + sizeof(INTVAL));
        *(LVALUE_CAST(INTVAL *, p)++) = 0;
        if (size > PObj_buflen(buffer))
            memset((char*)p + oldlen, 0, size - oldlen);
        PObj_bufstart(buffer) = p;
    }
    PObj_buflen(buffer) = size;
    return PObj_bufstart(buffer);
}

/*

=item C<void *
Parrot_allocate(struct Parrot_Interp *interpreter, void *buffer, size_t size)>

Allocates and returns the required memory. C<size> is the number of
bytes of memory required.

=cut

*/

void *
Parrot_allocate(struct Parrot_Interp *interpreter, void *buffer, size_t size)
{
    Buffer * b = buffer;
    PObj_bufstart(b) = xmalloc(size + sizeof(INTVAL));
    *(LVALUE_CAST(INTVAL *, PObj_bufstart(b))++) = 0;
    PObj_buflen(b) = size;
    return b;
}

/*

=item C<void *
Parrot_allocate_zeroed(struct Parrot_Interp *interpreter, void *buffer,
        size_t size)>

Allocates, zeros and returns the required memory. C<size> is the number
in bytes of memory required.

=cut

*/

void *
Parrot_allocate_zeroed(struct Parrot_Interp *interpreter, void *buffer,
        size_t size)
{
    Buffer * b = buffer;
    PObj_bufstart(b) = xcalloc(1, size + sizeof(INTVAL));
    *(LVALUE_CAST(INTVAL *, PObj_bufstart(b))++) = 0;  /*   *( (INTVAL*) PObj_bufstart(b) ++ ) = 0 */
    PObj_buflen(b) = size;
    return b;
}

/*

=item C<void *
Parrot_reallocate_string(struct Parrot_Interp *interpreter, STRING *str,
                         size_t size)>

Reallocates the string buffer in C<*str> and returns it. C<size> is the
number of bytes memory required.


=cut

*/

void *
Parrot_reallocate_string(struct Parrot_Interp *interpreter, STRING *str,
                         size_t size)
{
    void *p;
    size_t pad;
    if (!PObj_bufstart(str))
        Parrot_allocate_string(interpreter, str, size);
    else if (size) {
        pad = STRING_ALIGNMENT - 1;
        size = ((size + pad + sizeof(INTVAL)) & ~pad);
        p = xrealloc((char *)((INTVAL*)PObj_bufstart(str) - 1), size);
        PObj_bufstart(str) = str->strstart = (char *)p + sizeof(INTVAL);
        /* usable size at bufstart */
        PObj_buflen(str) = size - sizeof(INTVAL);
    }
    return str->strstart;
}

/*

=item C<void *
Parrot_allocate_string(struct Parrot_Interp *interpreter, STRING *str,
                       size_t size)>

Allocates the string buffer in C<*str> and returns it. C<size> is the
number bytes of memory required.

=cut

*/

void *
Parrot_allocate_string(struct Parrot_Interp *interpreter, STRING *str,
                       size_t size)
{
    void *p;
    size_t pad;
    pad = STRING_ALIGNMENT - 1;
    size = ((size + pad + sizeof(INTVAL)) & ~pad);
    p = xcalloc(1, size);
    *(INTVAL*)p = 0;
    PObj_bufstart(str) = str->strstart = (char *)p + sizeof(INTVAL);
    PObj_buflen(str) = size - sizeof(INTVAL);
    return str;
}

/*

=item C<void
Parrot_initialize_memory_pools(struct Parrot_Interp *interpreter)>

Does nothing.

=cut

*/

void
Parrot_initialize_memory_pools(struct Parrot_Interp *interpreter)
{
}

/*

=item C<void
Parrot_destroy_memory_pools(Interp *interpreter)>

Does nothing.

=cut

*/

void
Parrot_destroy_memory_pools(Interp *interpreter)
{
}

/*

=back

=head1 SEE ALSO

F<config/auto/gc.pl>, F<src/malloc.c>, F<include/parrot/resources.h>.

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
