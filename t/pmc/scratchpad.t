#! perl -w

use Parrot::Test tests => 3;

output_is(<<CODE, <<OUTPUT, "direct set and get on scratchpad pmc");
	new_pad P20, 0
	new P0, .PerlInt
	set P0, 12

        set P20[0;"foo"], P0

        set P3, P20[0;"foo"]
	print P3
        print "\\n"

        set P3, P20["foo"]
	print P3
        print "\\n"

        set P0, 7
        set P20["foo"], P0
        set P3, P20["foo"]
	print P3
        print "\\n"
	end
CODE
12
12
7
OUTPUT

output_is(<<CODE, <<OUTPUT, "test nested pads");
	new_pad 0
        new_pad 1
        new_pad P10, 2

	new P0, .PerlInt
	set P0, 100
	new P1, .PerlInt
	set P1, 101
	new P2, .PerlInt
	set P2, 102

        set P10[0;"a"], P0
        set P10[1;"a"], P1
        set P10[2;"a"], P2

        set P3, P10["a"]
	print P3
        print "\\n"

        set P3, P10[2;"a"]
	print P3
        print "\\n"

        set P3, P10[1;"a"]
	print P3
        print "\\n"

        set P3, P10[0;"a"]
	print P3
        print "\\n"

        set P3, P10[-1;"a"]
	print P3
        print "\\n"

        set P3, P10[-2;"a"]
	print P3
        print "\\n"

        set P3, P10[-3;"a"]
	print P3
        print "\\n"

        push_pad P10
        find_lex P3, -1, "a"
	print P3
        print "\\n"

        find_lex P3, -2, "a"
	print P3
        print "\\n"

        find_lex P3, -3, "a"
	print P3
        print "\\n"

	end
CODE
102
102
101
100
102
101
100
102
101
100
OUTPUT

output_is(<<CODE, <<OUTPUT, "name and position");
	new_pad 0
        new_pad 1
        new_pad P10, 2

	new P0, .PerlInt
	set P0, 100
	new P1, .PerlInt
	set P1, 101
	new P2, .PerlInt
	set P2, 102
	new P3, .PerlInt
	set P3, 200
	new P4, .PerlInt
	set P4, 201
	new P5, .PerlInt
	set P5, 202

        set P10[0;"a"], P0
        set P10[1;"a"], P1
        set P10[2;"a"], P2
        set P10[0;"b"], P3
        set P10[1;"b"], P4
        set P10[2;1], P5

        set P3, P10[0]
	print P3
        print "\\n"

        set P3, P10[1]
	print P3
        print "\\n"

        set P3, P10[2;0]
	print P3
        print "\\n"

        set P3, P10[2;1]
	print P3
        print "\\n"

        set P3, P10[-1;0]
	print P3
        print "\\n"

        set P3, P10[-1;1]
	print P3
        print "\\n"

        set P10[1], P1
        set P3, P10[1]
	print P3
        print "\\n"

	end
CODE
102
202
102
202
102
202
101
OUTPUT

1;

