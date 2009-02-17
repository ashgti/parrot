#! perl
# Copyright (C) 2007, Parrot Foundation.
# $Id$
# 037-run_single_step.t

use strict;
use warnings;

use Test::More tests => 5;
use Carp;
use lib qw( lib t/configure/testlib );
use Parrot::Configure;
use Parrot::Configure::Options qw( process_options );
use IO::CaptureOutput qw | capture |;

my ($args, $step_list_ref) = process_options(
    {
        argv => [],
        mode => q{configure},
    }
);
ok( defined $args, "process_options returned successfully" );
my %args = %$args;

my $conf = Parrot::Configure->new;
ok( defined $conf, "Parrot::Configure->new() returned okay" );

my $step = q{init::defaults};
$conf->add_step($step);
$conf->options->set( %{$args} );

{
    my $stdout;
    capture ( sub { eval { $conf->run_single_step($step); } }, \$stdout );
    ok( !$@, "run_single_step() completed without error" );
    like(
        $stdout,
#        qr/$step.*done./s, #'
#        qr/Setting up Configure's default values/s, #'
        qr/Set Configure's default values/s, #'
        "Got message expected upon running $step"
    );
}

pass("Completed all tests in $0");

################### DOCUMENTATION ###################

=head1 NAME

037-run_single_step.t - test C<run_single_step()> during configuration

=head1 SYNOPSIS

    % prove t/configure/037-run_single_step.t

=head1 DESCRIPTION

The files in this directory test functionality used by F<Configure.pl>.

The tests in this file examine what would happen if
C<Parrot::Configure::run_single_step()> were run during configuration.
This is not its typical or recommended use.  It is more typically called
post-configuration by F<tools/dev/reconfigure.pl>.

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
