/*
 * $Id$
 * Copyright (C) 2002-2009, Parrot Foundation.
 */

/*

=head1 NAME

compilers/imcc/sets.c

=head1 DESCRIPTION

An implementation of sets -- used for tracking register usage.

=head2 Functions

=over 4

=cut

*/

#include "imc.h"
#include "sets.h"

/* HEADERIZER HFILE: compilers/imcc/sets.h */

/* XXX */
#define fatal(e, s1, s2) do { \
    fprintf(stderr, "%s: %s", (s1), (s2)); \
    exit(e); \
} while (0)

#define NUM_BYTES(length)    (((length) / 8) + 1)
#define BYTE_IN_SET(element) ((element) >> 3)
#define BIT_IN_BYTE(element) (1 << ((element) & 7))

/*

=item C<Set* set_make(unsigned int length)>

Creates a new Set object.

=cut

*/

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
Set*
set_make(unsigned int length)
{
    ASSERT_ARGS(set_make)
    Set * const s = mem_allocate_zeroed_typed(Set);
    s->length     = length;
    s->bmp        = mem_allocate_n_zeroed_typed(NUM_BYTES(length),
                        unsigned char);

    return s;
}


/*

=item C<Set* set_make_full(unsigned int length)>

Creates a new Set object of C<length> items, setting them all to full.

=cut

*/

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
Set*
set_make_full(unsigned int length)
{
    ASSERT_ARGS(set_make_full)
    Set * const s      = set_make(length);
    const size_t bytes = NUM_BYTES(length);

    if (bytes)
        memset(s->bmp, 0xff, bytes);

    return s;
}


/*

=item C<void set_free(Set *s)>

Frees the given Set and its allocated memory.

=cut

*/

void
set_free(ARGMOD(Set *s))
{
    ASSERT_ARGS(set_free)
    if (s->bmp)
        mem_sys_free(s->bmp);

    mem_sys_free(s);
}


/*

=item C<void set_clear(Set *s)>

Clears all bits in the Set.

=cut

*/

void
set_clear(ARGMOD(Set *s))
{
    ASSERT_ARGS(set_clear)
    memset(s->bmp, 0, NUM_BYTES(s->length));
}


/*

=item C<Set* set_copy(const Set *s)>

Copies the set C<s>, returning a new set pointer.

=cut

*/

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
Set*
set_copy(ARGIN(const Set *s))
{
    ASSERT_ARGS(set_copy)
    Set * const d = set_make(s->length);

    memcpy(d->bmp, s->bmp, NUM_BYTES(d->length));
    return d;
}


/*

=item C<int set_equal(const Set *s1, const Set *s2)>

Compares two sets for equality; sets are equal if they contain the same
elements.

Raises a fatal error if the two Sets have different lengths.

=cut

*/

int
set_equal(ARGIN(const Set *s1), ARGIN(const Set *s2))
{
    ASSERT_ARGS(set_equal)
    int          mask;
    const size_t bytes = s1->length / 8;

    if (s1->length != s2->length)
        fatal(1, "set_equal", "Sets don't have the same length\n");

    if (bytes)
        if (memcmp(s1->bmp, s2->bmp, bytes) != 0)
            return 0;

    if (s1->length % 8 == 0)
        return 1;

    mask = (1 << (s1->length % 8)) - 1;

    if ((s1->bmp[bytes] & mask) != (s2->bmp[bytes] & mask))
        return 0;

    return 1;
}


/*

=item C<void set_add(Set *s, unsigned int element)>

Adds to set C<s> the element C<element>.

=cut

*/

void
set_add(ARGMOD(Set *s), unsigned int element)
{
    ASSERT_ARGS(set_add)
    const int elem_byte_in_set = BYTE_IN_SET(element);
    const int bytes_in_set     = BYTE_IN_SET(s->length);

    if (bytes_in_set < elem_byte_in_set) {
        s->bmp = (unsigned char *)mem_sys_realloc_zeroed(s->bmp,
            NUM_BYTES(element), NUM_BYTES(s->length));
        s->length += 8;
    }

    s->bmp[elem_byte_in_set] |= BIT_IN_BYTE(element);
}


/*

=item C<unsigned int set_first_zero(const Set *s)>

Sets the first unused item in the set.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_PURE_FUNCTION
unsigned int
set_first_zero(ARGIN(const Set *s))
{
    ASSERT_ARGS(set_first_zero)
    unsigned int i;


    for (i = 0; i < NUM_BYTES(s->length); ++i) {
        const int set_byte = s->bmp[i];
        int j;

        if (set_byte == 0xFF)
            continue;

        for (j = 0; j < 8; ++j) {
            unsigned int element = i * 8 + j;

            if (!set_contains(s, element))
                return element;
        }
    }

    return s->length;
}


/*

=item C<int set_contains(const Set *s, unsigned int element)>

Checks whether the specified element is present in the specified Set argument.
Returns 1 if it is, 0 otherwise.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_PURE_FUNCTION
int
set_contains(ARGIN(const Set *s), unsigned int element)
{
    ASSERT_ARGS(set_contains)

    if (element > s->length)
        return 0;
    else {
        /* workaround for another lcc bug.. */
        const int byte_in_set = element >> 3;
        const int pos_in_byte = BIT_IN_BYTE(element);


        return s->bmp[byte_in_set] & pos_in_byte;
    }
}


/*

=item C<Set * set_union(const Set *s1, const Set *s2)>

Computes the union of the two Set arguments, returning it as a new Set.

Raises a fatal error if the two Sets have different lengths.

=cut

*/

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
Set *
set_union(ARGIN(const Set *s1), ARGIN(const Set *s2))
{
    ASSERT_ARGS(set_union)
    unsigned int i;
    Set * const s = set_make(s1->length);

    if (s1->length != s2->length)
        fatal(1, "set_union", "Sets don't have the same length\n");

    for (i = 0; i < BYTE_IN_SET(s1->length); i++) {
        s->bmp[i] = s1->bmp[i] | s2->bmp[i];
    }

    return s;
}


/*

=item C<Set * set_intersec(const Set *s1, const Set *s2)>

Creates a new Set object that is the intersection of the Set arguments (defined
through the binary C<and> operator.)

Raises a fatal error if the two Sets have different lengths.

=cut

*/

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
Set *
set_intersec(ARGIN(const Set *s1), ARGIN(const Set *s2))
{
    ASSERT_ARGS(set_intersec)
    unsigned int i;
    Set * const  s = set_make(s1->length);

    if (s1->length != s2->length)
        fatal(1, "set_intersec", "Sets don't have the same length\n");

    for (i = 0; i < BYTE_IN_SET(s1->length); i++) {
        s->bmp[i] = s1->bmp[i] & s2->bmp[i];
    }

    return s;
}


/*

=item C<void set_intersec_inplace(Set *s1, const Set *s2)>

Performs a set intersection in place -- the first Set argument changes to
contain the result.

=cut

*/

void
set_intersec_inplace(ARGMOD(Set *s1), ARGIN(const Set *s2))
{
    ASSERT_ARGS(set_intersec_inplace)
    unsigned int i;

    if (s1->length != s2->length)
        fatal(1, "set_intersec_inplace", "Sets don't have the same length\n");

    for (i = 0; i < BYTE_IN_SET(s1->length); i++) {
        s1->bmp[i] &= s2->bmp[i];
    }
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
