#! perl
# Copyright (C) 2001-2006, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;

use ExtUtils::Manifest;

=head1 NAME

t/distro/manifest.t - check sanity of the MANIFEST file

=head1 SYNOPSIS

    % prove t/distro/manifest.t

=head1 DESCRIPTION

Checks that the distribution and the MANIFEST file agree.

=cut

ok( -e $ExtUtils::Manifest::MANIFEST, 'MANIFEST exists' );

ok( -e $ExtUtils::Manifest::MANIFEST . '.SKIP', 'MANIFEST.SKIP exists' );

diag "this may take a while...";

$ExtUtils::Manifest::Quiet = 1;

my @missing = ExtUtils::Manifest::manicheck();
ok( !@missing, 'manicheck()' )
    or diag("Missing files:\n\t@missing");

# remember to change the number of tests :-)
BEGIN { plan tests => 3; }

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
