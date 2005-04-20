#! perl -w
# Copyright: 2001-2005 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

t/pmc/perlstring.t - Perl Strings

=head1 SYNOPSIS

	% perl -Ilib t/pmc/perlstring.t

=head1 DESCRIPTION

Tests the C<PerlString> PMC. Checks Perl-specific string behaviour.

=cut

use Parrot::Test tests => 68;
use Test::More; # Included for skip().

my $fp_equality_macro = <<'ENDOFMACRO';
.macro fp_eq ( J, K, L )
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
.macro fp_ne ( J, K, L )
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

output_is(<<CODE, <<OUTPUT, "Set/get strings");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
        new P0, I25
        set P0, "foo"
        set S0, P0
        eq S0, "foo", OK1
        print "not "
OK1:    print "ok 1\\n"

        set P0, "\0"
        set S0, P0
        eq S0, "\0", OK2
        print "not "
OK2:    print "ok 2\\n"

        set P0, ""
        set S0, P0
        eq S0, "", OK3
        print "not "
OK3:    print "ok 3\\n"

        set P0, 123
        set S0, P0
        eq S0, "123", OK4
        print "not "
OK4:    print "ok 4\\n"

        set P0, 1.234567
        set S0, P0
        eq S0, "1.234567", OK5
        print "not "
OK5:    print "ok 5\\n"

        set P0, "0xFFFFFF"
        set S0, P0
        eq S0, "0xFFFFFF", OK6
        print "not "
OK6:    print "ok 6\\n"

        end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
ok 6
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "set/get string value");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I25
        set P0, "foo"
        set S0, P0
        eq S0, "foo", OK1
        print "not "
OK1:    print "ok 1\n"

        set P0, "\0"
        set S0, P0
        eq S0, "\0", OK2
        print "not "
OK2:    print "ok 2\n"

        set P0, ""
        set S0, P0
        eq S0, "", OK3
        print "not "
OK3:    print "ok 3\n"

        set P0, 0
        set S0, P0
        eq S0, "0", OK4
        print "not "
OK4:    print "ok 4\n"

        set P0, 0.0
        set S0, P0
        eq S0, "0", OK5
        print "not "
OK5:    print "ok 5\n"

        set P0, "0b000001"
        set S0, P0
        eq S0, "0b000001", OK6
        print "not "
OK6:    print "ok 6\n"

	end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
ok 6
OUTPUT

output_is(<<CODE, <<OUTPUT, "Setting integers");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
        new P0, I25
        set P0, "1"
        set I0, P0
        print I0
        print "\\n"

        new P0, I25
        set P0, "2.0"
        set I0, P0
        print I0
        print "\\n"

        new P0, I25
        set P0, ""
        set I0, P0
        print I0
        print "\\n"

        new P0, I25
        set P0, "\0"
        set I0, P0
        print I0
        print "\\n"

        new P0, I25
        set P0, "foo"
        set I0, P0
        print I0
        print "\\n"

        end
CODE
1
2
0
0
0
OUTPUT

output_is(<<"CODE", <<OUTPUT, "Setting numbers");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
@{[ $fp_equality_macro ]}
        new P0, I25
        set P0, "1"
        set N0, P0
        .fp_eq(N0, 1.0, OK1)
        print "not "
OK1:    print "ok 1\\n"

        new P0, I25
        set P0, "2.0"
        set N0, P0
        .fp_eq(N0, 2.0, OK2)
        print "not "
OK2:    print "ok 2\\n"

        new P0, I25
        set P0, ""
        set N0, P0
        .fp_eq(N0, 0.0, OK3)
        print "not "
OK3:    print "ok 3\\n"

        new P0, I25
        set P0, "\0"
        set N0, P0
        .fp_eq(N0, 0.0, OK4)
        print "not "
OK4:    print "ok 4\\n"

        new P0, I25
        set P0, "foo"
        set N0, P0
        .fp_eq(N0, 0.0, OK5)
        print "not "
OK5:    print "ok 5\\n"

        new P0, I25
        set P0, "1.3e5"
        set N0, P0
        .fp_eq(N0, 130000.0, OK6)
        print "not "
OK6:    print "ok 6\\n"

        end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
ok 6
OUTPUT

output_is(<<'CODE', <<OUTPUT, "assign string");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
    new P0, I23
    assign P0, "Albert"
    print P0
    print "\n"

    new P1, I24
    assign P1, "Beth"
    print P1
    print "\n"

    new P2, I25
    assign P2, "Charlie"
    print P2
    print "\n"

    new P3, I26
    assign P3, "Doris"
    print P3
    print "\n"

    end
CODE
Albert
Beth
Charlie
Doris
OUTPUT

output_is(<<CODE, <<OUTPUT, "ensure that concat ppp copies strings");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I25
	new P1, I25
	new P2, I25
	set P0, "foo"
	concat	P1, P0, P0

	print	P0
	print "\\n"

	print	P1
	print "\\n"

	set P1, "You can't teach an old dog new..."
	set P2, "clear physics"
	concat P0, P1, P2

	print P1
	print "\\n"
	print P2
	print "\\n"
	print P0
	print "\\n"
	end
