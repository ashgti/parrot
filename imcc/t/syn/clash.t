#!perl
use strict;
use TestCompiler tests => 9;

##############################
output_is(<<'CODE', <<'OUT', "if/unless");
.sub _test
	$I0 = 0
	if $I0 goto nok1
	print "ok 1\n"
nok1:
	unless $I0 goto ok1
	print "nok 1\n"
ok1:
	if I0, nok2
	print "ok 2\n"
nok2:
	unless I0 goto ok2
	print "nok 2\n"
ok2:
	end
.end
CODE
ok 1
ok 2
OUT

output_is(<<'CODE', <<'OUT', "if/unless");
.sub _test
	$I0 = 0
	$I1 = 1
	if $I0 == $I1 goto nok1
	print "ok 1\n"
nok1:
	unless $I0 == $I1 goto ok1
	print "nok 1\n"
ok1:
	if I0, nok2
	print "ok 2\n"
nok2:
	unless I0 goto ok2
	print "nok 2\n"
ok2:
	unless $I0 > $I1 goto ok3
	print "not "
ok3:	print "ok 3\n"
	end
.end
CODE
ok 1
ok 2
ok 3
OUT
##############################
output_is(<<'CODE', <<'OUT', "new");
.sub _test
	$P1 = new PerlString
	$P1 = "ok 1\n"
	new P1, .PerlString
	set P1, "ok 2\n"
	print $P1
	print P1
	end
.end
CODE
ok 1
ok 2
OUT

##############################
output_is(<<'CODE', <<'OUT', "clone");
.sub _test
	$P1 = new PerlString
	$P1 = "ok 1\n"
	$P0 = clone $P1
	new P1, .PerlString
	set P1, "ok 2\n"
	clone P0, P1
	print $P0
	print P0
	end
.end
CODE
ok 1
ok 2
OUT

##############################
output_is(<<'CODE', <<'OUT', "defined");
.sub _test
	$P1 = new PerlHash
	$I0 = defined $P1
	new P1, .PerlHash
	defined I0, P1
	print $I0
	print "\n"
	print I0
	print "\n"
	end
.end
CODE
1
1
OUT

##############################
output_is(<<'CODE', <<'OUT', "defined keyed");
.sub _test
	$P1 = new PerlHash
	$P1["a"] = "ok 1\n"
	$I0 = defined $P1["a"]
	new P1, .PerlHash
	set P1["a"], "ok 2\n"
	defined I0, P1["a"]
	defined I1, P1["b"]
	print $I0
	print "\n"
	print I0
	print "\n"
	print I1
	print "\n"
	end
.end
CODE
1
1
0
OUT

##############################
output_is(<<'CODE', <<'OUT', "parrot op as identifier");
.sub _test
	.local int set
	set = 5
	print set
	print "\n"
	inc set
	print set
	print "\n"
	end
.end
CODE
5
6
OUT

##############################
output_is(<<'CODE', <<'OUT', "parrot op as label");
.sub _test
	goto set
set:
	if I0, err
	if I0 goto err
	inc I0
	unless I0, err
	unless I0 goto err
	print "ok\n"
	end
err:
	print "nok\n"
	end
.end

CODE
ok
OUT

output_like(<<'CODE', <<'OUTPUT', "new with a native type");
.sub _main
        $P1 = new INTVAL
	print "never\n"
	end
.end
CODE
/error:\w+:Unknown PMC type 'INTVAL'/
OUTPUT

1;
