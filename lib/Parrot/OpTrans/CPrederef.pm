#! perl
# Copyright (C) 2001-2007, Parrot Foundation.
# $Id$

=head1 NAME

Parrot::OpTrans::CPrederef - C Predereferenced Transform

=head1 DESCRIPTION

C<Parrot::OpTrans::CPrederef> inherits from C<Parrot::OpTrans::C>
to provide basic functionality for predereferenced run loops (switch,
CGP).

=head2 Instance Methods

=over 4

=cut

package Parrot::OpTrans::CPrederef;

use strict;
use warnings;

use Parrot::OpTrans;
use base qw( Parrot::OpTrans::C );

=item C<defines()>

Returns the C C<#define> macros required by the ops.

=cut

sub defines {
    my $type = __PACKAGE__;
    return <<END;
/* defines - $0 -> $type */
#define REL_PC ((size_t)(cur_opcode - (opcode_t*)interp->code->prederef.code))
#define CUR_OPCODE \\
    ((opcode_t*)cur_opcode + Parrot_pcc_get_pred_offset(interp, CURRENT_CONTEXT(interp)))
#define OP_AS_OFFS(o) (_reg_base + ((opcode_t*)cur_opcode)[o])

END
}

=item expr_address($addr)

=item expr_address($offset)

=item expr_pop()

Create various address parts.

=cut

sub expr_address {
    my ( $self, $addr ) = @_;
    return "opcode_to_prederef(interp, $addr)";
}

sub expr_offset {
    my ( $self, $offset ) = @_;
    return "CUR_OPCODE + $offset";
}

sub expr_pop {
    my ($self) = @_;
    return "opcode_to_prederef(interp, pop_dest(interp))";
}

sub run_core_func_decl {
    my ( $self, $core ) = @_;

    my $type   = __PACKAGE__;
    my $prefix = $self->core_prefix;
    return <<END;
/* run_core_func_decl - $0 -> $type */
opcode_t * $prefix$core(opcode_t *cur_op, PARROT_INTERP)
END
}

=item C<access_arg($type, $num, $op)>

Returns the C code for the specified op argument type (see
C<Parrot::OpTrans>) and value. C<$op> is an instance of C<Parrot::Op>.

=cut

sub access_arg {
    my ( $self, $type, $num, $op ) = @_;

    my %arg_maps = (
        'op' => "cur_opcode[%ld]",

        'i'  => "(*(INTVAL *)OP_AS_OFFS(%ld))",
        'ki' => "(*(INTVAL *)OP_AS_OFFS(%ld))",
        'n'  => "(*(FLOATVAL *)OP_AS_OFFS(%ld))",
        'p'  => "(*(PMC **)OP_AS_OFFS(%ld))",
        's'  => "(*(STRING **)OP_AS_OFFS(%ld))",
        'k'  => "(*(PMC **)OP_AS_OFFS(%ld))",

        'ic'  => "((INTVAL)cur_opcode[%ld])",
        'kic' => "((INTVAL)cur_opcode[%ld])",
        'nc'  => "(*(FLOATVAL *)cur_opcode[%ld])",
        'sc'  => "((STRING *)cur_opcode[%ld])",
        'pc'  => "((PMC *)cur_opcode[%ld])",
        'kc'  => "((PMC *)cur_opcode[%ld])",
    );

    die "Unrecognized type '$type' for num '$num' in opcode @{[$op->full_name]}"
        unless exists $arg_maps{$type};

    return sprintf( $arg_maps{$type}, $num );
}

=back

=head1 SEE ALSO

=over 4

=item C<Parrot::OpTrans>

=item C<Parrot::OpTrans::C>

=item C<Parrot::OpTrans::CGP>

=item C<Parrot::OpTrans::CGoto>

=item C<Parrot::OpTrans::CSwitch>

=back

=cut

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