CODE
foo
foofoo
You can't teach an old dog new...
clear physics
You can't teach an old dog new...clear physics
OUTPUT

output_is(<<CODE, <<OUTPUT, "ensure that concat pps copies strings");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I25
	new P1, I25

	set S0, "Grunties"
	set P1, "fnargh"
	concat P0, P1, S0

	print S0
	print "\\n"
	print P1
	print "\\n"
	print P0
	print "\\n"

	end
CODE
Grunties
fnargh
fnarghGrunties
OUTPUT

output_is(<<CODE, <<OUTPUT, "Setting string references");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I25
	set S0, "C2H5OH + 10H20"
	set P0, S0
	chopn S0, 8

	print S0
	print "\\n"
	print P0
	print "\\n"
	end
CODE
C2H5OH
C2H5OH
OUTPUT

output_is(<<CODE, <<OUTPUT, "Assigning string copies");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I25
	set S0, "C2H5OH + 10H20"
	assign P0, S0
	chopn S0, 8

	print S0
	print "\\n"
	print P0
	print "\\n"
	end
CODE
C2H5OH
C2H5OH + 10H20
OUTPUT

output_is(<<CODE, <<OUTPUT, "if (P) - String");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new	P0, I25

	set	P0, "I've told you once, I've told you twice..."
	if	P0, OK1
	print	"not "
OK1:	print	"ok 1\\n"

	set	P0, "0.0"
	if	P0, OK2
	print	"not "
OK2:	print	"ok 2\\n"

	set	P0, ""
	if	P0, BAD3
	branch OK3
BAD3:	print	"not "
OK3:	print	"ok 3\\n"

	set	P0, "0"
	if	P0, BAD4
	branch OK4
BAD4:	print	"not "
OK4:	print	"ok 4\\n"

	set	P0, "0e0"
	if	P0, OK5
	print	"not "
OK5:	print	"ok 5\\n"

	set	P0, "x"
	if	P0, OK6
	print	"not "
OK6:	print	"ok 6\\n"

	set	P0, "\\x0"
	if	P0, OK7
	print	"not "
OK7:	print	"ok 7\\n"

	set	P0, "\\n"
	if	P0, OK8
	print	"not "
OK8:	print	"ok 8\\n"

	set	P0, " "
	if	P0, OK9
	print	"not "
OK9:	print	"ok 9\\n"

	end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
ok 6
ok 7
ok 8
ok 9
OUTPUT

output_is(<<CODE, <<OUTPUT, "unless (P) - String");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new	P0, I25

        set     P0, "0"
        unless  P0, OK1
        print   "not"
OK1:	print	"ok 1\\n"

	set	P0, "1"
	unless	P0, BAD2
        branch  OK2
BAD2:	print	"not "
OK2:	print	"ok 2\\n"

	end
CODE
ok 1
ok 2
OUTPUT

#
# Basic string number conversion
#
output_is(<<CODE, <<OUTPUT, "string to int");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new	P0, I25

	set	P0, "1"
	set	I0, P0
	print	I0
	print	P0
	print	"\\n"

	set	P0, " 1"
	set	I0, P0
	print	I0
	print	P0
	print	"\\n"

	set	P0, "-1"
	set	I0, P0
	print	I0
	print	P0
	print	"\\n"

	set	P0, "dull and void"
	set	I0, P0
	print	I0
	print	"\\n"

	set	P0, ""
	set	I0, P0
	print	I0
	print	"\\n"
	end
CODE
11
1 1
-1-1
0
0
OUTPUT

output_is(<<'CODE', <<OUTPUT, "nasty string -> int");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new	P0, I23
	set	P0, "Z1"
	set	I0, P0
	print	I0
	print	P0
	print	"\n"

	set	P0, "\x0 1"
	set	I0, P0
	print	I0
	print	"\n"

	set	P0, "1.23e2"
	set	I0, P0
	print	I0
	print	"\n"
       end
CODE
0Z1
0
123
OUTPUT

output_is(<<CODE, <<OUTPUT, "string to number conversion");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
@{[ $fp_equality_macro ]}
	new	P0, I24

	set	P0, "1"
	set	N0, P0
	.fp_eq(	N0, 1, EQ1)
	print	N0
	print	"not "
EQ1:	print	"ok 1\\n"

	set	P0, "1.2"
	set	N0, P0
	.fp_eq(	N0, 1.2, EQ2)
	print	N0
	print	"not "
EQ2:	print	"ok 2\\n"

	set	P0, "1.2e1"
	set	N0, P0
	.fp_eq(	N0, 12, EQ3)
	print	N0
	print	"not "
EQ3:	print	"ok 3\\n"

	set	P0, "1.2e-1"
	set	N0, P0
	.fp_eq(	N0, 0.12, EQ4)
	print	N0
	print	"not "
EQ4:	print	"ok 4\\n"

	set	P0, "1.2e2.1"
	set	N0, P0
	.fp_eq(	N0, 120, EQ5)
	print	N0
	print	"not "
EQ5:	print	"ok 5\\n"

	set	P0, "X1.2X"
	set	N0, P0
	.fp_eq(	N0, 0.0, EQ6)
	print	N0
	print	"not "
