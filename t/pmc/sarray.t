#! perl -w
# Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

t/pmc/sarray.t - Simple Array

=head1 SYNOPSIS

	% perl -Ilib t/pmc/sarray.t

=head1 DESCRIPTION

Tests the C<SArray> PMC, which is used for parameter-passing.

=cut

use Parrot::Test tests => 33;
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

output_is(<<'CODE', <<'OUTPUT', "Setting sarray size");
	new P0, .SArray
	set I0, P0
	eq I0, 0, OK_1
	print "not "
OK_1:	print "ok 1\n"

	set P0, 1
	set P0[0], 100
	set I0, P0
	eq I0, 1, OK_2
	print "not "
OK_2:	print "ok 2\n"
	end
CODE
ok 1
ok 2
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "attempt resize");
	new P0, .SArray
	set P0, 1
	set P0[0], 100
	set I0, P0
	eq I0, 1, OK_1
	print "not "
OK_1:	print "ok 1\n"

	set P0, 2
	end
CODE
ok 1
SArray: Can't resize!
OUTPUT
# '

output_is(<<'CODE', <<'OUTPUT', "indexed access");
	new P0, .SArray
	set P0, 3
	set P0[0], 100
	set P0[1], 200
	set P0[2], 300
	set I0, P0[0]
	eq I0, 100, ok1
	print "not "
ok1:	print "ok 1\n"
	set I0, P0[1]
	eq I0, 200, ok2
	print "not "
ok2:	print "ok 2\n"
	set I0, P0[2]
	eq I0, 300, ok3
	print "not "
ok3:	print "ok 3\n"
	set I0, P0[-1]
	eq I0, 300, ok4
	print "not "
ok4:	print "ok 4\n"
	set I0, P0[-2]
	eq I0, 200, ok5
	print "not "
ok5:	print "ok 5\n"
	set I0, P0[-3]
	eq I0, 100, ok6
	print "not "
ok6:	print "ok 6\n"
	end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
ok 6
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "push");
	new P0, .SArray
	set P0, 3
	push P0, 100
	push P0, 200
	push P0, 300
	set I0, P0[0]
	eq I0, 100, ok1
	print "not "
ok1:	print "ok 1\n"
	set I0, P0[1]
	eq I0, 200, ok2
	print "not "
ok2:	print "ok 2\n"
	set I0, P0[2]
	eq I0, 300, ok3
	print "not "
ok3:	print "ok 3\n"
	set I0, P0[-1]
	eq I0, 300, ok4
	print "not "
ok4:	print "ok 4\n"
	set I0, P0[-2]
	eq I0, 200, ok5
	print "not "
ok5:	print "ok 5\n"
	set I0, P0[-3]
	eq I0, 100, ok6
	print "not "
ok6:	print "ok 6\n"
	end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
ok 6
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "push / indexed");
	new P0, .SArray
	set P0, 3
	push P0, 100
	set P0[1], 200
	push P0, 300
	set I0, P0[0]
	eq I0, 100, ok1
	print "not "
ok1:	print "ok 1\n"
	set I0, P0[1]
	eq I0, 200, ok2
	print "not "
ok2:	print "ok 2\n"
	set I0, P0[2]
	eq I0, 300, ok3
	print "not "
ok3:	print "ok 3\n"
	set I0, P0[-1]
	eq I0, 300, ok4
	print "not "
ok4:	print "ok 4\n"
	set I0, P0[-2]
	eq I0, 200, ok5
	print "not "
ok5:	print "ok 5\n"
	set I0, P0[-3]
	eq I0, 100, ok6
	print "not "
ok6:	print "ok 6\n"
	end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
ok 6
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "mixed indexed");
	new P0, .SArray
	set P0, 4
	set P0[0], 1000
	set N0, 222.22
	set P0[1], N0
	set S0, "string\n"
	set P0[2], S0
	new P1, .Undef
	set P1, 42
	set P0[3], P1

	set I0, P0[0]
	eq I0, 1000, ok1
	print "not "
ok1:	print "ok 1\n"
	set N1, P0[1]
	eq N0, N1, ok2
	print "not "
ok2:	print "ok 2\n"
	set S1, P0[2]
	print S1
	set P3, P0[3]
	print P3
	print "\n"
	end
