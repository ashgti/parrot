#! perl
# Copyright (C) 2007, Parrot Foundation.
# $Id$
# auto/alignptrs-01.t

use strict;
use warnings;
use Test::More tests => 12;
use Carp;
use lib qw( lib t/configure/testlib );
use_ok('config::auto::alignptrs');
use Parrot::Configure::Options qw( process_options );
use Parrot::Configure::Step::Test;
use Parrot::Configure::Test qw(
    test_step_constructor_and_description
);


########## regular; singular ##########

my ($args, $step_list_ref) = process_options(
    {
        argv => [ ],
        mode => q{configure},
    }
);

my $conf = Parrot::Configure::Step::Test->new;
$conf->include_config_results( $args );

my $serialized = $conf->pcfreeze();

my $pkg = q{auto::alignptrs};
$conf->add_steps($pkg);
$conf->options->set( %{$args} );
my $step = test_step_constructor_and_description($conf);
my $align = 1;
$conf->data->set('ptr_alignment' => $align);
my $ret = $step->runstep($conf);
ok( $ret, "runstep() returned true value" );
is($step->result(), qq{configured:  $align byte}, "Expected result was set");

$conf->replenish($serialized);

########## regular; plural ##########

($args, $step_list_ref) = process_options(
    {
        argv => [ ],
        mode => q{configure},
    }
);

$conf->add_steps($pkg);
$conf->options->set( %{$args} );
$step = test_step_constructor_and_description($conf);
$align = 2;
$conf->data->set('ptr_alignment' => $align);
$ret = $step->runstep($conf);
ok( $ret, "runstep() returned true value" );
is($step->result(), qq{configured:  $align bytes}, "Expected result was set");

pass("Completed all tests in $0");

################### DOCUMENTATION ###################

=head1 NAME

auto/alignptrs-01.t - test auto::alignptrs

=head1 SYNOPSIS

    % prove t/steps/auto/alignptrs-01.t

=head1 DESCRIPTION

The files in this directory test functionality used by F<Configure.pl>.

The tests in this file test auto::alignptrs.

=head1 AUTHOR

James E Keenan

=head1 SEE ALSO

config::auto::alignptrs, F<Configure.pl>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