EQ6:	print	"ok 6\\n"

	set	P0, "E1-1.2e+2"
	set	N0, P0
	.fp_eq(	N0, 0.0, EQ7)
	print	N0
	print	"not "
EQ7:	print	"ok 7\\n"

	set	P0, "++-1"
	set	N0, P0
	.fp_eq(	N0, 0.0, EQ8)
	print	N0
	print	"not "
EQ8:	print	"ok 8\\n"

	set	P0, "1234.1234.5"
	set	N0, P0
	.fp_eq(	N0, 1234.1234, EQ9)
	print	N0
	print	"not "
EQ9:	print	"ok 9\\n"

	set	P0, "this is empty!"
	set	N0, P0
	.fp_eq(	N0, 0.0, EQ10)
	print	N0
	print	" not "
EQ10:	print	"ok 10\\n"

	set	P0, "0e123"
	set	N0, P0
	.fp_eq(	N0, 0, EQ11)
	print	N0
	print	" not "
EQ11:	print	"ok 11\\n"

	set	P0, "000000000000000000000000000000000000000001e-0"
	set	N0, P0
	.fp_eq(	N0, 1, EQ12)
	print	N0
	print	" not "
EQ12:	print	"ok 12\\n"

	end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
ok 6
ok 7
ok 8
ok 9
ok 10
ok 11
ok 12
OUTPUT

output_is(<<'CODE', <<OUTPUT, "concatenate string to number");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I25
	new P1, I24
	set P0, "bar"
	set P1, 2.7
	concat P0,P0,P1
	print P0
	print "\n"
	end
CODE
bar2.700000
OUTPUT

output_is(<<'CODE', <<OUTPUT, "concatenate string to string");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I25
	new P1, I25
	set P0, "foo"
	set P1, "bar"
	concat P0,P0,P1
	print P0
	print "\n"
	end
CODE
foobar
OUTPUT

output_is(<<'CODE', <<OUTPUT, "concatenate <foo> to undef");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I26
	new P1, I23
	set P1, 10
	concat P0, P0, P1
        set S0, P0
        eq S0, "10", OK1
        print "not "
OK1:    print "ok 1\n"

	new P0, I26
	new P1, I24
	set P1, 1.2
	concat P0, P0, P1
        set S0, P0
        eq S0, "1.200000", OK2
        print "not "
OK2:    print "ok 2\n"

	new P0, I26
	new P1, I25
	set P1, "Foo"
	concat P0, P0, P1
        set S0, P0
        eq S0, "Foo", OK3
        print "not "
OK3:    print "ok 3\n"

	new P0, I26
	new P1, I26
	concat P0, P0, P1
        set S0, P0
        eq S0, "", OK4
        print "not "
OK4:    print "ok 4\n"
	end
CODE
ok 1
ok 2
ok 3
ok 4
OUTPUT

output_is(<<'CODE', <<OUTPUT, "concatenate undef to <foo>");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I26
	new P1, I23
	set P1, 10
	concat P1, P1, P0
        set S0, P1
        eq S0, "10", OK1
        print "not "
OK1:    print "ok 1\n"

	new P0, I26
	new P1, I24
	set P1, 1.2
	concat P1, P1, P0
        set S0, P1
        eq S0, "1.200000", OK2
        print "not "
OK2:    print "ok 2\n"

	new P0, I26
	new P1, I25
	set P1, "Foo"
	concat P1, P1, P0
        set S0, P1
        eq S0, "Foo", OK3
        print "not "
OK3:    print "ok 3\n"

	end
CODE
ok 1
ok 2
ok 3
OUTPUT

output_is(<<'CODE', <<OUTPUT, "concatenate STRING to undef");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I26
	concat P0, P0, "Foo"
        set S0, P0
        eq S0, "Foo", OK1
        print "not "
OK1:    print "ok 1\n"
	end
CODE
ok 1
OUTPUT

output_is(<<'CODE', <<OUTPUT, "concatenate number to string");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I24
	new P1, I25
	set P0, 5.43
	set P1, "bar"
	concat P0,P0,P1
	print P0
	print "\n"
	end
CODE
5.430000bar
OUTPUT

output_is(<<'CODE', <<OUTPUT, "repeat");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I25
	set P0, "x"
	new P1, I23
	set P1, 12
	new P2, I25
	repeat P2, P0, P1
        print P2
        print "\n"

        set P0, "y"
        new P1, I24
        set P1, 6.5
        repeat P2, P0, P1
        print P2
        print "\n"

        set P0, "z"
        new P1, I25
        set P1, "3"
        repeat P2, P0, P1
        print P2
        print "\n"

        set P0, "a"
        new P1, I26
        repeat P2, P0, P1
        print P2
        print "\n"

	end
CODE
xxxxxxxxxxxx
yyyyyy
zzz

OUTPUT

output_is(<<'CODE', <<OUTPUT, "repeat_int");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I25
	set P0, "x"
	set I1, 12
	new P2, I25
	repeat P2, P0, I1
        print P2
        print "\n"

        set P0, "za"
        set I1, 3
        repeat P2, P0, I1
        print P2
        print "\n"
	end
