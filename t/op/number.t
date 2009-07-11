#!perl
# Copyright (C) 2001-2008, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 56;
use Parrot::Config;

=head1 NAME

t/op/number.t - Number Registers

=head1 SYNOPSIS

    % prove t/op/number.t

=head1 DESCRIPTION

Tests the use of Parrot floating-point number registers.

=cut

my $output;

pasm_output_is( <<CODE, <<OUTPUT, "set_n_nc" );
        set     N0, 1.0
        set     N1, 4.0
        set     N2, 16.0
        set     N3, 64.0
        set     N4, 256.0
        set     N5, 1024.0
        set     N6, 4096.0
        set     N7, 16384.0
        set     N8, 65536.0
        set     N9, 262144.0
        set     N10, 1048576.0
        set     N11, 4194304.0
        set     N12, 16777216.0
        set     N13, 67108864.0
        set     N14, 268435456.0
        set     N15, 1073741824.0
        set     N16, 4294967296.0
        set     N17, 17179869184.0
        set     N18, 68719476736.0
        set     N19, 274877906944.0
        set     N20, 1099511627776.0
        set     N21, 4398046511104.0
        set     N22, 17592186044416.0
        set     N23, 70368744177664.0
        set     N24, 281474976710656.0
        set     N25, 1.12589990684262e+15

        print   N0
        print   "\\n"
        print   N1
        print   "\\n"
        print   N2
        print   "\\n"
        print   N3
        print   "\\n"
        print   N4
        print   "\\n"
        print   N5
        print   "\\n"
        print   N6
        print   "\\n"
        print   N7
        print   "\\n"
        print   N8
        print   "\\n"
        print   N9
        print   "\\n"
        print   N10
        print   "\\n"
        print   N11
        print   "\\n"
        print   N12
        print   "\\n"
        print   N13
        print   "\\n"
        print   N14
        print   "\\n"
        print   N15
        print   "\\n"
        print   N16
        print   "\\n"
        print   N17
        print   "\\n"
        print   N18
        print   "\\n"
        print   N19
        print   "\\n"
        print   N20
        print   "\\n"
        print   N21
        print   "\\n"
        print   N22
        print   "\\n"
        print   N23
        print   "\\n"
        print   N24
        print   "\\n"
        print   N25
        print   "\\n"
        end
CODE
1
4
16
64
256
1024
4096
16384
65536
262144
1048576
4194304
16777216
67108864
268435456
1073741824
4294967296
17179869184
68719476736
274877906944
1099511627776
4398046511104
17592186044416
70368744177664
281474976710656
1.12589990684262e+15
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "set_n" );
        set     N0, 42.0
        set     N1, N0
        print   N1
        print   "\\n"
        end
CODE
42
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "add_n_n_n" );
        set     N0, 1.0
        add     N1, N0, N0
        print   N1
        print   "\\n"

        add     N2, N0, N1
        print   N2
        print   "\\n"

        add     N2, N2, N2
        print   N2
        print   "\\n"
        end
CODE
2
3
6
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "add_n_n" );
        set     N0, 1.0
        add     N0, N0
        print   N0
        print   "\\n"

        set     N1, 1.0
        add     N0, N1
        print   N0
        print   "\\n"

        add     N0, 3.0
        print   N0
        print   "\\n"
        end
CODE
2
3
6
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "sub_n_n_n" );
        set     N0, 424242.0
        set     N1, 4200.0
        sub     N2, N0, N1
        print   N2
        print   "\\n"
        end
CODE
420042
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "sub_n_n" );
        set     N0, 424242.0
        set     N1, 4200.0
        sub     N0, N1
        print   N0
        print   "\\n"

        sub     N0, N0
        print   N0
        print   "\\n"
        end
CODE
420042
0
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "abs(n, i|ic|n|nc)" );
        set     I0, -1
        abs     N0, I0
        abs     N1, -1
        set     I1, 1
        abs     N2, I1
        abs     N3, 1
        set     N4, -1
        abs     N4, N4
        abs     N5, -1.0
        set     N6, 1.0
        abs     N6, N6
        abs     N7, 1.0

        print N0
        print "\n"
        print N1
        print "\n"
        print N2
        print "\n"
        print N3
        print "\n"
        print N4
        print "\n"
        print N5
        print "\n"
        print N6
        print "\n"
        print N7
        print "\n"
        end
