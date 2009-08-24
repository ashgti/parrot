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
 
=item C<STRING* Parrot_cx_get_string_constant(PARROT_INTERP, PMC *ctx, INTVAL
idx)>

Get string constant from context.

=cut

*/

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
STRING*
Parrot_cx_get_string_constant(PARROT_INTERP, ARGIN(PMC *ctx), INTVAL idx)
{
    ASSERT_ARGS(Parrot_cx_get_string_constant)
    Parrot_Context const * c = Parrot_cx_get_context(interp, ctx);
    return c->constants[idx]->u.string;
}


/*
 
=item C<PMC* Parrot_cx_get_pmc_constant(PARROT_INTERP, PMC *ctx, INTVAL idx)>

Get PMC constant from context.

=cut

*/

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
PMC*
Parrot_cx_get_pmc_constant(PARROT_INTERP, ARGIN(PMC *ctx), INTVAL idx)
{
    ASSERT_ARGS(Parrot_cx_get_pmc_constant)
    Parrot_Context const * c = Parrot_cx_get_context(interp, ctx);
    return c->constants[idx]->u.key;
}


/*

=item C<struct PackFile_Constant ** Parrot_cx_constants(PARROT_INTERP, PMC
*ctx)>

Get reference to constants.

=cut

*/
PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
struct PackFile_Constant **
Parrot_cx_constants(PARROT_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_cx_constants)
    return ((Parrot_Context*)(VTABLE_get_pointer(interp, ctx)))->constants;
}



/*

=item C<Parrot_Context* Parrot_cx_get_context(PARROT_INTERP, PMC *ctx)>

Fetch Parrot_Context from Context PMC.

=cut

*/
PARROT_EXPORT
PARROT_CAN_RETURN_NULL
Parrot_Context*
Parrot_cx_get_context(PARROT_INTERP, ARGIN_NULLOK(PMC *ctx))
{
    ASSERT_ARGS(Parrot_cx_get_context)
    if (PMC_IS_NULL(ctx))
        return NULL;

    return (Parrot_Context*)(VTABLE_get_pointer(interp, ctx));
}

/*

=item C<UINTVAL Parrot_cx_inc_recursion_depth(PARROT_INTERP, PMC *ctx)>

Get recursion depth from context.

=cut

*/

PARROT_EXPORT
UINTVAL
Parrot_cx_get_recursion_depth(PARROT_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_cx_inc_recursion_depth)
    Parrot_Context *c = Parrot_cx_get_context(interp, ctx);
    return c->recursion_depth;
}

/*

=item C<INTVAL Parrot_cx_inc_recursion_depth(PARROT_INTERP, PMC *ctx)>

Increase recurtion depth. Returns new recursion_depth value.

=cut

*/

PARROT_EXPORT
UINTVAL
Parrot_cx_inc_recursion_depth(PARROT_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_cx_inc_recursion_depth)
    Parrot_Context *c = Parrot_cx_get_context(interp, ctx);
    return ++c->recursion_depth;
}

/*

=item C<UINTVAL Parrot_cx_dec_recursion_depth(PARROT_INTERP, PMC *ctx)>

Decrease recurtion depth. Returns new recursion_depth value.

=cut

*/

PARROT_EXPORT
UINTVAL
Parrot_cx_dec_recursion_depth(PARROT_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_cx_dec_recursion_depth)
    Parrot_Context *c = Parrot_cx_get_context(interp, ctx);
    return --c->recursion_depth;
}

/*

=item C<PMC* Parrot_cx_get_caller_ctx(PARROT_INTERP, PMC *ctx)>

Get caller Context.

=cut

*/

PARROT_EXPORT
PMC*
Parrot_cx_get_caller_ctx(PARROT_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_cx_dec_recursion_depth)
    Parrot_Context *c = Parrot_cx_get_context(interp, ctx);
    return c->caller_ctx;
}


/*

=item C<void Parrot_cx_set_caller_ctx(PARROT_INTERP, PMC *ctx, PMC *caller_ctx)>

Set caller Context.

=cut

*/

PARROT_EXPORT
void
Parrot_cx_set_caller_ctx(PARROT_INTERP, ARGIN(PMC *ctx), ARGIN(PMC *caller_ctx))
{
    ASSERT_ARGS(Parrot_cx_dec_recursion_depth)
    Parrot_Context *c = Parrot_cx_get_context(interp, ctx);
    c->caller_ctx = caller_ctx;
}



/*

=back

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
