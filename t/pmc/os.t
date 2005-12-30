#! perl
# Copyright: 2001-2005 The Perl Foundation.  All Rights Reserved.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test;
use Parrot::Config;
use Cwd;

=head1 NAME

t/pmc/os.t - Files and Dirs

=head1 SYNOPSIS

	% prove t/pmc/os.t

=head1 DESCRIPTION

Tests the C<OS> PMC.

=cut

my $cwd = getcwd;
pir_output_is(<<'CODE', <<"OUT", "cwd");
.sub main :main
        $P1 = new .OS
        $S1 = $P1."cwd"()
        print $S1
        print "\n"
        end
.end
CODE
$cwd
OUT

## remember to change the number of tests :-)
BEGIN { plan tests => 1; }