CODE
1
1
1
1
1
1
1
1
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "mul_i" );
        set     N0, 2.0
        mul     N1, N0, N0
        mul     N1, N1, N0
        mul     N1, N1, N0
        mul     N1, N1, N0
        mul     N1, N1, N0
        mul     N1, N1, N0
        mul     N1, N1, N0
        print   N1
        print   "\\n"
        end
CODE
256
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "div_i" );
        set     N0, 10.0
        set     N1, 2.0
        div     N2, N0, N1
        print   N2
        print   "\\n"

        set     N3, 7.0
        set     N4, 2.0
        div     N3, N3, N4
        print   N3
        print   "\\n"

        set     N5, 9.0
        set     N6, -4.0
        div     N7, N5, N6
        print   N7
        print   "\\n"
        end
CODE
5
3.5
-2.25
OUTPUT

pasm_output_like( <<CODE, <<OUTPUT, "mod_n" );
        set     N0, 5.0
        set     N1, 0.0
        mod     N2, N0, N1
        print   N2
        print   "\\n"

        set     N0, 0.0
        set     N1, 3.0
        mod     N2, N0, N1
        print   N2
        print   "\\n"

        set     N0, 5.0
        set     N1, 3.0
        mod     N2, N0, N1
        print   N2
        print   "\\n"

        set     N0, 5.0
        set     N1, -3.0
        mod     N2, N0, N1
        print   N2
        print   "\\n"

        set     N0, -5.0
        set     N1, 3.0
        mod     N2, N0, N1
        print   N2
        print   "\\n"

        set     N0, -5.0
        set     N1, -3.0
        mod     N2, N0, N1
        print   N2
        print   "\\n"

        end
CODE
/5\
-?0\
2\
-1\
1\
-2\
/
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "cmod_n" );
        set     N0, 5.000
        set     N1, 3.000
        cmod    N2, N0, N1
        print   N2
        print   "\\n"

        end
CODE
2
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "eq_n_ic" );
        set     N0, 5.000001
        set     N1, 5.000001
        set     N2, 5.000002

        eq      N0, N1, ONE
        branch  ERROR
        print   "bad\\n"

ONE:
        print   "ok 1\\n"
        eq      N1, N2, ERROR
        branch  TWO
        print   "bad\\n"

TWO:
        print   "ok 2\\n"
        end

ERROR:
        print   "bad\\n"
        end
CODE
ok 1
ok 2
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "eq_nc_ic" );
        set     N0, 1.000001

        eq      N0, 1, ERROR
        branch  ONE
        print   "bad\\n"

ONE:
        print   "ok 1\\n"
        eq      N0, 1.000001, TWO
        branch  ERROR
        print   "bad\\n"

TWO:
        print   "ok 2\\n"
        end

ERROR:
        print   "bad\\n"
        end
CODE
ok 1
ok 2
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "ne_n_ic" );
        set     N0, -22.222222
        set     N1, -22.222222
        set     N2, 0.0

        ne      N0, N2, ONE
        branch  ERROR
        print   "bad\\n"

ONE:
        print   "ok 1\\n"
        ne      N0, N1, ERROR
        branch  TWO
        print   "bad\\n"

TWO:
        print   "ok 2\\n"
        end

ERROR:
        print   "bad\\n"
        end
CODE
ok 1
ok 2
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "ne_n_nc_ic" );
        set     N0, 1073741824.0
        ne      N0, 1073741824.0, nok1
        print   "ok 1\\n"
        branch  ONE
nok1:
        print   "bad 1\\n"
ONE:
        ne      N0, 0.0, TWO
        branch  ERROR
TWO:
        print   "ok 2\\n"
        end
ERROR:
        print   "bad 2\\n"
        end
CODE
ok 1
ok 2
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "lt_n_ic" );
        set     N0, 1000.0
        set     N1, 500.0
        set     N2, 0.0
        set     N3, 0.0

        lt      N1, N0, ONE
        branch  ERROR
        print   "bad\\n"

ONE:
        print   "ok 1\\n"
        lt      N0, N1, ERROR
        branch  TWO
        print   "bad\\n"

TWO:
        print   "ok 2\\n"
        lt      N2, N3, ERROR
        branch  THREE
        print   "bad\\n"

THREE:
        print   "ok 3\\n"
        end

ERROR:
        print "bad\\n"
        end
CODE
ok 1
ok 2
ok 3
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "lt_nc_ic" );
        set     N0, 1000.0
        set     N1, 500.0
        set     N2, 0.0

        lt      N0, 500.0, ERROR
        branch  ONE
        print   "bad\\n"

ONE:
        print   "ok 1\\n"
        lt      N1, 1000.0, TWO
        branch  ERROR
        print   "bad\\n"

