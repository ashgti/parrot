# Copyright (C) 2009, Parrot Foundation.
# $Id$

=head1 NAME

config/auto/frmes - 

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

    my $can_build_call_frames = 0;

    my $osname  = $conf->data->get('osname');
    my $cpuarch = $conf->data->get('cpuarch');
    my $nvsize  = $conf->data->get('nvsize');

    if (defined $conf->options->get('buildframes') || ($nvsize == 8
        && $cpuarch eq 'i386' && $osname ne 'darwin')) {
        $can_build_call_frames = 1;
    }

    if ( $can_build_call_frames ) {
        $conf->data->set(
            can_build_call_frames => 1,
            cc_build_call_frames  => '-DCAN_BUILD_CALL_FRAMES',
        );
    }
    else {
        $conf->data->set(
            can_build_call_frames => 0,
            cc_build_call_frames  => '',
        );
    }


    $self->set_result($can_build_call_frames?'yes':'no');
    return 1;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
