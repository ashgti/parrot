# Copyright (C) 2001-2003, The Perl Foundation.
# $Id$

=head1 NAME

config/auto/aio.pm - Test for AIO

=head1 DESCRIPTION

Determines whether the platform supports AIO.

=cut

package auto::aio;

use strict;
use warnings;

use base qw(Parrot::Configure::Step::Base);

use Parrot::Configure::Step ':auto';


sub _init {
    my $self = shift;
    my %data;
    $data{description} = q{Determining if your platform supports AIO};
    $data{args}        = [ qw( verbose ) ];
    $data{result}      = q{};
    return \%data;
}

sub runstep {
    my ( $self, $conf ) = ( shift, shift );

    my $test;
    my $verbose = $conf->options->get('verbose');
    my $libs    = $conf->data->get('libs');
    $conf->data->add( ' ', libs => '-lrt' );

    cc_gen('config/auto/aio/aio.in');
    eval { cc_build(); };
    if ( !$@ ) {
        $test = cc_run(35);

        # if the test is failing with sigaction err
        # we should repeat it with a different signal number
        if (
            $test =~ /SIGRTMIN=(\d+)\sSIGRTMAX=(\d+)\n
                INFO=42\n
                ok/x
            )
        {
            print " (yes) " if $verbose;
            $self->set_result('yes');

            $conf->data->set(
                aio        => 'define',
                HAS_AIO    => 1,
                D_SIGRTMIN => $1,
                D_SIGRTMAX => $2,
            );
        }

    }
    else {
        $conf->data->set( libs => $libs );
        print " (no) " if $verbose;
        $self->set_result('no');
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
