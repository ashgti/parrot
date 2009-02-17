# Copyright (C) 2004-2008, Parrot Foundation.
# $Id$

package Parrot::Pmc2c::MULTI;
#use base 'Parrot::Pmc2c::PCCMETHOD';
use strict;
use warnings;
use Carp qw(longmess croak);

=head1 NAME

Parrot::Pmc2c::MULTI - Parses and preps MULTI dispatch subs

=head1 SYNOPSIS

    use Parrot::Pmc2c::MULTI;

=head1 DESCRIPTION

Parrot::Pmc2c::MULTI - Parses and preps MULTI multiple dispatch declarations
called from F<Parrot:Pmc2c::Pmc2cMain>

=cut

=head1 FUNCTIONS

=head2 Publicly Available Methods

=head3 C<rewrite_multi_sub($method, $pmc)>

B<Purpose:>  Parse and Build PMC multiple dispatch subs.

B<Arguments:>

=over 4

=item * C<self>

=item * C<method>

Current Method Object

=item * C<body>

Current Method Body

=back

=cut

sub rewrite_multi_sub {
    my ( $self, $pmc ) = @_;
    my @param_types = ();
    my @new_params = ();
    my $short_sig = "JP"; # prepend the short signature interpreter and invocant

    # Fixup the parameters, standardizing PMC types and extracting type names
    # for the multi name.
    for my $param ( split /,/, $self->parameters ) {
        my ( $type, $name, $rest ) = split /\s+/, &Parrot::Pmc2c::PCCMETHOD::trim($param), 3;
        my $sig_char;

        die "Invalid MULTI parameter '$param': missing type or name\n"
             unless defined $name;
        die "Invalid MULTI parameter '$param': attributes not allowed on multis\n"
             if defined $rest;

        # Clean any '*' out of the name or type.
        if ($name =~ /[\**]?(\"?\w+\"?)/) {
            $name = $1;
        }
        $type =~ s/\*+//;

        # Capture the actual type for the sub name
        push @param_types, $type;

        # Pass standard parameter types unmodified.
        # All other param types are rewritten as PMCs.
        if ($type eq 'STRING' or $type eq 'PMC' or $type eq 'INTVAL') {
            $sig_char = substr($type, 0, 1); # short signature takes first character of name
            push @new_params, $param;
        }
        elsif ($type eq 'FLOATVAL') {
            $sig_char = 'N';
            push @new_params, $param;
        }
        else {
            $sig_char = 'P';
            push @new_params, "PMC *$name";
        }

        $short_sig .= $sig_char;
    }

    # prepend the short signature return type
    if ($self->name =~ /^i_/) {
        $short_sig = "v" . $short_sig;
    }
    else {
        $short_sig = $self->trans( $self->return_type() ) . $short_sig;
    }

    $self->parameters(join (",", @new_params));

    my $sub_name = "multi_" . $self->name . "_" . join ('_', @param_types);
    $self->name($sub_name);

    $self->{MULTI_short_sig} = $short_sig;
    $self->{MULTI_full_sig}  = join(',', @param_types);
    $self->{MULTI} = 1;

    return 1;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
