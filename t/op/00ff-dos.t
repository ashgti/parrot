#!perl
# Copyright (C) 2001-2005, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 2;

=head1 NAME

t/op/00ff-dos.t - DOS File Format

=head1 SYNOPSIS

        % prove t/op/00ff-dos.t

=head1 DESCRIPTION

Tests file formats.

=cut

my $code = qq(print "ok\\n"\r\nend\r\n);
pasm_output_is( $code, <<'OUT', "fileformat dos" );
ok
OUT

$code = qq(print "ok\\n"\r\nend\r\n\cZ\r\n);
pasm_output_is( $code, <<'OUT', "fileformat dos w ctrl-z" );
ok
OUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