TWO:
        print   "ok 2\\n"
        lt      N0, 0.0, ERROR
        branch  THREE
        print   "bad\\n"

THREE:
        print   "ok 3\\n"
        end

ERROR:
        print   "bad\\n"
        end
CODE
ok 1
ok 2
ok 3
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "le_n_ic" );
        set     N0, 1000.0
        set     N1, 500.0
        set     N2, 0.0
        set     N3, 0.0

        le      N1, N0, ONE
        branch  ERROR
        print   "bad\\n"

ONE:
        print   "ok 1\\n"
        le      N0, N1, ERROR
        branch  TWO
        print   "bad\\n"

TWO:
        print   "ok 2\\n"
        le      N2, N3, THREE
        branch  ERROR
        print   "bad\\n"

THREE:
        print   "ok 3\\n"
        end

ERROR:
        print "bad\\n"
        end
CODE
ok 1
ok 2
ok 3
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "le_nc_ic" );
        set     N0, 1000.0
        set     N1, 500.0
        set     N2, 0.0

        le      N0, 500.0, ERROR
        branch  ONE
        print   "bad\\n"

ONE:
        print   "ok 1\\n"
        le      N1, 1000.0, TWO
        branch  ERROR
        print   "bad\\n"

TWO:
        print   "ok 2\\n"
        le      N2, 0.0, THREE
        branch  ERROR
        print   "bad\\n"

THREE:
        print   "ok 3\\n"
        end

ERROR:
        print   "bad\\n"
        end
CODE
ok 1
ok 2
ok 3
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "gt_n_ic" );
        set     N0, 500.0
        set     N1, 1000.0
        set     N2, 0.0
        set     N3, 0.0

        gt      N1, N0, ONE
        branch  ERROR
        print   "bad\\n"

ONE:
        print   "ok 1\\n"
        gt      N0, N1, ERROR
        branch  TWO
        print   "bad\\n"

TWO:
        print   "ok 2\\n"
        gt      N2, N3, ERROR
        branch  THREE
        print   "bad\\n"

THREE:
        print   "ok 3\\n"
        end

ERROR:
        print "bad\\n"
        end
CODE
ok 1
ok 2
ok 3
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "gt_nc_ic" );
        set     N0, 500.0
        set     N1, 1000.0
        set     N2, 0.0

        gt      N0, 1000.0, ERROR
        branch  ONE
        print   "bad\\n"

ONE:
        print   "ok 1\\n"
        gt      N1, 500.0, TWO
        branch  ERROR
        print   "bad\\n"

TWO:
        print   "ok 2\\n"
        gt      N2, 0.0, ERROR
        branch  THREE
        print   "bad\\n"

THREE:
        print   "ok 3\\n"
        end

ERROR:
        print   "bad\\n"
        end
CODE
ok 1
ok 2
ok 3
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "ge_n_ic" );
        set     N0, 500.0
        set     N1, 1000.0
        set     N2, 0.0
        set     N3, 0.0

        ge      N1, N0, ONE
        branch  ERROR
        print   "bad\\n"

ONE:
        print   "ok 1\\n"
        ge      N0, N1, ERROR
        branch  TWO
        print   "bad\\n"

TWO:
        print   "ok 2\\n"
        ge      N2, N3, THREE
        branch  ERROR
        print   "bad\\n"

THREE:
        print   "ok 3\\n"
        end

ERROR:
        print "bad\\n"
        end
CODE
ok 1
ok 2
ok 3
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "ge_nc_ic" );
        set     N0, 500.0
        set     N1, 1000.0
        set     N2, 0.0

        ge      N0, 1000.0, ERROR
        branch  ONE
        print   "bad\\n"

ONE:
        print   "ok 1\\n"
        ge      N1, 500.0, TWO
        branch  ERROR
        print   "bad\\n"

TWO:
        print   "ok 2\\n"
        ge      N2, 0.0, THREE
        branch  ERROR
        print   "bad\\n"

THREE:
        print   "ok 3\\n"
        end

ERROR:
        print   "bad\\n"
        end
CODE
ok 1
ok 2
ok 3
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "if_n_ic" );
        set     N0, 1000.0
        set     N1, 500.0
        set     N2, 0.0

        if      N0, ONE
        branch  ERROR
        print   "bad\\n"

ONE:
        print   "ok 1\\n"
        if      N1, TWO
        branch  ERROR
        print   "bad\\n"

TWO:
        print   "ok 2\\n"
        if      N2, ERROR
        branch  THREE
        print   "bad\\n"

