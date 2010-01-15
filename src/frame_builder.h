/*
 * Copyright (C) 2002-2009, Parrot Foundation.
 */

/*
 * frame_builder.h
 *
 * i386
 *
 * $Id$
 */

#ifndef PARROT_FRAME_BUILDER_H_GUARD
#define PARROT_FRAME_BUILDER_H_GUARD

#if defined(__cplusplus)
#  define EXTERN extern "C"
#else
#  define EXTERN
#endif

#include <assert.h>
#include "parrot/parrot.h"

/*
 * NCI interface
 */

void *
Parrot_jit_build_call_func(Interp *, PMC *, STRING *, void **);

/* custom pmc callback functions */
void
Parrot_jit_free_buffer(PARROT_INTERP, void *ptr, void *priv);

PMC*
Parrot_jit_clone_buffer(PARROT_INTERP, PMC *pmc, void *priv);

/*
 * helper funcs - get argument n
 */

#endif /* PARROT_FRAME_BUILDER_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
