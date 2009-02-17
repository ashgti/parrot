#!perl
# Copyright (C) 2001-2005, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 1;

=head1 NAME

t/op/00ff-unix.t - UNIX File format

=head1 SYNOPSIS

        % prove t/op/00ff-unix.t

=head1 DESCRIPTION

Tests file formats.

=cut

pasm_output_is( <<'CODE', <<'OUT', "fileformat unix" );
    print "ok\n"
    end
CODE
ok
OUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
