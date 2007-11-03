# Copyright (C) 2001-2005, The Perl Foundation.
# $Id$

=head1 NAME

config/auto/alignptrs.pm - pointer alignment

=head1 DESCRIPTION

Determine the minimum pointer alignment.

=cut

package auto::alignptrs;

use strict;
use warnings;

use base qw(Parrot::Configure::Step::Base);

use Parrot::Configure::Step ':auto';
use Config;


sub _init {
    my $self = shift;
    my %data;
    $data{description} = q{Determining your minimum pointer alignment};
    $data{args}        = [ qw( miniparrot ) ];
    $data{result}      = q{};
    return \%data;
}

sub runstep {
    my ( $self, $conf ) = ( shift, shift );

    if ( $conf->options->get('miniparrot') ) {
        $self->set_result('skipped');
        return 1;
    }

    $self->set_result('');
    my $align;
    if ( defined( $conf->data->get('ptr_alignment') ) ) {
        $align = $conf->data->get('ptr_alignment');
        $self->set_result("configured: ");
    }
    elsif ( $^O eq 'hpux' && $Config{ccflags} !~ /DD64/ ) {

        # HP-UX 10.20/32 hangs in this test.
        $align = 4;
        $conf->data->set( ptr_alignment => $align );
        $self->set_result("for hpux: ");
    }
    else {

        # Now really test by compiling some code
        cc_gen('config/auto/alignptrs/test_c.in');
        cc_build();
        for my $try_align ( 64, 32, 16, 8, 4, 2, 1 ) {
            my $results = cc_run_capture($try_align);
            if ( $results =~ /OK/ && $results !~ /align/i ) {
                $align = $try_align;
            }
        }
        cc_clean();

        die "Can't determine alignment!\n" unless defined $align;
        $conf->data->set( ptr_alignment => $align );
    }

    $self->set_result( $self->result . " $align byte" );
    $self->set_result( $self->result . 's' ) unless $align == 1;

    return 1;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
