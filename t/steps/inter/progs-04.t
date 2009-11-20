#! perl
# Copyright (C) 2007, Parrot Foundation.
# $Id$
# inter/progs-04.t

use strict;
use warnings;

use Test::More tests =>  8;
use Carp;
use lib qw( lib t/configure/testlib );
use_ok('config::inter::progs');
use Parrot::Configure::Options qw( process_options );
use Parrot::Configure::Step::Test;
use Parrot::Configure::Test qw(
    test_step_constructor_and_description
);
use Tie::Filehandle::Preempt::Stdin;
use IO::CaptureOutput qw| capture |;

=for hints_for_testing Testing and refactoring of inter::progs should
entail understanding of issues discussed in
https://trac.parrot.org/parrot/ticket/854

=cut

########## ask ##########

my ($args, $step_list_ref) = process_options(
    {
        argv => [q{--ask}],
        mode => q{configure},
    }
);

my $conf = Parrot::Configure::Step::Test->new;
$conf->include_config_results( $args );

my $pkg = q{inter::progs};

$conf->add_steps($pkg);

$conf->options->set( %{$args} );
my $step = test_step_constructor_and_description($conf);

my @prompts;
my $object;
my ($stdout, $debug, $debug_validity);

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
push @prompts, q{0};

$object = tie *STDIN, 'Tie::Filehandle::Preempt::Stdin', @prompts;
can_ok( 'Tie::Filehandle::Preempt::Stdin', ('READLINE') );
isa_ok( $object, 'Tie::Filehandle::Preempt::Stdin' );

my $rv;
capture( sub {
    $rv = $step->runstep($conf);
}, \$stdout);
ok( ! defined $rv, "runstep returned undef as expected" );

$object = undef;
untie *STDIN;

pass("Completed all tests in $0");

################### DOCUMENTATION ###################

=head1 NAME

inter/progs-04.t - test inter::progs

=head1 SYNOPSIS

    % prove t/steps/inter/progs-04.t

=head1 DESCRIPTION

The files in this directory test functionality used by F<Configure.pl>.

The tests in this file test inter::progs.

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
