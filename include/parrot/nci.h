/* nci.h
 *  Copyright (C) 2001-2007, Parrot Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     The nci API handles building native call frames
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#ifndef PARROT_NCI_H_GUARD
#define PARROT_NCI_H_GUARD

#include "parrot/parrot.h"

/* NCI PMC interface constants */
/* &gen_from_enum(nci.pasm) */
typedef enum {
    PARROT_NCI_ARITY,
    PARROT_NCI_PCC_SIGNATURE_PARAMS,
    PARROT_NCI_PCC_SIGNATURE_RET,
    PARROT_NCI_LONG_SIGNATURE,
    PARROT_NCI_MULTI_SIG,
} parrot_nci_enum_t;
/* &end_gen */

void *build_call_func(PARROT_INTERP, SHIM(PMC *pmc_nci), NOTNULL(STRING *signature), NOTNULL(int *jitted));

#endif /* PARROT_NCI_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
