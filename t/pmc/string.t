#! perl
# Copyright (C) 2001-2005, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 46;

=head1 NAME

t/pmc/string.t - Strings

=head1 SYNOPSIS

	% prove t/pmc/string.t

=head1 DESCRIPTION

Tests the C<String> PMC.

=cut

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

pasm_output_is(<<CODE, <<OUTPUT, "Set/get strings");
        new P0, .String
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

# XXX: can't handle double yet - string_from_num() in src/string.c
#        set P0, 1.23456789
#        print P0
#        set S0, P0
#        print S0
#        eq S0, "1.23456789", OK5
#        print "not "
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

pasm_output_is(<<CODE, <<OUTPUT, "Setting integers");
        new P0, .String
        set P0, "1"
        set I0, P0
        print I0
        print "\\n"

        new P0, .String
        set P0, "2.0"
        set I0, P0
        print I0
        print "\\n"

        new P0, .String
        set P0, ""
        set I0, P0
        print I0
        print "\\n"

        new P0, .String
        set P0, "\0"
        set I0, P0
        print I0
        print "\\n"

        new P0, .String
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

pasm_output_is(<<"CODE", <<OUTPUT, "Setting numbers");
@{[ $fp_equality_macro ]}
        new P0, .String
        set P0, "1"
        set N0, P0
        .fp_eq(N0, 1.0, OK1)
        print "not "
OK1:    print "ok 1\\n"

        new P0, .String
        set P0, "2.0"
        set N0, P0
        .fp_eq(N0, 2.0, OK2)
        print "not "
OK2:    print "ok 2\\n"

        new P0, .String
        set P0, ""
        set N0, P0
        .fp_eq(N0, 0.0, OK3)
        print "not "
OK3:    print "ok 3\\n"

        new P0, .String
        set P0, "\0"
        set N0, P0
        .fp_eq(N0, 0.0, OK4)
        print "not "
OK4:    print "ok 4\\n"

        new P0, .String
        set P0, "foo"
        set N0, P0
        .fp_eq(N0, 0.0, OK5)
        print "not "
OK5:    print "ok 5\\n"

        new P0, .String
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

pasm_output_is(<<CODE, <<OUTPUT, "ensure that concat ppp copies strings");
	new P0, .String
	new P1, .String
	new P2, .String
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

pasm_output_is(<<CODE, <<OUTPUT, "ensure that concat pps copies strings");
	new P0, .String
	new P1, .String

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

pasm_output_is(<<CODE, <<OUTPUT, "Setting string references");
	new P0, .String
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

pasm_output_is(<<CODE, <<OUTPUT, "Assigning string copies");
	new P0, .String
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

pasm_output_is(<<'CODE', <<OUTPUT, "repeat");
	new P0, .String
	set P0, "x"
	new P1, .Integer
	set P1, 12
	new P2, .String
	repeat P2, P0, P1
        print P2
        print "\n"

        set P0, "y"
        new P1, .Float
        set P1, 6.5
        repeat P2, P0, P1
        print P2
        print "\n"

        set P0, "z"
        new P1, .String
        set P1, "3"
        repeat P2, P0, P1
        print P2
        print "\n"

        set P0, "a"
        new P1, .Undef
        repeat P2, P0, P1
        print P2
        print "\n"

	end
CODE
xxxxxxxxxxxx
yyyyyy
zzz

OUTPUT

pasm_output_is(<<'CODE', <<OUTPUT, "n_repeat");
	new P0, .String
	set P0, "x"
	new P1, .Integer
	set P1, 12
	n_repeat P2, P0, P1
        print P2
        print "\n"

        set P0, "y"
        new P1, .Float
        set P1, 6.5
        n_repeat P3, P0, P1
        print P3
        print "\n"

        set P0, "z"
        new P1, .String
        set P1, "3"
        n_repeat P4, P0, P1
        print P4
        print "\n"

        set P0, "a"
        new P1, .Undef
        n_repeat P5, P0, P1
        print P5
        print "\n"

	end
CODE
xxxxxxxxxxxx
yyyyyy
zzz

OUTPUT
pasm_output_is(<<'CODE', <<OUTPUT, "repeat_int");
	new P0, .String
	set P0, "x"
	set I1, 12
	new P2, .String
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

pasm_output_is(<<'CODE', <<OUTPUT, "n_repeat_int");
	new P0, .String
	set P0, "x"
	set I1, 12
	n_repeat P2, P0, I1
        print P2
        print "\n"

        set P0, "za"
        n_repeat P3, P0, 3
        print P3
        print "\n"
	end
CODE
xxxxxxxxxxxx
zazaza
OUTPUT

pasm_output_is(<<CODE, <<OUTPUT, "if(String)");
        new P0, .String
	set S0, "True"
	set P0, S0
        if P0, TRUE
        print "false"
        branch NEXT
