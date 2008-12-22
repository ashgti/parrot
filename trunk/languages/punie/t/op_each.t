#!perl

# Copyright (C) 2007, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw(t . lib ../lib ../../lib ../../../lib);
use Parrot::Test tests => 2;
use Test::More;

language_output_is( 'punie', <<'CODE', <<'OUT', 'a basic hash' );
$h{'a'} = 'A';
$h{'b'} = 'B';
print $h{'a'};
print $h{'b'};
print "\n";

CODE
AB
OUT

TODO: {
    local $TODO = 'unimplemented feature';
    language_output_is( 'punie', <<'CODE', <<'OUT', 'each op' );
#!./perl

# $Header: op.each,v 1.0 87/12/18 13:13:23 root Exp $

print "1..2\n";

$h{'abc'} = 'ABC';
$h{'def'} = 'DEF';
$h{'jkl'} = 'JKL';
$h{'xyz'} = 'XYZ';
$h{'a'} = 'A';
$h{'b'} = 'B';
$h{'c'} = 'C';
$h{'d'} = 'D';
$h{'e'} = 'E';
$h{'f'} = 'F';
$h{'g'} = 'G';
$h{'h'} = 'H';
$h{'i'} = 'I';
$h{'j'} = 'J';
$h{'k'} = 'K';
$h{'l'} = 'L';
$h{'m'} = 'M';
$h{'n'} = 'N';
$h{'o'} = 'O';
$h{'p'} = 'P';
$h{'q'} = 'Q';
$h{'r'} = 'R';
$h{'s'} = 'S';
$h{'t'} = 'T';
$h{'u'} = 'U';
$h{'v'} = 'V';
$h{'w'} = 'W';
$h{'x'} = 'X';
$h{'y'} = 'Y';
$h{'z'} = 'Z';

@keys = keys(h);
@values = values(h);

if ($#keys == 29 && $#values == 29) {print "ok 1\n";} else {print "not ok 1\n";}

while (($key,$value) = each(h)) {
    if ($key eq $keys[$i] && $value eq $values[$i] && $key gt $value) {
	$key =~ y/a-z/A-Z/;
	$i++ if $key eq $value;
    }
}

if ($i == 30) {print "ok 2\n";} else {print "not ok 2\n";}

CODE
1..2
ok 1
ok 2
OUT

}

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
