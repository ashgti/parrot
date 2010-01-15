#! perl
# Copyright (C) 2009, Parrot Foundation.
# $Id$
# gen/libjit-01.t

use strict;
use warnings;

use constant NUM_GENERATED_FILES => 2;
# use Test::More tests => 8 + 2*NUM_GENERATED_FILES;
use Test::More qw( no_plan );

use File::Copy 'move';
use File::Temp 'tempfile';

use lib 'lib';
use Parrot::Configure;
use Parrot::Configure::Options 'process_options';
use Parrot::Configure::Test qw(
    test_step_thru_runstep
    rerun_defaults_for_testing
    test_step_constructor_and_description
);

use_ok('config::gen::libjit');

my ($args, $step_list_ref) = process_options(
    {
	argv => [],
	mode => 'configure',
    }
);

my $conf = Parrot::Configure->new;

my $serialized = $conf->pcfreeze();

my $pkg = 'gen::libjit';
$conf->add_steps($pkg);
$conf->options->set( %$args );
my $step = test_step_constructor_and_description($conf);

is( scalar keys %{$step->{targets}}, NUM_GENERATED_FILES,
    "Expected number of generated files");
is_deeply([keys %{$step->{targets}}], [keys %{$step->{templates}}],
    "Templates match targets");

foreach (keys %{$step->{templates}}) {
    ok(-f $step->{templates}{$_}, "Able to locate $_ template")
}

my %orig_files;
foreach (keys %{$step->{targets}}) {
    if (-f (my $targ_name = $step->{targets}{$_})) {
        $orig_files{$_} = tempfile();
        move($targ_name, $orig_files{$_});
    }
}

my %orig_conf = map { $_ => $conf->data->get($_) } qw[ iv nv ];
$conf->data->set( iv => 'int', nv => 'float' );
# Set a value for 'libjit_has_alloca' to avoid uninitialized value warning.
$conf->data->set( 'libjit_has_alloca' => 1 );
my $ret = $step->runstep($conf);
ok( $ret, "runstep() returned true value" );
foreach (keys %{$step->{targets}}) {
    ok(-f $step->{targets}{$_}, "$_ target generated");
}

# re-set for next test
$conf->data->set(%orig_conf);
$step->set_result( '' );
foreach (keys %{$step->{targets}}) {
    if (exists $orig_files{$_}) {
        move( $orig_files{$_}, $step->{targets}{$_} );
    } else {
        unlink $_;
    }
}

$conf->replenish($serialized);

pass("Completed all tests in $0");

################### DOCUMENTATION ###################

=head1 NAME

  gen/libjit-01.t - test gen::libjit

=head1 SYNOPSIS

    % prove t/steps/gen/libjit-01.t

=head1 DESCRIPTION

The files in this directory test functionality used by F<Configure.pl>.

The tests in this file test gen::libjit.

=head1 SEE ALSO

config::gen::libjit, F<Configure.pl>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
