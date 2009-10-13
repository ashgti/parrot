/*
 * Copyright (C) 2002-2009, Parrot Foundation.
 *
 * frame_builder.h
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
INTVAL get_nci_I(PARROT_INTERP, ARGMOD(call_state *st), int n);

FLOATVAL get_nci_N(PARROT_INTERP, ARGMOD(call_state *st), int n);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING*
get_nci_S(PARROT_INTERP, ARGMOD(call_state *st), int n);

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC*
get_nci_P(PARROT_INTERP, ARGMOD(call_state *st), int n);

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
void*
get_nci_p(PARROT_INTERP, ARGMOD(call_state *st), int n);

#define GET_NCI_I(n) get_nci_I(interp, &st, (n))
#define GET_NCI_S(n) get_nci_S(interp, &st, (n))
#define GET_NCI_N(n) get_nci_N(interp, &st, (n))
#define GET_NCI_P(n) get_nci_P(interp, &st, (n))
#define GET_NCI_p(n) get_nci_p(interp, &st, (n))

/*
 * set return value
 */
void set_nci_I(PARROT_INTERP, ARGOUT(call_state *st), INTVAL val);

void set_nci_N(PARROT_INTERP, ARGOUT(call_state *st), FLOATVAL val);

void set_nci_S(PARROT_INTERP, ARGOUT(call_state *st), STRING *val);

void set_nci_P(PARROT_INTERP, ARGOUT(call_state *st), PMC* val);

#endif /* PARROT_FRAME_BUILDER_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