CODE
xxxxxxxxxxxx
zazaza
OUTPUT

output_is(<<CODE, <<OUTPUT, "if(PerlString)");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
        new P0, I25
	set S0, "True"
	set P0, S0
        if P0, TRUE
        print "false"
        branch NEXT
TRUE:   print "true"
NEXT:   print "\\n"

        new P1, I25
        set S1, ""
        set P1, S1
        if P1, TRUE2
        print "false"
        branch NEXT2
TRUE2:  print "true"
NEXT2:  print "\\n"

        new P2, I25
        set S2, "0"
        set P2, S2
        if P2, TRUE3
        print "false"
        branch NEXT3
TRUE3:  print "true"
NEXT3:  print "\\n"

        new P3, I25
        set S3, "0123"
        set P3, S3
        if P3, TRUE4
        print "false"
        branch NEXT4
TRUE4:  print "true"
NEXT4:  print "\\n"

        new P4, I25
        if P4, TRUE5
        print "false"
        branch NEXT5
TRUE5:  print "true"
NEXT5:  print "\\n"
        end
CODE
true
false
false
true
false
OUTPUT

# XXX these tests better should get generated
#     with all combinations of params and ops
output_is(<<'CODE', <<OUTPUT, "add str_int, str_int");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I25
	set P0, "23"
	new P1, I25
	set P1, "2"
	new P2, I26
	add P2, P0, P1
	print P2
	print "\n"
	end
CODE
25
OUTPUT

output_is(<<"CODE", <<OUTPUT, "add str_int, str_num");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
@{[ $fp_equality_macro ]}
	new P0, I25
	set P0, "23"
	new P1, I25
	set P1, "2.5"
	new P2, I26
	add P2, P0, P1
        .fp_eq(P2, 25.5, EQ1)
        print "not "
EQ1:    print "ok 1\\n"
	end
CODE
ok 1
OUTPUT

output_is(<<'CODE', <<OUTPUT, "add str_int, int");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I25
	set P0, "23"
	new P1, I23
	set P1, 2
	new P2, I26
	add P2, P0, P1
	print P2
	print "\n"
	end
CODE
25
OUTPUT

output_is(<<"CODE", <<OUTPUT, "add str_int, num");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
@{[ $fp_equality_macro ]}
	new P0, I25
	set P0, "23"
	new P1, I24
	set P1, 2.5
	new P2, I26
	add P2, P0, P1
        .fp_eq(P2, 25.5, EQ1)
        print "not "
EQ1:    print "ok 1\\n"
	end
CODE
ok 1
OUTPUT

output_is(<<"CODE", <<OUTPUT, "add str_num, int");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
@{[ $fp_equality_macro ]}
	new P0, I25
	set P0, "23.5"
	new P1, I23
	set P1, 2
	new P2, I26
	add P2, P0, P1
        .fp_eq(P2, 25.5, EQ1)
        print "not "
EQ1:    print "ok 1\\n"
	end
CODE
ok 1
OUTPUT

# XXX - should test for appropriate warnings
output_is(<<"CODE", <<OUTPUT, "add non-numeric string");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
@{[ $fp_equality_macro ]}
        new P0, I25
        set P0, "12"
        new P1, I25
        set P1, "USC"
        new P2, I26
        add P2, P0, P1
        eq P2, 12, OK1
        print "not "
OK1:    print "ok 1\\n"
        set P0, "12.5"
        new P2, I26
        add P2, P0, P1
        .fp_eq(P2, 12.5, OK2)
        print "not "
OK2:    print "ok 2\\n"
        set P0, "Auburn"
        new P2, I26
        add P2, P0, P1
        eq P2, 0, OK3
        print "not "
OK3:    print "ok 3\\n"
        end
CODE
ok 1
ok 2
ok 3
OUTPUT

output_is(<<'CODE', <<OUTPUT, "sub str_int, str_int");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I25
	set P0, "23"
	new P1, I25
	set P1, "2"
	new P2, I26
	sub P2, P0, P1
	print P2
	print "\n"
	end
CODE
21
OUTPUT

output_is(<<"CODE", <<OUTPUT, "sub str_int, str_num");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
@{[ $fp_equality_macro ]}
	new P0, I25
	set P0, "23"
	new P1, I25
	set P1, "2.5"
	new P2, I26
	sub P2, P0, P1
        .fp_eq(P2, 20.5, EQ1)
        print "not "
EQ1:    print "ok 1\\n"
	end
CODE
ok 1
OUTPUT

output_is(<<'CODE', <<OUTPUT, "sub str_int, int");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I25
	set P0, "23"
	new P1, I23
	set P1, 2
	new P2, I26
	sub P2, P0, P1
	print P2
	print "\n"
	end
CODE
21
OUTPUT

output_is(<<"CODE", <<OUTPUT, "sub str_int, num");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
@{[ $fp_equality_macro ]}
	new P0, I25
	set P0, "23"
	new P1, I24
	set P1, 2.5
	new P2, I26
	sub P2, P0, P1
        .fp_eq(P2, 20.5, EQ1)
        print "not "
