#! perl -w

use Parrot::Test tests => 10;
use Test::More qw(skip);
output_is(<<'CODE', <<'OUTPUT', "new iter");
	new P2, .PerlArray
	new P1, .Iterator, P2
	print "ok 1\n"
	end
CODE
ok 1
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "int test");
    .include "iterator.pasm"
	new P0, .PerlArray	# empty array
	new P2, .PerlArray	# array with 2 elements
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

output_is(<<'CODE', <<'OUTPUT', "hash iter 1");
    .include "iterator.pasm"
	new P0, .PerlHash	# empty Hash
	new P2, .PerlHash	# Hash with 2 elements
	set P2["ab"], 100
	set P2["xy"], "value"
	set I0, P2
	new P1, .Iterator, P2
	print "ok 1\n"
	set I1, P1
	eq I0, I1, ok2		# iter.length() == hash.length()
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
	# while (P1) { key = shift(P1) }
	unless P1, nok6
        shift S3, P1		# get hash.key
	eq S3, "ab", ok6
	eq S3, "xy", ok6
nok6:	print " not "
ok6:	print "ok 6\n"
	unless P1, nok7
        shift S3, P1
	eq S3, "ab", ok7
	eq S3, "xy", ok7
nok7:	print "not "
ok7:	print "ok 7\n"
	unless P1, ok8		# if(iter) == false after last
	print "not "
ok8:	print "ok 8\n"
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
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "hash iter 2");
    .include "iterator.pasm"
	new P0, .PerlHash	# Hash for iteration
	new P2, .PerlHash	# for test

	set I0, 65
	set I1, 35
	set I10, I1
fill:
	chr S0, I0
	set P0[S0], I0
	# XXX
	# swapping the next two lines breaks JIT/i386
	# the reason is the if/unless optimization: When the
	# previous opcode sets flags, these are used - but
	# there is no check, that the same register is used in the "if".
	inc I0
	dec I1
	if I1, fill

	new P1, .Iterator, P0
	set I0, P1
	eq I0, I10, ok1
	print "not "
ok1:
	print "ok 1\n"
	set P1, .ITERATE_FROM_START
get:
	unless P1, done
        shift S3, P1		# get hash.key
	set I0, P0[S3]		# and value
	set P2[S3], I0
	branch get

done:
	set I0, P2
	eq I0, I10, ok2
	print "not "
ok2:
	print "ok 2\n"
	end
CODE
ok 1
ok 2
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "hash iter - various types");
    .include "datatypes.pasm"	# type constants
    .include "iterator.pasm"
    new P1, .PerlHash
    set P1["int"], 10
    set P1["num"], 2.5
    set P1["str"], "string"
    new P2, .PerlString
    set P2, "string"
    set P1["pmc"], P2

    set I10, 0			# count items
    new P0, .Iterator, P1	# setup iterator for hash P1
    set P0, .ITERATE_FROM_START
iter_loop:
    unless P0, iter_end		# while (entries) ...
      inc I10
      shift S2, P0		# get key for entry
      typeof I0, P0[S2]		# get type of entry for key S2
      ne I0, .DATATYPE_INTVAL, no_int
      set I1, P0[S2]
      eq I1, 10, iter_loop
      print "not ok int\n"
    no_int:
      ne I0, .DATATYPE_FLOATVAL, no_num
      set N1, P0[S2]
      eq N1, 2.5, iter_loop
      print "not ok num\n"
    no_num:
      ne I0, .DATATYPE_STRING, no_str
      set S1, P0[S2]
      eq S1, "string", iter_loop
      print "not ok str\n"
    no_str:
      set P4, P0[S2]
      eq P4, P2, iter_loop
      print "not ok pmc\n"
      branch iter_loop
iter_end:
    print I10
    print "\n"
    end
CODE
4
OUTPUT

output_is(<<'CODE', <<OUTPUT, "string iteration forward");
    .include "iterator.pasm"
	new P2, .PerlString
	set P2, "parrot"
	new P1, .Iterator, P2
	set P1, .ITERATE_FROM_START
iter_loop:
        unless P1, iter_end		# while (entries) ...
	shift S1, P1
	print S1
	branch iter_loop
iter_end:
	print "\n"
	print P2
	print "\n"
	end
CODE
parrot
parrot
OUTPUT

output_is(<<'CODE', <<OUTPUT, "string iteration backward");
    .include "iterator.pasm"
	new P2, .PerlString
	set P2, "parrot"
	new P1, .Iterator, P2
	set P1, .ITERATE_FROM_END
iter_loop:
        unless P1, iter_end		# while (entries) ...
	pop S1, P1
	print S1
	branch iter_loop
iter_end:
	print "\n"
	print P2
	print "\n"
	end
CODE
torrap
parrot
OUTPUT

output_is(<<'CODE', <<OUTPUT, "string iteration forward get ord");
    .include "iterator.pasm"
	new P2, .PerlString
	set P2, "ABC"
	new P1, .Iterator, P2
	set P1, .ITERATE_FROM_START
iter_loop:
        unless P1, iter_end		# while (entries) ...
	shift I1, P1
	print I1
	branch iter_loop
iter_end:
	print "\n"
	print P2
	print "\n"
	end
CODE
656667
ABC
OUTPUT

output_is(<<'CODE', <<OUTPUT, "string iteration backward get ord");
.include "iterator.pasm"
	new P2, .PerlString
	set P2, "ABC"
	new P1, .Iterator, P2
	set P1, .ITERATE_FROM_END
iter_loop:
        unless P1, iter_end		# while (entries) ...
	pop I1, P1
	print I1
	branch iter_loop
iter_end:
	print "\n"
	print P2
	print "\n"
	end
CODE
676665
ABC
OUTPUT

SKIP: {
skip("N/Y: get_keyed_int gets rest of array", 1);
output_is(<<'CODE', <<'OUTPUT', "shift + index access");
    .include "iterator.pasm"

	new P2, .PerlArray	# array with 2 elements
	push P2, 10
	push P2, 20
	push P2, 30
	push P2, 40
	new P1, .Iterator, P2
	set P1, .ITERATE_FROM_START

	set I0, P1		# arr.length
	eq I0, 4, ok1
	print I0
	print " not "
ok1:	print "ok 1\n"

	shift I0, P1		# get one
	eq I0, 10, ok2
	print "not "
ok2:	print "ok 2\n"

        set I0, P1[0]		# first element of iter = next
	eq I0, 20, ok3
	print I0
	print " not "
ok3:	print "ok 3\n"

	set I0, P1		# arr.length of rest
	eq I0, 3, ok4
	print I0
	print " not "
ok4:	print "ok 4\n"
	end
CODE
ok 1
ok 2
ok 3
ok 4
OUTPUT
}
