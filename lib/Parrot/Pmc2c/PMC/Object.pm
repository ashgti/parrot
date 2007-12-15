package Parrot::Pmc2c::PMC::Object;

# Copyright (C) 2007, The Perl Foundation.
# $Id$

use base 'Parrot::Pmc2c::PMC';
use strict;
use warnings;

=head1 C<body($method, $line, $out_name)>

Returns the C code for the method body.

Overrides the default implementation to direct all unknown methods to
first check if there is an implementation of the vtable method in the
vtable methods hash of this class of any others, and delegates up to
any PMCs in the MRO.

=cut

sub pre_method_gen {
    my ($self) = @_;

    # vtable methods
    foreach my $method ( @{ $self->vtable->methods } ) {
        my $vt_method_name = $method->name;
        next unless $self->normal_unimplemented_vtable($vt_method_name);
        my $new_default_method = $method->clone(
            {
                parent_name => $self->name,
                type        => Parrot::Pmc2c::Method::VTABLE,
            }
        );

        my ( $return_prefix, $ret_suffix, $args, $sig, $return_type_char, $null_return ) =
            $new_default_method->signature;
        my $void_return  = $return_type_char eq 'v' ? 'return;'    : '';
        my $return       = $return_type_char eq 'v' ? ''           : $return_prefix;
        my $superargs    = $args;
        $superargs       =~ s/^,//;

        $new_default_method->body( Parrot::Pmc2c::Emitter->text(<<"EOC") );
    Parrot_Object * const obj       = PARROT_OBJECT(pmc);
    Parrot_Class  * const _class    = PARROT_CLASS(obj->_class);
    STRING        * const meth_name = CONST_STRING(interp, "$vt_method_name");

    /* Walk and search for the vtable method. */
    const int num_classes = VTABLE_elements(interp, _class->all_parents);
    int i;
    for (i = 0; i < num_classes; i++) {
        /* Get the class. */
        PMC * const cur_class = VTABLE_get_pmc_keyed_int(interp, _class->all_parents, i);

        PMC * const meth = Parrot_oo_find_vtable_override_for_class(interp, cur_class, meth_name);
        if (!PMC_IS_NULL(meth)) {
            ${return}Parrot_run_meth_fromc_args$ret_suffix(interp, meth, pmc, meth_name, "$sig"$args);
            $void_return
        }

        if (cur_class->vtable->base_type == enum_class_PMCProxy) {
            /* Get the PMC instance and call the vtable method on that. */
            PMC * const del_object = 
                VTABLE_get_attr_keyed(interp, SELF, cur_class, CONST_STRING(interp, "proxy"));

            if (!PMC_IS_NULL(del_object)) {
                ${return}VTABLE_$vt_method_name(interp, del_object$args); 
                $void_return
            }
        }
    }
    ${return}SUPER($superargs);
    $void_return
EOC
        $self->add_method($new_default_method);
    }
    return 1;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