CODE
ok 1
ok 2
string
42
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "mixed push");
	new P0, .SArray
	set P0, 4
	push P0, 1000
	set N0, 222.22
	push P0, N0
	set S0, "string\n"
	push P0, S0
	new P1, .Undef
	set P1, 42
	push P0, P1

	set I0, P0[0]
	eq I0, 1000, ok1
	print "not "
ok1:	print "ok 1\n"
	set N1, P0[1]
	eq N0, N1, ok2
	print "not "
ok2:	print "ok 2\n"
	set S1, P0[2]
	print S1
	set P3, P0[3]
	print P3
	print "\n"
	end
CODE
ok 1
ok 2
string
42
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "mixed push - clone");
	new P2, .SArray
	set P2, 4
	push P2, 1000
	set N0, 222.22
	push P2, N0
	set S0, "string\n"
	push P2, S0
	new P1, .Undef
	set P1, 42
	push P2, P1

	clone P0, P2

	set I0, P0[0]
	eq I0, 1000, ok1
	print "not "
ok1:	print "ok 1\n"
	set N1, P0[1]
	eq N0, N1, ok2
	print "not "
ok2:	print "ok 2\n"
	set S1, P0[2]
	print S1
	set P3, P0[3]
	print P3
	print "\n"
	end
CODE
ok 1
ok 2
string
42
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "shift_integer");
	new P0, .SArray
	set P0, 3
	set P0[0], 100
	set P0[1], 200
	set P0[2], 300

	shift I0, P0
	eq I0, 100, ok1
	print "not "
ok1:	print "ok 1\n"
	set I0, P0
	eq I0, 2, ok2
ok2:	print "ok 2\n"

	shift I0, P0
	eq I0, 200, ok3
	print "not "
ok3:	print "ok 3\n"
	set I0, P0
	eq I0, 1, ok4
ok4:	print "ok 4\n"

	shift I0, P0
	eq I0, 300, ok5
	print "not "
ok5:	print "ok 5\n"
	set I0, P0
	eq I0, 0, ok6
ok6:	print "ok 6\n"
	end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
ok 6
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "mixed shift");
	new P0, .SArray
	set P0, 4
	push P0, 1000
	set N0, 222.22
	push P0, N0
	set S0, "string\n"
	push P0, S0
	new P1, .Undef
	set P1, 42
	push P0, P1

	shift I0, P0
	eq I0, 1000, ok1
	print "not "
ok1:	print "ok 1\n"
	shift N1, P0
	eq N0, N1, ok2
	print "not "
ok2:	print "ok 2\n"
	shift S1, P0
	print S1
	shift P3, P0
	print P3
	print "\n"
	end
CODE
ok 1
ok 2
string
42
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "iterator");
    .include "iterator.pasm"
	new P0, .SArray		# empty array
	new P2, .SArray		# array with 2 elements
	set P2, 2
	push P2, 10
	push P2, 20
	set I0, P2
	new P1, .Iterator, P2
	print "ok 1\n"
	set I1, P1
	eq I0, I1, ok2		# iter.length() == array.length()
	print "not "
ok2:	print "ok 2\n"
	new P1, .Iterator, P0
	set P1, .ITERATE_FROM_START
	print "ok 3\n"
	unless P1, ok4		# if(iter) == false on empty
	print "not "
ok4:	print "ok 4\n"
	new P1, .Iterator, P2
	set P1, .ITERATE_FROM_START
	if P1, ok5		# if(iter) == true on non empty
	print "not "
ok5:	print "ok 5\n"
	# now iterate over P2
	# while (P1) { element = shift(P1) }
	unless P1, nok6
        shift I3, P1
	eq I3, 10, ok6
nok6:	print "not "
ok6:	print "ok 6\n"
	unless P1, nok7
        shift I3, P1
	eq I3, 20, ok7
nok7:	print "not "
ok7:	print "ok 7\n"
	unless P1, ok8		# if(iter) == false after last
	print "not "
ok8:	print "ok 8\n"

	# now iterate from end
	set P1, .ITERATE_FROM_END
	if P1, ok9		# if(iter) == true on non empty
	print "not "
ok9:	print "ok 9\n"
	# while (P1) { element = pop(P1) }
	unless P1, nok10
        pop I3, P1
	eq I3, 20, ok10
nok10:	print "not "
ok10:	print "ok 10\n"
	unless P1, nok11
        pop I3, P1
	eq I3, 10, ok11
