/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 NAME

src/pmc_freeze.c - Freeze and thaw functionality

=head1 DESCRIPTION

Thawing PMCs uses a list with (maximum) size of the amount of PMCs to
keep track of retrieved PMCs.

The individual information of PMCs is frozen/thawed by their vtables.

To avoid recursion, the whole functionality is driven by
C<< pmc->vtable->visit >>, which is called for the first PMC initially.
Container PMCs call a "todo-callback" for all contained PMCs. The
individual action vtable (freeze/thaw) is then called for all todo-PMCs.

In the current implementation C<visit_info> is a stand-in for some kind of
serializer PMC which will eventually be written. It associates a Parrot
C<STRING> with a vtable.

=cut

*/

#include "parrot/parrot.h"
#include "pmc/pmc_callcontext.h"
#include "pmc_freeze.str"

/* HEADERIZER HFILE: include/parrot/pmc_freeze.h */
/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static void create_buffer(PARROT_INTERP,
    ARGIN_NULLOK(PMC *pmc),
    ARGMOD(visit_info *info))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*info);

PARROT_INLINE
static void ensure_buffer_size(PARROT_INTERP,
    ARGIN(visit_info *io),
    size_t len)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static INTVAL get_visit_integer(PARROT_INTERP, ARGIN(visit_info *io))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_INLINE
static INTVAL INFO_HAS_DATA(ARGIN(visit_info *io))
        __attribute__nonnull__(1);

PARROT_INLINE
static INTVAL OUTPUT_LENGTH(ARGIN(visit_info *io))
        __attribute__nonnull__(1);

static void push_opcode_integer(PARROT_INTERP,
    ARGIN(visit_info *io),
    INTVAL v)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void push_opcode_number(PARROT_INTERP,
    ARGIN(visit_info *io),
    FLOATVAL v)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void push_opcode_string(PARROT_INTERP,
    ARGIN(visit_info *io),
    ARGIN(STRING *v))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

static INTVAL shift_opcode_integer(SHIM_INTERP, ARGIN(visit_info *io))
        __attribute__nonnull__(2);

static FLOATVAL shift_opcode_number(SHIM_INTERP, ARGIN(visit_info *io))
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static STRING* shift_opcode_string(PARROT_INTERP, ARGIN(visit_info *io))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void visit_info_init(PARROT_INTERP,
    ARGOUT(visit_info *info),
    visit_enum_type what,
    ARGIN(STRING *input),
    ARGIN(PMC *pmc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*info);

static void visit_loop_todo_list(PARROT_INTERP,
    ARGIN_NULLOK(PMC *current),
    ARGIN(visit_info *info))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

static void visit_todo_list_freeze(PARROT_INTERP,
    ARGIN_NULLOK(PMC* pmc),
    ARGIN(visit_info* info))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

PARROT_INLINE
static void visit_todo_list_thaw(PARROT_INTERP,
    SHIM(PMC* pmc_not_used),
    ARGIN(visit_info* info))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

#define ASSERT_ARGS_create_buffer __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(info))
#define ASSERT_ARGS_ensure_buffer_size __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(io))
#define ASSERT_ARGS_get_visit_integer __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(io))
#define ASSERT_ARGS_INFO_HAS_DATA __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(io))
#define ASSERT_ARGS_OUTPUT_LENGTH __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(io))
#define ASSERT_ARGS_push_opcode_integer __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(io))
#define ASSERT_ARGS_push_opcode_number __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(io))
#define ASSERT_ARGS_push_opcode_string __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(io) \
    , PARROT_ASSERT_ARG(v))
#define ASSERT_ARGS_shift_opcode_integer __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(io))
#define ASSERT_ARGS_shift_opcode_number __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(io))
#define ASSERT_ARGS_shift_opcode_string __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(io))
#define ASSERT_ARGS_visit_info_init __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(info) \
    , PARROT_ASSERT_ARG(input) \
    , PARROT_ASSERT_ARG(pmc))
#define ASSERT_ARGS_visit_loop_todo_list __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(info))
#define ASSERT_ARGS_visit_todo_list_freeze __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(info))
#define ASSERT_ARGS_visit_todo_list_thaw __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(info))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

/* when thawing a string longer then this size, we first do a GC run and then
 * block GC - the system can't give us more headers */

#define THAW_BLOCK_GC_SIZE 100000

/*

=back

=head2 Public Interface

=over 4

=item C<STRING* Parrot_freeze(PARROT_INTERP, PMC *pmc)>

Freeze using either method.

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
STRING*
Parrot_freeze(PARROT_INTERP, ARGIN(PMC *pmc))
{
    ASSERT_ARGS(Parrot_freeze)
    PMC *image = pmc_new(interp, enum_class_ImageIO);
    VTABLE_set_pmc(interp, image, pmc);
    return VTABLE_get_string(interp, image);
}


/*

=item C<PMC* Parrot_thaw(PARROT_INTERP, STRING *image)>

Thaws a PMC.  Called from the C<thaw> opcode.

For now it seems cheaper to use a list for remembering contained
aggregates. We could of course decide dynamically, which strategy to
use, e.g.: given a big image, the first thawed item is a small
aggregate. This implies, it probably contains (or some big strings) more
nested containers, for which another approach could be a win.

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC*
Parrot_thaw(PARROT_INTERP, ARGIN(STRING *image))
{
    ASSERT_ARGS(Parrot_thaw)

    PMC        *info     = pmc_new(interp, enum_class_ImageIO);
    int         gc_block = 0;
    PMC        *result;

    /*
     * if we are thawing a lot of PMCs, it's cheaper to do
     * a GC run first and then block GC - the limit should be
     * chosen so that no more then one GC run would be triggered
     *
     * XXX
     *
     * md5_3.pir shows a segfault during thawing the config hash
     * info->thaw_ptr becomes invalid - seems that the hash got
     * collected under us.
     */
    if (1 || (Parrot_str_byte_length(interp, image) > THAW_BLOCK_GC_SIZE)) {
        Parrot_block_GC_mark(interp);
        Parrot_block_GC_sweep(interp);
        gc_block = 1;
    }

    VTABLE_set_pointer(interp, info, &result);
    VTABLE_set_string_native(interp, info, image);

    if (gc_block) {
        Parrot_unblock_GC_mark(interp);
        Parrot_unblock_GC_sweep(interp);
    }

    return result;
}


/*

=item C<PMC* Parrot_thaw_constants(PARROT_INTERP, STRING *image)>

Thaws constants, used by PackFile for unpacking PMC constants.
This is a lie. It does nothing different from Parrot_thaw at the moment.

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC*
Parrot_thaw_constants(PARROT_INTERP, ARGIN(STRING *image))
{
    ASSERT_ARGS(Parrot_thaw_constants)
    return Parrot_thaw(interp, image);
}


/*

=item C<PMC* Parrot_clone(PARROT_INTERP, PMC *pmc)>

There are for sure shortcuts to clone faster, e.g. always thaw the image
immediately or use a special callback.  For now we just thaw a frozen PMC.

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC*
Parrot_clone(PARROT_INTERP, ARGIN(PMC *pmc))
{
    ASSERT_ARGS(Parrot_clone)
    return VTABLE_clone(interp, pmc);
}


/*

=back

=head1 TODO

The seen-hash version for freezing might go away sometime.

=head1 SEE ALSO

Lot of discussion on p6i and F<docs/dev/pmc_freeze.pod>.

=head1 HISTORY

Initial version by leo 2003.11.03 - 2003.11.07.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
