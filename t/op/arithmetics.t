#!perl
# Copyright (C) 2001-2009, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );

use Test::More;
use Parrot::Test tests => 21;

# test for GMP
use Parrot::Config;

=head1 NAME

t/op/arithmetics.t - Arithmetic Ops

=head1 SYNOPSIS

        % prove t/op/arithmetics.t

=head1 DESCRIPTION

Tests basic arithmetic on various combinations of Parrot integer and
number types.

=cut

#
# Operations on a single INTVAL
#
pasm_output_is( <<'CODE', <<OUTPUT, "take the negative of a native integer" );
        set I0, 0
        neg I0
        say I0
        set I0, 1234567890
        neg I0
        say I0
        set I0, -1234567890
        neg I0
        say I0
        set I0, 0
        set I1, 0
        neg I1, I0
        say I1
        set I0, 1234567890
        neg I1, I0
        say I1
        set I0, -1234567890
        neg I1, I0
        say I1
        end
CODE
0
-1234567890
1234567890
0
-1234567890
1234567890
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "take the absolute of a native integer" );
        set I0, 0
        abs I0
        say I0
        set I0, 1234567890
        abs I0
        say I0
        set I0, -1234567890
        abs I0
        say I0
        set I0, 0
        set I1, 0
        abs I1, I0
        say I1
        set I0, 1234567890
        abs I1, I0
        say I1
        set I0, -1234567890
        abs I1, I0
        say I1
        end
CODE
0
1234567890
1234567890
0
1234567890
1234567890
OUTPUT

#
# first arg is INTVAL, second arg is INTVAL
#
pasm_output_is( <<'CODE', <<OUTPUT, "add native integer to native integer" );
        set I0, 4000
        set I1, -123
        add I2, I0, I1
        say I2
        add I0, I0, I1
        say I0
        end
CODE
3877
3877
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "subtract native integer from native integer" );
        set I0, 4000
        set I1, -123
        sub I2, I0, I1
        say I2
        sub I0, I0, I1
        say I0
        end
CODE
4123
4123
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "multiply native integer with native integer" );
        set I0, 4000
        set I1, -123
        mul I2, I0, I1
        say I2
        mul I0, I0, I1
        say I0
        end
CODE
-492000
-492000
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "divide native integer by native integer" );
        set I0, 4000
        set I1, -123
        div I2, I0, I1
        say I2
        div I0, I0, I1
        say I0
        end
CODE
-32
-32
OUTPUT

#
# print -0.0 as -0
#

pasm_output_is( <<'CODE', <<OUTPUT, 'negate -0.0' );
        set N0, 0
        neg N0
        say N0
        set N0, -0.0
        neg N0
        say N0
        set N0, -0.0
        neg N1, N0
        say N1
        set N0, 0
        set N1, 1
        neg N1, N0
        say N1
        end
CODE
-0
0
0
-0
OUTPUT



#
# Operations on a single NUMVAL
#

pasm_output_is( <<'CODE', <<OUTPUT, 'negate a native number' );
        set N0, 123.4567890
        neg N0
        say N0
        set N0, -123.4567890
        neg N0
        say N0
        set N0, 123.4567890
        neg N1, N0
        say N1
        set N0, -123.4567890
        neg N1, N0
        say N1
        end
CODE
-123.456789
123.456789
-123.456789
123.456789
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "take the absolute of a native number" );
        set N0, 0
        abs N0
        say N0
        set N0, -0.0
        abs N0
        say N0
        set N0, 123.45678901
        abs N0
        say N0
        set N0, -123.45678901
        abs N0
        say N0
        set N0, 0
        set N1, 1
        abs N1, N0
        say N1
        set N0, 0.0
        set N1, 1
        abs N1, N0
        say N1
        set N0, 123.45678901
        set N1, 1
        abs N1, N0
        say N1
        set N0, -123.45678901
        set N1, 1
        abs N1, N0
        say N1
        end
CODE
0
0
123.45678901
123.45678901
0
0
123.45678901
123.45678901
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "ceil of a native number" );
       set N0, 0
       ceil N0
       say N0
       set N0, 123.45678901
       ceil N0
       say N0
       set N0, -123.45678901
       ceil N0
       say N0
       set N0, 0
       set N1, 1
       ceil N1, N0
       say N1
       set N0, 0.0
       set N1, 1
       ceil N1, N0
       say N1
       set N0, 123.45678901
       set N1, 1
       ceil N1, N0
       say N1
       set N0, -123.45678901
       set N1, 1
       ceil N1, N0
       say N1
       set N0, 0
       set I1, 1
       ceil I1, N0
       say I1
       set N0, 0.0
       set I1, 1
       ceil I1, N0
       say I1
       set N0, 123.45678901
       set I1, 1
       ceil I1, N0
       say I1
       set N0, -123.45678901
       set I1, 1
       ceil I1, N0
       say I1
       end