TRUE:   print "true"
NEXT:   print "\\n"

        new P1, .String
        set S1, ""
        set P1, S1
        if P1, TRUE2
        print "false"
        branch NEXT2
TRUE2:  print "true"
NEXT2:  print "\\n"

        new P2, .String
        set S2, "0"
        set P2, S2
        if P2, TRUE3
        print "false"
        branch NEXT3
TRUE3:  print "true"
NEXT3:  print "\\n"

        new P3, .String
        set S3, "0123"
        set P3, S3
        if P3, TRUE4
        print "false"
        branch NEXT4
TRUE4:  print "true"
NEXT4:  print "\\n"

        new P4, .String
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

pasm_output_is(<<'CODE', <<OUTPUT, "concat");
	new P0, .String
	new P1, .Undef
	set P0, "foo"
	concat	P1, P0, P0

	print	P0
	print "\n"
	print	P1
	print "\n"

	new P0, .String
	new P1, .Undef
	set P0, "bar"
	concat	P0, P0, P1

	print	P0
	print "\n"
	print	P1
	print "\n"

	new P0, .String
	new P1, .Undef
	set P1, "str"
	concat	P1, P0, P1

	print	P0
	print "\n"
	print	P1
	print "\n"
	end
CODE
foo
foofoo
bar


str
OUTPUT

pasm_output_is(<<'CODE', <<OUTPUT, "n_concat");
	new P0, .String
	set P0, "foo"
	n_concat	P1, P0, P0

	print	P0
	print "\n"
	print	P1
	print "\n"

	new P0, .String
	set P0, "foo"
	n_concat P2, P0, "bar"

	print	P0
	print "\n"
	print	P2
	print "\n"

	end
CODE
foo
foofoo
foo
foobar
OUTPUT
pasm_output_is(<<'CODE', <<OUTPUT, "cmp");
	new P1, .String
	new P2, .String

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

pasm_output_is(<<'CODE', <<OUTPUT, "cmp with Integer");
	new P1, .Integer
	new P2, .String
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

pasm_output_is(<<'CODE', <<OUTPUT, "substr");
        new P0, .String
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

pasm_output_like(<<'CODE', <<'OUTPUT', "Out-of-bounds substr, +ve offset");
        new P0, .String
        set P0, "Woburn"
        substr S0, P0, 123, 22
        end
CODE
/^Cannot take substr outside string$/
OUTPUT

pasm_output_like(<<'CODE', <<'OUTPUT', "Out-of-bounds substr, -ve offset");
        new P0, .String
        set P0, "Woburn"
        substr S0, P0, -123, 22
        end
CODE
/^Cannot take substr outside string$/
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "bands NULL string");
        new P1, .String
	new P2, .String
	new P3, .String
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

pasm_output_is( <<'CODE', <<OUTPUT, "bands 2");
        new P1, .String
	new P2, .String
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

pasm_output_is( <<'CODE', <<OUTPUT, "bands 3");
        new P1, .String
	new P2, .String
	new P0, .String
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

pasm_output_is( <<'CODE', <<OUTPUT, "bors NULL string");
        new P1, .String
	new P2, .String
	new P3, .String
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

pasm_output_is( <<'CODE', <<OUTPUT, "bors 2");
        new P1, .String
	new P2, .String
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

pasm_output_is( <<'CODE', <<OUTPUT, "bors 3");
        new P1, .String
	new P2, .String
	new P0, .String
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

pasm_output_is( <<'CODE', <<OUTPUT, "bxors NULL string");
     new P1, .String
     new P2, .String
     new P3, .String
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

pasm_output_is( <<'CODE', <<OUTPUT, "bxors 2");
    new P1, .String
    new P2, .String
    new P3, .String
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

pasm_output_is( <<'CODE', <<OUTPUT, "bxors 3");
    new P1, .String
    new P2, .String
    new P0, .String
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

pasm_output_is( <<'CODE', <<OUTPUT, "bnots NULL string");
     new P1, .String
     new P2, .String
     new P3, .String
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

pasm_output_is( <<'CODE', <<OUTPUT, "eq_str");
        new P1, .String
        new P2, .String
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

        new P3, .Integer
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

pasm_output_is( <<'CODE', <<OUTPUT, "ne_str");
        new P1, .String
        new P2, .String
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

        new P3, .Integer
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

pasm_output_is( <<'CODE', <<OUTPUT, "set const and chop");
   new P0, .String
   set P0, "str"
   set S0, P0
   chopn S0, 2
   print "str"
   print "\n"
   end
CODE
str
OUTPUT


pir_output_is(<< 'CODE', << 'OUTPUT', "check whether interface is done");

.sub _main
    .local pmc pmc1
    pmc1 = new String
    .local int bool1
    does bool1, pmc1, "scalar"
    print bool1
    print "\n"
    does bool1, pmc1, "string"
    print bool1
    print "\n"
    does bool1, pmc1, "no_interface"
    print bool1
    print "\n"
    end
