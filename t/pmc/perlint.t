#! perl -w

use Parrot::Test tests => 19;
use Parrot::PMC '%pmc_types';
my $perlint = $pmc_types{'PerlInt'};
my $ok = '"ok 1\n"';
my $fp_equality_macro = <<'ENDOFMACRO';
.macro fp_eq (	J, K, L )
	save	N0
	save	N1
	save	N2

	set	N0, .J
	set	N1, .K
	sub	N2, N1,N0
	abs	N2, N2
	gt	N2, 0.000001, .$FPEQNOK

	restore N2
	restore	N1
	restore	N0
	branch	.L
.local $FPEQNOK:
	restore N2
	restore	N1
	restore	N0
.endm
.macro fp_ne(	J,K,L)
	save	N0
	save	N1
	save	N2

	set	N0, .J
	set	N1, .K
	sub	N2, N1,N0
	abs	N2, N2
	lt	N2, 0.000001, .$FPNENOK

	restore	N2
	restore	N1
	restore	N0
	branch	.L
.local $FPNENOK:
	restore	N2
	restore	N1
	restore	N0
.endm
ENDOFMACRO


output_is(<<"CODE", <<'OUTPUT', "type");
    new P0,.PerlInt
# type
    typeof I0,P0
    eq I0,$perlint,ok_1
    print "not "
ok_1:
    print $ok
    end
CODE
ok 1
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "inheritance");
    new P0,.PerlInt
# clone
    set P0, 10
    clone P1, P0
    set P1, 20
    set I0, P0
    eq I0,10,ok_1
    print "not "
ok_1:
    print "ok 1\n"
    set I0, P1
    eq I0,20,ok_2
    print "not "
ok_2:
    print "ok 2\n"
    end
CODE
ok 1
ok 2
OUTPUT

SKIP: { skip("add_keyed: not yet", 1);
output_is(<<'CODE', <<'OUTPUT', "add_keyed");
# add keyed
    new P2, .PerlArray
    set P2[10], 10
    set P0, 20
    new P11, .Key
    new P12, .Key
    new P13, .Key
    set P12,10
    add P1[P11],P2[P12],P0[P13]
    set I1,P1
    eq I1,30,ok_3
    print "not "
ok_3:
    print "ok 1\n"

    end
CODE
ok 1
OUTPUT
}

output_is(<<'CODE', <<'OUTPUT', "bor");
    new P0, .PerlInt
    set P0, 0b11110000
    bor P0, 0b00001111
    print P0
    print "\n"

    new P1, .PerlInt
    set P0, 0
    set P1, 12
    bor P0, P1
    print P0
    print "\n"

    new P1, .PerlNum
    set P1, 47.11
    set P0, 7
    bor P1, P0, 8
    print P1
    print "\n"

    new P2, .PerlString
    set P2, "String"
    set P0, 128
    set P1, 1
    bor P2, P0, P1
    print P2
    print "\n"

    new P4, .PerlUndef
    set P0, 12
    set P1, 24
    bor P4, P0, P1
    print P4
    print "\n"
    end
CODE
255
12
15
129
28
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "bxor");
    new P0, .PerlInt
    set P0, 0b11111000
    bxor P0, 0b00011111
    print P0
    print "\n"

    new P1, .PerlInt
    set P0, 16
    set P1, 31
    bxor P0, P1
    print P0
    print "\n"

    new P1, .PerlNum
    set P1, 47.11
    set P0, 7
    bxor P1, P0, 7
    print P1
    print "\n"

    new P2, .PerlString
    set P2, "String"
    set P0, 127
    set P1, 1
    bxor P2, P0, P1
    print P2
    print "\n"

    new P4, .PerlUndef
    set P0, 200
    set P1, 100
    bxor P4, P0, P1
    print P4
    print "\n"
    end
