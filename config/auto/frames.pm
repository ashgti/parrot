# Copyright (C) 2009, Parrot Foundation.
# $Id$

=head1 NAME

config/auto/frames

=head1 DESCRIPTION

Determines whether the current platform is capable of building NCI call
frames dynamically.  Use the C<--buildframes> option to override the
default value for your CPU architecture and operating system.

=cut

package auto::frames;

use strict;
use warnings;

use base qw(Parrot::Configure::Step);

sub _init {
    my $self = shift;
    my %data;
    $data{description} = q{Determine call frame building capability};
    $data{result}      = q{};
    return \%data;
}

sub runstep {
    my ( $self, $conf ) = @_;

    my $can_build_call_frames = _call_frames_buildable($conf);

    $self->_handle_can_build_call_frames( $conf, $can_build_call_frames );

    return 1;
}

sub _call_frames_buildable {
    my $conf = shift;
    my $can_build_call_frames;

    if (defined $conf->options->get('buildframes')) {
        $can_build_call_frames = $conf->options->get('buildframes');
    }
    else {
        $can_build_call_frames = $conf->data->get('HAS_LIBJIT');
    }
    return $can_build_call_frames;
}

sub _handle_can_build_call_frames {
    my ($self, $conf, $can_build_call_frames) = @_;
    if ( $can_build_call_frames ) {
        $conf->data->set(
            cc_build_call_frames => '-DCAN_BUILD_CALL_FRAMES',
            has_exec_protect     => 1,
        );
        $self->set_result( 'yes' );
    }
    else {
        $conf->data->set(cc_build_call_frames  => '');
        $self->set_result( 'no' );
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
