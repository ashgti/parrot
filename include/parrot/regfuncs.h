/* register_funcs.h
 *  Copyright: (When this is determined...it will go here)
 *  CVS Info
 *     $Id$
 *  Overview:
 *     Defines the register api
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#if !defined(PARROT_REGISTER_FUNCS_H_GUARD)
#define PARROT_REGISTER_FUNCS_H_GUARD

#include "parrot/parrot.h"
void Parrot_clear_i(struct Parrot_Interp *);
void Parrot_clear_s(struct Parrot_Interp *);
void Parrot_clear_p(struct Parrot_Interp *);
void Parrot_clear_n(struct Parrot_Interp *);

void Parrot_push_i(struct Parrot_Interp *);
void Parrot_push_n(struct Parrot_Interp *);
void Parrot_push_s(struct Parrot_Interp *);
void Parrot_push_p(struct Parrot_Interp *);

/*
void Parrot_clone_i(struct Parrot_Interp *);
void Parrot_clone_n(struct Parrot_Interp *);
void Parrot_clone_s(struct Parrot_Interp *);
void Parrot_clone_p(struct Parrot_Interp *);
*/
void Parrot_pop_i(struct Parrot_Interp *);
void Parrot_pop_n(struct Parrot_Interp *);
void Parrot_pop_s(struct Parrot_Interp *);
void Parrot_pop_p(struct Parrot_Interp *);

void Parrot_push_on_stack(void *thing, INTVAL size, INTVAL type);
void Parrot_pop_off_stack(void *thing, INTVAL type);

#endif /* PARROT_REGISTER_FUNCS_H */

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil 
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
