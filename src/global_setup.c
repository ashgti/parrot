/* global_setup.c
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  CVS Info
 *     $Id$
 *  Overview:
 *      Performs all the global setting up of things. This includes the
 *      (very few) global variables that Parrot totes around
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#define INSIDE_GLOBAL_SETUP
#include "parrot/parrot.h"

/* These functions are defined in the auto-generated file core_pmcs.c */
extern void Parrot_initialize_core_pmcs(Interp *interp);
extern void Parrot_register_core_pmcs(Interp *interp, PMC *registry);

void
init_world(Interp *interpreter)
{
    INTVAL i;

    PMC *iglobals;
    PMC *classname_hash;
    PMC *self;

    string_init();              /* Set up the string subsystem */

    /* allocate core vtable */
#if 0
    /* no - we can't move existing vtables */
    Parrot_base_vtables =
        mem_sys_allocate(sizeof(VTABLE *) * enum_class_core_max);
#endif
    enum_class_max = enum_class_core_max;

    /* Call base vtable class constructor methods */
    Parrot_initialize_core_pmcs(interpreter);

    /* Now register the names of the PMCs */

    /* We need a hash */
    classname_hash = pmc_new(interpreter, enum_class_PerlHash);

    /* Now fill the hash */
    Parrot_register_core_pmcs(interpreter, classname_hash);

    /* init the interpreter globals array */
    iglobals = pmc_new(interpreter, enum_class_SArray);
    interpreter->iglobals = iglobals;
    VTABLE_set_integer_native(interpreter, iglobals, (INTVAL)IGLOBALS_SIZE);
    /* clear the array */
    for (i = 0; i < (INTVAL)IGLOBALS_SIZE; i++)
        VTABLE_set_pmc_keyed_int(interpreter, iglobals, i, NULL);
    VTABLE_set_pmc_keyed_int(interpreter, iglobals,
            (INTVAL)IGLOBALS_CLASSNAME_HASH, classname_hash);
    self = pmc_new_noinit(interpreter, enum_class_ParrotInterpreter);
    PMC_data(self) = interpreter;
    VTABLE_set_pmc_keyed_int(interpreter, iglobals,
            (INTVAL) IGLOBALS_INTERPRETER, self);
}

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
