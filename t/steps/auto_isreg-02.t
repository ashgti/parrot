#! perl
# Copyright (C) 2007, The Perl Foundation.
# $Id$
# auto_isreg-02.t

use strict;
use warnings;
use Test::More tests =>  12;
use Carp;
use lib qw( lib t/configure/testlib );
use_ok('config::init::defaults');
use_ok('config::auto::isreg');
use Parrot::Configure;
use Parrot::Configure::Options qw( process_options );
use Parrot::Configure::Test qw( test_step_thru_runstep);
use IO::CaptureOutput qw| capture |;

my $args = process_options( {
    argv            => [ q{--verbose} ],
    mode            => q{configure},
} );

my $conf = Parrot::Configure->new();

test_step_thru_runstep($conf, q{init::defaults}, $args);

my ($task, $step_name, $step, $ret);
my $pkg = q{auto::isreg};

$conf->add_steps($pkg);
$conf->options->set(%{$args});
$task = $conf->steps->[-1];
$step_name   = $task->step;

$step = $step_name->new();
ok(defined $step, "$step_name constructor returned defined value");
isa_ok($step, $step_name);


{
    my $anyerror;
    my $stdout;
    my $ret = capture(
        sub { $step->_evaluate_isreg($conf, $anyerror) },
        \$stdout
    );
    ok($ret, "_evaluate_isreg returned true value");
    is($conf->data->get('isreg'), 1, "'isreg' set to true value as expected");
    is($step->result, 'yes', "Got expected result");
}

pass("Completed all tests in $0");

################### DOCUMENTATION ###################

=head1 NAME

auto_isreg-02.t - test config::auto::isreg

=head1 SYNOPSIS

    % prove t/steps/auto_isreg-02.t

=head1 DESCRIPTION

The files in this directory test functionality used by F<Configure.pl>.

The tests in this file test subroutines exported by config::auto::isreg.

=head1 AUTHOR

James E Keenan

=head1 SEE ALSO

config::auto::isreg, F<Configure.pl>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
