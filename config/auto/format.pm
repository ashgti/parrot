# Copyright (C) 2001-2003, The Perl Foundation.
# $Id$

=head1 NAME

config/auto/format.pm - Sprintf Formats

=head1 DESCRIPTION

Figures out what formats should be used for C<sprintf()>.

=cut

package auto::format;

use strict;
use warnings;
use vars qw($description @args);

use base qw(Parrot::Configure::Step::Base);

use Parrot::Configure::Step;

$description = 'Figuring out what formats should be used for sprintf';

@args = ();

sub runstep {
    my ( $self, $conf ) = @_;

    my ( $ivformat, $nvformat, $nvsize );
    my ( $iv, $nv, $floatsize, $doublesize, $ldsize ) =
        $conf->data->get(qw(iv nv floatsize doublesize hugefloatvalsize));

    if ( $iv eq "int" ) {
        $ivformat = "%d";
    }
    elsif ( ( $iv eq "long" ) || ( $iv eq "long int" ) ) {
        $ivformat = "%ld";
    }
    elsif ( ( $iv eq "long long" ) || ( $iv eq "long long int" ) ) {
        $ivformat = "%lld";
    }
    else {
        die "Configure.pl:  Can't find a printf-style format specifier for type \"$iv\"\n";
    }

    $nvsize = $floatsize;
    if ( $nv eq "double" ) {
        $nvsize   = $doublesize;
        $nvformat = "%f";
    }
    elsif ( $nv eq "long double" ) {

        # Stay way from long double for now (it may be 64 or 80 bits)
        # die "long double not supported at this time, use double.";
        $nvsize   = $ldsize;
        $nvformat = "%Lf";
    }
    else {
        die "Configure.pl:  Can't find a printf-style format specifier for type \"$nv\"\n";
    }

    $conf->data->set(
        intvalfmt   => $ivformat,
        floatvalfmt => $nvformat,
        nvsize      => $nvsize
    );

    return $self;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