EQ1:    print "ok 1\\n"
	end
CODE
ok 1
OUTPUT

output_is(<<"CODE", <<OUTPUT, "sub str_num, int");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
@{[ $fp_equality_macro ]}
	new P0, I25
	set P0, "23.5"
	new P1, I23
	set P1, 2
	new P2, I26
	sub P2, P0, P1
        .fp_eq(P2, 21.5, EQ1)
        print P2
        print "not "
EQ1:    print "ok 1\\n"
	end
CODE
ok 1
OUTPUT

# XXX - should test for appropriate warnings
output_is(<<"CODE", <<OUTPUT, "sub non-numeric string");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
@{[ $fp_equality_macro ]}
        new P0, I25
        set P0, "24"
        new P1, I25
        set P1, "Oklahoma"
        new P2, I26
        sub P2, P0, P1
        eq P2, 24, OK1
        print "not "
OK1:    print "ok 1\\n"
        set P0, "5.12"
        new P2, I26
        sub P2, P0, P1
        .fp_eq(P2, 5.12, OK2)
        print "not "
OK2:    print "ok 2\\n"
        set P0, "Virginia Tech"
        new P2, I26
        sub P2, P0, P1
        eq P2, 0, OK3
        print "not "
OK3:    print "ok 3\\n"
        end
CODE
ok 1
ok 2
ok 3
OUTPUT

output_is(<<'CODE', <<OUTPUT, "mul str_int, str_int");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I25
	set P0, "23"
	new P1, I25
	set P1, "2"
	new P2, I26
	mul P2, P0, P1
	print P2
	print "\n"
	end
CODE
46
OUTPUT

output_is(<<"CODE", <<OUTPUT, "mul str_int, str_num");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
@{[ $fp_equality_macro ]}
	new P0, I25
	set P0, "5"
	new P1, I25
	set P1, "2.5"
	new P2, I26
	mul P2, P0, P1
        .fp_eq(P2, 12.5, EQ1)
        print "not "
EQ1:    print "ok 1\\n"
	end
CODE
ok 1
OUTPUT

output_is(<<'CODE', <<OUTPUT, "mul str_int, int");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I25
	set P0, "23"
	new P1, I23
	set P1, 2
	new P2, I26
	mul P2, P0, P1
	print P2
	print "\n"
	end
CODE
46
OUTPUT

output_is(<<"CODE", <<OUTPUT, "mul str_int, num");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
@{[ $fp_equality_macro ]}
	new P0, I25
	set P0, "19"
	new P1, I24
	set P1, 2.5
	new P2, I26
	mul P2, P0, P1
        .fp_eq(P2, 47.5, EQ1)
        print "not "
EQ1:    print "ok 1\\n"
	end
CODE
ok 1
OUTPUT

output_is(<<"CODE", <<OUTPUT, "mul str_num, int");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
@{[ $fp_equality_macro ]}
	new P0, I25
	set P0, "23.4"
	new P1, I23
	set P1, 2
	new P2, I26
	mul P2, P0, P1
        .fp_eq(P2, 46.8, EQ1)
        print P2
        print "not "
EQ1:    print "ok 1\\n"
	end
CODE
ok 1
OUTPUT

# XXX - should test for appropriate warnings
output_is(<<"CODE", <<OUTPUT, "mul non-numeric string");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
@{[ $fp_equality_macro ]}
        new P0, I25
        set P0, "24"
        new P1, I25
        set P1, "Oklahoma"
        new P2, I26
        mul P2, P0, P1
        eq P2, 0, OK1
        print "not "
OK1:    print "ok 1\\n"
        set P0, "5.12"
        new P2, I26
        mul P2, P0, P1
        .fp_eq(P2, 0.0, OK2)
        print "not "
OK2:    print "ok 2\\n"
        set P0, "Virginia Tech"
        new P2, I26
        mul P2, P0, P1
        eq P2, 0, OK3
        print "not "
OK3:    print "ok 3\\n"
        end
CODE
ok 1
ok 2
ok 3
OUTPUT

output_is(<<'CODE', <<OUTPUT, "concat must morph dest to a string");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P0, I25
	new P1, I26
	set P0, "foo"
	concat	P1, P0, P0

	print	P0
	print "\n"
	print	P1
	print "\n"

	new P0, I25
	new P1, I26
	set P0, "bar"
	concat	P1, P1, P0

	print	P0
	print "\n"
	print	P1
	print "\n"
	end
CODE
foo
foofoo
bar
bar
OUTPUT

output_is(<<'CODE', <<OUTPUT, "cmp");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P1, I25
	new P2, I25

        set P1, "abc"
        set P2, "abc"
        cmp I0, P1, P2
        print I0
        print "\n"

        set P1, "abcde"
        set P2, "abc"
        cmp I0, P1, P2
        print I0
        print "\n"

        set P1, "abc"
        set P2, "abcde"
        cmp I0, P1, P2
        print I0
        print "\n"

        end
CODE
0
1
-1
OUTPUT

output_is(<<'CODE', <<OUTPUT, "cmp with PerlInt");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
	new P1, I23
	new P2, I25
        set P2, "10"