CODE
0
124
-123
0
0
124
-123
0
0
124
-123
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "floor of a native number" );
       set N0, 0
       floor N0
       say N0
       set N0, 123.45678901
       floor N0
       say N0
       set N0, -123.45678901
       floor N0
       say N0
       set N0, 0
       set N1, 1
       floor N1, N0
       say N1
       set N0, 0.0
       set N1, 1
       floor N1, N0
       say N1
       set N0, 123.45678901
       set N1, 1
       floor N1, N0
       say N1
       set N0, -123.45678901
       set N1, 1
       floor N1, N0
       say N1
       set N0, 0
       set I1, 1
       floor I1, N0
       say I1
       set N0, 0.0
       set I1, 1
       floor I1, N0
       say I1
       set N0, 123.45678901
       set I1, 1
       floor I1, N0
       say I1
       set N0, -123.45678901
       set I1, 1
       floor I1, N0
       say I1
       end
CODE
0
123
-124
0
0
123
-124
0
0
123
-124
OUTPUT

#
# FLOATVAL and INTVAL tests
#
pasm_output_is( <<'CODE', <<OUTPUT, "add native integer to native number" );
        set I0, 4000
        set N0, -123.123
        add N1, N0, I0
        say N1
        add N0, N0, I0
        say N0
        add N0, I0
        say N0
        end
CODE
3876.877
3876.877
7876.877
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "subtract native integer from native number" );
        set I0, 4000
        set N0, -123.123
        sub N1, N0, I0
        say N1
        sub N0, N0, I0
        say N0
        sub N0, I0
        say N0
        end
CODE
-4123.123
-4123.123
-8123.123
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "multiply native number with native integer" );
        set I0, 4000
        set N0, -123.123
        mul N1, N0, I0
        say N1
        mul N0, N0, I0
        say N0
        mul N0, -2
        say N0
        end
CODE
-492492
-492492
984984
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "divide native number by native integer" );
        set I0, 4000
        set N0, -123.123
        div N1, N0, I0
        say N1
        div N0, N0, I0
        say N0
        div N0, 1
        say N0
        set N0, 100.000
        div N0, 100
        say N0
        div N0, 0.01
        say N0
        end
CODE
-0.03078075
-0.03078075
-0.03078075
1
100
OUTPUT

#
# FLOATVAL and FLOATVAL tests
#
pasm_output_is( <<'CODE', <<OUTPUT, "add native number to native number" );
        set N2, 4000.246
        set N0, -123.123
        add N1, N0, N2
        say N1
        add N0, N0, N2
        say N0
        end
CODE
3877.123
3877.123
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "subtract native number from native number" );
        set N2, 4000.246
        set N0, -123.123
        sub N1, N0, N2
        say N1
        sub N0, N0, N2
        say N0
        end
CODE
-4123.369
-4123.369
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "multiply native number with native number" );
        set N2, 4000.246
        set N0, -123.123
        mul N1, N0, N2
        say N1
        mul N0, N0, N2
        say N0
        end
CODE
-492522.288258
-492522.288258
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "divide native number by native number" );
        set N2, 4000.246
        set N0, -123.123
        div N1, N0, N2
        say N1
        div N0, N0, N2
        say N0
        end
CODE
-0.0307788571002883
-0.0307788571002883
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "lcm_I_I_I" );
        set I0, 10
        set I1, 10
        lcm I2, I1, I0
        eq I2, 10, OK1
        print "not "
OK1:    say "ok 1"

        set I1, 17
        lcm I2, I1, I0
        eq I2, 170, OK2
        print I2
        print "not "
OK2:    print "ok 2\n"

        set I0, 17
        set I1, 10
        lcm I2, I1, I0
        eq I2, 170, OK3
        print "not "
OK3:    print "ok 3\n"

        set I0, 10
        set I1, 0
        lcm I2, I1, I0
        eq I2, 0, OK4
        print "not "
OK4:    print "ok 4\n"

        set I0, 0
        set I1, 10
        lcm I2, I1, I0
        eq I2, 0, OK5
        print "not "
OK5:    print "ok 5\n"

        end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
OUTPUT

SKIP: {
    skip( 'No integer overflow for 32-bit INTVALs without GMP installed', 1 )
        if $PConfig{intvalsize} == 4 && !$PConfig{gmp};

    pir_output_is( <<'CODE', <<OUTPUT, "integer overflow with 'pow'" );
.sub main
    .local pmc i1, i2, r
    i1 = new 'Integer'
    i2 = new 'Integer'
    i1 = 2
    i2 = 1
next:
    null r
    r = pow i1, i2
    $S0 = r
    say $S0
    inc i2
# XXX: this must be extended to at least 64 bit range
# when sure that the result is not floating point.
# In the meantime, make sure it overflows nicely
# on 32 bit.
    unless i2 > 40 goto next
.end
CODE
2
4
8
16
32
64
128
256
512
1024
2048
4096
8192
16384
32768
65536
131072
262144
524288
1048576
2097152
4194304
8388608
16777216
33554432
67108864
134217728
268435456
536870912
1073741824
2147483648
4294967296
8589934592
17179869184
34359738368
68719476736
137438953472
274877906944
549755813888
1099511627776
OUTPUT
}


# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
