#! perl

use Parrot::Test tests => 24;
use Test::More;

output_is(<<CODE, <<OUTPUT, "Initial PerlHash tests");
	new	P0, .PerlHash

	set	P0["foo"], -7
	set	P0["bar"], 3.5
	set	P0["baz"], "value"

	set	I0, P0["foo"]
	set	N0, P0["bar"]
	set	S0, P0["baz"]

	eq	I0,-7,OK_1
	print	"not "
OK_1:	print	"ok 1\\n"
	eq	N0,3.500000,OK_2
	print	N0
OK_2:	print	"ok 2\\n"
	eq	S0,"value",OK_3
	print	S0
OK_3:	print	"ok 3\\n"

        set     S1, "oof"
        set     S2, "rab"
        set     S3, "zab"

	set	P0[S1], 7
	set	P0[S2], -3.5
	set	P0[S3], "VALUE"

	set	I0, P0[S1]
	set	N0, P0[S2]
	set	S0, P0[S3]

	eq	I0,7,OK_4
	print	"not "
OK_4:	print	"ok 4\\n"
	eq	N0,-3.500000,OK_5
	print	N0
OK_5:	print	"ok 5\\n"
	eq	S0,"VALUE",OK_6
	print	S0
OK_6:	print	"ok 6\\n"

	end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
ok 6
OUTPUT

output_is(<<'CODE', <<OUTPUT, "more than one PerlHash");
	new P0, .PerlHash
	set S0, "key"
	set P0[S0], 1

        new P1, .PerlHash
        set S1, "another_key"
        set P1[S1], 2

	set I0, P0[S0]
	set I1, P1[S1]

	print I0
	print "\n"
	print I1
	print "\n"
        end
CODE
1
2
OUTPUT

output_is(<<'CODE', <<OUTPUT, "hash keys with nulls in them");
	new P0, .PerlHash
	set S0, "parp\0me"
	set S1, "parp\0you"

	set P0[S0], 1		# $P0{parp\0me} = 1
	set P0[S1], 2		# $P0{parp\0you} = 2

	set I0, P0[S0]
	set I1, P0[S1]

	print I0
	print "\n"
	print I1
	print "\n"
	end
CODE
1
2
OUTPUT

output_is(<<'CODE', <<OUTPUT, "nearly the same hash keys");
	new P0, .PerlHash
	set S0, "a\0"
	set S1, "\0a"

	set P0[S0], 1
	set P0[S1], 2

	set I0, P0[S0]
	set I1, P0[S1]

	print I0
	print "\n"
	print I1
	print "\n"

	end
CODE
1
2
OUTPUT

output_is(<<'CODE', <<OUTPUT, "The same hash keys");
	new P0, .PerlHash
	set S0, "Happy"
	set S1, "Happy"

	set P0[S0], 1
	set I0, P0[S0]
	print I0
	print "\n"

	set P0[S1], 2
	set I1, P0[S1]

	print I1
	print "\n"

	end
CODE
1
2
OUTPUT

# NB Next test depends on "key2" hashing to zero, which it does with
# the current algorithm; if the algorithm changes, change the test!

output_is(<<'CODE', <<OUTPUT, "key that hashes to zero");
        new P0, .PerlHash
        set S0, "key2"
        set P0[S0], 1
        set I0, P0[S0]
	print I0
	print "\n"
	end
CODE
1
OUTPUT

output_is(<<'CODE', <<OUTPUT, "size of the hash");
	new P0, .PerlHash

	set P0["0"], 1
	set I0, P0
	print I0
	print "\n"

	set P0["1"], 1
	set I0, P0
	print I0
	print "\n"

	set P0["0"], 1
	set I0, P0
	print I0
	print "\n"

	end
CODE
1
2
2
OUTPUT

output_is(<<CODE, <<OUTPUT, "stress test: loop(set, check)");
	new	P0, .PerlHash

        set I0, 200
        set S0, "mikey"
        set P0[S0], "base"
        concat S1, S0, "s"
        set P0[S1], "bases"
        set S2, I0
        concat S1, S0, S2
        set P0[S1], "start"
        set S3, P0["mikey"]
        print S3
        print "\\n"
        set S3, P0["mikeys"]
        print S3
        print "\\n"
        set S3, P0["mikey200"]
        print S3
        print "\\n"
