#!perl
# Copyright (C) 2001-2006, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );

use Test::More;
use Parrot::Config;
use Parrot::Test tests => 32;

pir_output_is( <<'CODE', <<'OUT', "+=" );
.sub test :main
    $I0 = 10
    $I0 += 20
    print $I0
    print "\n"

    $I0 += -20
    print $I0
    print "\n"
    end
.end
CODE
30
10
OUT

pir_output_is( <<'CODE', <<'OUT', "-=" );
.sub test :main
    $I0 = 10
    $I0 -= 20
    print $I0
    print "\n"

    $I0 -= -20
    print $I0
    print "\n"
    end
.end
CODE
-10
10
OUT

pir_output_is( <<'CODE', <<'OUT', "*=" );
.sub test :main
    $I0 = 10
    $I0 *= 20
    print $I0
    print "\n"

    $I0 *= -2
    print $I0
    print "\n"
    end
.end
CODE
200
-400
OUT

pir_output_is( <<'CODE', <<'OUT', "/=" );
.sub test :main
    $I0 = 20
    $I0 /= 2
    print $I0
    print "\n"

    $N0 = 20
    $N0 /= .5
    $I0 = $N0
    print $I0
    print "\n"

    end
.end
CODE
10
40
OUT

pir_output_is( <<'CODE', <<'OUT', "%=" );
.sub test :main
    $I0 = 20
    $I0 %= 7
    print $I0
    print "\n"

    $I0 = 200
    $I0 %= 2
    print $I0
    print "\n"
    end
.end
CODE
6
0
OUT

pir_output_is( <<'CODE', <<'OUT', ".=" );
.sub test :main
    $S0 = "ab"
    $S0 .= "cd"
    print $S0
    print "\n"

    $S0 .= ""
    print $S0
    print "\n"
    end
.end
CODE
abcd
abcd
OUT

pir_output_is( <<'CODE', <<'OUT', "&=" );
.sub test :main
    $I0 =  0b1011
    $I0 &= 0b1000
    print $I0
    print "\n"

    $I0 &= 0b0000
    print $I0
    print "\n"
    end
.end
CODE
8
0
OUT

pir_output_is( <<'CODE', <<'OUT', "|=" );
.sub test :main
    $I0 =  0b1011
    $I0 |= 0b1000
    print $I0
    print "\n"

    $I0 |= 0b0100
    print $I0
    print "\n"
    end
.end
CODE
11
15
OUT

pir_output_is( <<'CODE', <<'OUT', "~=" );
.sub test :main
    $I0 =  0b1011
    $I0 ~= 0b1000
    print $I0
    print "\n"

    $I0 ~= 0b0011
    print $I0
    print "\n"
    end
.end
CODE
3
0
OUT

pir_output_is( <<'CODE', <<'OUT', ">>=" );
.sub test :main
    $I0 =  0b1011
    $I0 >>= 1
    print $I0
    print "\n"
    end
.end
CODE
5
OUT

pir_output_is( <<'CODE', <<'OUT', ">>>=" );
.sub test :main
    $I0 =  0b1011
    $I0 >>>= 1
    print $I0
    print "\n"
    end
.end
CODE
5
OUT

pir_output_is( <<'CODE', <<'OUT', "<<=" );
.sub test :main
    $I0 =  0b1011
    $I0 <<= 1
    print $I0
    print "\n"
    end
.end
CODE
22
OUT

pir_output_is( <<'CODE', <<'OUT', "x = defined" );
.sub test :main
    .local pmc a
    a = new 'ResizablePMCArray'
    push a, 10
    $I0 = defined a
    print $I0
    $I0 = defined a[0]
    print $I0
    $I0 = defined a[1]
    print $I0
    print "\n"
    end
.end
CODE
110
OUT

pir_output_is( <<'CODE', <<'OUT', "x = clone" );
.sub test :main
    .local pmc a
    a = new 'Integer'
    a = 10
    .local pmc b
    b = clone a
    print b
    print "\n"
    end
.end
CODE
10
OUT

pir_output_is( <<'CODE', <<'OUT', "x = length" );
.sub test :main
    .local string s
    s = "abc"
    $I0 = length s
    print $I0
    print "\n"

    s = ""
    $I0 = length s
    print $I0
    print "\n"
    end
.end
CODE
3
0
OUT

pir_output_is( <<'CODE', <<'OUT', "x = sin" );
.sub test :main
    $N0 = sin 0
    print $N0
    print "\n"
    end
.end
CODE
0
OUT

