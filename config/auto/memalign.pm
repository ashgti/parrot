# Copyright (C) 2001-2003, The Perl Foundation.
# $Id$

=head1 NAME

config/auto/memalign.pm - Memory Alignment

=head1 DESCRIPTION

Determines if the C library supports C<memalign()>.

=cut

package auto::memalign;

use strict;
use warnings;

use base qw(Parrot::Configure::Step::Base);

use Parrot::Configure::Step ':auto';


sub _init {
    my $self = shift;
    my %data;
    $data{description} = q{Determining if your C library supports memalign};
    $data{args}        = [ qw( miniparrot verbose ) ];
    $data{result}      = q{};
    return \%data;
}

sub runstep {
    my ( $self, $conf ) = @_;

    my $verbose = $conf->options->get('verbose');

    if ( $conf->options->get('miniparrot') ) {
        $conf->data->set( memalign => '' );
        $self->set_result('skipped');
        return 1;
    }

    if ( defined $conf->data->get('memalign') ) {

        # already set; leave it alone
        $self->set_result('already set');
        return 1;
    }
    my $test = 0;

    if ( $conf->data->get('i_malloc') ) {
        $conf->data->set( malloc_header => 'malloc.h' );
    }
    else {
        $conf->data->set( malloc_header => 'stdlib.h' );
    }

    if ( $conf->data->get('ptrsize') == $conf->data->get('intsize') ) {
        $conf->data->set( ptrcast => 'int' );
    }
    else {
        $conf->data->set( ptrcast => 'long' );
    }

    cc_gen('config/auto/memalign/test_c.in');
    eval { cc_build(); };
    unless ( $@ || cc_run_capture() !~ /ok/ ) {
        $test = 1;
    }
    cc_clean();

    my $test2 = 0;

    cc_gen('config/auto/memalign/test_c2.in');
    eval { cc_build(); };
    unless ( $@ || cc_run_capture() !~ /ok/ ) {
        $test2 = 1;
    }
    cc_clean();

    $conf->data->set( malloc_header => undef );

    my $f =
          $test2 ? 'posix_memalign'
        : $test  ? 'memalign'
        :          '';
    $conf->data->set( memalign => $f );
    print( $test ? " (Yep:$f) " : " (no) " ) if $verbose;
    $self->set_result( $test ? 'yes' : 'no' );

    return 1;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