nok11:	print "not "
ok11:	print "ok 11\n"
	unless P1, ok12		# if(iter) == false after last
	print "not "
ok12:	print "ok 12\n"
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

output_like(<<'CODE', <<'OUTPUT', "const SArray 1");
    new P0, .ConstSArray
    set P0, 10
    new P1, .Integer
    inc P1
    setprop P0, "_ro", P1
    set P0, 20
    end
CODE
/^set_integer_native\(\) in ConstSArray/
OUTPUT

output_like(<<'CODE', <<'OUTPUT', "const SArray 2");
    new P0, .ConstSArray
    set P0, 2
    push P0, 10
    new P1, .Integer
    inc P1
    setprop P0, "_ro", P1
    set P2, P0[0]
    printerr P2
    printerr "\n"
    getprop P3, "_ro", P0
    printerr P3
    printerr "\n"
    shift I4, P0
    end
CODE
/^10
1
shift_integer\(\) in ConstSArray/
OUTPUT

output_like(<<'CODE', <<'OUTPUT', "const SArray try to unset _ro");
    new P0, .ConstSArray
    set P0, 10
    new P1, .Integer
    inc P1
    setprop P0, "_ro", P1
    printerr "ok\n"
    dec P1
    setprop P0, "_ro", P1
    end
CODE
/^ok
morph\(\) in ConstSArray/
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "check whether interface is done");

.sub _main
    .local pmc pmc1
    pmc1 = new SArray
    .local int bool1
    does bool1, pmc1, "scalar"
    print bool1
    print "\n"
    does bool1, pmc1, "array"
    print bool1
    print "\n"
    does bool1, pmc1, "no_interface"
    print bool1
    print "\n"
    end
.end
CODE
0
1
0
OUTPUT

output_is(<< "CODE", << 'OUTPUT', "Access via Key PMC");
@{[ $fp_equality_macro ]}
	new P0, .SArray
	set P0, 4
	set P0[0], 100
	set P0[1], 12.298
	set P0[2], "yarrAS"
        new P31, .Hash
        set P31["Test"], "ok"
        set P0[3], P31
        new P1, .Key
        set P1, 0
	set I0, P0[P1]
	eq I0, 100, ok1
	print "not "
ok1:	print "ok 1\\n"
        new P2, .Key
        set P2, 1
	set N0, P0[P2]
	.fp_eq(N0, 12.298, ok2)
	print "not "
ok2:	print "ok 2\\n"
        new P3, .Key
        set P3, 2
	set S0, P0[P3]
	eq S0, "yarrAS", ok3
	print "not "
ok3:	print "ok 3\\n"
        new P4, .Key
        set P4, 3
	set P5, P0[P4]
        set S1, P5["Test"]
	eq S1, "ok", ok4
	print "not "
ok4:	print "ok 4\\n"
	end
CODE
ok 1
ok 2
ok 3
ok 4
OUTPUT

output_is(<< 'CODE', << 'OUTPUT', "Store PMC, get int");
	new P0, .SArray
	set P0, 2
        new P1, .Integer
        set P1, 11
        new P2, .Float
        set P2, 1.1
	set P0[0], P1
	set P0[1], P2
	set I0, P0[0]
	eq I0, 11, ok1
	print "not "
ok1:	print "ok 1\n"
	set I0, P0[1]
	eq I0, 1, ok2
	print "not "
ok2:	print "ok 2\n"
        end
CODE
ok 1
ok 2
OUTPUT

output_like(<< 'CODE', << 'OUTPUT', "Store num, get int");
	new P0, .SArray
	set P0, 1
        set P0[0], 4.2
        set I0, P0[0]
        print I0
        end
CODE
/SArray: Entry not an integer!/
OUTPUT

output_like(<< 'CODE', << 'OUTPUT', "Store string, get int");
	new P0, .SArray
	set P0, 1
        set P0[0], "Non-numeric string"
        set I0, P0[0]
        print I0
        end
CODE
/SArray: Entry not an integer!/
OUTPUT

output_is(<< "CODE", << 'OUTPUT', "Store PMC, get num");
@{[ $fp_equality_macro ]}
	new P0, .SArray
	set P0, 2
        new P1, .Integer
        set P1, 11
        new P2, .Float
        set P2, 1.1
	set P0[0], P1
	set P0[1], P2
	set N0, P0[0]
	.fp_eq(N0, 11.0, ok1)
	print "not "