THREE:
        print   "ok 3\\n"
        end

ERROR:
        print   "bad\\n"
        end
CODE
ok 1
ok 2
ok 3
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "inc_n" );
        set     N0, 0.0

        inc     N0
        print   N0
        print   "\\n"

        inc     N0
        inc     N0
        inc     N0
        inc     N0
        print   N0
        print   "\\n"
        end
CODE
1
5
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "dec_n" );
        set     N0, 0.0

        dec     N0
        print   N0
        print   "\\n"

        dec     N0
        dec     N0
        dec     N0
        dec     N0
        print   N0
        print   "\\n"
        end
CODE
-1
-5
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "set_i_n" );
        set     N0, 0.0
        set     I0, N0
        print   I0
        print   "\\n"

        set     N1, 2147483647.0
        set     I1, N1
        print   I1
        print   "\\n"

        set     N2, -2147483648.0
        set     I2, N2
        print   I2
        print   "\\n"
        end
CODE
0
2147483647
-2147483648
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "clearn" );
        set     N0, 547972.0
        set     N1, 547972.0
        set     N2, 547972.0
        set     N3, 547972.0
        set     N4, 547972.0
        set     N5, 547972.0
        set     N6, 547972.0
        set     N7, 547972.0
        set     N8, 547972.0
        set     N9, 547972.0
        set     N10, 547972.0
        set     N11, 547972.0
        set     N12, 547972.0
        set     N13, 547972.0
        set     N14, 547972.0
        set     N15, 547972.0
        set     N16, 547972.0
        set     N17, 547972.0
        set     N18, 547972.0
        set     N19, 547972.0
        set     N20, 547972.0
        set     N21, 547972.0
        set     N22, 547972.0
        set     N23, 547972.0
        set     N24, 547972.0
        set     N25, 547972.0
        set     N26, 547972.0
        set     N27, 547972.0
        set     N28, 547972.0
        set     N29, 547972.0
        set     N30, 547972.0
        set     N31, 547972.0
        clearn
        print   N0
        print   N1
        print   N2
        print   N3
        print   N4
        print   N5
        print   N6
        print   N7
        print   N8
        print   N9
        print   N10
        print   N11
        print   N12
        print   N13
        print   N14
        print   N15
        print   N16
        print   N17
        print   N18
        print   N19
        print   N20
        print   N21
        print   N22
        print   N23
        print   N24
        print   N25
        print   N26
        print   N27
        print   N28
        print   N29
        print   N30
        print   N31
        print   "\\n"
        end
CODE
00000000000000000000000000000000
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "neg_n" );
    neg N0,3.0
    neg N0,N0
    neg N0
    print N0
    print "\\n"
    end
CODE
-3
OUTPUT

pasm_output_like( <<CODE, <<OUTPUT, "neg 0.0" );
    set N1, 0
    neg N1
    print N1
    print "\\n"
    end
CODE
/-?0\$/
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "mul_n_n" );
    set N0,3.0
    set N1,4.0
    mul N0,N1
    print N0
    print "\\n"
    end
CODE
12
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "fact_n_i" );
    set I0, 3
    set I1, 11
    set I2, 0
    set I3, -563
    fact N5, I0
    print N5
    print "\\n"
    fact N6, I1
    print N6
    print "\\n"
    fact N7, I2
    print N7
    print "\\n"
    fact N8, I3
    print N8
    print "\\n"
    end
CODE
6
39916800
1
1
OUTPUT

pasm_output_is( <<CODE, <<OUTPUT, "fact_n_ic" );
    fact N5, 3
    print N5
    print "\\n"
    fact N6, 11
    print N6
    print "\\n"
    fact N7, 0
    print N7
    print "\\n"
    fact N8, -563
    print N8
    print "\\n"
    end
CODE
6
39916800
1
1
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "exchange" );
    set N1, 1.234560
    set N2, 9.876540
    exchange N1, N2
    print N1
    print "\n"
    print N2
    print "\n"

    set N3, -100.200300
    exchange N3, N3
    print N3
    print "\n"
    end
CODE
9.87654
1.23456
-100.2003
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "op_n_nc_nc" );
    add N1, 2.0, 3.0
    print N1
    print "\n"
    sub N1, 2.0, 4.0
    print N1
    print "\n"
    end
CODE
5
-2
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "lt_nc_nc_ic" );
    lt 2.0, 1.0, nok
    print "ok 1\n"
    lt 3.0, 4.0, ok_2
nok:
    print "nok\n"
    end
