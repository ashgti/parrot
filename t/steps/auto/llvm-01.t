#!perl
# Copyright (C) 2001-2007, Parrot Foundation.
# $Id$
# auto/llvm-01.t

use strict;
use warnings;
use File::Temp qw( tempdir );
use Test::More qw(no_plan); # tests =>  43;
use Carp;
use lib qw( lib t/configure/testlib );
use_ok('config::init::defaults');
use_ok('config::inter::progs');
use_ok('config::auto::llvm');
use Parrot::Configure;
use Parrot::Configure::Options qw( process_options );
use Parrot::Configure::Test qw(
    test_step_thru_runstep
    rerun_defaults_for_testing
    test_step_constructor_and_description
);
use IO::CaptureOutput qw( capture );

########## regular ##########

my ($args, $step_list_ref) = process_options( {
    argv => [ ],
    mode => q{configure},
} );

my $conf = Parrot::Configure->new;

my $serialized = $conf->pcfreeze();

test_step_thru_runstep($conf, q{init::defaults}, $args);
test_step_thru_runstep($conf, q{inter::progs},   $args);

my $pkg = q{auto::llvm};

$conf->add_steps($pkg);
$conf->options->set( %{$args} );
my $step = test_step_constructor_and_description($conf);
my $ret = $step->runstep($conf);
ok( $ret, "runstep() returned true value" );
like( $step->result(), qr/yes|no/,
  "Result was either 'yes' or 'no'" );

$conf->replenish($serialized);

########## --verbose ##########

($args, $step_list_ref) = process_options( {
    argv => [ q{--verbose} ],
    mode => q{configure},
} );
rerun_defaults_for_testing($conf, $args );
$conf->add_steps($pkg);
$conf->options->set( %{$args} );
$step = test_step_constructor_and_description($conf);
{
    my $stdout;
    my $ret = capture(
        sub { $step->runstep($conf) },
        \$stdout
    );
    ok( $ret, "runstep() returned true value" );
    like( $step->result(), qr/yes|no/,
        "Result was either 'yes' or 'no'" );
    SKIP: {
        skip 'No sense testing for verbose output if LLVM not present',
        2 unless ( $step->result() =~ /yes/ );
        like( $stdout, qr/llvm-gcc/s,
            "Got expected verbose output" );
        like( $stdout, qr/Low Level Virtual Machine/s,
            "Got expected verbose output" );
    }
}

$step->set_result( undef );
$conf->data->set( 'has_llvm' => undef );
auto::llvm::_handle_result($step, $conf, 1);
is( $step->result(), 'yes', "Got expected 'yes' result" );
ok( $conf->data->get( 'has_llvm' ),
    "'has_llvm' set to true value, as expected" );

$step->set_result( undef );
$conf->data->set( 'has_llvm' => undef );
auto::llvm::_handle_result($step, $conf, 0);
is( $step->result(), 'no', "Got expected 'no' result" );
ok( ! $conf->data->get( 'has_llvm' ),
    "'has_llvm' set to false  value, as expected" );

##### _handle_component_version_output() #####

my ($prog, $output, $llvm_lacking, $verbose);
$prog = [ 'llvm-gcc'    => 'llvm-gcc' ];

$verbose = 0;

$output = 'llvm-gcc';
$llvm_lacking = 0;
$llvm_lacking = auto::llvm::_handle_component_version_output(
    $prog, $output, $llvm_lacking, $verbose
);
ok( ! $llvm_lacking, "llvm reported as not lacking" );

$output = 'foobar';
$llvm_lacking = 0;
$llvm_lacking = auto::llvm::_handle_component_version_output(
    $prog, $output, $llvm_lacking, $verbose
);
ok( $llvm_lacking, "llvm reported as lacking: wrong output" );

$output = undef;
$llvm_lacking = 0;
$llvm_lacking = auto::llvm::_handle_component_version_output(
    $prog, $output, $llvm_lacking, $verbose
);
ok( $llvm_lacking, "llvm reported as lacking: output undefined" );

$verbose = 1;

my ($stdout, $stderr);
my $exp = $prog->[0];

$output = 'llvm-gcc';
$llvm_lacking = 0;
capture(
    sub {
        $llvm_lacking = auto::llvm::_handle_component_version_output(
            $prog, $output, $llvm_lacking, $verbose
        );
    },
    \$stdout,
    \$stderr,
);
ok( ! $llvm_lacking, "llvm reported as not lacking" );
like( $stdout, qr/$output/, "Got expected verbose output: llvm not lacking" );

