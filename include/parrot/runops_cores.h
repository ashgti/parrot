/* runops_cores.h
 *  Copyright: (When this is determined...it will go here)
 *  CVS Info
 *     $Id$
 *  Overview:
 *     Header for runops cores.
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#if !defined(PARROT_RUNOPS_CORES_H_GUARD)
#define PARROT_RUNOPS_CORES_H_GUARD

#include "parrot/parrot.h"
#include "parrot/op.h"

typedef opcode_t *(*runops_core_f)(struct Parrot_Interp *, opcode_t *);

opcode_t *runops_fast_core(struct Parrot_Interp *, opcode_t *);

opcode_t *runops_cgoto_core(struct Parrot_Interp *, opcode_t *);

opcode_t *runops_slow_core(struct Parrot_Interp *, opcode_t *);

#endif

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil 
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