LOOP:
        eq I0, 0, DONE
        sub I0, I0, 1
        set S2, I0
        concat S1, S0, S2
        concat S4, S0, S2
        eq S1, S4, L1
        print "concat mismatch: "
        print S1
        print " vs "
        print S4
        print "\\n"
L1:
        set P0[S1], I0
        set I1, P0[S1]
        eq I0, I1, L2
        print "lookup mismatch: "
        print I0
        print " vs "
        print I1
        print "\\n"
L2:
        branch LOOP
DONE:
        set I0, P0["mikey199"]
        print I0
        print "\\n"
        set I0, P0["mikey117"]
        print I0
        print "\\n"
        set I0, P0["mikey1"]
        print I0
        print "\\n"
        set I0, P0["mikey23"]
        print I0
        print "\\n"
        set I0, P0["mikey832"]
        print I0
        print "\\n"
        end
CODE
base
bases
start
199
117
1
23
0
OUTPUT

# Check all values after setting all of them
output_is(<<CODE, <<OUTPUT, "stress test: loop(set), loop(check)");
	new	P0, .PerlHash

        set I0, 200
        set S0, "mikey"
SETLOOP:
        eq I0, 0, DONE
        sub I0, I0, 1
        set S2, I0
        concat S1, S0, S2
        set P0[S1], I0
        branch SETLOOP

        set I0, 200
GETLOOP:
        eq I0, 0, DONE
        sub I0, I0, 1
        set S2, I0
        concat S1, S0, S2
        set I1, P0[S1]
        eq I0, I1, L2
        print "lookup mismatch: "
        print I0
        print " vs "
        print I1
        print "\\n"
L2:
        branch GETLOOP

DONE:
        print "done\\n"
        end
CODE
done
OUTPUT

output_is(<<'CODE', <<OUTPUT, "Testing two hash indices with integers at a time");
      new P0, .PerlHash

      set P0["foo"],37
      set P0["bar"],-15

      set I0,P0["foo"]
      eq I0,37,OK_1
      print "not "
OK_1: print "ok 1\n"

      set I0,P0["bar"]
      eq I0,-15,OK_2
      print "not "
OK_2: print "ok 2\n"

      set S1,"foo"
      set I0,P0[S1]
      eq I0,37,OK_3
      print "not "
OK_3: print "ok 3\n"

      set S1,"bar"
      set I0,P0[S1]
      eq I0,-15,OK_4
      print "not "
OK_4: print "ok 4\n"

      end
CODE
ok 1
ok 2
ok 3
ok 4
OUTPUT

output_is(<<'CODE', <<OUTPUT, "Testing two hash indices with numbers at a time");
      new P0, .PerlHash

      set P0["foo"],37.100000
      set P0["bar"],-15.100000

      set N0,P0["foo"]
      eq N0,37.100000,OK_1
      print "not "
OK_1: print "ok 1\n"

      set N0,P0["bar"]
      eq N0,-15.100000,OK_2
      print "not "
OK_2: print "ok 2\n"

      set S1,"foo"
      set N0,P0[S1]
      eq N0,37.100000,OK_3
      print "not "
OK_3: print "ok 3\n"

      set S1,"bar"
      set N0,P0[S1]
      eq N0,-15.100000,OK_4
      print "not "
OK_4: print "ok 4\n"

      end
CODE
ok 1
ok 2
ok 3
ok 4
OUTPUT

output_is(<<'CODE', <<OUTPUT, "Testing two hash indices with strings at a time");
      new P0, .PerlHash

      set P0["foo"],"baz"
      set P0["bar"],"qux"

      set S0,P0["foo"]
      eq S0,"baz",OK_1
      print "not "
OK_1: print "ok 1\n"

      set S0,P0["bar"]
      eq S0,"qux",OK_2
      print "not "
OK_2: print "ok 2\n"

      set S1,"foo"
      set S0,P0[S1]
      eq S0,"baz",OK_3
      print "not "
OK_3: print "ok 3\n"

      set S1,"bar"
      set S0,P0[S1]
      eq S0,"qux",OK_4
      print "not "
OK_4: print "ok 4\n"

      end
CODE
ok 1
ok 2
ok 3
ok 4
OUTPUT


# So far, we've only used INTVALs, FLOATVALs and STRINGs as values
# and/or keys. Now we try PMCs.

