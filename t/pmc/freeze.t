#! perl -w
# Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

t/pmc/freeze.t - Archiving

=head1 SYNOPSIS

	% perl -Ilib t/pmc/freeze.t

=head1 DESCRIPTION

Tests the freeze/thaw archiving subsystem.

=cut

use Parrot::Test tests => 21;
use Test::More;

END { unlink "temp.fpmc"; };

output_is(<<'CODE', <<'OUTPUT', "freeze/thaw a PerlInt");
    new P1, .PerlInt
    set P1, 777
    freeze S0, P1

    thaw P10, S0
    typeof S10, P10
    print S10
    print " "
    print P10
    print "\n"
    end
CODE
PerlInt 777
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "freeze/thaw a PerlString");
    new P1, .PerlString
    set P1, "foo"
    freeze S0, P1

    thaw P10, S0
    typeof S10, P10
    print S10
    print " "
    print P10
    print "\n"
    end
CODE
PerlString foo
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "freeze/thaw a PerlNum");
    new P1, .PerlNum
    set P1, 3.14159
    freeze S0, P1

    thaw P10, S0
    typeof S10, P10
    print S10
    print " "
    print P10
    print "\n"
    end
CODE
PerlNum 3.141590
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "freeze/thaw a PerlArray");
    new P0, .PerlArray
    new P1, .PerlInt
    set P1, 666
    push P0, P1
    new P1, .PerlInt
    set P1, 777
    push P0, P1
    freeze S0, P0

    thaw P10, S0
    typeof S10, P10	# type
    print S10
    print " "
    set I11, P10	# elements
    print I11
    print "\n"
    set P12, P10[0]
    print P12
    print "\n"
    set P12, P10[1]
    print P12
    print "\n"
    end
CODE
PerlArray 2
666
777
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "freeze PerlArray self-ref");
    new P0, .PerlArray
    new P1, .PerlInt
    set P1, 666
    push P0, P1
    push P0, P0
    freeze S0, P0
    print "ok\n"
    end
CODE
ok
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "freeze/thaw PerlArray self-ref");
    new P1, .PerlInt
    set P1, 666
    new P0, .PerlArray
    push P0, P1
    new P1, .PerlInt
    set P1, 777
    push P0, P1
    push P0, P0
    freeze S0, P0

    thaw P10, S0
    typeof S10, P10
    print S10
    print " "
    set I11, P10
    print I11
    print "\n"
    set P12, P10[0]
    print P12
    print "\n"
    set P12, P10[1]
    print P12
    print "\n"
    set P12, P10[2]
    set P13, P12[0]
    print P13
    print "\n"
    end
CODE
PerlArray 3
666
777
666
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "freeze/thaw PerlArray self-ref, contained Arrray");
    new P1, .PerlInt
    set P1, 666
    new P0, .PerlArray
    push P0, P1
    new P1, .PerlInt
    set P1, 777
    push P0, P1

    new P2, .PerlArray
    new P4, .PerlInt
    set P4, 4
    push P2, P4
    push P2, P0

    push P0, P2
    push P0, P1
    freeze S0, P0

    thaw P10, S0
    typeof S10, P10
    print S10
    print " "
    set I11, P10
    print I11
    print "\n"
    set P12, P10[0]
    print P12
    print "\n"
    set P12, P10[1]
    print P12
    print "\n"
    set P12, P10[2]
    set P13, P12[0]
    print P13
    print "\n"
    set P13, P12[1]
    set P14, P13[0]
    print P14
    print "\n"
    set P12, P10[3]
    print P12
    print "\n"
    end
CODE
PerlArray 4
666
777
4
666
777
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "freeze/thaw a PerlHash");
    new P1, .PerlInt
    set P1, 666
    new P0, .PerlHash
    set P0["k1"], P1
    new P1, .PerlInt
    set P1, 777
    set P0["k2"], P1
    freeze S0, P0

    thaw P10, S0
    typeof S10, P10
    print S10
    print " "
    set I11, P10
    print I11
    print "\n"
    set P12, P10["k1"]
    print P12
    print "\n"
    set P12, P10["k2"]
    print P12
    print "\n"
    end
