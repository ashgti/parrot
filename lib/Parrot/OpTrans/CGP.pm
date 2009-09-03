# Copyright (C) 2002, Parrot Foundation.
# $Id$

=head1 NAME

Parrot::OpTrans::CGP - C Goto Predereferenced Transform

=head1 DESCRIPTION

C<Parrot::OpTrans::CGP> inherits from C<Parrot::OpTrans::CPrederef> and
C<Parrot::OpTrans::CGoto> to provide predereferenced register addressing
and C C<goto> run loop.

=head2 Instance Methods

=over 4

=cut

package Parrot::OpTrans::CGP;

use strict;
use warnings;

use base qw( Parrot::OpTrans::CPrederef Parrot::OpTrans::CGoto );

=item C<core_type()>

Returns C<PARROT_CGP_CORE>.

=cut

sub core_type {
    return 'PARROT_CGP_CORE';
}

=item C<suffix()>

The suffix is C<'_cgp'>.

=cut

sub suffix {
    return "_cgp";
}

=item C<core_prefix()>

The core prefix is C<'cgp_'>.

=cut

sub core_prefix {
    return "cgp_";
}

=item C<defines()>

Returns the C C<#define> macros required by the ops.

=cut

sub defines {
    my ( $self, $pred_def );
    $self     = shift;
    $pred_def = $self->SUPER::defines();
    my $type = __PACKAGE__;
    return $pred_def . <<END;
/* defines - $0 -> $type */
#  define opcode_to_prederef(i, op)   \\
     (opcode_t *) (op   - Parrot_pcc_get_pred_offset(interp, i->ctx))
END
}

=item C<goto_address($address)>

Transforms the C<goto ADDRESS($address)> macro in an ops file into the
relevant C code.

=cut

sub goto_address {
    my ( $self, $addr ) = @_;

    #print STDERR "pbcc: map_ret_abs($addr)\n";

    if ( $addr eq '0' ) {
        return "return (0);";
    }
    else {
        return "if ($addr == 0)
          return 0;
   Parrot_cx_handle_tasks(interp, interp->scheduler);
   _reg_base = (char*)Parrot_pcc_get_regs_ni(interp, CURRENT_CONTEXT(interp))->regs_i;
   goto **(void **)(cur_opcode = opcode_to_prederef(interp, $addr))";
    }
}

=item C<goto_offset($offset)>

Transforms the C<goto OFFSET($offset)> macro in an ops file into the
relevant C code.

=cut

sub goto_offset {
    my ( $self, $offset ) = @_;

    # this must be a single expression, in case it's in a single-statement if
    return "do {\nParrot_pcc_set_pc(interp, CURRENT_CONTEXT(interp), CUR_OPCODE + $offset);\n"
    .      "goto **(void **)(cur_opcode += $offset);\n} while (1)";
}

sub run_core_func_start {
    my $type = __PACKAGE__;
    return <<END_C;
/* run_core_func_start - $0 -> $type */
    /* at least gcc 2.95.2 miscompiles set_args - %edi
     * is used for the vtable call and _reg_base is clobbered
     * # if 1191 := PARROT_OP_set_args_pc
     * (gdb) disas l_ops_addr[1191] l_ops_addr[1192]
     */
#if defined(__GNUC__) && defined(I386) && defined(PARROT_CGP_REGS)
    register opcode_t *   cur_opcode __asm__ ("esi") = cur_op;
    register char *   _reg_base   __asm__ ("edi");
#else
    opcode_t *cur_opcode = cur_op;
    char * _reg_base;
#endif

    static void *const l_ops_addr[] = {
END_C
}

=back

=head1 SEE ALSO

=over 4

=item C<Parrot::OpTrans>

=item C<Parrot::OpTrans::C>

=item C<Parrot::OpTrans::CGoto>

=item C<Parrot::OpTrans::CPrederef>

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
