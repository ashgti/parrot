# Copyright (C) 2001-2009, Parrot Foundation.
# $Id$

=head1 NAME

Parrot::OpTrans::CSwitch - C Switch Transform

=head1 DESCRIPTION

C<Parrot::OpTrans::CSwitch> inherits from C<Parrot::OpTrans::CPrederef>
to provide a mixture of predereferenced register addressing and a
C<switch>ed run loop.

=head2 Instance Methods

=over 4

=cut

package Parrot::OpTrans::CSwitch;

use strict;
use warnings;

use Parrot::OpTrans;
use base qw( Parrot::OpTrans::CPrederef );

sub new {
    my $class = shift;
    my $self  = $class->SUPER::new(@_);
    $self->{split_count} ||= 0;
    return $self;
}

=item C<core_type()>

The core type is C<PARROT_SWITCH_CORE>.

=cut

sub core_type {
    return 'PARROT_SWITCH_CORE';
}

=item C<core_prefix()>

The prefix is C<'switch_'>.

=cut

sub core_prefix {
    return "switch_";
}

=item C<suffix()>

The suffix is C<'_switch'>.

=cut

sub suffix {
    return "_switch";
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
#  define opcode_to_prederef(i, op)   (op ? \\
     (opcode_t*) (op   - Parrot_pcc_get_pred_offset(interp, i->ctx)) : (opcode_t*)NULL)
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
        return <<EOC;
            {
               cur_opcode = opcode_to_prederef(interp, $addr);
               goto SWITCH_RELOAD;
            }
EOC
    }
}

=item C<goto_offset($offset)>

Transforms the C<goto OFFSET($offset)> macro in an ops file into the
relevant C code.

=cut

sub goto_offset {
    my ( $self, $offset ) = @_;
    return "{ cur_opcode += $offset; goto SWITCH_AGAIN; }";
}

=item C<init_func_init1($base)>

Returns the C code for the init function.

=cut

sub init_func_init1 {
    my ( $self, $base ) = @_;
    my $cg_func = $self->core_prefix . $base;
    my $bs      = $base . $self->suffix . '_';

    return <<END_C;
        if (!${bs}op_lib.op_func_table)
            ${bs}op_lib.op_func_table = (op_func_t *)&${bs}op_lib;
END_C
}


=item C<run_core_func_start()>

Returns the C code prior to the run core function.

=cut

sub run_core_func_start {
    my $type = __PACKAGE__;
    return <<END_C;
/* run_core_func_start - $0 -> $type */
#if defined(__GNUC__) && defined(I386) && defined(PARROT_SWITCH_REGS)
    register opcode_t *   cur_opcode __asm__ ("esi") = cur_op;
    register char *   _reg_base   __asm__ ("edi");
#else
    opcode_t * cur_opcode = cur_op;
    char * _reg_base;
#endif

SWITCH_RELOAD:
    _reg_base = (char*)Parrot_pcc_get_regs_ni(interp, CURRENT_CONTEXT(interp))->regs_i;
    do {
SWITCH_AGAIN:
    Parrot_cx_handle_tasks(interp, interp->scheduler);
    if (!cur_opcode)
        break;
    switch (*(opcode_t*)cur_opcode) {
END_C
}

=item C<run_core_split($base)>

If defined return code to split e.g. a switch.

=cut

sub run_core_split {
    my ($self) = @_;
    $self->{split_count}++;

    return <<END_C;
    default:
    switch (*(opcode_t*)cur_opcode) {
END_C
}

=item C<run_core_finish($base)>

Returns the C code following the run core function.

=cut

sub run_core_finish {
    my ( $self, $base ) = @_;
    my $bs = $base . $self->suffix . '_';
    my $c  = <<END_C;
        default:
            if (*(opcode_t*)cur_opcode >= 0 &&
                *(opcode_t*)cur_opcode < (opcode_t)${bs}op_lib.op_count) {
                *(opcode_t*)cur_opcode = CORE_OPS_wrapper__;
                continue;
            }
            Parrot_ex_throw_from_c_args(interp, NULL, 1,
                "illegal opcode in switch core\\n");
            break;
        } /* switch */
END_C
    for ( my $i = 0 ; $i < $self->{split_count} ; $i++ ) {
        $c .= <<END_C;
    } /* switch $i */
END_C
    }
    $c .= <<END_C;
    } while (1);
    return NULL;
}
END_C

    $c .= " /* " . $self->core_prefix . "$base */\n\n";

    return $c;
}

=back

=head1 SEE ALSO

=over 4

=item C<Parrot::OpTrans>

=item C<Parrot::OpTrans::C>

=item C<Parrot::OpTrans::CGP>

=item C<Parrot::OpTrans::CGoto>

=item C<Parrot::OpTrans::CPrederef>

=back

=cut

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
