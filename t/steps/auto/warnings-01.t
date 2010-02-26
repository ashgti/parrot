#! perl
# Copyright (C) 2007, Parrot Foundation.
# $Id$
# auto/warnings-01.t

use strict;
use warnings;
use Test::More qw(no_plan); # tests =>  26;
use Carp;
use lib qw( lib t/configure/testlib );
use_ok('config::auto::warnings');
use Parrot::Configure::Options qw( process_options );
use Parrot::Configure::Step::Test;
use Parrot::Configure::Test qw(
    test_step_constructor_and_description
);
use IO::CaptureOutput qw | capture |;

my ($args, $step_list_ref) = process_options(
    {
        argv => [ ],
        mode => q{configure},
    }
);

my $conf = Parrot::Configure::Step::Test->new;
$conf->include_config_results( $args );

my $pkg = q{auto::warnings};

$conf->add_steps($pkg);

my $serialized = $conf->pcfreeze();

$conf->options->set( %{$args} );
SKIP: {
    skip 'Tests not yet passing on Sun/Solaris',
    23
    if $^O =~ m/sun|solaris/i;

my $step = test_step_constructor_and_description($conf);

#$step = test_step_constructor_and_description($conf);
#my $warning = q{-Wphony_warning};
#auto::warnings::_set_warning($conf, $warning, 1, undef);
#ok(! $conf->data->get($warning),
#    "Got expected setting for warning");
#
#$conf->replenish($serialized);
#
#$conf->options->set( %{$args} );
#$step = test_step_constructor_and_description($conf);
#{
#    my $warning = q{-Wphony_warning};
#    my $stdout;
#    capture(
#        sub { auto::warnings::_set_warning($conf, $warning, 1, 1); },
#        \$stdout,
#    );
#    ok(! $conf->data->get($warning),
#        "Got expected setting for warning");
#    like($stdout, qr/exit code:\s+1/, "Got expected verbose output");
#}

#$conf->replenish($serialized);

$conf->options->set( %{$args} );
$step = test_step_constructor_and_description($conf);
# Mock case where C compiler is not gcc.
$conf->data->set( gccversion => undef );
ok($step->runstep($conf), "runstep() returned true value");
is($step->result(), q{skipped}, "Got expected result");

##### _set_ccwarn() #####

my ($output, $warnings, $verbose);

#$output = 'some output';
#$warnings = '-Wsome_warning';
#$verbose = undef;
#$conf->data->set( ccwarn => undef );
#ok( auto::warnings::_set_ccwarn($conf, $output, $warnings, $verbose),
#    "_set_ccwarn returned true as expected" );
#is( $conf->data->get( 'ccwarn' ), $warnings,
#    "Warnings set as expected" );
#
#{
#    $output = 'some output';
#    $warnings = '-Wsome_warning';
#    $verbose = 1;
#    $conf->data->set( ccwarn => undef );
#    my ($stdout, $stderr, $rv);
#    capture(
#        sub { $rv = auto::warnings::_set_ccwarn(
#                $conf, $output, $warnings, $verbose); },
#        \$stdout,
#        \$stderr,
#    );
#    ok( $rv, "_set_ccwarn returned true as expected" );
#    like($stdout, qr/output:\s+$output/s,
#        "Got expected verbose output from _set_ccwarn()" );
#    like($stdout, qr/ccwarn:\s+$warnings/s,
#        "Got expected verbose output from _set_ccwarn()" );
#}
#
#$output = 'error';
#$warnings = '-Wsome_warning';
#$verbose = undef;
#$conf->data->set( ccwarn => undef );
#ok( ! auto::warnings::_set_ccwarn($conf, $output, $warnings, $verbose),
#    "_set_ccwarn returned false as expected" );
#ok( ! defined $conf->data->get( 'ccwarn' ), "Warnings set as expected" );

} # End SKIP block for Sun/Solaris

$conf->cc_clean();

pass("Completed all tests in $0");

################### DOCUMENTATION ###################

=head1 NAME

auto/warnings-01.t - test auto::warnings

=head1 SYNOPSIS

    % prove t/steps/auto/warnings-01.t

=head1 DESCRIPTION

The files in this directory test functionality used by F<Configure.pl>.

The tests in this file test aspects of auto::warnings.

=head1 AUTHOR

James E Keenan

=head1 SEE ALSO

config::auto::warnings, F<Configure.pl>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