$output = 'foobar';
$llvm_lacking = 0;
capture(
    sub {
        $llvm_lacking = auto::llvm::_handle_component_version_output(
            $prog, $output, $llvm_lacking, $verbose
        );
    },
    \$stdout,
    \$stderr,
);
ok( $llvm_lacking, "llvm reported as lacking: wrong output" );
like(
    $stdout,
    qr/Could not get expected '--version' output for $exp/,
    "Got expected verbose output: llvm lacking",
);

$output = undef;
$llvm_lacking = 0;
capture(
    sub {
        $llvm_lacking = auto::llvm::_handle_component_version_output(
            $prog, $output, $llvm_lacking, $verbose
        );
    },
    \$stdout,
    \$stderr,
);
ok( $llvm_lacking, "llvm reported as lacking: output undefined" );
like(
    $stdout,
    qr/Could not get expected '--version' output for $exp/,
    "Got expected verbose output: llvm lacking",
);

##### _handle_native_assembly_output() #####

{
    local $@ = '';
    $output = 'hello world';
    $verbose = 0;
    ok( $step->_handle_native_assembly_output( $conf, $output, $verbose ),
        "_handle_native_assembly_output() returned true value" );
    is( $step->result(), 'yes', "Got expected 'yes' result" );
}

{
    local $@ = 'error';
    $output = 'hello world';
    $verbose = 0;
    ok( $step->_handle_native_assembly_output( $conf, $output, $verbose ),
        "_handle_native_assembly_output() returned true value" );
    is( $step->result(), 'no', "Got expected 'no' result" );
}

{
    local $@ = '';
    $output = 'goodbye, cruel world';
    $verbose = 0;
    ok( $step->_handle_native_assembly_output( $conf, $output, $verbose ),
        "_handle_native_assembly_output() returned true value" );
    is( $step->result(), 'no', "Got expected 'no' result" );
}

{
    local $@ = 'error';
    $output = 'hello world';
    $verbose = 1;
    capture(
        sub { $step->_handle_native_assembly_output(
                $conf, $output, $verbose); },
        \$stdout,
        \$stderr,
    );
    is( $step->result(), 'no', "Got expected 'no' result" );
    like(
        $stdout, 
        qr/Unable to execute native assembly program successfully/,
        "Got expected verbose output: native assembly program",
    );
}

{
    local $@ = '';
    $output = 'goodbye, cruel world';
    $verbose = 1;
    capture(
        sub { $step->_handle_native_assembly_output(
                $conf, $output, $verbose); },
        \$stdout,
        \$stderr,
    );
    is( $step->result(), 'no', "Got expected 'no' result" );
    like(
        $stdout, 
        qr/Unable to execute native assembly program successfully/,
        "Got expected verbose output: native assembly program",
    );
}

##### _cleanup_llvm_files() #####

my ( $bcfile, $sfile, $nativefile );
my $count_unlinked;

$count_unlinked =
    auto::llvm::_cleanup_llvm_files( $bcfile, $sfile, $nativefile );
is( $count_unlinked, 0, "no files existed, hence none unlinked" );

( $bcfile, $sfile, $nativefile ) = ( '', '', '' );
$count_unlinked =
    auto::llvm::_cleanup_llvm_files( $bcfile, $sfile, $nativefile );
is( $count_unlinked, 0, "no files existed, hence none unlinked" );

{
    my $tdir = tempdir( CLEANUP => 1 );
    $bcfile = qq|$tdir/bcfile|;
    open my $FH, '>', $bcfile
        or die "Unable to open handle for writing: $!";
    print $FH qq|bcfile hello world\n|;
    close $FH or die "Unable to close handle after writing: $!";
    $count_unlinked =
        auto::llvm::_cleanup_llvm_files( $bcfile, $sfile, $nativefile );
    is( $count_unlinked, 1, "one file existed, hence one unlinked" );
}

$conf->cc_clean();

pass("Completed all tests in $0");

################### DOCUMENTATION ###################

=head1 NAME

t/steps/auto/llvm-01.t - tests Parrot::Configure step auto::llvm

=head1 SYNOPSIS

    prove t/steps/auto/llvm-01.t

=head1 DESCRIPTION

This file holds tests for auto::llvm.

=head1 AUTHOR

James E Keenan

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
