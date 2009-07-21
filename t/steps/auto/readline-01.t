#! perl
# Copyright (C) 2007, Parrot Foundation.
# $Id$
# auto/readline-01.t

use strict;
use warnings;
use Test::More tests => 14;
use Carp;
use Cwd;
use File::Spec;
use File::Temp qw( tempdir );
use lib qw( lib );
use_ok('config::init::defaults');
use_ok('config::auto::readline');
use Parrot::Configure;
use Parrot::Configure::Options qw( process_options );
use Parrot::Configure::Test qw(
    test_step_thru_runstep
    test_step_constructor_and_description
);
use IO::CaptureOutput qw | capture |;

########## _select_lib() ##########

my ($args, $step_list_ref) = process_options(
    {
        argv => [ ],
        mode => q{configure},
    }
);

my $conf = Parrot::Configure->new;

test_step_thru_runstep( $conf, q{init::defaults}, $args );

my $pkg = q{auto::readline};

$conf->add_steps($pkg);
$conf->options->set( %{$args} );
my $step = test_step_constructor_and_description($conf);

# Mock values for OS and C-compiler
my ($osname, $cc);
$osname = 'mswin32';
$cc = 'gcc';
is($step->_select_lib( {
    conf            => $conf,
    osname          => $osname,
    cc              => $cc,
    win32_nongcc    => 'readline.lib',
    default         => '-lreadline',
} ),
   '-lreadline',
   "_select_lib() returned expected value");

$osname = 'mswin32';
$cc = 'cc';
is($step->_select_lib( {
    conf            => $conf,
    osname          => $osname,
    cc              => $cc,
    win32_nongcc    => 'readline.lib',
    default         => '-lreadline',
} ),
   'readline.lib',
   "_select_lib() returned expected value");

$osname = 'foobar';
$cc = undef;
is($step->_select_lib( {
    conf            => $conf,
    osname          => $osname,
    cc              => $cc,
    win32_nongcc    => 'readline.lib',
    default         => '-lreadline',
} ),
   '-lreadline',
   "_select_lib() returned true value");

$osname = 'foobar';
$cc = undef;
eval {
    $step->_select_lib( [
        conf            => $conf,
        osname          => $osname,
        cc              => $cc,
        win32_nongcc    => 'readline.lib',
        default         => '-lreadline',
    ] );
};
like($@, qr/_select_lib\(\) takes hashref/,
    "Bad argument to _select_lib correctly detected");

pass("Completed all tests in $0");

################### DOCUMENTATION ###################

=head1 NAME

auto/readline-01.t - test auto::readline

=head1 SYNOPSIS

    % prove t/steps/auto/readline-01.t

=head1 DESCRIPTION

The files in this directory test functionality used by F<Configure.pl>.

The tests in this file test auto::readline.

=head1 AUTHOR

James E Keenan

=head1 SEE ALSO

config::auto::readline, F<Configure.pl>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
