/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 NAME

src/context.c - Parrot_Context functions.

=head1 DESCRIPTION

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "parrot/context.h"


/* HEADERIZER HFILE: include/parrot/context.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

/*
 
=item C<Parrot_ctx_get_string_constant()>

Get string constant from context.

=cut

*/

STRING*
Parrot_ctx_get_string_constant(PARROT_INTERP, ARGIN(PMC* ctx), INTVAL idx) {
    Parrot_Context const * c = Parrot_ctx_get_context(interp, ctx);
    return c->constants[idx]->u.string;
}

/*
 
=item C<Parrot_ctx_get_pmc_constant()>

Get PMC constant from context.

=cut

*/

PARROT_CAN_RETURN_NULL
PMC*
Parrot_ctx_get_pmc_constant(PARROT_INTERP, ARGIN(PMC* ctx), INTVAL idx) {
    Parrot_Context const * c = Parrot_ctx_get_context(interp, ctx);
    return c->constants[idx]->u.key;
}


/*

=item C<struct PackFile_Constant ** Parrot_ctx_constants(PARROT_INTERP, PMC
*ctx)>

=cut

*/

PARROT_CANNOT_RETURN_NULL
struct PackFile_Constant **
Parrot_ctx_constants(PARROT_INTERP, ARGIN(PMC *ctx)) {
    ASSERT_ARGS(Parrot_ctx_constants)
    return ((Parrot_Context*)(VTABLE_get_pointer(interp, ctx)))->constants;
}



/*

=item C<Parrot_Context* Parrot_ctx_get_context(PARROT_INTERP, PMC *ctx)>

Fetch Parrot_Context from Context PMC.

=cut

*/

PARROT_CAN_RETURN_NULL
Parrot_Context*
Parrot_ctx_get_context(PARROT_INTERP, ARGIN(PMC *ctx)) {
    if (PMC_IS_NULL(ctx))
        return NULL;

    return (Parrot_Context*)(VTABLE_get_pointer(interp, ctx));
}

