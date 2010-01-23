#! perl
# Copyright (C) 2009, Parrot Foundation.
# $Id$
# auto/libjit-01.t

use strict;
use warnings;

use Test::More tests => 34;
use lib qw( lib t/configure/testlib );
use Parrot::Configure;
use Parrot::Configure::Options 'process_options';
use Parrot::Configure::Test qw(
    test_step_thru_runstep
    rerun_defaults_for_testing
    test_step_constructor_and_description
);
use IO::CaptureOutput qw( capture );

use_ok('config::init::defaults');
use_ok('config::auto::libjit');

my ($args, $step_list_ref) = process_options( {
        argv => [ q{--without-libjit} ],
        mode => 'configure',
} );

my $conf = Parrot::Configure->new;

my $serialized = $conf->pcfreeze();

test_step_thru_runstep( $conf, 'init::defaults', $args );

my $pkg = 'auto::libjit';
my ( $step, $ret );

$conf->add_steps($pkg);
$conf->options->set(%$args);
$step = test_step_constructor_and_description($conf);
$ret = $step->runstep($conf);
ok( $ret, "runstep() returned true value" );
is( $step->result(), 'no', "Result is 'no', as expected" );
is( $conf->data->get( 'HAS_LIBJIT' ), 0,
   "Got expected result with --without-libjit option" );
$conf->cc_clean();

$conf->replenish($serialized);

($args, $step_list_ref) = process_options( {
    argv => [ ],
    mode => q{configure},
} );
rerun_defaults_for_testing($conf, $args );
$conf->add_steps($pkg);
$conf->options->set( %{$args} );
$step = test_step_constructor_and_description($conf);
# Next line avoids an uninitialized value warning.
$conf->data->set( 'osname' => 'mswin32' );
$ret = $step->runstep($conf);
ok( $ret, "runstep() returned true value" );
like( $step->result(), qr/yes|no/, "Result is either 'yes' or 'no'" );
ok( defined( $conf->data->get( 'HAS_LIBJIT' ) ),
   "'HAS_LIBJIT' has defined value" );
$conf->cc_clean();
$conf->data->set( 'osname' => undef );

########## _evaluate_cc_run ##########

my ($test, $has_libjit, $verbose);

$step->set_result( undef );

$test = q{USES INTERPRETER: 33};
$has_libjit = 0;
$verbose = 0;
$has_libjit = $step->_evaluate_cc_run($test, $has_libjit, $verbose);
ok( $has_libjit, "_evaluate_cc_run() returned true value, as expected" );
is( $step->result(), 'yes', "result is yes, as expected" );

$step->set_result( undef );

$test = q{foobar};
$has_libjit = 0;
$verbose = 0;
$has_libjit = $step->_evaluate_cc_run($test, $has_libjit, $verbose);
ok( ! $has_libjit, "_evaluate_cc_run() returned false value, as expected" );
ok( ! defined($step->result()), "result is undefined, as expected" );

$step->set_result( undef );

$test = q{USES INTERPRETER: 33};
$has_libjit = 0;
$verbose = 1;
{
    my ($stdout, $stderr);
    capture(
        sub { $has_libjit =
            $step->_evaluate_cc_run($test, $has_libjit, $verbose); },
        \$stdout,
        \$stderr,
    );
    ok( $has_libjit, "_evaluate_cc_run() returned true value, as expected" );
    is( $step->result(), 'yes', "result is yes, as expected" );
    like( $stdout, qr/\(yes\)/, "Got expected verbose output" );
}

########## _handle_has_libjit() ##########

my $extra_libs;

$conf->data->set( 'libjit_has_alloca' => undef );
$conf->data->set( 'libs' => '' );

$has_libjit = 1;
$extra_libs = 'mylibs';
$conf->data->set( 'cpuarch' => 'i386' );

auto::libjit::_handle_has_libjit($conf, $has_libjit, $extra_libs);
ok( $conf->data->get( 'libjit_has_alloca'),
    "on i386 with libJIT, 'libjit_has_alloca' has true value" );
is( $conf->data->get( 'libs' ), " $extra_libs",
    "Got expected value for libs" );

$conf->data->set( 'libjit_has_alloca' => undef );
$conf->data->set( 'libs' => '' );

$has_libjit = 1;
$extra_libs = 'mylibs';
$conf->data->set( 'cpuarch' => 'ppc' );

auto::libjit::_handle_has_libjit($conf, $has_libjit, $extra_libs);
ok( ! $conf->data->get( 'libjit_has_alloca'),
    "on non-i386 with libJIT, 'libjit_has_alloca' has false value" );
is( $conf->data->get( 'libs' ), " $extra_libs",
    "Got expected value for libs" );

$conf->data->set( 'libjit_has_alloca' => undef );
$conf->data->set( 'libs' => '' );

$has_libjit = 0;
$extra_libs = 'mylibs';

auto::libjit::_handle_has_libjit($conf, $has_libjit, $extra_libs);
ok( ! $conf->data->get( 'libjit_has_alloca'),
    "without libJIT, 'libjit_has_alloca' has false value" );
is( $conf->data->get( 'libs' ), "",
    "Got expected value for libs" );
$conf->cc_clean();

################### DOCUMENTATION ###################

=head1 NAME

auto/libjit-01.t - test auto::libjit

=head1 SYNOPSIS

    % prove t/steps/auto/libjit-01.t

=head1 DESCRIPTION

The files in this directory test functionality used by F<Configure.pl>.

The tests in this file test auto::libjit.

=head1 SEE ALSO

config::auto::libjit, F<Configure.pl>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
