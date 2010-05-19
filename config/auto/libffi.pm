# Copyright (C) 2005-2010, Parrot Foundation.
# $Id$

=head1 NAME

config/auto/libffi - Check whether libffi

=head1 DESCRIPTION

=cut

package auto::libffi;

use strict;
use warnings;

use base qw(Parrot::Configure::Step);

use Parrot::Configure::Utils ':auto';

sub _init {
    my $self = shift;
    my %data;
    $data{description} = q{Is libffi installed};
    $data{result}      = q{};
    return \%data;
}

my @pkgconfig_variations =
    defined( $ENV{TEST_PKGCONFIG} )
    ? @{ $ENV{TEST_PKGCONFIG} }
    : qw( pkg-config );

sub runstep {
    my ( $self, $conf ) = @_;

    my ( $verbose, $without ) = $conf->options->get(
        qw|
            verbose
            without-libffi
        |
    );

    if ($without) {
        $conf->data->set( HAS_LIBFFI => 0 );
        $self->set_result('no');
        return 1;
    }

    my $osname = $conf->data->get('osname');
    print "\n" if $verbose;
    my $pkgconfig_exec = check_progs([ @pkgconfig_variations ], $verbose);

    my $libffi_options = { ccflags => '', libs => '-lffi' };
    if ($pkgconfig_exec) {
        my $out = capture_output($pkgconfig_exec, 'libffi --libs');
        chomp $out;
        $libffi_options->{'libs'} = $out;
        $out = capture_output($pkgconfig_exec, 'libffi --cflags');
        chomp $out;
        $libffi_options->{'ccflags'} = $out;
    } 

    my $extra_libs = $self->_select_lib( {
        conf            => $conf,
        osname          => $osname,
        cc              => $conf->data->get('cc'),
        default         => join(' ', values %$libffi_options),
    } );
    
    $conf->cc_gen('config/auto/libffi/test_c.in');
    eval { $conf->cc_build( q{}, $extra_libs ) };
    my $has_libffi = 0;
    if ( !$@ ) {
        my $test = $conf->cc_run();
        $has_libffi = _evaluate_cc_run($test, $verbose);
    }
    $conf->cc_clean();

    if ($has_libffi) {
        $conf->data->set( HAS_LIBFFI => $has_libffi);
        $conf->data->add( ' ', ccflags => $libffi_options->{'ccflags'} );
        $conf->data->add( ' ', libs => $libffi_options->{'libs'} );
        if ($verbose) {
            print 'libffi cflags: ', $libffi_options->{'ccflags'}, "libffi libs: ", $libffi_options->{'libs'}, "\n";
        }
    }
    else {
        $conf->data->set( HAS_LIBFFI => 0 );
        $self->set_result('no');
        print "No libffi found." if ($verbose);
    }

    return 1;
}

sub _evaluate_cc_run {
    my ($output, $verbose) = @_;
    my $has_libffi = ( $output =~ m/libffi => \d\.\d(?:\.\d)?/ ) ? 1 : 0;
    return $has_libffi;
}

1;

=head1 AUTHOR

Paul Cochrane <paultcochrane at gmail dot com>

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
