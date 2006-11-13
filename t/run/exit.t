#! perl
# Copyright (C) 2001-2004, The Perl Foundation.
# $Id$

=head1 NAME

t/run/exit.t - test parrot exit codes

=head1 SYNOPSIS

    % prove t/run/exit.t

=head1 DESCRIPTION

Tests C<Parrot> exit codes. Tests variations of normal and abnormal exit
with combinations of STDERR and STDOUT open and closed.

=cut

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );

use Test::More;
use Parrot::Test tests => 8;
use Parrot::Config;
use File::Spec;


my $PARROT = ".$PConfig{slash}$PConfig{test_prog}";
my $redir = File::Spec->devnull;

# copy file descriptors
open *OLDOUT, ">&STDOUT" or die qq|Cannot dup STDOUT: $!|; ## no critic
open *OLDERR, ">&STDERR" or die qq|Cannot dup STDERR: $!|; ## no critic

sub exits
{
    my $pre = shift;
    $pre ||= '';

    is( system(qq|$PARROT --version $redir > $redir 2> $redir|) >> 8, 0, "$pre: normal exit" );
    isnt( system(qq|$PARROT --foo $redir > $redir 2> $redir|) >> 8, 0, "$pre: abnormal exit" );
}

# all open
exits( 'STDERR & STDOUT open' );

# close stderr
close *STDERR or die qq|Cannot close STDERR: $!|;
exits( 'STDERR closed' );

# close stdout too
close *STDOUT or die qq|Cannot close STDOUT: $!|;
exits( 'STDERR & STDOUT closed' );

# restore stderr
open *STDERR, ">&OLDERR" or die qq|Cannot restore stderr: $!|; ## no critic
exits( 'STDOUT closed' );

# restore stdout
open *STDOUT, ">&OLDOUT" or die qq|Cannot restore stdout: $!|; ## no critic

# close copies
close *OLDOUT or die qq|Cannot close OLDOUT: $!|;
close *OLDERR or die qq|Cannot close OLDERR: $!|;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
