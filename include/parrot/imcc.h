/*
 * Copyright (C) 2007, The Perl Foundation
 * $Id$
 */

#ifndef PARROT_IMCC_H_GUARD
#define PARROT_IMCC_H_GUARD

PARROT_API void imcc_initialize(PARROT_INTERP);
PARROT_API char * parseflags(PARROT_INTERP, int *argc, char **argv[]);
PARROT_API int imcc_run(PARROT_INTERP, const char *sourcefile, int argc, char * argv[]);

#endif /* PARROT_IMCC_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
