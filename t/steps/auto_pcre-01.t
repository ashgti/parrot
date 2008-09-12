#! perl
# Copyright (C) 2007-2008, The Perl Foundation.
# $Id$
# auto_pcre-01.t

use strict;
use warnings;
use Test::More tests => 44;
use Carp;
use lib qw( lib );
use_ok('config::init::defaults');
use_ok('config::auto::pcre');
use Parrot::Configure;
use Parrot::Configure::Options qw( process_options );
use Parrot::Configure::Test qw(
    test_step_thru_runstep
    rerun_defaults_for_testing
    test_step_constructor_and_description
);
use IO::CaptureOutput qw| capture |;

########## --without-pcre ##########

my ($args, $step_list_ref) = process_options(
    {
        argv => [ q{--without-pcre} ],
        mode => q{configure},
    }
);

my $conf = Parrot::Configure->new;

my $serialized = $conf->pcfreeze();

test_step_thru_runstep( $conf, q{init::defaults}, $args );

my $pkg = q{auto::pcre};

$conf->add_steps($pkg);
$conf->options->set( %{$args} );
my $step = test_step_constructor_and_description($conf);
ok( $step->runstep($conf), "runstep() returned true value");
is( $step->result(), 'no', "Got expected result" );
is( $conf->data->get( 'has_pcre' ), 0,
    "Got expected value for 'has_pre'");

$conf->replenish($serialized);

########## _add_to_libs() ##########

($args, $step_list_ref) = process_options( {
    argv => [ ],
    mode => q{configure},
} );
rerun_defaults_for_testing($conf, $args );
$conf->add_steps($pkg);
$conf->options->set( %{$args} );
$step = test_step_constructor_and_description($conf);

# Mock values for OS and C-compiler
my ($osname, $cc, $initial_value);

$osname = 'mswin32';
$cc = 'gcc';
$initial_value = $conf->data->get( 'libs' );
ok($step->_add_to_libs( {
    conf            => $conf,
    osname          => $osname,
    cc              => $cc,
    win32_nongcc    => 'pcre.lib',
    default         => '-lpcre',
} ),
    "_add_to_libs() returned true value");
like($conf->data->get( 'libs' ), qr/-lpcre/,
    "'libs' modified as expected");
# Restore value for next test.
$conf->data->set( 'libs' => $initial_value );

$osname = 'mswin32';
$cc = 'cc';
$initial_value = $conf->data->get( 'libs' );
ok($step->_add_to_libs( {
    conf            => $conf,
    osname          => $osname,
    cc              => $cc,
    win32_nongcc    => 'pcre.lib',
    default         => '-lpcre',
} ),
    "_add_to_libs() returned true value");
like($conf->data->get( 'libs' ), qr/pcre\.lib/,
    "'libs' modified as expected");
# Restore value for next test.
$conf->data->set( 'libs' => $initial_value );

$osname = 'foobar';
$cc = 'gcc';
$initial_value = $conf->data->get( 'libs' );
ok($step->_add_to_libs( {
    conf            => $conf,
    osname          => $osname,
    cc              => $cc,
    win32_nongcc    => 'pcre.lib',
    default         => '-lpcre',
} ),
    "_add_to_libs() returned true value");
like($conf->data->get( 'libs' ), qr/-lpcre/,
    "'libs' modified as expected");
# Restore value for next test.
$conf->data->set( 'libs' => $initial_value );

########## _evaluate_cc_run() ##########

# Mock different outcomes of _evaluate_cc_run
my ($test, $has_pcre);
my $verbose = $conf->options->get('verbose');

$test = q{pcre foobar};
ok(! $step->_evaluate_cc_run($test, $verbose),
    "Got expected setting for has_pcre");

$test = q{pcre 4.1};
ok($step->_evaluate_cc_run($test, $verbose),
    "_evaluate_cc_run returned true value as expected");
is($step->result(), q{yes, 4.1}, "Got expected PCRE version");

# Mock different outcomes of _recheck_settings()
my ($libs, $ccflags, $linkflags);

$libs = q{-lalpha};
$ccflags = q{-Ibeta};
$linkflags = q{-Lgamma};
$verbose = undef;
$step->_recheck_settings($conf, $libs, $ccflags, $linkflags, $verbose);
like($conf->data->get('libs'), qr/$libs/,
    "Got expected value for 'libs'");
like($conf->data->get('ccflags'), qr/$ccflags/,
    "Got expected value for 'ccflags'");
like($conf->data->get('linkflags'), qr/$linkflags/,
    "Got expected value for 'linkflags'");

$conf->replenish($serialized);

########## --verbose; _evaluate_cc_run() ##########

($args, $step_list_ref) = process_options( {
    argv => [ q{--verbose} ],
    mode => q{configure},
} );
rerun_defaults_for_testing($conf, $args );
$conf->add_steps($pkg);
$conf->options->set( %{$args} );
$step = test_step_constructor_and_description($conf);

# Mock different outcomes of _evaluate_cc_run
$verbose = $conf->options->get('verbose');

$test = q{pcre 4.0};
{
    my ($stdout, $stderr);
    capture(
        sub {
            $has_pcre = $step->_evaluate_cc_run($test, $verbose);
        },
        \$stdout,
    );
    ok($has_pcre, "_evaluate_cc_run returned true value as expected");
    is($step->result(), q{yes, 4.0}, "Got expected PCRE version");
    like($stdout, qr/\(yes, 4\.0\)/, "Got expected verbose output");
}

# Mock different outcomes of _recheck_settings()
$libs = q{-ldelta};
$ccflags = q{-Iepsilon};
$linkflags = q{-Lzeta};
{
    my ($stdout, $stderr);
    capture(
        sub { $step->_recheck_settings(
            $conf, $libs, $ccflags, $linkflags, $verbose); },
        \$stdout,
    );
    like($conf->data->get('libs'), qr/$libs/,
        "Got expected value for 'libs'");
    like($conf->data->get('ccflags'), qr/$ccflags/,
        "Got expected value for 'ccflags'");
    like($conf->data->get('linkflags'), qr/$linkflags/,
        "Got expected value for 'linkflags'");
    like($stdout, qr/\(no\)/, "Got expected verbose output");
}

pass("Completed all tests in $0");

################### DOCUMENTATION ###################

=head1 NAME

  auto_pcre-01.t - test auto::pcre

=head1 SYNOPSIS

    % prove t/steps/auto_pcre-01.t

=head1 DESCRIPTION

The files in this directory test functionality used by F<Configure.pl>.

The tests in this file test configuration step class auto::pcre.

=head1 AUTHOR

Alberto Sim�es.

=head1 SEE ALSO

config::auto::pcre, F<Configure.pl>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
