# Copyright (C) 2001-2003, The Perl Foundation.
# $Id$

=head1 NAME

config/auto/pack.pm - Packing

=head1 DESCRIPTION

Figures out how to C<pack()> Parrot's types.

=cut

package auto::pack;

use strict;
use warnings;
use vars qw($description @args);

use base qw(Parrot::Configure::Step::Base);

use Parrot::Configure::Step;
use Config;

$description = q{Figuring out how to pack() Parrot's types};

@args = ();

sub runstep {
    my ( $self, $conf ) = @_;

    #
    # Alas perl5.7.2 doesn't have an INTVAL flag for pack().
    # The ! modifier only works for perl 5.6.x or greater.
    #

    my $intsize  = $conf->data->get('intsize');
    my $longsize = $conf->data->get('longsize');
    my $ptrsize  = $conf->data->get('ptrsize');

    foreach ( 'intvalsize', 'opcode_t_size' ) {
        my $which = $_ eq 'intvalsize' ? 'packtype_i' : 'packtype_op';
        my $size = $conf->data->get($_);
        my $format;
        if ( ( $] >= 5.006 ) && ( $size == $longsize ) && ( $size == $Config{longsize} ) ) {
            $format = 'l!';
        }
        elsif ( $size == 4 ) {
            $format = 'l';
        }
        elsif ( $size == 8 || $Config{use64bitint} eq 'define' ) {

            # pp_pack is annoying, and this won't work unless sizeof(UV) >= 8
            $format = 'q';
        }
        warn "Configure.pl:  Unable to find a suitable packtype for $_.\n"
            unless $format;

        my $test = eval { pack $format, 0 };
        unless ( defined $test ) {
            warn <<"AARGH"
Configure.pl:  Unable to find a functional packtype for $_.
               '$format' failed: $@
AARGH
        }
        if ($test) {
            unless ( length $test == $size ) {
                warn sprintf <<"AARGH", $size, length $test;
Configure.pl:  Unable to find a functional packtype for $_.
               Need a format for %d bytes, but '$format' gave %d bytes.
AARGH
            }
        }
        else {
            $format = '?';
        }

        $conf->data->set( $which => $format );
    }

    $conf->data->set(
        packtype_b => 'C',
        packtype_n => ( $conf->data->get('numvalsize') == 12 ? 'D' : 'd' )
    );

    #
    # Find out what integer constant type we can use
    # for pointers.
    #

    if ( $intsize == $ptrsize ) {
        $conf->data->set( ptrconst => "u" );
    }
    elsif ( $longsize == $ptrsize ) {
        $conf->data->set( ptrconst => "ul" );
    }
    else {
        warn <<"AARGH";
Configure.pl:  Unable to find an integer type that fits a pointer.
AARGH
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
