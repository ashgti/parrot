/*
Copyright (C) 2001-2007, The Perl Foundation.
$Id$

=head1 NAME

src/vtables.c - Functions to build and manipulate vtables

=head1 DESCRIPTION

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "parrot/vtables.h"

/* HEADERIZER HFILE: include/parrot/vtables.h */

/*

=item C<PARROT_API
PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
VTABLE *
Parrot_new_vtable(SHIM_INTERP)>

Creates and returns a pointer to the new C<VTABLE>.

=cut

*/

PARROT_API
PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
VTABLE *
Parrot_new_vtable(SHIM_INTERP)
{
    return mem_allocate_zeroed_typed(VTABLE);
}

/*

=item C<PARROT_API
PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
VTABLE *
Parrot_clone_vtable(SHIM_INTERP, NOTNULL(const VTABLE *base_vtable))>

Clones C<*base_vtable> and returns a pointer to the new C<VTABLE>.

=cut

*/

PARROT_API
PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
VTABLE *
Parrot_clone_vtable(SHIM_INTERP, NOTNULL(const VTABLE *base_vtable))
{
    VTABLE * const new_vtable = mem_allocate_typed(VTABLE);

    if (new_vtable)
        STRUCT_COPY(new_vtable, base_vtable);

    return new_vtable;
}


/*

=item C<PARROT_API
void
Parrot_destroy_vtable(SHIM_INTERP, NULLOK(VTABLE *vtable))>

Destroys C<*vtable>.

=cut

*/

PARROT_API
void
Parrot_destroy_vtable(SHIM_INTERP, NULLOK(VTABLE *vtable))
{
    /* We sometimes get a type number allocated without any corresponding
     * vtable. E.g. if you load perl_group, perlscalar is this way.  */
    assert(vtable);

    if (vtable->ro_variant_vtable)
        mem_sys_free(vtable->ro_variant_vtable);

    mem_sys_free(vtable);
}

/*

=item C<void
parrot_alloc_vtables(PARROT_INTERP)>

TODO: Not yet documented!!!

=cut

*/

void
parrot_alloc_vtables(PARROT_INTERP)
{
    interp->vtables          = (VTABLE **)mem_sys_allocate_zeroed(
        sizeof (VTABLE *) * PARROT_MAX_CLASSES);

    interp->n_vtable_max     = enum_class_core_max;
    interp->n_vtable_alloced = PARROT_MAX_CLASSES;
}

/*

=item C<void
parrot_realloc_vtables(PARROT_INTERP)>

TODO: Not yet documented!!!

=cut

*/

void
parrot_realloc_vtables(PARROT_INTERP)
{
    /* 16 bigger seems reasonable, though it's only a pointer
       table and we could get bigger without blowing much memory
       */
    const INTVAL new_max     = interp->n_vtable_alloced + 16;
    const INTVAL new_size    = new_max              * sizeof (VTABLE *);
    const INTVAL old_size    = interp->n_vtable_max * sizeof (VTABLE *);
    interp->n_vtable_alloced = new_max;
    interp->vtables          = (VTABLE **)mem_sys_realloc_zeroed(
        interp->vtables, new_size, old_size);
}

/*

=item C<void
parrot_free_vtables(PARROT_INTERP)>

TODO: Not yet documented!!!

=cut

*/

void
parrot_free_vtables(PARROT_INTERP)
{
    int i;

    for (i = 1; i < interp->n_vtable_max; i++)
        Parrot_destroy_vtable(interp, interp->vtables[i]);

    mem_sys_free(interp->vtables);
}

/*

=item C<void
mark_vtables(PARROT_INTERP)>

TODO: Not yet documented!!!

=cut

*/

void
mark_vtables(PARROT_INTERP)
{
    INTVAL i;

    for (i = 1; i < interp->n_vtable_max; i++) {
        const VTABLE * const vtable = interp->vtables[i];

        /* XXX dynpmc groups have empty slots for abstract objects */
        if (!vtable)
            continue;

        if (vtable->mro)
            pobject_lives(interp, (PObj *)vtable->mro);
        if (vtable->_namespace)
            pobject_lives(interp, (PObj *)vtable->_namespace);
        if (vtable->whoami)
            pobject_lives(interp, (PObj *)vtable->whoami);
        if (vtable->does_str)
            pobject_lives(interp, (PObj *)vtable->does_str);
        if (vtable->isa_str)
            pobject_lives(interp, (PObj *)vtable->isa_str);
        if (vtable->pmc_class)
            pobject_lives(interp, (PObj *)vtable->pmc_class);
    }
}

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
