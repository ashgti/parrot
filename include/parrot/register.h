/* register.h
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

#if !defined(PARROT_REGISTER_H_GUARD)
#define PARROT_REGISTER_H_GUARD

#include "parrot/string.h"

struct IReg {
    INTVAL registers[NUM_REGISTERS];
};

struct NReg {
    FLOATVAL registers[NUM_REGISTERS];
};

struct SReg {
    STRING *registers[NUM_REGISTERS];
};

struct PReg {
    PMC *registers[NUM_REGISTERS];
};

struct Stack_Chunk;

void Parrot_push_regs(Interp *);
void Parrot_pop_regs(Interp *);

void Parrot_clear_i(Interp *);
void Parrot_clear_s(Interp *);
void Parrot_clear_p(Interp *);
void Parrot_clear_n(Interp *);

void setup_register_stacks(Interp*);
void mark_register_stack(Interp* interpreter,
                             struct Stack_Chunk* stack);

#endif /* PARROT_REGISTER_H_GUARD */

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
