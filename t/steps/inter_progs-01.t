#! perl
# Copyright (C) 2007, The Perl Foundation.
# $Id$
# inter_progs-01.t

use strict;
use warnings;

use Test::More qw(no_plan); # tests => 21;
use Carp;
use lib qw( lib t/configure/testlib );
use_ok('config::init::defaults');
use_ok('config::init::install');
use_ok('config::init::hints');
use_ok('config::inter::progs');
use Parrot::Configure;
use Parrot::Configure::Options qw( process_options );
use Parrot::Configure::Test qw( test_step_thru_runstep);
use Tie::Filehandle::Preempt::Stdin;
use IO::CaptureOutput qw| capture |;

=for hints_for_testing Testing and refactoring of inter::progs should
entail understanding of issues discussed in the following RT tickets:
http://rt.perl.org/rt3/Ticket/Display.html?id=43174;
http://rt.perl.org/rt3/Ticket/Display.html?id=43173; and
http://rt.perl.org/rt3/Ticket/Display.html?id=41168.  You will have to
determine a way to test a user response to a prompt.

=cut

my $args = process_options(
    {
        argv => [q{--ask}],
        mode => q{configure},
    }
);

my $conf = Parrot::Configure->new;

test_step_thru_runstep( $conf, q{init::defaults}, $args );
test_step_thru_runstep( $conf, q{init::install},  $args );
test_step_thru_runstep( $conf, q{init::hints},    $args );

my ( $task, $step_name, $step, $ret );
my $pkg = q{inter::progs};

$conf->add_steps($pkg);
$conf->options->set( %{$args} );

$task        = $conf->steps->[-1];
$step_name   = $task->step;

$step = $step_name->new();
ok( defined $step, "$step_name constructor returned defined value" );
isa_ok( $step, $step_name );
ok( $step->description(), "$step_name has description" );

my @prompts;
foreach my $p (
    qw|
        cc
        link
        ld
        ccflags
        linkflags
        ldflags
        libs
        cxx
    |
    )
{
    push @prompts, $conf->data->get($p);
}
push @prompts, q{y};

my $object = tie *STDIN, 'Tie::Filehandle::Preempt::Stdin', @prompts;
can_ok( 'Tie::Filehandle::Preempt::Stdin', ('READLINE') );
isa_ok( $object, 'Tie::Filehandle::Preempt::Stdin' );

my ($stdout, $debug, $debug_validity);
capture( sub {
    my $verbose = inter::progs::_get_verbose($conf);
    my $ask = inter::progs::_prepare_for_interactivity($conf);
    my $cc;
    ($conf, $cc) = inter::progs::_get_programs($conf, $verbose, $ask);
    $debug = inter::progs::_get_debug($conf, $ask);
    $debug_validity = inter::progs::_is_debug_setting_valid($debug);
}, \$stdout);
ok( defined $debug_validity, "'debug_validity' set as expected" );

capture( sub {
    $conf = inter::progs::_set_debug_and_warn($conf, $debug);
}, \$stdout);
ok( defined $conf, "Components of $step_name runstep() tested okay" );

$object = undef;
untie *STDIN;

pass("Completed all tests in $0");

################### DOCUMENTATION ###################

=head1 NAME

inter_progs-01.t - test config::inter::progs

=head1 SYNOPSIS

    % prove t/steps/inter_progs-01.t

=head1 DESCRIPTION

The files in this directory test functionality used by F<Configure.pl>.

The tests in this file test subroutines exported by config::inter::progs.

=head1 AUTHOR

James E Keenan

=head1 SEE ALSO

config::inter::progs, F<Configure.pl>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