CODE
PerlHash 2
666
777
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "freeze/thaw a PerlInt with prop");
    new P1, .PerlInt
    set P1, 666
    new P2, .PerlInt
    set P2, 42
    setprop P1, "answer", P2
    freeze S0, P1

    thaw P10, S0
    typeof S10, P10
    print S10
    print " "
    set I11, P10
    print I11
    print "\n"
    getprop P12, "answer", P10
    print P12
    print "\n"
    end
CODE
PerlInt 666
42
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "freeze/thaw Array w PerlInt with prop");
    new P0, .PerlArray
    new P1, .PerlInt
    set P1, 666
    push P0, P1
    new P2, .PerlInt
    set P2, 777
    push P0, P2
    new P3, .PerlInt
    set P3, 42
    setprop P1, "answer", P3

    freeze S0, P0

    thaw P10, S0
    typeof S10, P10
    print S10
    print " "
    set I11, P10
    print I11
    print "\n"
    set P12, P10[0]
    print P12
    print "\n"
    set P13, P10[1]
    print P13
    print "\n"
    getprop P12, "answer", P12
    print P12
    print "\n"
    end
CODE
PerlArray 2
666
777
42
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "freeze/thaw a NULL pmc");
    null P0
    freeze S0, P0
    thaw P10, S0
    defined I0, P10
    unless I0, ok
    print "not "
ok: print "ok\n"
    end
CODE
ok
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "freeze/thaw array w NULL pmc");
    new P0, .PerlArray
    null P1
    push P0, P1
    new P1, .PerlInt
    set P1, 10
    push P0, P1

    freeze S0, P0
    thaw P10, S0

    typeof S10, P10
    print S10
    print " "
    set I11, P10
    print I11
    print "\n"
    set P11, P10[0]
    defined I0, P11
    unless I0, ok
    print "not "
ok: print "ok\n"
    set P11, P10[1]
    print P11
    print "\n"
    end
CODE
PerlArray 2
ok
10
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "freeze/thaw a Sub");
    find_global P1, "_foo"
    freeze S0, P1

    thaw P0, S0
    typeof S10, P0
    print S10
    print "\n"
    invokecc
    print "back\n"
    end
.pcc_sub _foo:
    print "in sub _foo\n"
    invoke P1
CODE
Sub
in sub _foo
back
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "freeze/thaw a FixedPMCArray");
    new P0, .FixedPMCArray
    set P0, 3
    new P1, .PerlInt
    set P1, 666
    set P0[0], P1
    new P2, .PerlInt
    set P2, 777
    set P0[1], P2
    new P1, .PerlInt
    set P1, 666
    set P0[2], P1
    freeze S0, P0

    thaw P10, S0
    typeof S10, P10	# type
    print S10
    print " "
    set I11, P10	# elements
    print I11
    print "\n"
    set P12, P10[0]
    print P12
    print "\n"
    set P13, P10[1]
    print P13
    print "\n"
    set P14, P10[2]
    print P14
    print "\n"
    ne_addr P12, P14, ok
    print "not "
ok: print "ok diff\n"
    end
CODE
FixedPMCArray 3
666
777
666
ok diff
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "freeze/thaw a FixedPMCArray");
    new P0, .FixedPMCArray
    set P0, 3
    new P1, .PerlInt
    set P1, 666
    set P0[0], P1
    new P2, .PerlInt
    set P2, 777
    set P0[1], P2
    set P0[2], P1
    freeze S0, P0

    thaw P10, S0
    typeof S10, P10	# type
    print S10
    print " "
    set I11, P10	# elements
    print I11
    print "\n"
    set P12, P10[0]
    print P12
    print "\n"
    set P13, P10[1]
    print P13
    print "\n"
    set P14, P10[2]
    print P14
    print "\n"
    eq_addr P12, P14, ok
    print "not "