pir_output_is( <<'CODE', <<'OUT', "x = can" );
.sub test :main
    $P0 = new 'FileHandle'
    $I0 = can $P0, "puts"
    print $I0
    print "\n"
    end
.end
CODE
1
OUT

pir_output_is( <<'CODE', <<'OUT', "x = isa" );
.sub test :main
    $P0 = new 'Integer'
    $I0 = isa $P0, "scalar"
    print $I0
    print "\n"
    end
.end
CODE
1
OUT

pir_output_is( <<'CODE', <<'OUT', "x = add" );
.sub test :main
    $I0 = 10
    $I1 = add $I0, 10
    print $I1
    print "\n"
    end
.end
CODE
20
OUT

pir_output_is( <<'CODE', <<'OUT', "x = invoke" );
.sub test :main
    $P0 = get_global "_s"
    $P0 = invokecc
    $S0 = "done\n"
    $S0 = print
    end
.end
.sub _s
    print "in sub\n"
    returncc
.end
CODE
in sub
done
OUT

# ticket 32393
pir_output_is( <<'CODE', '', "empty sub" );
.sub _foo
.end

.sub _foo :anon
.end
CODE

pir_output_is( <<'CODE', <<'OUT', "if null X goto Y" );
.sub main :main
    null $P0
    if null $P0 goto BLAH
    print "NOT A "
BLAH:
   print "PASS\n"
.end
CODE
PASS
OUT

pir_output_is( <<'CODE', <<'OUT', "unless null X goto Y" );
.sub main :main
    null $P0
    unless null $P0 goto BLAH
    print "PASS\n"
    end
BLAH:
   print "FAIL"
.end
CODE
PASS
OUT

pir_output_is( <<'CODE', <<'OUT', "if null X goto Y" );
.sub main :main
    $S0 = "hello"
    if null $S0 goto BLAH
    print "PASS\n"
    end
BLAH:
   print "FAIL"
.end
CODE
PASS
OUT

pir_output_is( <<'CODE', <<'OUT', 'unless null X goto Y, $P0 = null' );
.sub main :main
    $P0 = null
    unless null $P0 goto BLAH
    print "PASS\n"
    end
BLAH:
   print "FAIL"
.end
CODE
PASS
OUT

pir_output_is( <<'CODE', <<'OUT', 'X = A == B' );
.sub main :main
    $I0 = 1 == 1
    print $I0
    print "\n"

    $I0 = 1 == 0
    print $I0
    print "\n"
.end
CODE
1
0
OUT

pir_output_is( <<'CODE', <<'OUT', 'X = A < B' );
.sub main :main
    $I0 = 1 < 1
    print $I0
    print "\n"

    $I0 = 0 < 1
    print $I0
    print "\n"
.end
CODE
0
1
OUT

pir_output_is( <<'CODE', <<'OUT', 'X = A > B' );
.sub main :main
    $I0 = 1 > 1
    print $I0
    print "\n"

    $I0 = 4 > 1
    print $I0
    print "\n"
.end
CODE
0
1
OUT

pir_output_is( <<'CODE', <<'OUT', 'X = A >= B' );
.sub main :main
    $I0 = 1 >= 1
    print $I0
    print "\n"

    $I0 = 4 >= 1
    print $I0
    print "\n"

    $I0 = 0 >= 1
    print $I0
    print "\n"
.end
CODE
1
1
0
OUT

pir_output_is( <<'CODE', <<'OUT', 'X = A <= B' );
.sub main :main
    $I0 = 1 <= 1
    print $I0
    print "\n"

    $I0 = 4 <= 1
    print $I0
    print "\n"

    $I0 = 0 <= 1
    print $I0
    print "\n"
.end
CODE
1
0
1
OUT

pir_output_is( <<'CODE', <<'OUT', 'X = A != B' );
.sub main :main
    $I0 = 1 != 1
    print $I0
    print "\n"

    $I0 = 4 != 1
    print $I0
    print "\n"
.end
CODE
0
1
OUT

pir_output_is( <<'CODE', <<'OUT', 'Test octal/hex/bin/dec numbers' );
.sub main :main
    $I0 = 0077
    print $I0
    print "\n"

    $I0 = 0o77
    print $I0
    print "\n"

    $I0 = 0xfF
    print $I0
    print "\n"

    $I0 = 0b1101
    print $I0
    print "\n"

    $I0 = 0O10
    print $I0
    print "\n"

    $I0 = 0X10
    print $I0
    print "\n"

    $I0 = 0B10
    print $I0
    print "\n"

    $I0 = 10
    print $I0
    print "\n"

.end
CODE
77
63
255
13
8
16
2
10
OUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
