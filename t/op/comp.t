#! perl -w

use Parrot::Test tests => 6;

# some of these were failing with JIT/i386

output_is(<<'CODE', <<OUTPUT, "gt_ic_i_ic");
	set I0, 10
	gt 11, I0, ok1
	print "nok gt\n"
ok1:
	print "ok 1\n"
	gt 9, I0, nok1
	print "ok 2\n"
	branch ok2
nok1:
	print "nok gt 2\n"
ok2:
	end
CODE
ok 1
ok 2
OUTPUT

output_is(<<'CODE', <<OUTPUT, "ge_ic_i_ic");
	set I0, 10
	ge 11, I0, ok1
	print "nok ge\n"
ok1:
	print "ok 1\n"
	ge 9, I0, nok1
	print "ok 2\n"
	branch ok2
nok1:
	print "nok ge 2\n"
ok2:
	ge 10, I0, ok3
	print "nok ge 3\n"
ok3:
	print "ok 3\n"
	end
CODE
ok 1
ok 2
ok 3
OUTPUT

output_is(<<'CODE', <<OUTPUT, "le_ic_i_ic");
	set I0, 10
	le 9, I0, ok1
	print "nok le\n"
ok1:
	print "ok 1\n"
	le 11, I0, nok1
	print "ok 2\n"
	branch ok2
nok1:
	print "nok le 2\n"
ok2:
	le 10, I0, ok3
	print "nok le 3\n"
ok3:
	print "ok 3\n"
	end
CODE
ok 1
ok 2
ok 3
OUTPUT

output_is(<<'CODE', <<OUTPUT, "lt_ic_i_ic");
	set I0, 10
	lt 9, I0, ok1
	print "nok lt\n"
ok1:
	print "ok 1\n"
	lt 10, I0, nok1
	print "ok 2\n"
	branch ok2
nok1:
	print "nok lt 2\n"
ok2:
	end
CODE
ok 1
ok 2
OUTPUT

output_is(<<'CODE', <<OUTPUT, "eq_ic_i_ic");
	set I0, 10
	eq 9, I0, nok1
	print "ok 1\n"
	branch ok1
nok1:
	print "nok eq\n"
ok1:
	eq 10, I0, ok2
	print "nok eq 2\n"
	end
ok2:
	print "ok 2\n"
	eq 11, 10, nok3
	print "ok 3\n"
	end
nok3:
	print "nok 3 eq \n"
	end
CODE
ok 1
ok 2
ok 3
OUTPUT

output_is(<<'CODE', <<OUTPUT, "ne_ic_i_ic");
	set I0, 10
	ne 9, I0, ok1
	print "nok 1\n"
	branch nok1
ok1:
	print "ok 1\n"
nok1:
	ne 10, I0, nok2
	print "ok 2\n"
	branch ok2
nok2:
	print "nok 2\n"
ok2:
	ne 11, 10, ok3
	print "nok 3\n"
	end
ok3:
	print "ok 3\n"
	end
CODE
ok 1
ok 2
ok 3
OUTPUT

1;