output_is(<<'CODE', <<OUTPUT, "Setting & getting scalar PMCs");
      new P0, .PerlHash
      new P1, .PerlInt
      new P2, .PerlInt

      set S0, "non-PMC key"

      set P1, 10
      set P0[S0], P1
      set P2, P0[S0]
      eq P2, P1, OK1
      print "not "
OK1:  print "ok 1\n"

      set P1, -1234.000000
      set P0[S0], P1
      set P2, P0[S0]
      eq P2, P1, OK2
      print "not "
OK2:  print "ok 2\n"

      set P1, "abcdefghijklmnopq"
      set P0[S0], P1
      set P2, P0[S0]
      eq P2, P1, OK3
      print "not "
OK3:  print "ok 3\n"

      new P1, .PerlUndef
      set P0[S0], P1
      set P2, P0[S0]
      typeof S1, P2
      eq S1, "PerlUndef", OK4
      print "not "
OK4:  print "ok 4\n"

      end
CODE
ok 1
ok 2
ok 3
ok 4
OUTPUT

output_is(<<'CODE', <<OUTPUT, "Setting scalar PMCs & getting scalar values");
      new P0, .PerlHash
      new P1, .PerlInt

      set S0, "A rather large key"

      set I0, 10
      set P1, I0
      set P0[S0], P1
      set I1, P0[S0]
      eq I1, I0, OK1
      print "not "
OK1:  print "ok 1\n"

      set N0, -1234.000000
      set P1, N0
      set P0[S0], P1
      set N1, P0[S0]
      eq N1, N0, OK2
      print "not "
OK2:  print "ok 2\n"

      set S1, "abcdefghijklmnopq"
      set P1, S1
      set P0[S0], P1
      set S2, P0[S0]
      eq S2, S1, OK3
      print "not "
OK3:  print "ok 3\n"

      end
CODE
ok 1
ok 2
ok 3
OUTPUT

output_is(<<'CODE', <<OUTPUT, "Getting values from undefined keys");
      new P2, .PerlHash

      set I0, P2["qwerty"]
      set N0, P2["asdfgh"]
      set S0, P2["zxcvbn"]
      set P0, P2["123456"]

      eq I0, 0, OK1
      print "not "
OK1:  print "ok 1\n"

      eq N0, 0.0, OK2
      print "not "
OK2:  print "ok 2\n"

      eq S0, "", OK3
      print "not "
OK3:  print "ok 3\n"

      typeof S1, P0
      eq S1, "PerlUndef", OK4
      print "not "
OK4:  print "ok 4\n"
      end
CODE
ok 1
ok 2
ok 3
ok 4
OUTPUT

output_is(<<CODE, <<OUTPUT, "Setting & getting non-scalar PMCs");
        new P0,.PerlHash
        new P1,.PerlArray
        new P2,.PerlArray
        set P1[4],"string"
        set P0["one"],P1
        set P2,P0["one"]
        set S0,P2[4]
        print S0
        print "\\n"
        end
CODE
string
OUTPUT

output_is(<<'CODE', <<OUTPUT, "Testing clone");
    new P0, .PerlHash
    set S0, "a"
    set P0[S0], S0
    new P2, .PerlArray
    set P2, 2
    set P0["b"], P2

    # P0 = { a => "a", b => [undef, undef] }

    clone P1, P0
    set P0["c"], 4
    set P3, P0["b"]
    set P3, 3
    set P0["b"], P3
    set P1["a"], "A"

    # P0 = { a => "a", b => [undef, undef, undef], c => 4 }
    # P1 = { a => "A", b => [undef, undef] }

    set S0, P0["a"]
    eq S0, "a", ok1
    print "not "
ok1:
    print "ok 1\n"

    set P5, P0["b"]
    set I0, P5
    eq I0, 3, ok2
    print "not "
ok2:
    print "ok 2\n"

    set I0, P0["c"]
    eq I0, 4, ok3
    print "not "
ok3:
    print "ok 3\n"

    set S0, P1["a"]
    eq S0, "A", ok4
    print "not "
ok4:
    print "ok 4\n"

    set P5, P1["b"]
    set I0, P5
    eq I0, 2, ok5
    print "not ("
    print I0
    print ") "
ok5:
    print "ok 5\n"

# XXX: this should return undef or something, but it dies instead.
#     set P3, P0["c"]
#     unless P3, ok6
#     print "not "
# ok6:
#     print "ok 6\n"
     end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
OUTPUT

