#! perl
# Copyright: 2005 The Perl Foundation.  All Rights Reserved.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More skip_all => "Python dynamic PMCs are not maintained";
use Parrot::Test tests => 26;
use Parrot::Config;


=head1 NAME

t/pmc/pyint.t - Python Integer basic type

=head1 SYNOPSIS

	% prove t/pmc/pyint.t

=head1 DESCRIPTION

Tests the Python Integer PMC.

=cut


pir_output_is(<< 'CODE', << 'OUTPUT', "abs");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P1, $I0

    set $P1, -13

    new $P2, $I0
    abs $P2, $P1
    print $P2
    print " "
    $P2 = $P1.__abs__($P1)
    print $P2
    print "\n"
.end
CODE
13 13
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "add");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0
    new $P1, $I0

    set $P0, 31
    set $P1, -13

    new $P2, $I0
    $P2 = $P0 + $P1
    print $P2
    print " "
    $P2 = $P0.__add__($P1)
    print $P2
    print " "
    $P2 = $P1.__radd__($P0)
    print $P2
    print "\n"
.end
CODE
18 18 18
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "and");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0
    new $P1, $I0

    set $P0, 31
    set $P1, -13

    new $P2, $I0
    and $P2, $P0, $P1
    print $P2
    print " "
    band $P2, $P0, $P1
    print $P2
    print " "
    $P2 = $P0.__and__($P1)
    print $P2
    print " "
    $P2 = $P1.__rand__($P0)
    print $P2
    print "\n"
.end
CODE
-13 19 19 19
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "cmp");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0
    new $P1, $I0

    set $P0, 31
    set $P1, -13

    new $P2, $I0
    cmp $I2, $P0, $P1
    $P2 = $I2
    print $P2
    print " "
    cmp $I2, $P0, -13
    $P2 = $I2
    print $P2
    print " "
    $P2 = $P0.__cmp__($P1)
    print $P2
    print "\n"
.end
CODE
1 1 1
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "div");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0
    new $P1, $I0

    set $P0, 31
    set $P1, -13

    new $P2, $I0
    $P2 = $P0 / $P1
    print $P2
    print " "
    $P2 = $P0 / -13
    print $P2
    print " "
    $P2 = $P0.__div__($P1)
    print $P2
    print " "
    $P2 = $P1.__rdiv__($P0)
    print $P2
    print "\n"
.end
CODE
-3 -3 -3 -3
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "float");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0

    set $P0, -13

    $P2 = $P0.__float__()
    print $P2
    print "\n"
.end
CODE
-13.0
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "floordiv");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0
    new $P1, $I0

    set $P0, 31
    set $P1, -13

    new $P2, $I0
    $P2 = $P0 // $P1
    print $P2
    print " "
    $P2 = $P0 // -13
    print $P2
    print " "
    $P2 = $P0.__floordiv__($P1)
    print $P2
    print " "
    $P2 = $P1.__rfloordiv__($P0)
    print $P2
    print "\n"
.end
CODE
-3 -3 -3 -3
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "hex");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0
    set $P0, 31

    $P2 = $P0.__hex__()
    print $P2
    print "\n"
.end
CODE
0x1f
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "int");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0

    set $P0, -13

    $P2 = $P0.__int__()
    print $P2
    print "\n"
.end
CODE
-13
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "invert");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P1, $I0

    set $P1, -13

    new $P2, $I0
    $P2 = ~ $P1
    print $P2
    print " "
    $P2 = $P1.__invert__()
    print $P2
    print "\n"
.end
CODE
12 12
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "is");

.sub main :main
    loadlib P1, "python_group"
    new $P0, "PyInt"
    new $P1, "PyInt"

    set $P0, 31
    set $P1, 31

    new $P2, "PyBoolean"
    issame $I0, $P0, $P1
    set $P2, $I0
    print $P2
    print "\n"
.end
CODE
True
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "lshift");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0
    new $P1, $I0

    set $P0, 31
    set $P1, 3

    new $P2, $I0
    $P2 = $P0 << $P1
    print $P2
    print " "
    $P2 = $P0 << 3
    print $P2
    print " "
    $P2 = $P0.__lshift__($P1)
    print $P2
    print " "
    $P2 = $P1.__rlshift__($P0)
    print $P2
    print "\n"
.end
CODE
248 248 248 248
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "mod");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0
    new $P1, $I0

    set $P0, 31
    set $P1, -13

    new $P2, $I0
    $P2 = $P0 % $P1
    print $P2
    print " "
    $P2 = $P0 % -13
    print $P2
    print " "
    $P2 = $P0.__mod__($P1)
    print $P2
    print " "
    $P2 = $P1.__rmod__($P0)
    print $P2
    print "\n"
