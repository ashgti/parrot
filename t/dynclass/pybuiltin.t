#! perl -w
# Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

t/dynclass/pybuiltin.t - Python Builtins

=head1 SYNOPSIS

	% perl -Ilib t/dynclass/pybuiltin.t

=head1 DESCRIPTION

Tests the Python Builtins.

=cut

use Parrot::Test tests => 4;
use Parrot::Config;

output_is(<< 'CODE', << 'OUTPUT', "delegating");
##PIR##
.sub main @MAIN
    new_pad 0
    loadlib $P0, "python_group"
    find_global P0, "PyBuiltin", "__load__"
    invoke

    find_type $I0, "PyInt"
    new $P0, $I0
    new $P1, $I0

    set $P0, 31
    set $P1, -13

    find_lex $P2, "abs"
    $P3 = $P2($P1)
    print $P3
    print "\n"

    find_lex $P2, "cmp"
    $P3 = $P2($P0,$P1)
    print $P3
    print "\n"

    find_lex $P2, "complex"
    $P3 = $P2($P0,$P1)
    print $P3
    print "\n"

    find_lex $P2, "float"
    $P3 = $P2($P0)
    print $P3
    print "\n"

    find_lex $P2, "hex"
    $P3 = $P2($P0)
    print $P3
    print "\n"

    find_lex $P2, "int"
    $P3 = $P2($P0)
    print $P3
    print "\n"

    find_lex $P2, "oct"
    $P3 = $P2($P0)
    print $P3
    print "\n"
.end
CODE
13
1
(31-13j)
31.0
0x1f
31
037
OUTPUT

SKIP: { skip("No BigInt Lib configured", 1) if !$PConfig{gmp};
output_is(<< 'CODE', << 'OUTPUT', "bigint");
##PIR##
.sub main @MAIN
    new_pad 0
    loadlib $P0, "python_group"
    find_global P0, "PyBuiltin", "__load__"
    invoke

    find_type $I0, "PyInt"
    new $P0, $I0
    set $P0, 31

    find_lex $P1, "long"
    $P2 = $P1($P0)
    print $P2
    print "\n"
.end
CODE
31L
OUTPUT
}

output_is(<< 'CODE', << 'OUTPUT', "range");
##PIR##
.sub main @MAIN
    new_pad 0
    loadlib $P0, "python_group"
    find_global P0, "PyBuiltin", "__load__"
    invoke

    find_type $I0, "PyInt"
    new $P0, $I0
    new $P1, $I0
    new $P2, $I0
    new $P3, $I0
    new $P4, $I0

    set $P0, 0
    set $P1, 1
    set $P2, 2
    set $P3, 3
    set $P4, -2

    find_lex $P5, "range"
    $P6 = $P5($P0,$P0,$P1)
    print $P6
    print "\n"

    $P6 = $P5($P0,$P3,$P1)
    print $P6
    print "\n"

    $P6 = $P5($P0,$P3,$P2)
    print $P6
    print "\n"

    $P6 = $P5($P3,$P0,$P4)
    print $P6
    print "\n"

    $P6 = $P5($P0,$P3)
    print $P6
    print "\n"

    $P6 = $P5($P2)
    print $P6
    print "\n"
.end
CODE
[]
[0, 1, 2]
[0, 2]
[3, 1]
[0, 1, 2]
[0, 1]
OUTPUT

output_is(<< 'CODE', << 'OUTPUT', "boolean");
##PIR##
.sub main @MAIN
    new_pad 0
    loadlib $P0, "python_group"
    find_global P0, "PyBuiltin", "__load__"
    invoke

    find_lex $P1, "False"
    find_lex $P2, "True"

    print $P1
    print " "
    print $P2
    print "\n"
.end
CODE
False True
OUTPUT
