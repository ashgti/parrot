#! perl -w
# Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

t/pmc/complex.t - Complex Numbers

=head1 SYNOPSIS

	% perl t/pmc/complex.t

=head1 DESCRIPTION

Tests the Complex PMC.

=cut

use Parrot::Test tests => 11;
use Test::More;

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


output_is(<<'CODE', <<'OUTPUT', "String parsing");
    new P0, .Complex
    new P1, .String

	set P0, "4"
	print P0
    print "\n"

	set P0, "3.14"
	print P0
    print "\n"

	set P0, ".5"
	print P0
    print "\n"

	set P0, "-13"
	print P0
    print "\n"

	set P0, "-.3"
	print P0
    print "\n"

	set P0, "i"
	print P0
    print "\n"

	set P0, "-i"
	print P0
    print "\n"

	set P0, ".3i"
	print P0
    print "\n"

	set P0, "2 + 3i"
	print P0
    print "\n"

	set P0, "4 + 3.5i"
	print P0
    print "\n"

	set P0, "2 + .1 i"
	print P0
    print "\n"

	set P0, "10 - i"
	print P0
    print "\n"

	set P0, "5 - .3i"
	print P0
    print "\n"

	set P1, "-4-i"
	assign P0, P1
	print P0
    print "\n"

	set P1, "- 20 - .5 i"
	assign P0, P1
	print P0
    print "\n"

	set P1, "-13 +2i"
	assign P0, P1
	print P0
    print "\n"

	end
CODE
4+0i
3.14+0i
0.5+0i
-13+0i
-0.3+0i
0+1i
0-1i
0+0.3i
2+3i
4+3.5i
2+0.1i
10-1i
5-0.3i
-4-1i
-20-0.5i
-13+2i
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "add");
	new P0, .Complex
	new P1, .Complex
	new P2, .Float

    set P0, "1 + i"
    add P0, P0, P0
    print P0
    print "\n"

    set P0, "1 - i"
    set P1, "1 + i"
    add P0, P0, P1
    print P0
    print "\n"
    print P1
    print "\n"

    set P0, "-i"
    set P1, "1"
    add P1, P0, P1
    print P0
    print "\n"
    print P1
    print "\n"

#    set P0, "2 + i"
#    set P2, 3.3
#    add P1, P0, P2
#    print P1
#    print "\n"

    set P0, "3 + 5i"
    add P1, P0, 2
    print P1
    print "\n"

    set P0, "2 + 2i"
    add P1, P0, -2.0
    print P1
    print "\n"

    end
CODE
2+2i
2+0i
1+1i
0-1i
1-1i
5+5i
0+2i
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "substract");
	new P0, .Complex
	new P1, .Complex
	new P2, .Float

    set P0, "1 + i"
    sub P0, P0, P0
    print P0
    print "\n"

    set P0, "1 - i"
    set P1, "1 + i"
    sub P0, P0, P1
    print P0
    print "\n"
    print P1
    print "\n"

    set P0, "-i"
    set P1, "1"
    sub P1, P0, P1
    print P0
    print "\n"
    print P1
    print "\n"

#    set P0, "1 - 4i"
#    set P2, -1.0
#    sub P1, P0, P2
#    print P1
#    print "\n"

    set P0, "- 2 - 2i"
    sub P1, P0, -4
    print P1
    print "\n"

    set P0, "3 + i"
    sub P1, P0, 1.2
    print P1
    print "\n"

    end
CODE
0+0i
0-2i
1+1i
0-1i
-1-1i
2-2i
1.8+1i
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "multiply");
	new P0, .Complex
	new P1, .Complex
	new P2, .Float

    set P0, "2 + 3i"
    mul P0, P0, P0
    print P0
    print "\n"

    set P0, "5 - 2i"
    set P1, "5 + 2i"
    mul P0, P0, P1
    print P0
    print "\n"
    print P1
    print "\n"

    set P0, "3i"
    set P1, "2 - i"
    mul P1, P0, P1
    print P0
    print "\n"
    print P1
    print "\n"

