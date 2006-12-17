# Copyright (C) 2001-2003, The Perl Foundation.
# $Id$

=head1 NAME

config/auto/byteorder.pm - Native Byteorder

=head1 DESCRIPTION

Computes the native byteorder for Parrot's wordsize.

=cut

package auto::byteorder;

use strict;
use warnings;
use vars qw($description @args);

use Parrot::Configure::Step qw(:auto);
use base qw(Parrot::Configure::Step::Base);

$description = q{Computing native byteorder for Parrot's wordsize};

@args = ();

sub runstep {
    my ( $self, $conf ) = @_;

    cc_gen('config/auto/byteorder/test_c.in');
    cc_build();
    my $byteorder = cc_run() or die "Can't run the byteorder testing program: $!";
    cc_clean();

    chomp $byteorder;

    if ( $byteorder =~ /^1234/ ) {
        $conf->data->set(
            byteorder => $byteorder,
            bigendian => 0
        );
        $self->set_result('little-endian');
    }
    elsif ( $byteorder =~ /^(8765|4321)/ ) {
        $conf->data->set(
            byteorder => $byteorder,
            bigendian => 1
        );
        $self->set_result('big-endian');
    }
    else {
        die "Unsupported byte-order [$byteorder]!";
    }

    return $self;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