# Int. vs Str.
        set P1, 10
        cmp I0, P1, P2
        print I0
        print "\n"

        set P1, 20
        cmp I0, P1, P2
        print I0
        print "\n"

        set P1, 0
        cmp I0, P1, P2
        print I0
        print "\n"

# Str. vs Int.
        set P1, 0
        cmp I0, P2, P1
        print I0
        print "\n"

        set P1, 20
        cmp I0, P2, P1
        print I0
        print "\n"

        set P1, 10
        cmp I0, P2, P1
        print I0
        print "\n"

        end
CODE
0
1
-1
1
-1
0
OUTPUT

output_is(<<'CODE', <<OUTPUT, "repeat");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
        new P0, I26
        new P1, I25
        new P2, I23

        set P2, 1024
        repeat P1, P0, P2
        set S1, P1
        eq S1, "", OK1
        print "not "
OK1:    print "ok 1\n"

        new P0, I26
        new P1, I25
        repeat P1, P0, 1024
        set S1, P1
        eq S1, "", OK2
        print "not "
OK2:    print "ok 2\n"
	end
CODE
ok 1
ok 2
OUTPUT

output_is(<<'CODE', <<OUTPUT, "substr");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
        new P0, I25
        set P0, "This is a test\n"
        substr S0, P0, 0, 5
        substr S1, P0, 10, 4
        substr S2, P0, -11, 3
        substr S3, P0, 7, 1000  # Valid offset, but length > string length
        print S0
        print S1
        print S2
        print S3
        print P0 # Check that the original is unmodified
        end
CODE
This test is a test
This is a test
OUTPUT

output_like(<<'CODE', <<'OUTPUT', "Out-of-bounds substr, +ve offset");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
        new P0, I25
        set P0, "Woburn"
        substr S0, P0, 123, 22
        end
CODE
/^Cannot take substr outside string$/
OUTPUT

output_like(<<'CODE', <<'OUTPUT', "Out-of-bounds substr, -ve offset");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
        new P0, I25
        set P0, "Woburn"
        substr S0, P0, -123, 22
        end
CODE
/^Cannot take substr outside string$/
OUTPUT

output_is( <<'CODE', <<OUTPUT, "bands NULL string");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
        new P1, I25
	new P2, I25
	new P3, I25
	null S1
	set S2, "abc"
	set P1, S1
	set P2, S2
	bands P1, P2
	null S3
	set P3, S3
	eq P1, P3, ok1
	print "not "
ok1:	print "ok 1\n"
	set P1, ""
	bands P1, P2
	unless P1, ok2
	print "not "
ok2:	print "ok 2\n"

	null S2
	set P2, S2
	set P1, "abc"
	bands P1, P2
	null S3
	set P3, S3
	eq P1, P3, ok3
	print "not "
ok3:	print "ok 3\n"
	set P2, ""
	bands P1, P2
	unless P1, ok4
	print "not "
ok4:	print "ok 4\n"
	end
CODE
ok 1
ok 2
ok 3
ok 4
OUTPUT

output_is( <<'CODE', <<OUTPUT, "bands 2");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
        new P1, I25
	new P2, I25
	set P1, "abc"
	set P2, "EE"
	bands P1, P2
	print P1
	print "\n"
	print P2
	print "\n"
	end
CODE
A@
EE
OUTPUT

output_is( <<'CODE', <<OUTPUT, "bands 3");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
        new P1, I25
	new P2, I25
	new P0, I25
	set P1, "abc"
	set P2, "EE"
	bands P0, P1, P2
	print P0
	print "\n"
	print P1
	print "\n"
	print P2
	print "\n"
	end
CODE
A@
abc
EE
OUTPUT

output_is( <<'CODE', <<OUTPUT, "bors NULL string");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
        new P1, I25
	new P2, I25
	new P3, I25
	null S1
	null S2
	set P1, S1
	set P2, S2
	bors P1, P2
	null S3
	set P3, S3
	eq P1, P3, OK1
	print "not "
OK1:    print "ok 1\n"

	null S1
	set P1, S1
	set P2, ""
	bors P1, P2
	null S3
	set P3, S3
	eq P1, P3, OK2
	print "not "
OK2:    print "ok 2\n"
        bors P2, P1
        eq P2, P3, OK3
        print "not "
OK3:    print "ok 3\n"

	null S1
	set P1, S1
	set P2, "def"
	bors P1, P2
	eq P1, "def", OK4
	print "not "
OK4:    print "ok 4\n"
        null S2
	set P2, S2
        bors P1, P2
        eq P1, "def", OK5
        print "not "
OK5:    print "ok 5\n"

        null S1
        null S2
	set P1, S1
	set P2, S2
        bors P3, P1, P2
        null S4
        eq P3, S4, OK6
        print "not "
OK6:    print "ok 6\n"

        set P1, ""
        bors P3, P1, P2
        eq P3, S4, OK7
        print "not "
OK7:    print "ok 7\n"
        bors P3, P2, P1
        eq P3, S4, OK8
        print "not "
OK8:    print "ok 8\n"

        set P1, "def"
        bors P3, P1, P2
        eq P3, "def", OK9
        print "not "