.end
CODE
1
1
0
OUTPUT

pasm_output_is(<< 'CODE', << 'OUTPUT', "Clone");
    new P0, .String
    set P0, "Tacitus\n"
    clone P1, P0
    set P0, ""
    print P1
    end
CODE
Tacitus
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "set P[x], i");
  new P0, .String
  set P0, "abcdef\n"
  set P0[2], 65
  print P0
  end
CODE
abAdef
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "set P[x], s");
  new P0, .String
  set P0, "abcdef\n"
  set P0[2], "AB"
  print P0
  end
CODE
abABef
OUTPUT

pir_output_is( <<'CODE', <<OUTPUT, 'String."replace"' );
.sub _main
  $P0 = new String
  $P0 = "hello world\n"
  print $P0
  $P0."replace"("l", "-")
  print $P0
  $P0."replace"("wo", "!!!!")
  print $P0
  $P0."replace"("he-", "")
  print $P0
.end
CODE
hello world
he--o wor-d
he--o !!!!r-d
-o !!!!r-d
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "set I0, P0 - string_to_int");
  new P0, .String
  set P0, "12.3E5\n"
  set I0, P0
  print I0
  print "\n"
  end
CODE
12
OUTPUT

pir_output_is( <<'CODE', <<OUTPUT, 'String."trans"' );

# tr{wsatugcyrkmbdhvnATUGCYRKMBDHVN}
#            {WSTAACGRYMKVHDBNTAACGRYMKVHDBN};

# create tr table at compile-time
.sub tr_00_init :immediate
    .local pmc tr_array
    tr_array = new .FixedIntegerArray   # Todo char array
    tr_array = 256                      # Python compat ;)
    .local string from, to
    from = 'wsatugcyrkmbdhvnATUGCYRKMBDHVN'
    to   = 'WSTAACGRYMKVHDBNTAACGRYMKVHDBN'
    .local int i, ch, r, len
    len = length from
    null i
loop:
    ch = ord from, i
    r  = ord to,   i
    tr_array[ch] = r
    inc i
    if i < len goto loop
    .return(tr_array)
.end

.sub tr_test :main
    .local string s, t
    .local int el
    s = "atugcsATUGCS"
    .const .Sub tr_00 = 'tr_00_init'
    el = elements tr_00
    print el
    print "\n"
    trans s, tr_00
    print s
    print "\n"
.end
CODE
256
TAACGSTAACGS
OUTPUT


pir_output_is( <<'CODE', <<OUTPUT, "reverse P0 - reverse string");
.sub main :main
  $P0 = new String
  $P0 = "torrap"
  reverse $P0
  print $P0
  print "\n"
  end
.end
CODE
parrot
OUTPUT


pir_output_is( <<'CODE', <<OUTPUT, "is_integer - check integer");
.sub main :main
  $P0 = new String

  $P0 = "543"
  $I0 = is_integer $P0
  print $I0
  print "\n"

  $P0 = "4.3"
  $I0 = is_integer $P0
  print $I0
  print "\n"

  $P0 = "foo"
  $I0 = is_integer $P0
  print $I0
  print "\n"

  $P0 = "-1"
  $I0 = is_integer $P0
  print $I0
  print "\n"

  $P0 = "+-1"
  $I0 = is_integer $P0
  print $I0
  print "\n"

  $P0 = "+1"
  $I0 = is_integer $P0
  print $I0
  print "\n"

  end
.end
CODE
1
0
0
1
0
1
OUTPUT

pir_output_is( <<'CODE', <<OUTPUT, "new_from_string");
.sub main :main
    .const .String ok = "ok\n"
    print ok
.end
CODE
ok
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "get_string returns COW string");
.sub main :main
  $P0 = new .String
  $P0 = "Foo"

  $S0 = $P0
  substr $S0, 0, 1, "B"

  print $P0
  print "\n"
  print $S0
  print "\n"
.end
CODE
Foo
Boo
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "to_int 1");
.sub 'main' :main
    .local pmc s
    s = new .String
    s = "123"
    $I0 = s.to_int(10)
    print $I0
    print "\n"
    s = "2a"
    $I0 = s.to_int(16)
    print $I0
    print "\n"
    s = "1001"
    $I0 = s.to_int(2)
    print $I0
    print "\n"
.end
CODE
123
42
9
OUTPUT

pir_output_like( <<'CODE', <<'OUTPUT', "to_int 2");
.sub 'main' :main
    .local pmc s
    s = new .String
    s = "123"
    $I0 = s.to_int(3)
    print "never"
.end
CODE
/invalid conversion to int - bad char 3/
OUTPUT

pir_output_like( <<'CODE', <<'OUTPUT', "to_int 3");
.sub 'main' :main
    .local pmc s
    s = new .String
    s = "123"
    $I0 = s.to_int(37)
    print "never"
.end
CODE
/invalid conversion to int - bad base 37/
OUTPUT