ok: print "ok same\n"
    end
CODE
FixedPMCArray 3
666
777
666
ok same
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "freeze class");
    newclass P10, "Foo"
    classname S10, P10
    print S10
    print "\n"
    freeze S11, P10
    print "ok 1\n"
    open P3, "temp.fpmc", ">"
    print P3, S11
    close P3
    print "ok 2\n"
    end
CODE
Foo
ok 1
ok 2
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "thaw class into new interpreter");
    set S3, "temp.fpmc"
    .include "stat.pasm"
    stat I0, S3, .STAT_FILESIZE
    gt I0, 1, ok1
    print "stat failed\n"
    exit 1
ok1:
    open P3, S3, "<"
    read S3, P3, I0
    close P3
    print "ok 1\n"
    thaw P4, S3
    print "ok 2\n"
    classname S10, P4
    print S10
    print "\n"
    end
CODE
ok 1
ok 2
Foo
OUTPUT

SKIP: {
	skip("todo class exists", 1);
output_is(<<'CODE', <<'OUTPUT', "freeze/thaw simple class");
    newclass P10, "Foo"
    classname S10, P10
    print S10
    print "\n"
    freeze S11, P10

    thaw P4, S11
    print "ok\n"
    classname S12, P4
    print S12
    print "\n"
    end
CODE
Foo
ok
Foo
OUTPUT
}

output_is(<<'CODE', <<'OUTPUT', "freeze class w attr");
    newclass P10, "Foo"
    addattribute P10, ".aa"
    classname S10, P10
    print S10
    print "\n"
    freeze S11, P10
    print "ok 1\n"
    open P3, "temp.fpmc", ">"
    print P3, S11
    close P3
    print "ok 2\n"
    end
CODE
Foo
ok 1
ok 2
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "thaw class  w attr into new interpreter");
    set S3, "temp.fpmc"
    .include "stat.pasm"
    stat I0, S3, .STAT_FILESIZE
    gt I0, 1, ok1
    print "stat failed\n"
    exit 1
ok1:
    open P3, S3, "<"
    read S3, P3, I0
    close P3
    # print S3
    # print "\n"
    print "ok 1\n"
    thaw P4, S3
    print "ok 2\n"
    classname S10, P4
    print S10
    print "\n"

    find_type I4, S10
    new P5, I4
    print "ok 3\n"
    classoffset I5, P5, S10
    new P6, .PerlString
    set P6, "ok 5\n"
    setattribute P5, I5, P6
    print "ok 4\n"
    getattribute P7, P5, I5
    print P7
    end
CODE
ok 1
ok 2
Foo
ok 3
ok 4
ok 5
OUTPUT

SKIP: {
	skip("todo class exists", 1);
output_is(<<'CODE', <<'OUTPUT', "thaw class  w attrr");
    newclass P10, "Foo"
    addattribute P10, ".aa"
    addattribute P10, ".bb"
    classname S10, P10
    print S10
    print "\n"
    freeze S3, P10
    open P3, "temp.fpmc", ">"
    print P3, S3
    close P3

    # print S3
    # print "\n"
    print "ok 1\n"
    thaw P4, S3
    print "ok 2\n"
    classname S10, P4
    print S10
    print "\n"

    find_type I4, S10
    new P5, I4
    print "ok 3\n"
    classoffset I5, P5, S10
    new P6, .PerlString
    set P6, "ok 5\n"
    setattribute P5, "Foo\0.aa", P6
    new P6, .PerlString
    set P6, "ok 6\n"
    setattribute P5, "Foo\0.bb", P6
    print "ok 4\n"
    getattribute P7, P5, I5
    print P7
    inc I5
    getattribute P7, P5, I5
    print P7
    end


CODE
Foo
ok 1
ok 2
Foo
ok 3
ok 4
ok 5
ok 6
OUTPUT
}
