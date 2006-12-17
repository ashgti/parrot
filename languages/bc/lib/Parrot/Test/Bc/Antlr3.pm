# $Id$

=head1 NAME

Parrot::Test::Bc::Antlr3 - Testing ANTLR3 implementation of Parrot Bc.

=cut

package Parrot::Test::Bc::Antlr3;

# pragmata
use strict;
use warnings;

use base 'Parrot::Test::Bc';

sub get_out_fn {
    my $self = shift;
    my ( $count, $options ) = @_;

    return Parrot::Test::per_test( '_antlr3.out', $count );
}

sub get_test_prog {
    my $self = shift;
    my ( $count, $options ) = @_;

    my $lang_fn = Parrot::Test::per_test( '.bc',         $count );
    my $pir_fn  = Parrot::Test::per_test( '_antlr3.pir', $count );

    return ( "java Bc languages/${lang_fn} languages/${pir_fn}",
        "$self->{parrot} languages/${pir_fn}" );
}

sub skip_why {
    my $self = shift;
    my ($options) = @_;

    if ( !exists $options->{with_antlr3} || $options->{with_antlr3} ) {
        return;
    }
    else {
        return 'Not implemented with ANTLR3';
    }
}

1;