OK9:    print "ok 9\n"
        bors P3, P2, P1
        eq P3, "def", OK10
        print "not "
OK10:   print "ok 10\n"
        end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
ok 6
ok 7
ok 8
ok 9
ok 10
OUTPUT

output_is( <<'CODE', <<OUTPUT, "bors 2");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
        new P1, I25
	new P2, I25
	set P1, "abc"
	set P2, "EE"
	bors P1, P2
	print P1
	print "\n"
	print P2
	print "\n"
	end
CODE
egc
EE
OUTPUT

output_is( <<'CODE', <<OUTPUT, "bors 3");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
        new P1, I25
	new P2, I25
	new P0, I25
	set P1, "abc"
	set P2, "EE"
	bors P0, P1, P2
	print P0
	print "\n"
	print P1
	print "\n"
	print P2
	print "\n"
	end
CODE
egc
abc
EE
OUTPUT

output_is( <<'CODE', <<OUTPUT, "bxors NULL string");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
     new P1, I25
     new P2, I25
     new P3, I25
     null S1
     null S2
     set P1, S1
     set P2, S2
     bxors P1, P2
     null S3
     eq P1, S3, OK1
     print "not "
OK1: print "ok 1\n"

     null S1
     set P1, S1
     set P2, ""
     bxors P1, P2
     null S3
     eq P1, S3, OK2
     print "not "
OK2: print "ok 2\n"
     bxors P2, P1
     eq S2, S3, OK3
     print "not "
OK3: print "ok 3\n"

     null S1
     set P1, S1
     set P2, "abc"
     bxors P1, P2
     eq P1, "abc", OK4
     print "not "
OK4: print "ok 4\n"
     null S2
     set P2, S2
     bxors P1, P2
     eq P1, "abc", OK5
     print "not "
OK5: print "ok 5\n"

     null S1
     null S2
     set P1, S1
     set P2, S2
     bxors P3, P1, P2
     null S4
     eq P3, S4, OK6
     print "not "
OK6: print "ok 6\n"

     set P1, ""
     bxors P3, P1, P2
     eq P3, S4, OK7
     print "not "
OK7: print "ok 7\n"
     bxors P3, P2, P1
     eq P3, S4, OK8
     print "not "
OK8: print "ok 8\n"

     set P1, "abc"
     bxors P3, P1, P2
     eq P3, "abc", OK9
     print "not "
OK9: print "ok 9\n"
     bxors P3, P2, P1
     eq P3, "abc", OK10
     print "not "
OK10: print "ok 10\n"
     end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
ok 6
ok 7
ok 8
ok 9
ok 10
OUTPUT

output_is( <<'CODE', <<OUTPUT, "bxors 2");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
    new P1, I25
    new P2, I25
    new P3, I25
    set P1, "a2c"
    set P2, "Dw"
    bxors P1, P2
    print P1
    print "\n"
    print P2
    print "\n"
    set P1, "abc"
    set P2, "   X"
    bxors P1, P2
    print P1
    print "\n"
    print P2
    print "\n"
    end
CODE
%Ec
Dw
ABCX
   X
OUTPUT

output_is( <<'CODE', <<OUTPUT, "bxors 3");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
    new P1, I25
    new P2, I25
    new P0, I25
    set P1, "a2c"
    set P2, "Dw"
    bxors P0, P1, P2
    print P0
    print "\n"
    print P1
    print "\n"
    print P2
    print "\n"
    set P1, "abc"
    set P2, "   Y"
    bxors P0, P1, P2
    print P0
    print "\n"
    print P1
    print "\n"
    print P2
    print "\n"
    end
CODE
%Ec
a2c
Dw
ABCY
abc
   Y
OUTPUT

output_is( <<'CODE', <<OUTPUT, "bnots NULL string");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
     new P1, I25
     new P2, I25
     new P3, I25
     null S1
     null S2
     set P1, S1
     set P2, S2
     bnots P1, P2
     null S3
     eq P1, S3, OK1
     print "not "
OK1: print "ok 1\n"

     null S1
     set P1, S1
     set P2, ""
     bnots P1, P2
     null S3
     eq P1, S3, OK2
     print "not "
OK2: print "ok 2\n"
     bnots P2, P1
     eq S2, S3, OK3
     print "not "
OK3: print "ok 3\n"
     end
CODE
ok 1
ok 2
ok 3
OUTPUT

SKIP: {
skip("No unicode yet", 1);
output_is( <<'CODE', <<OUTPUT, "bnots 2");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
 getstdout P0
 push P0, "utf8"
 new P1, I25
 new P2, I25
 set P1, "a2c"
 bnots P2, P1
 print P1
 print "\n"
 print P2
 print "\n"
 bnots P1, P1
 print P1
 print "\n"
 bnots P1, P1
 print P1
 print "\n"
 end
CODE
a2c
\xC2\x9E\xC3\x8D\xC2\x9C
\xC2\x9E\xC3\x8D\xC2\x9C
a2c
OUTPUT
}

