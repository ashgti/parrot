#!perl

use strict;
use warnings;
use lib qw(t . lib ../lib ../../lib ../../../lib);
use Parrot::Test tests => 1;

language_output_is( 'punie', <<'CODE', <<'OUT', 'simple string ops' );
print 'a' . 'b', "\n";
print 'a' x 5, "\n";
CODE
ab
aaaaa
OUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