CODE
231
15
0
126
172
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "band");
    new P0, .PerlInt
    set P0, 0b10101010
    band P0, 0b10011001
    print P0
    print "\n"

    new P1, .PerlInt
    new P2, .PerlInt
    set P1, 255
    set P2, 29
    band P1, P2
    print P1
    print "\n"

    new P3, .PerlNum
    set P3, 3.14
    set P1, 0b00001111
    set P2, 0b00001100
    band P3, P1, P2
    print P3
    print "\n"

    new P3, .PerlString
    set P3, "Foo"
    band P3, P1, 0b00001100
    print P3
    print "\n"

    new P3, .PerlUndef
    set P1, 100
    set P2, 100
    band P3, P1, P2
    print P3
    print "\n"
    end
CODE
136
29
12
12
100
OUTPUT




output_is(<<'CODE', <<'OUTPUT', "bnot");
    new P0, .PerlInt
    set P0, 0b10101010

# We use band in these tests to null out the high bits, and make the
# tests independent of the size of our INTVALs
    bnot P0, P0
    band P0, 0b01010101
    print P0
    print "\n"

    new P1, .PerlInt
    set P0, 0b01100110
    bnot P1, P0
    band P1, 0b10011001
    print P1
    print "\n"

    new P1, .PerlNum
    set P0, 0b00001111
    bnot P1, P0
    band P1, 0b11110000
    print P1
    print "\n"

    new P1, .PerlString
    set P0, 0b00110011
    bnot P1, P0
    band P1, 0b11001100
    print P1
    print "\n"

    new P1, .PerlUndef
    set P0, 0b00000000
    bnot P1, P0
    band P1, 0b11111111
    print P1
    print "\n"

    end
CODE
85
153
240
204
255
OUTPUT

# shl/shr tests adapted from t/op/bitwise.t

output_is(<<'CODE', <<'OUTPUT', "shr (>>)");
    new P0, .PerlInt
    new P1, .PerlInt
    new P2, .PerlInt
    new P3, .PerlInt
    set P0, 0b001100
    set P1, 0b010100
    set P2, 1
    set P3, 2

    new P4, .PerlInt
    new P5, .PerlNum
    new P6, .PerlString
    new P7, .PerlUndef

    shr P4, P0, P2
    shr P5, P0, P3
    shr P6, P1, P2
    shr P7, P1, P3

    print P4
    print "\n"
    print P5
    print "\n"
    print P6
    print "\n"
    print P7
    print "\n"
    end
CODE
6
3
10
5
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "shl (<<)");
    new P0, .PerlInt
    new P1, .PerlInt
    new P2, .PerlInt
    new P3, .PerlInt
    set P0, 0b001100
    set P1, 0b010100
    set P2, 1
    set P3, 2

    new P4, .PerlInt
    new P5, .PerlNum
    new P6, .PerlString
    new P7, .PerlUndef

    shl P4, P0, P2
    shl P5, P0, P3
    shl P6, P1, P2
    shl P7, P1, P3

    print P4
    print "\n"
    print P5
    print "\n"
    print P6
    print "\n"
    print P7
    print "\n"
    end
CODE
24
48
40
80
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "inc/dec a PerlUndef");
    new P0, .PerlUndef
    new P1, .PerlUndef
    inc P0
    print P0
    inc P0
    print P0
    dec P1
    print P1
    dec P1
    print P1
    print "\n"
    end
CODE
12-1-2
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "divide gives int if possible");
    new P0, .PerlInt
    new P1, .PerlInt
    new P2, .PerlUndef
    set P0, 12
    set P1, 2
    div P2, P0, P1
    print P2
    print "\n"
    div P2, P1
    print P2
    print "\n"
    div P2, 2
    print P2
    print "\n"
    end
CODE
6
3
1.500000
OUTPUT

output_is(<<'CODE', <<OUTPUT, "subtract native integer from PerlInt");
	new P0, .PerlInt
	new P1, .PerlInt
	set I0, 4000
	set P0, 123
	sub P1, P0, I0
	print P1
	print "\n"
	sub P0, P0, I0
	print P0
	print "\n"
        sub P0, -3876
	print P0
	print "\n"
	end
CODE
-3877
-3877
-1
OUTPUT

