/* nci.h
 *  Copyright (C) 2001-2003, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     The nci api handles building native call frames
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#ifndef PARROT_NCI_H_GUARD
#define PARROT_NCI_H_GUARD

#include "parrot/parrot.h"

void *build_call_func(Interp *, PMC *, String *);

#endif /* PARROT_NCI_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
