# Copyright (C) 2006-2007, The Perl Foundation.
# $Id$

=head1 NAME

config/auto/socklen_t.pm - Is there a socklen_t

=head1 DESCRIPTION

Determines whether there is a socklen_t

=cut

package auto::socklen_t;

use strict;
use warnings;


use base qw(Parrot::Configure::Step::Base);

use Config;

use Parrot::Configure::Step ':auto';

sub _init {
    my $self = shift;
    my %data;
    $data{description} = q{Determining whether there is socklen_t};
    $data{args}        = [ qw(  ) ];
    $data{result}      = q{};
    return \%data;
}

sub runstep {
    my ( $self, $conf ) = @_;

    my $verbose = $conf->options->get('verbose');

    my $d_socklen_t = $conf->data->get('has_socklen_t');
    $d_socklen_t = $Config{d_socklen_t} unless defined $d_socklen_t;
    my $has_socklen_t = ( $d_socklen_t && $d_socklen_t ne 'undef' ) ? 1 : 0;
    $self->set_result( $has_socklen_t ? 'yes' : 'no' );
    $conf->data->set( has_socklen_t => $has_socklen_t, );

    return 1;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