output_is(<<'CODE', <<OUTPUT, "multiply PerlInt with native integer");
	new P0, .PerlInt
	new P1, .PerlInt
	set I0, 4000
	set P0, 123
	mul P1, P0, I0
	print P1
	print "\n"
	mul P0, P0, I0
	print P0
	print "\n"
        mul P0, -1
	print P0
	print "\n"
        mul P0, 0
	print P0
	print "\n"
	end
CODE
492000
492000
-492000
0
OUTPUT

output_is(<<"CODE", <<OUTPUT, "divide PerlInt by native integer");
@{[ $fp_equality_macro ]}
	new P0, .PerlInt
	set I0, 4000
	set P0, 123
	div P0, P0, I0
	.fp_eq( P0, 0.03075, EQ1)
	print P0
	print "not "
EQ1:	print "ok 1"
	print "\\n"

        div P0, 5
        .fp_eq(P0, 0.00615, EQ2)
	print P0
	print "not "
EQ2:	print "ok 2"
	print "\\n"
	end
CODE
ok 1
ok 2
OUTPUT

#
# PerlInt and FLOATVAL, tests
#
output_is(<<"CODE", <<OUTPUT, "add native number to integer");
@{[ $fp_equality_macro ]}
	new P0, .PerlInt
	new P1, .PerlInt
	set N0, 4000.04
	set P0, 123
	add P1, P0, N0
	.fp_eq( P1, 4123.04, EQ1)
	print "not "
EQ1:	print "ok 1\\n"
	add P0, P0, N0
	.fp_eq( P0, 4123.04, EQ2)
        print "not "
EQ2:	print "ok 2\\n"
        new P0, .PerlInt
        set P0, 12
        add P0, 0.16
	.fp_eq( P0, 12.16, EQ3)
        print "not "
EQ3:	print "ok 3\\n"
	end
CODE
ok 1
ok 2
ok 3
OUTPUT

output_is(<<"CODE", <<OUTPUT, "subtract native number from integer");
@{[ $fp_equality_macro ]}
	new P0, .PerlInt
	new P1, .PerlInt
	set N0, 4000.04
	set P0, 123
	sub P1, P0, N0
	.fp_eq( P1, -3877.04, EQ1)
	print "not "
EQ1:	print "ok 1\\n"
	sub P0, P0, N0
	.fp_eq( P0, -3877.04, EQ2)
	print "not "
EQ2:	print "ok 2\\n"
        new P0, .PerlInt
        set P0, -3877
        sub P0, 23.01
	.fp_eq( P0, -3900.01, EQ3)
	print "not "
EQ3:	print "ok 3\\n"
	end
CODE
ok 1
ok 2
ok 3
OUTPUT

output_is(<<'CODE', <<OUTPUT, "multiply integer with native number");
	new P0, .PerlInt
	new P1, .PerlInt
	set N0, 4000.04
	set P0, 123
	mul P1, P0, N0
	print P1
	print "\n"
	mul P0, P0, N0
	print P0
	print "\n"

        new P0, .PerlInt
        set P0, 12
        mul P0, 25.5
	print P0
	print "\n"
	end
CODE
492004.920000
492004.920000
306
OUTPUT

output_is(<<"CODE", <<OUTPUT, "divide integer by native number");
@{[ $fp_equality_macro ]}
	new P0, .PerlInt
	set N0, 4000
	set P0, 123
	div P0, P0, N0

	.fp_eq( P0, 0.03074969250307496925, EQ1)
	print P0
	print "not "
EQ1:	print "ok 1"
	print "\\n"

        new P0, .PerlInt
        set P0, 250
        div P0, 0.01
	.fp_eq( P0, 25000.0, EQ2)
        print P0
	print "not "
EQ2:	print "ok 2"
	print "\\n"
	end
CODE
ok 1
ok 2
OUTPUT

#
# PerlInt and INTVAL tests
#
output_is(<<'CODE', <<OUTPUT, "add native integer to PerlInt");
	new P0, .PerlInt
	new P1, .PerlInt
	set I0, 4000
	set P0, 123
	add P1, P0, I0
	print P1
	print "\n"
	add P0, P0, I0
	print P0
	print "\n"
        add P0, 20
	print P0
	print "\n"
	end
CODE
4123
4123
4143
OUTPUT