#    set P0, "2 - 2i"
#    set P2, 0.5
#    mul P1, P0, P2
#    print P1
#    print "\n"

    set P0, "1 - i"
    mul P1, P0, 2
    print P1
    print "\n"

    set P0, "-1 + i"
    mul P1, P0, -1.0
    print P1
    print "\n"

    end
CODE
-5+12i
29+0i
5+2i
0+3i
3+6i
2-2i
1-1i
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "divide");
	new P0, .Complex
	new P1, .Complex
	new P2, .Float

    set P0, "2 + 3i"
    div P0, P0, P0
    print P0
    print "\n"

    set P0, "3 + 5i"
    set P1, "5 - 3i"
    div P0, P0, P1
    print P0
    print "\n"
    print P1
    print "\n"

    set P0, "25"
    set P1, "3 + 4i"
    div P1, P0, P1
    print P0
    print "\n"
    print P1
    print "\n"

#    set P0, "-3 + 6i"
#    set P2, 3.0
#    div P1, P0, P2
#    print P1
#    print "\n"

    set P0, "-2 + 3i"
    div P1, P0, 2
    print P1
    print "\n"

    set P0, "2 - 3i"
    div P1, P0, 0.5
    print P1
    print "\n"

    end
CODE
1+0i
0+1i
5-3i
25+0i
3-4i
-1+1.5i
4-6i
OUTPUT

SKIP: {
  skip("modulus not implemented", 1);
output_is(<<'CODE', <<'OUTPUT', "get int/num/bool");
        new P0, .Complex
        set P0, "1.6 - .9i"
        print P0
        print "\n"

        set I0, P0
        print I0
        print "\n"

        set N0, P0
        print N0
        print "\n"

        if P0, TRUE
        print "not "
TRUE:   print "true\n"

        set P0, "0"
        unless P0, FALSE
        print "not "
FALSE:  print "false\n"

        end
CODE
1.6-0.9i
2
2.5
true
false
OUTPUT
}

output_is(<<"CODE", <<'OUTPUT', "get keyed");
@{[ $fp_equality_macro ]}
        new P0, .Complex
        new P1, .String
        set P0, "- 3.3 + 1.2i"
        set P1, "imag"

        set N0, P0["real"]
        set N1, P0["imag"]
        set N2, P0[P1]
        print "N0 "
        .fp_ne( N0, -3.3, BAD1)
        print "N1 "
        .fp_ne( N1, 1.2, BAD1)
        print "N2 "
        .fp_ne( N2, 1.2, BAD1)
        branch OK1

BAD1:   print "not "
OK1:    print "OK\\n"

        set P2, P0["real"]
        set P3, P0[P1]
        print P2
        print "\\n"
        print P3
        print "\\n"

        set I0, P0["real"]
        set I1, P0[P1]
        print I0
        print "\\n"
        print I1
        print "\\n"

        end
CODE
N0 N1 N2 OK
-3.3
1.2
-3
1
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "set int/num");
    new P0, .Complex

    set P0, "3 + 4i"
    set P0, -2
    print P0
    print "\n"

    set P0, "2 + 5i"
    set P0, .4
    print P0
    print "\n"

    end
CODE
-2+0i
0.4+0i
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "set keyed");
    new P0, .Complex
    new P1, .String
    new P2, .String
    set P1, "real"

    set P0[P1], 1
    set P0["imag"], 4
    print P0
    print "\n"

    set P0[P1], 3.2
    set P0["imag"], -2.3
    print P0
    print "\n"

    set P2, ".5"
    set P0[P1], P2
    set P2, 6
    set P0["imag"], P2
    print P0
    print "\n"

    end
CODE
1+4i
3.2-2.3i
0.5+6i
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "is_equal");
    new P0, .Complex
    new P1, .Complex

    set P0, "2 + 3i"
    set P1["real"], 2
    set P1["imag"], 3

    eq P0, P1, OK1
    print "not "
OK1:print "ok1\n"

    set P1, 0
    ne P0, P1, OK2
    print "not "
OK2:print "ok2\n"

    end
CODE
ok1
ok2
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "abs");
    new P0, .Complex
    set P0, "4 + 3j"
    new P1, .Undef
    abs P1, P0
    print P1
    print "\n"

    end
CODE
5
OUTPUT