output_is( <<'CODE', <<OUTPUT, "eq");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
        new P1, I25
        set P1, "ABC"
        set S1, "ABC"
        set S2, "CBA"
        set S3, "abc"
        set S4, "ABCD"
        set S5, "\0"
        set S6, ""

        eq  P1, S1, OK1
        print "not "
OK1:    print "ok 1\n"

        eq P1, S2, BAD2
        branch OK2
BAD2:   print "not "
OK2:    print "ok 2\n"

        eq P1, S3, BAD3
        branch OK3
BAD3:   print "not "
OK3:    print "ok 3\n"

        eq P1, S4, BAD4
        branch OK4
BAD4:   print "not "
OK4:    print "ok 4\n"

        eq P1, S5, BAD5
        branch OK5
BAD5:   print "not "
OK5:    print "ok 5\n"

        eq P1, S6, BAD6
        branch OK6
BAD6:   print "not "
OK6:    print "ok 6\n"
        end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
ok 6
OUTPUT

output_is( <<'CODE', <<OUTPUT, "eq_str");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
        new P1, I25
        new P2, I25
        set P1, "ABC"
        set P2, "ABC"
        eq_str P2, P1, OK1
        print "not "
OK1:    print "ok 1\n"

        set P2, "abc"
        eq_str P2, P1, BAD2
        branch OK2
BAD2:   print "not "
OK2:    print "ok 2\n"

        new P3, I23
        set P3, 0
        eq_str P2, P3, BAD3
        branch OK3
BAD3:   print "not "
OK3:    print "ok 3\n"

        eq_str P3, P2, BAD4
        branch OK4
BAD4:   print "not "
OK4:    print "ok 4\n"

        end
CODE
ok 1
ok 2
ok 3
ok 4
OUTPUT

output_is( <<'CODE', <<OUTPUT, "ne");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
        new P1, I25
        set P1, "ABC"
        set S1, "CBA"
        set S2, "ABC"
        set S3, "abc"
        set S4, "ABCD"
        set S5, "\0"
        set S6, ""

        ne  P1, S1, OK1
        print "not "
OK1:    print "ok 1\n"

        ne P1, S2, BAD2
        branch OK2
BAD2:   print "not "
OK2:    print "ok 2\n"

        ne  P1, S3, OK3
        print "not "
OK3:    print "ok 3\n"

        ne  P1, S4, OK4
        print "not "
OK4:    print "ok 4\n"

        ne  P1, S5, OK5
        print "not "
OK5:    print "ok 5\n"

        ne  P1, S6, OK6
        print "not "
OK6:    print "ok 6\n"

        end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
ok 6
OUTPUT

output_is( <<'CODE', <<OUTPUT, "ne_str");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
        new P1, I25
        new P2, I25
        set P1, "ABC"
        set P2, "abc"
        ne_str P2, P1, OK1
        print "not "
OK1:    print "ok 1\n"

        set P2, "ABC"
        ne_str P2, P1, BAD2
        branch OK2
BAD2:   print "not "
OK2:    print "ok 2\n"

        new P3, I23
        set P3, 0
        ne_str P2, P3, OK3
        print "not "
OK3:    print "ok 3\n"

        ne_str P3, P2, OK4
        print "not "
OK4:    print "ok 4\n"

        end
CODE
ok 1
ok 2
ok 3
ok 4
OUTPUT

output_is( <<'CODE', <<OUTPUT, "set const and chop");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
   new P0, I25
   set P0, "str"
   set S0, P0
   chopn S0, 2
   print "str"
   print "\n"
   end
CODE
str
OUTPUT

output_is( <<'CODE', <<OUTPUT, "increment");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
   new P0, I25
   set P0, 'a'
   inc P0
   print P0
   inc P0
   print P0
   set P0, 'E'
   inc P0
   print P0
   inc P0
   print P0
   print "\n"
   end
CODE
bcFG
OUTPUT

output_is( <<'CODE', <<OUTPUT, "decrement");
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
   new P0, I25
   set P0, '9'
   dec P0
   print P0
   dec P0
   print P0
   set P0, '-7'
   dec P0
   print P0
   dec P0
   print P0
   print "\n"
   end
CODE
87-8-9
OUTPUT


pir_output_is(<< 'CODE', << 'OUTPUT', "check whether interface is done");

.sub _main
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
    .local pmc pmc1
    pmc1 = new I25
    .local int bool1
    does bool1, pmc1, "scalar"
    print bool1
    print "\n"
    does bool1, pmc1, "no_interface"
    print bool1
    print "\n"
    end
.end
CODE
1
0
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "lower method");

.sub _main @MAIN
    find_type I21, "PerlArray"
    find_type I22, "PerlHash"
    find_type I23, "PerlInt"
    find_type I24, "PerlNum"
    find_type I25, "PerlString"
    find_type I26, "PerlUndef"
    .local pmc pmc1
    pmc1 = new String
    pmc1 = "ABCdef\n"
    $P0 = pmc1."lower"()
    print $P0
    # PerlString should inherit the method
    pmc1 = new I25
    pmc1 = "ABCdef\n"
    $P0 = pmc1."lower"()
    print $P0
.end
CODE
abcdef
abcdef
OUTPUT
