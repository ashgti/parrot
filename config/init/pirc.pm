# Copyright (C) 2009, Parrot Foundation.
# $Id: $

=head1 NAME

config/init/pirc.pm - select between IMCC and PIRC for PIR compilation

=head1 DESCRIPTION

Determines whether the default of 'imcc' should be overridden. Temporary
step to get us using pirc.

=cut

package init::pirc;

use strict;
use warnings;

use base qw(Parrot::Configure::Step);

use Parrot::Configure::Step;
#use Parrot::Configure::Utils ':gen';

sub _init {
    my $self = shift;
    my %data;
    $data{description} = q{Select PIR compiler};
    $data{result}      = q{};
    return \%data;
}

sub runstep {
    my ( $self, $conf ) = @_;


    my $opt = $conf->options->get('pirc');
    if (defined($opt) )  {
        $conf->data->set('pirc', $opt);
    }

    my $pirc = $conf->data->get('pirc');
    if ($pirc ne 'imcc' && $pirc ne 'pirc') {
        warn "\ninvalid --pirc='$pirc': must be imcc or pirc\n";
        exit 1;
    }

    if ($pirc eq 'pirc') {
        $conf->data->add(' ', 'ccflags', '-DPARROT_PIRC_COMPILER=pirc');
    }
 
    $self->set_result($pirc);
    return 1;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