ok_2:
    print "ok 2\n"
    end
CODE
ok 1
ok 2
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "string -> num" );
    set S0, "1"
    set S1, "12.0"
    set S2, "-2.45"
    set S3, "25e2"
    set S4, "Banana"

    set N0, S0
    set N1, S1
    set N2, S2
    set N3, S3
    set N4, S4

    print N0
    print "\n"
    print N1
    print "\n"
    print N2
    print "\n"
    print N3
    print "\n"
    print N4
    print "\n"

    end
CODE
1
12
-2.45
2500
0
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "null" );
    set N31, 12.5
    print N31
    print "\n"

    null N31
    print N31
    print "\n"

    end
CODE
12.5
0
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, ".dig parsing" );
   set N0, .5
   print N0
   print "\n"
   end
CODE
0.5
OUTPUT

# Don't check exact string representation. Last digit part can be different */
pasm_output_like( <<'CODE', <<OUTPUT, "sqrt_n_n" );
        set N1, 2
        sqrt N2, N1
        say N2
        sqrt N2, 2.0
        say N2
        end
CODE
/^1.414213562373.*\n1.414213562373.*/
OUTPUT

pasm_error_output_like( <<'CODE', <<OUTPUT, "div_n_n by zero" );
        set N0, 0
        set N1, 10
        div N1, N0
        end
CODE
/.*Divide by zero.*/
OUTPUT

pasm_error_output_like( <<'CODE', <<OUTPUT, "div_n_nc by zero" );
        set N1, 10
        div N1, 0
        end
CODE
/.*Divide by zero.*/
OUTPUT

pasm_error_output_like( <<'CODE', <<OUTPUT, "div_n_n_n by zero" );
        set N0, 0
        set N1, 10
        div N2, N1, N0
        end
CODE
/.*Divide by zero.*/
OUTPUT

pasm_error_output_like( <<'CODE', <<OUTPUT, "div_n_nc_n by zero" );
        set N0, 0
        div N2, 10, N0
        end
CODE
/.*Divide by zero.*/
OUTPUT

pasm_error_output_like( <<'CODE', <<OUTPUT, "div_n_n_nc by zero" );
        set N1, 10
        div N2, N1, 0
        end
CODE
/.*Divide by zero.*/
OUTPUT

pasm_error_output_like( <<'CODE', <<OUTPUT, "fdiv_n_n by zero" );
        set N0, 0
        set N1, 10
        fdiv N1, N0
        end
CODE
/.*Divide by zero.*/
OUTPUT

pasm_error_output_like( <<'CODE', <<OUTPUT, "fdiv_n_nc by zero" );
        set N1, 10
        fdiv N1, 0
        end
CODE
/.*Divide by zero.*/
OUTPUT

pasm_error_output_like( <<'CODE', <<OUTPUT, "fdiv_n_n_n by zero" );
        set N0, 0
        set N1, 10
        fdiv N2, N1, N0
        end
CODE
/.*Divide by zero.*/
OUTPUT

pasm_error_output_like( <<'CODE', <<OUTPUT, "fdiv_n_nc_n by zero" );
        set N0, 0
        fdiv N2, 10, N0
        end
CODE
/.*Divide by zero.*/
OUTPUT

pasm_error_output_like( <<'CODE', <<OUTPUT, "fdiv_n_n_nc by zero" );
        set N1, 10
        fdiv N2, N1, 0
        end
CODE
/.*Divide by zero.*/
OUTPUT

pasm_error_output_like( <<'CODE', <<OUTPUT, "cmod_n_n_n by zero" );
        set N0, 0
        set N1, 10
        cmod N2, N1, N0
        end
CODE
/.*Divide by zero.*/
OUTPUT

pasm_error_output_like( <<'CODE', <<OUTPUT, "cmod_n_nc_n by zero" );
        set N0, 0
        cmod N2, 10, N0
        end
CODE
/.*Divide by zero.*/
OUTPUT

pasm_error_output_like( <<'CODE', <<OUTPUT, "cmod_n_n_nc by zero" );
        set N1, 10
        cmod N2, N1, 0
        end
CODE
/.*Divide by zero.*/
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "mod_n_n_n by zero" );
        set N0, 0
        set N1, 10
        mod N2, N1, N0
        end
CODE
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "mod_n_nc_n by zero" );
        set N0, 0
        mod N2, 10, N0
        end
CODE
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "mod_n_n_nc by zero" );
        set N1, 10
        mod N2, N1, 0
        end
CODE
OUTPUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