ok1:	print "ok 1\\n"
	set N0, P0[1]
	.fp_eq(N0, 1.1, ok2)
	print "not "
ok2:	print "ok 2\\n"
        end
CODE
ok 1
ok 2
OUTPUT

output_like(<< 'CODE', << 'OUTPUT', "Store int, get num");
	new P0, .SArray
	set P0, 1
        set P0[0], 12
        set N0, P0[0]
        print N0
        end
CODE
/SArray: Entry not a number!/
OUTPUT

output_like(<< 'CODE', << 'OUTPUT', "Store string, get num");
	new P0, .SArray
	set P0, 1
        set P0[0], "Non-numeric string"
        set N0, P0[0]
        print N0
        end
CODE
/SArray: Entry not a number!/
OUTPUT

output_is(<< 'CODE', << 'OUTPUT', "Store PMC, get string");
	new P0, .SArray
	set P0, 2
        new P1, .String
        set P1, "Hello"
        new P2, .Integer
        set P2, 1010
	set P0[0], P1
	set P0[1], P2
	set S0, P0[0]
	eq S0, "Hello", ok1
	print "not "
ok1:	print "ok 1\n"
	set S0, P0[1]
	eq S0, "1010", ok2
	print "not "
ok2:	print "ok 2\n"
        end
CODE
ok 1
ok 2
OUTPUT

output_like(<< 'CODE', << 'OUTPUT', "Store int, get string");
	new P0, .SArray
	set P0, 1
        set P0[0], 12
        set S0, P0[0]
        print S0
        end
CODE
/SArray: Entry not a string!/
OUTPUT

output_like(<< 'CODE', << 'OUTPUT', "Store num, get string");
	new P0, .SArray
	set P0, 1
        set P0[0], 12.5
        set S0, P0[0]
        print S0
        end
CODE
/SArray: Entry not a string!/
OUTPUT

output_is(<< "CODE", << 'OUTPUT', "Store num, get PMC");
@{[ $fp_equality_macro ]}
	new P0, .SArray
	set P0, 2
	set P0[0], 12.239
	set P0[1], -1.9742
        new P1, .Float
        set P1, P0[0]
        set N0, P1
	.fp_eq(N0, 12.239, ok1)
	print "not "
ok1:	print "ok 1\\n"
        new P2, .Integer
        set P2, P0[1]
        set N0, P2
	.fp_eq(N0, -1.9742, ok2)
	print "not "
ok2:	print "ok 2\\n"
        end
CODE
ok 1
ok 2
OUTPUT

output_like(<< 'CODE', << 'OUTPUT', "Out-of-bounds access: int");
	new P0, .SArray
	set P0, 1
        set P0[5], 12
        set I0, P0[5]
        print I0
        end
CODE
/SArray index out of bounds/
OUTPUT

output_like(<< 'CODE', << 'OUTPUT', "Out-of-bounds access: num");
	new P0, .SArray
	set P0, 1
        set P0[5], 12.5
        set N0, P0[5]
        print N0
        end
CODE
/SArray index out of bounds/
OUTPUT

output_like(<< 'CODE', << 'OUTPUT', "Out-of-bounds access: string");
	new P0, .SArray
	set P0, 1
        set P0[5], "asdf"
        set S0, P0[5]
        print S0
        end
CODE
/SArray index out of bounds/
OUTPUT

output_like(<< 'CODE', << 'OUTPUT', "Out-of-bounds access: push int");
	new P0, .SArray
        push P0, 12
        set I0, P0[0]
        print I0
        end
CODE
/SArray index out of bounds/
OUTPUT

output_like(<< 'CODE', << 'OUTPUT', "Out-of-bounds access: push num");
	new P0, .SArray
        push P0, 12.09
        set N0, P0[0]
        print N0
        end
CODE
/SArray index out of bounds/
OUTPUT

output_like(<< 'CODE', << 'OUTPUT', "Out-of-bounds access: push string");
	new P0, .SArray
        push P0, "Ygnve"
        set S0, P0[0]
        print S0
        end
CODE
/SArray index out of bounds/
OUTPUT

output_like(<< 'CODE', << 'OUTPUT', "Out-of-bounds access: push pmc");
	new P0, .SArray
        new P1, .Integer
        set P1, 1234
        push P0, P1
        set I0, P0[0]
        print I0
        end
CODE
/SArray index out of bounds/
OUTPUT


