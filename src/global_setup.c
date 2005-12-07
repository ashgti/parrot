/*
Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
$Id$

=head1 NAME

src/global_setup.c - Global setup

=head1 DESCRIPTION

Performs all the global setting up of things. This includes the (very
few) global variables that Parrot totes around.

I<What are these global variables?>

=head2 Functions

=over 4

=cut

*/

#define INSIDE_GLOBAL_SETUP
#include "parrot/parrot.h"
#include "global_setup.str"

/* These functions are defined in the auto-generated file core_pmcs.c */
extern void Parrot_initialize_core_pmcs(Interp *interp);

static void
create_config_hash(Interp *interpreter, PMC *iglobals)
{
    STRING *config = parrot_get_config_string(interpreter);
    PMC *hash;

    if (config) {
        hash = Parrot_thaw(interpreter, config);
    }
    else
        hash = pmc_new(interpreter, enum_class_Hash);
    VTABLE_set_pmc_keyed_int(interpreter, iglobals,
            (INTVAL) IGLOBALS_CONFIG_HASH, hash);

}
/*

=item C<void init_world(Interp *interpreter)>

This is the actual initialization code called by C<Parrot_init()>.

It sets up the Parrot system, running any platform-specific init code if
necessary, then initializing the string subsystem, and setting up the
base vtables and core PMCs.

C<interpreter> should be the root interpreter returned by
C<Parrot_new(NULL)>.

=cut

*/

void
init_world(Interp *interpreter)
{
    PMC *iglobals;
    PMC *self;

#ifdef PARROT_HAS_PLATFORM_INIT_CODE
    Parrot_platform_init_code();
#endif

    /*
     * TODO allocate core vtable table only once - or per interpreter
     *      divide globals into real globals and per interpreter
     */
    if (!Parrot_base_vtables) {
        Parrot_base_vtables =
            mem_sys_allocate_zeroed(sizeof(VTABLE *) * PARROT_MAX_CLASSES);
        enum_class_max = enum_class_core_max;
        class_table_size = PARROT_MAX_CLASSES;
    }

    /* Call base vtable class constructor methods */
    Parrot_initialize_core_pmcs(interpreter);

    iglobals = interpreter->iglobals;
    VTABLE_set_pmc_keyed_int(interpreter, iglobals,
            (INTVAL)IGLOBALS_CLASSNAME_HASH, interpreter->class_hash);
    self = pmc_new_noinit(interpreter, enum_class_ParrotInterpreter);
    PMC_data(self) = interpreter;
    VTABLE_set_pmc_keyed_int(interpreter, iglobals,
            (INTVAL) IGLOBALS_INTERPRETER, self);
    /*
     * create runtime config hash
     */
    create_config_hash(interpreter, iglobals);
    /*
     * HLL support
     */
    if (interpreter->parent_interpreter)
        interpreter->HLL_info = interpreter->parent_interpreter->HLL_info;
    else {
        STRING *parrot = CONST_STRING(interpreter, "parrot");
        interpreter->HLL_info = constant_pmc_new(interpreter,
                enum_class_ResizablePMCArray);
        Parrot_register_HLL(interpreter, parrot, NULL);
    }
    /*
     * lib search paths
     */
    parrot_init_library_paths(interpreter);
}

/*

=back

=head1 SEE ALSO

F<include/parrot/global_setup.h>.

=cut

*/

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