.end
CODE
-8 -8 -8 -8
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "mul");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0
    new $P1, $I0

    set $P0, 31
    set $P1, -13

    new $P2, $I0
    $P2 = $P0 * $P1
    print $P2
    print " "
    $P2 = $P0 * -13
    print $P2
    print " "
    $P2 = $P0.__mul__($P1)
    print $P2
    print " "
    $P2 = $P1.__rmul__($P0)
    print $P2
    print "\n"
.end
CODE
-403 -403 -403 -403
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "neg");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P1, $I0

    set $P1, -13

    new $P2, $I0
    $P2 = - $P1
    print $P2
    print " "
    $P2 = $P1.__neg__()
    print $P2
    print "\n"
.end
CODE
13 13
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "nonzero");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P1, $I0

    set $P1, -13

    new $P2, $I0
    $P2 = $P1.__nonzero__()
    print $P2
    print "\n"
.end
CODE
True
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "oct");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0
    set $P0, 31

    $P2 = $P0.__oct__()
    print $P2
    print "\n"
.end
CODE
037
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "or");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0
    new $P1, $I0

    set $P0, 31
    set $P1, -13

    new $P2, $I0
    or $P2, $P0, $P1
    print $P2
    print " "
    bor $P2, $P0, $P1
    print $P2
    print " "
    $P2 = $P0.__or__($P1)
    print $P2
    print " "
    $P2 = $P1.__ror__($P0)
    print $P2
    print "\n"
.end
CODE
31 -1 -1 -1
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "pos");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P1, $I0

    set $P1, -13

    new $P2, $I0
    $P2 = $P1.__pos__()
    print $P2
    print "\n"
.end
CODE
-13
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "pow");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0
    new $P1, $I0
    new $P2, $I0

    set $P0, 31
    set $P1, 2
    set $P2, -1

    new $P3, $I0
    $P3 = $P0 ** $P1
    print $P3
    print " "
    $P3 = $P0 ** 3
    print $P3
    print " "
    $P3 = $P0.__pow__($P1)
    print $P3
    print " "
    $P3 = $P1.__pow__($P2)
    print $P3
    print " "
    $P3 = $P2.__rpow__($P1)
    print $P3
    print "\n"
.end
CODE
961 29791 961 0.5 0.5
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "repr");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0
    new $P1, $I0

    set $P1, -13

    new $P2, $I0
    $P2 = $P1.__repr__()
    print $P2
    print " "
    $P2 = $P1.__str__()
    print $P2
    print "\n"
.end
CODE
-13 -13
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "rshift");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0
    new $P1, $I0

    set $P0, 31
    set $P1, 3

    new $P2, $I0
    $P2 = $P0 >> $P1
    print $P2
    print " "
    $P2 = $P0 >> 3
    print $P2
    print " "
    $P2 = $P0.__rshift__($P1)
    print $P2
    print " "
    $P2 = $P1.__rrshift__($P0)
    print $P2
    print "\n"
.end
CODE
3 3 3 3
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "sub");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0
    new $P1, $I0

    set $P0, 31
    set $P1, -13

    new $P2, $I0
    $P2 = $P0 - $P1
    print $P2
    print " "
    $P2 = $P0.__sub__($P1)
    print $P2
    print " "
    $P2 = $P1.__rsub__($P0)
    print $P2
    print "\n"
.end
CODE
44 44 44
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "truediv");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0
    new $P1, $I0

    set $P0, 31
    set $P1, -4

    new $P2, $I0
    $P2 = $P0.__truediv__($P1)
    print $P2
    print "\n"
.end
CODE
-7.75
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "xor");

.sub main :main
    loadlib P1, "python_group"
    find_type $I0, "PyInt"
    new $P0, $I0
    new $P1, $I0

    set $P0, 31
    set $P1, -13

    new $P2, $I0
    bxor $P2, $P0, $P1
    print $P2
    print " "
    $P2 = $P0.__xor__($P1)
    print $P2
    print " "
    $P2 = $P1.__rxor__($P0)
    print $P2
    print "\n"
.end
CODE
-20 -20 -20
OUTPUT

SKIP:
{
  skip("No bigint support", 1) unless $PConfig{gmp};

pir_output_like(<< 'CODE', << 'OUTPUT', "sub - propagate to PyLong");
.sub main :main
    .include "sysinfo.pasm"
    .local pmc d, l, r
    .local int b, i
    d = new "PyObject"
    l = new "PyInt"
    r = new "PyInt"
    b = sysinfo .SYSINFO_PARROT_INTSIZE
    b *= 8  # CHAR_BITS
    dec b
    i = 1 << b
    dec i   # sys.maxint
    print i
    print "\n"
    r = i
    neg i
    l = i
    d = l - r
    print d
    print "\n"
    typeof $S0, d
    print $S0
    print "\n"
.end
CODE
/^\d+
-\d+
PyLong/
OUTPUT
}
