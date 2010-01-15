# Copyright (C) 2009, Parrot Foundation.
# $Id$

package Parrot::NativeCall;

use strict;
use warnings;

use base 'Exporter';
our @EXPORT_OK = qw{ signature_nci_to_pcc };

=head1 NAME

Parrot::NativeCall - Tools for building native call routines

=head1 SYNOPSIS

  use Parrot::NativeCall 'signature_nci_to_pcc';

  my $pcc_sig = signature_nci_to_pcc("v  VVV");

=head1 DESCRIPTION

C<Parrot::NativeCall> knows how to map NCI signatures to nci frame
functions.

=head1 GLOBAL VARIABLES

=over

=item C<%signature_table>

Maps NCI signature items to elements of a native call routine.

For use by F<tools/build/nativecall.pl>. New code should probably write
a wrapper in this module to encapsulate the access.

=cut

our %signature_table = (
    p => {
        as_proto => "void *",
        other_decl => "PMC * const final_destination = pmc_new(interp, enum_class_UnManagedStruct);",
        sig_char => "P",
        ret_assign => "VTABLE_set_pointer(interp, final_destination, return_data);\n    Parrot_pcc_fill_returns_from_c_args(interp, call_object, \"P\", final_destination);",
    },
    i => { as_proto => "int",    sig_char => "I" },
    l => { as_proto => "long",   sig_char => "I" },
    c => { as_proto => "char",   sig_char => "I" },
    s => { as_proto => "short",  sig_char => "I" },
    f => { as_proto => "float",  sig_char => "N" },
    d => { as_proto => "double", sig_char => "N" },
    t => { as_proto => "char *",
           other_decl => "STRING *final_destination;",
           ret_assign => "final_destination = Parrot_str_new(interp, return_data, 0);\n    Parrot_pcc_fill_returns_from_c_args(interp, call_object, \"S\", final_destination);",
           sig_char => "S" },
    v => { as_proto => "void",
           return_type => "void *",
           sig_char => "v",
           ret_assign => "",
           func_call_assign => ""
         },
    P => { as_proto => "PMC *", sig_char => "P" },
    O => { as_proto => "PMC *", returns => "", sig_char => "Pi" },
    J => { as_proto => "PARROT_INTERP", returns => "", sig_char => "" },
    S => { as_proto => "STRING *", sig_char => "S" },
    I => { as_proto => "INTVAL", sig_char => "I" },
    N => { as_proto => "FLOATVAL", sig_char => "N" },
    b => { as_proto => "void *", as_return => "", sig_char => "S" },
    B => { as_proto => "char **", as_return => "", sig_char => "S" },
    # These should be replaced by modifiers in the future
    2 => { as_proto => "short *",  sig_char => "P", return_type => "short",
           ret_assign => 'Parrot_pcc_fill_returns_from_c_args(interp, call_object, "I", return_data);' },
    3 => { as_proto => "int *",  sig_char => "P", return_type => "int",
           ret_assign => 'Parrot_pcc_fill_returns_from_c_args(interp, call_object, "I", return_data);' },
    4 => { as_proto => "long *",  sig_char => "P", return_type => "long",
           ret_assign => 'Parrot_pcc_fill_returns_from_c_args(interp, call_object, "I", return_data);' },
    L => { as_proto => "long *", as_return => "" },
    T => { as_proto => "char **", as_return => "" },
    V => { as_proto => "void **", as_return => "", sig_char => "P" },
    '@' => { as_proto => "PMC *", as_return => "", cname => "xAT_", sig_char => 'Ps' },
);

for (values %signature_table) {
    if (not exists $_->{as_return}) { $_->{as_return} = $_->{as_proto} }
    if (not exists $_->{return_type}) { $_->{return_type} = $_->{as_proto} }
    if (not exists $_->{return_type_decl}) { $_->{return_type_decl} = $_->{return_type} }
    if (not exists $_->{ret_assign} and exists $_->{sig_char}) {
        $_->{ret_assign} = 'Parrot_pcc_fill_returns_from_c_args(interp, call_object, "'
                           . $_->{sig_char} . '", return_data);';
    }
    if (not exists $_->{func_call_assign}) {
        $_->{func_call_assign} = "return_data = "
    }
}

=back

=head1 FUNCTIONS

=over

=item C<signature_nci_to_pcc>

Converts an NCI signature to a PCC signature.

=cut

sub signature_nci_to_pcc {
    my $nci_sig = shift;
    my ($nci_ret, $nci_params) = $nci_sig =~ /^(.)\s*(\S*)/;
    my $pcc_ret =  $signature_table{$nci_ret}{sig_char};
    my $pcc_params = join '', map $signature_table{$_}{sig_char}, split //, $nci_params;
    return "${pcc_params}->${pcc_ret}";
}

1;

=back

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
