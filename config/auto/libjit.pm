# Copyright (C) 2009, Parrot Foundation.
# $Id$

=head1 NAME

config/auto/libjit - Check whether LibJIT is installed

=head1 DESCRIPTION

Determines whether LibJIT is present is installed and
functional on the system. It is OK when it doesn't exist.

=cut

package auto::libjit;

use strict;
use warnings;

use base 'Parrot::Configure::Step';

use Parrot::Configure::Utils ':auto';

sub _init {
    my $self = shift;
    my %data = (
	description => 'Is LibJIT installed',
	result	    => '',
    );
    return \%data;
}

sub runstep {
    my ($self, $conf) = @_;

    my ($verbose, $without) = $conf->options->get( qw{
                                    verbose
                                    without-libjit
    });

    if ($without) {
        $conf->data->set( HAS_LIBJIT => 0 );
        $self->set_result('no');
        return 1;
    }

    my $extra_libs = $self->_select_lib( {
        conf         => $conf,
        osname       => $conf->data->get_p5('OSNAME'),
        cc           => $conf->data->get('cc'),
        win32_nongcc => 'libjit.lib',
        default      => '-ljit',
    } );
    my $has_libjit = 0;

    $conf->cc_gen('config/auto/libjit/libjit_c.in');
    eval { $conf->cc_build('', $extra_libs) };
    if ($@) {
        print "cc_build() failed: $@\n" if $verbose;
        $conf->data->set( HAS_LIBJIT => 0 );
        $self->set_result('no');
        return 1;
    }
    else {
        my $test;
        eval { $test = $conf->cc_run(); };
        if ($@) {
            print "cc_run() failed: $@\n" if $verbose;
            $conf->data->set( HAS_LIBJIT => 0 );
            $self->set_result('no');
            return 1;
        }
        else {
            $has_libjit = $self->_evaluate_cc_run($test, $has_libjit, $verbose);
        }
    }

    $conf->data->set( HAS_LIBJIT => $has_libjit );
    $self->set_result( $has_libjit ? 'yes' : 'no' );

    if ($has_libjit) {
        $conf->data->set(
            cc_build_call_frames => '-DCAN_BUILD_CALL_FRAMES',
            has_exec_protect => 1,
        );
        $conf->data->add( ' ', libs => $extra_libs );
    }

    return 1;
}

sub _evaluate_cc_run {
    my $self = shift;
    my ($test, $has_libjit, $verbose) = @_;
    if ($test =~ m/^USES INTERPRETER: \d+/ ) {
        $has_libjit = 1;
        print " (yes) " if $verbose;
        $self->set_result("yes");
    }
    return $has_libjit;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
