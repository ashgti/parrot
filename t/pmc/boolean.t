#! perl -w

use Parrot::Test tests => 7;
use Test::More;

output_is(<<'CODE', <<'OUTPUT', "Initialization, and integer tests");
	new P0,.Boolean

	set I0, P0
	eq I0,0,OK_1
	print "not "
OK_1:	print "ok 1\n"

        set I0, 1
        set P0, I0
        set I1, P0
        eq I1,1,OK_2
        print "not "
OK_2:   print "ok 2\n"

        set P0, -4
        set I0, P0
        eq I0,1,OK_3
        print "not "
OK_3:   print "ok 3\n"

        end
CODE
ok 1
ok 2
ok 3
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "Number tests");
        new P0, .Boolean

        set N0, 0
        set P0, N0
        set I0, P0
        eq I0, 0, OK_1
        print "not "
OK_1:   print "ok 1\n"


        set N0, 0.001
        set P0, N0
        set I0, P0
        eq I0, 1, OK_2
        print "not "
OK_2:   print "ok 2\n"

        end
CODE
ok 1
ok 2
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "String tests");
        new P0, .Boolean

        set S0, "0"
        set P0, S0
        set I0, P0
        eq I0, 0, OK_1
        print "not "
OK_1:   print "ok 1\n"


        set S0, "foo"
        set P0, S0
        set I0, P0
        eq I0, 1, OK_2
        print "not "
OK_2:   print "ok 2\n"

        set S0, ""
        set P0, S0
        set I0, P0
        eq I0, 0, OK_3
        print "not "
OK_3:   print "ok 3\n"

        end
CODE
ok 1
ok 2
ok 3
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "PMC to PMC");
        new P0, .Boolean
        new P1, .Boolean

        set P0, 1
        clone P1, P0
        set I0, P1
        eq I0, 1, OK_1
        print "not "
OK_1:   print "ok 1\n"

        set P0, 0
        set I0, P1
        eq I0, 1, OK_2
        print "not "
OK_2:   print "ok 2\n"

        set P1, 0
        set I0, P1
        eq I0, 0, OK_3
        print "not "
OK_3:   print "ok 3\n"

        end
CODE
ok 1
ok 2
ok 3
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "As boolean");
        new P0, .Boolean

        set P0, 1
        if P0, OK_1
        print "not "
OK_1:   print "ok 1\n"

        end
CODE
ok 1
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "Logic operations");
        new P0, .Boolean
        new P1, .Boolean
        new P2, .Boolean

        set P0, 1
        set P1, 0
        or P2, P0, P1
        set I0, P2
        eq I0, 1, OK_1
        print "not "
OK_1:   print "ok 1\n"

        or P2, P1, P1
        set I0, P2
        eq I0, 0, OK_2
        print "not "
OK_2:   print "ok 2\n"

        and P2, P0, P1
        set I0, P2
        eq I0, 0, OK_3
        print "not "
OK_3:   print "ok 3\n"

        set P0, 0
        set P1, 0
        and P2, P0, P1
        set I0, P2
        eq I0, 0, OK_4
        print "not "
OK_4:   print "ok 4\n"

        not P1, P1
        set I0, P1
        eq I0, 1, OK_5
        print "not "
OK_5:   print "ok 5\n"

        not P0, P0
        and P2, P0, P1
        set I0, P2
        eq I0, 1, OK_6
        print "not "
OK_6:   print "ok 6\n"

        xor P2, P0, P1
        set I0, P2
        eq I0, 0, OK_7
        print "not "
OK_7:   print "ok 7\n"

        not P0, P0
        xor P2, P0, P1
        set I0, P2
        eq I0, 1, OK_8
        print "not "
OK_8:   print "ok 8\n"

        not P1, P1
        xor P2, P0, P1
        set I0, P2
        eq I0, 0, OK_9
        print "not "
OK_9:   print "ok 9\n"

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

output_is(<<'CODE', <<'OUTPUT', "neg");
	new P0, .Boolean
        set P0, 1
        neg P0
        if P0, OK1
        print "not "
OK1:    print "ok 1\n"

        set P0, 0
        neg P0
        unless P0, OK2
        print "not "
OK2:    print "ok 2\n"

        end
CODE
ok 1
ok 2
OUTPUT

1;