output_is(<<'CODE', <<OUTPUT, "Compound keys");
    new P0, .PerlHash
    new P1, .PerlHash
    new P2, .PerlArray
    set P1["b"], "ab"
    set P0["a"], P1
    set S0, P0["a";"b"]
    eq S0, "ab", ok1
    print "not "
ok1:
    print "ok 1\n"
    set P2[20], 77
    set P1["n"], P2
    set I0, P0["a";"n";20]
    eq I0, 77, ok2
    print "not "
ok2:
    print "ok 2\n"
    set S0, "a"
    set S1, "n"
    set I0, 20
    set I0, P0[S0;S1;I0]
    eq I0, 77, ok3
    print "not "
ok3:
    print "ok 3\n"
    set P0["c"], P2
    set P2[33], P1
    set S0, P0["c";33;"b"]
    eq S0, "ab", ok4
    print "not "
ok4:
    print "ok 4\n"
    set S0, "c"
    set I1, 33
    set S2, "b"
    set S0, P0[S0;I1;S2]
    eq S0, "ab", ok5
    print "not "
ok5:
    print "ok 5\n"
    set P1["b"], 47.11
    set N0, P0["c";I1;S2]
    eq N0, 47.11, ok6
    print "not "
ok6:
    print "ok 6\n"
    end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
ok 6
OUTPUT

output_is(<<'CODE', <<OUTPUT, "Getting PMCs from compound keys");
    new P0, .PerlHash
    new P1, .PerlHash
    new P2, .PerlInt
    set P2, 12
    set P1["b"], P2
    set P0["a"], P1
    set P3, P0["a";"b"]
    print P3
    print "\n"
    end
CODE
12
OUTPUT

# A hash is only false if it has size 0

output_is(<<'CODE', <<OUTPUT, "if (PerlHash)");
      new P0, .PerlHash

      if P0, BAD1
      print "ok 1\n"
      branch OK1
BAD1: print "not ok 1\n"
OK1:

      set P0["key"], "value"
      if P0, OK2
      print "not "
OK2:  print "ok 2\n"

      set P0["key"], ""
      if P0, OK3
      print "not "
OK3:  print "ok 3\n"

      new P1, .PerlUndef
      set P0["key"], P1
      if P0, OK4
      print "not "
OK4:  print "ok 4\n"

      end
CODE
ok 1
ok 2
ok 3
ok 4
OUTPUT

output_is(<<'CODE', <<OUTPUT, "unless (PerlHash)");
      new P0, .PerlHash

      unless P0, OK1
      print "not "
OK1:  print "ok 1\n"

      set P0["key"], "value"
      unless P0, BAD2
      print "ok 2\n"
      branch OK2
BAD2: print "not ok 2"
OK2:

      set P0["key"], "\0"
      unless P0, BAD3
      print "ok 3\n"
      branch OK3
BAD3: print "not ok 3"
OK3:

      new P1, .PerlUndef
      set P0["key"], P1
      unless P0, BAD4
      print "ok 4\n"
      branch OK4
BAD4: print "not ok 4"
OK4:

      end
CODE
ok 1
ok 2
ok 3
ok 4
OUTPUT

output_is(<<'CODE', <<OUTPUT, "defined");
    new P0, .PerlHash
    defined I0, P0
    print I0
    print "\n"
    defined I0, P1
    print I0
    print "\n"
    set P0["a"], 1
    defined I0, P0["a"]
    print I0
    print "\n"
    defined I0, P0["b"]
    print I0
    print "\n"
    new P1, .PerlUndef
    set P0["c"], P1
    defined I0, P0["c"]
    print I0
    print "\n"
    end

CODE
1
0
1
0
0
OUTPUT

output_is(<<'CODE', <<OUTPUT, "exists");
    new P0, .PerlHash
    set P0["a"], 1
    exists I0, P0["a"]
    print I0
    print "\n"
    exists I0, P0["b"]
    print I0
    print "\n"
    new P1, .PerlUndef
    set P0["c"], P1
    exists I0, P0["c"]
    print I0
    print "\n"
    end

CODE
1
0
1
OUTPUT

output_is(<<'CODE', <<OUTPUT, "delete");
    new P0, .PerlHash
    set P0["a"], 1
    exists I0, P0["a"]
    print I0
    print "\n"
    delete P0["a"]
    exists I0, P0["a"]
    print I0
    print "\n"
    end
CODE
1
0
OUTPUT

1;

