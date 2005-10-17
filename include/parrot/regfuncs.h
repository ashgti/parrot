/* register_funcs.h
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
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

void Parrot_push_regs(Interp *);
void Parrot_pop_regs(Interp *);

void Parrot_clear_i(Interp *);
void Parrot_clear_s(Interp *);
void Parrot_clear_p(Interp *);
void Parrot_clear_n(Interp *);

void Parrot_push_on_stack(void *thing, INTVAL size, INTVAL type);
void Parrot_pop_off_stack(void *thing, INTVAL type);

#endif /* PARROT_REGISTER_FUNCS_H_GUARD */

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
