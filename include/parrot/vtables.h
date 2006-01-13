/* vtables.h
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  CVS Info
 *     $Id$
 *  Overview:
 *     Vtable manipulation code. Not to be confused with vtable.h
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#if !defined(PARROT_VTABLES_H_GUARD)
#define PARROT_VTABLES_H_GUARD

PARROT_API VTABLE *Parrot_new_vtable(Parrot_Interp);
PARROT_API VTABLE *Parrot_clone_vtable(Parrot_Interp, const VTABLE *base_vtable);
PARROT_API void Parrot_destroy_vtable(Parrot_Interp, VTABLE *vtable);

PARROT_API void Parrot_vtable_set_type(Parrot_Interp, VTABLE *, INTVAL);
PARROT_API void Parrot_vtable_set_name(Parrot_Interp, VTABLE *, STRING *);
PARROT_API void Parrot_vtable_set_data(Parrot_Interp, VTABLE *, void *);

#endif /* PARROT_VTABLES_H_GUARD */

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
