#! perl
# Copyright (C) 2007, The Perl Foundation.
# $Id$
# 010-verbose_step_num_uncalled.t

use strict;
use warnings;

use Test::More tests => 13;
use Carp;
use lib qw( lib t/configure/testlib );
use Parrot::Configure;
use Parrot::Configure::Options qw( process_options );
use IO::CaptureOutput qw | capture |;

$| = 1;
is( $|, 1, "output autoflush is set" );

my $args = process_options(
    {
        argv => [q{--verbose-step=2}],
        mode => q{configure},
    }
);
ok( defined $args, "process_options returned successfully" );
my %args = %$args;

my $conf = Parrot::Configure->new;
ok( defined $conf, "Parrot::Configure->new() returned okay" );

my $step        = q{init::foobar};
my $description = 'Determining if your computer does foobar';

$conf->add_steps($step);
my @confsteps = @{ $conf->steps };
isnt( scalar @confsteps, 0,
    "Parrot::Configure object 'steps' key holds non-empty array reference" );
is( scalar @confsteps, 1, "Parrot::Configure object 'steps' key holds ref to 1-element array" );
my $nontaskcount = 0;
foreach my $k (@confsteps) {
    $nontaskcount++ unless $k->isa("Parrot::Configure::Task");
}
is( $nontaskcount, 0, "Each step is a Parrot::Configure::Task object" );
is( $confsteps[0]->step, $step, "'step' element of Parrot::Configure::Task struct identified" );
is( ref( $confsteps[0]->params ),
    'ARRAY', "'params' element of Parrot::Configure::Task struct is array ref" );
ok( !ref( $confsteps[0]->object ),
    "'object' element of Parrot::Configure::Task struct is not yet a ref" );

$conf->options->set(%args);
is( $conf->options->{c}->{debugging},
    1, "command-line option '--debugging' has been stored in object" );

{
    my $rv;
    my $stdout;
    capture ( sub {$rv    = $conf->runsteps}, \$stdout );
    ok( $rv, "runsteps successfully ran $step" );
    like(
        $stdout,
        qr/$description\.\.\..*done\.(?!\s+Setting Configuration Data)/s,
        "Got message expected upon running $step"
    );
}

pass("Completed all tests in $0");

################### DOCUMENTATION ###################

=head1 NAME

010-verbose_step_num_uncalled.t - test bad step failure case in Parrot::Configure

=head1 SYNOPSIS

    % prove t/configure/010-verbose_step_num_uncalled.t

=head1 DESCRIPTION

The files in this directory test functionality used by F<Configure.pl>.

The tests in this file examine what happens when you configure with the
<--verbose-step> option set to a number that does not correspond to the
sequence position of the associated step.

=head1 AUTHOR

James E Keenan

=head1 SEE ALSO

Parrot::Configure, F<Configure.pl>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
