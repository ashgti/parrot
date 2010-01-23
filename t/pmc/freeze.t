#! perl
# Copyright (C) 2001-2009, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );

use Test::More;
use Parrot::Test tests => 25;

=head1 NAME

t/pmc/freeze.t - Archiving

=head1 SYNOPSIS

    % prove t/pmc/freeze.t

=head1 DESCRIPTION

Tests the freeze/thaw archiving subsystem.

=cut

END { unlink "temp.fpmc"; }

pasm_output_is( <<'CODE', <<'OUTPUT', "freeze/thaw a Integer" );
    new P1, ['Integer']
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
Integer 777
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "freeze/thaw a String" );
    new P1, ['String']
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
String foo
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "freeze/thaw a Float" );
    new P1, ['Float']
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
Float 3.14159
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "freeze/thaw a Hash" );
    new P1, ['Integer']
    set P1, 666
    new P0, ['Hash']
    set P0["k1"], P1
    new P1, ['Integer']
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
Hash 2
666
777
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "freeze/thaw a Hash" );
    new P1, ['Integer']
    set P1, 666
    new P0, ['Hash']
    set P0["k1"], P1
    new P1, ['Integer']
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
Hash 2
666
777
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "freeze/thaw a Integer with prop" );
    new P1, ['Integer']
    set P1, 666
    new P2, ['Integer']
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
Integer 666
42
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "freeze/thaw Array w Integer with prop" );
    new P0, ['ResizablePMCArray']
    new P1, ['Integer']
    set P1, 666
    push P0, P1
    new P2, ['Integer']
    set P2, 777
    push P0, P2
    new P3, ['Integer']
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
ResizablePMCArray 2
666
777
42
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "freeze/thaw a NULL pmc" );
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

pasm_output_is( <<'CODE', <<'OUTPUT', "freeze/thaw array w NULL pmc" );
    new P0, ['ResizablePMCArray']
    null P1
    push P0, P1
    new P1, ['Integer']
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
ResizablePMCArray 2
ok
10
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "freeze/thaw a Sub" );
    get_global P1, "_foo"
    freeze S0, P1

    thaw P0, S0
    typeof S10, P0
    print S10
    print "\n"
    invokecc P0
    print "back\n"
    end
.pcc_sub _foo:
    print "in sub _foo\n"
    returncc
CODE
Sub
in sub _foo
back
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "freeze/thaw a FixedPMCArray" );
    new P0, ['FixedPMCArray']
    set P0, 3
    new P1, ['Integer']
    set P1, 666
    set P0[0], P1
    new P2, ['Integer']
    set P2, 777
    set P0[1], P2
    new P1, ['Integer']
    set P1, 666
    set P0[2], P1
    freeze S0, P0

    thaw P10, S0
    typeof S10, P10 # type
    print S10
    print " "
    set I11, P10    # elements
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

pasm_output_is( <<'CODE', <<'OUTPUT', "freeze/thaw a FixedPMCArray" );
    new P0, ['FixedPMCArray']
    set P0, 3
    new P1, ['Integer']
    set P1, 666
    set P0[0], P1
    new P2, ['Integer']
    set P2, 777
    set P0[1], P2
    set P0[2], P1
    freeze S0, P0

    thaw P10, S0
    typeof S10, P10 # type
    print S10
    print " "
    set I11, P10    # elements
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

pasm_output_is( <<'CODE', <<'OUTPUT', "freeze class" );
    newclass P10, "Foo"
    set S10, P10
    print S10
    print "\n"
    freeze S11, P10
    print "ok 1\n"
    open P3, "temp.fpmc", 'w'
    print P3, S11
    close P3
    print "ok 2\n"
    end
CODE
Foo
ok 1
ok 2
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "thaw class into new interpreter" );
    set S3, "temp.fpmc"
    .include "stat.pasm"
    stat I0, S3, .STAT_FILESIZE
    gt I0, 1, ok1
    print "stat failed\n"
    exit 1
ok1:
    open P3, S3, 'r'
    read S3, P3, I0
    close P3
    print "ok 1\n"
    thaw P4, S3
    print "ok 2\n"
    set S10, P4
    print S10
    print "\n"
    end
CODE
ok 1
ok 2
Foo
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "freeze/thaw simple class" );
    newclass P10, "Foo"
    set S10, P10
    print S10
    print "\n"
    freeze S11, P10

    thaw P4, S11
    print "ok\n"
    set S12, P4
    print S12
    print "\n"
    end
CODE
Foo
ok
Foo
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "freeze class w attr" );
    newclass P10, "Foo"
    addattribute P10, ".aa"
    set S10, P10
    print S10
    print "\n"
    freeze S11, P10
    print "ok 1\n"
    open P3, "temp.fpmc", 'w'
    print P3, S11
    close P3
    print "ok 2\n"
    end
CODE
Foo
ok 1
ok 2
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "thaw class w attr into new interpreter" );
    set S3, "temp.fpmc"
    .include "stat.pasm"
    stat I0, S3, .STAT_FILESIZE
    gt I0, 1, ok1
    print "stat failed\n"
    exit 1
ok1:
    open P3, S3, 'r'
    read S3, P3, I0
    close P3
    # print S3
    # print "\n"
    print "ok 1\n"
    thaw P4, S3
    print "ok 2\n"
    set S10, P4
    print S10
    print "\n"

    new P5, S10
    print "ok 3\n"
    new P6, ['String']
    set P6, "ok 5\n"
    setattribute P5, '.aa', P6
    print "ok 4\n"
    getattribute P7, P5, '.aa'
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

pasm_output_is( <<'CODE', <<'OUTPUT', "thaw class w attr same interp" );
    newclass P10, "Foo"
    addattribute P10, ".aa"
    addattribute P10, ".bb"
    set S10, P10
    print S10
    print "\n"
    freeze S3, P10
    open P3, "temp.fpmc", 'w'
    print P3, S3
    close P3

    # print S3
    # print "\n"
    print "ok 1\n"
    thaw P4, S3
    print "ok 2\n"
    set S10, P4
    print S10
    print "\n"

    new P5, S10
    print "ok 3\n"
    new P6, ['String']
    set P6, "ok 5\n"
    setattribute P5, ["Foo"], ".aa", P6
    new P6, ['String']
    set P6, "ok 6\n"
    setattribute P5, ["Foo"], ".bb", P6
    print "ok 4\n"
    getattribute P7, P5, ".aa"
    print P7
    getattribute P7, P5, ".bb"
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

pasm_output_is( <<'CODE', <<'OUTPUT', "thaw object w attr into same interpreter" );
    newclass P10, "Foo"
    addattribute P10, ".aa"
    addattribute P10, ".bb"
    new P10, ['Foo']
    print S10
    freeze S3, P10
    open P3, "temp.fpmc", 'w'
    print P3, S3
    close P3
    print "ok 1\n"

    thaw P5, S3
    print "ok 2\n"
    typeof S10, P5
    print S10
    print "\n"

    print "ok 3\n"
    new P6, ['String']
    set P6, "ok 5\n"
    setattribute P5, ["Foo"], ".aa", P6
    new P6, ['String']
    set P6, "ok 6\n"
    setattribute P5, ["Foo"], ".bb", P6
    print "ok 4\n"
    getattribute P7, P5, ".aa"
    print P7
    getattribute P7, P5, ".bb"
    print P7
    end
CODE
ok 1
ok 2
Foo
ok 3
ok 4
ok 5
ok 6
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "thaw object w attr into new interpreter" );
    set S3, "temp.fpmc"
    .include "stat.pasm"
    stat I0, S3, .STAT_FILESIZE
    gt I0, 1, ok1
    print "stat failed\n"
    exit 1
ok1:
    open P3, S3, 'r'
    read S3, P3, I0
    close P3

    thaw P5, S3
    print "ok 2\n"
    typeof S10, P5
    print S10
    print "\n"

    print "ok 3\n"
    new P6, ['String']
    set P6, "ok 5\n"
    setattribute P5, ["Foo"], ".aa", P6
    new P6, ['String']
    set P6, "ok 6\n"
    setattribute P5, ["Foo"], ".bb", P6
    print "ok 4\n"
    getattribute P7, P5, ".aa"
    print P7
    getattribute P7, P5, ".bb"
    print P7
    end
CODE
ok 2
Foo
ok 3
ok 4
ok 5
ok 6
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "freeze Key" );
    new P0, ['Hash']
    new P1, ['FixedPMCArray']
    set P1, 2
    set P1[1], P0
    set P0["foo"], "ok\n"
    set S0, P1[1; "foo"]
    print S0

    new P3, ['Key']
    set P3, 1
    new P4, ['Key']
    set P4, "foo"
    push P3, P4

    set S0, P1[P3]
    print S0

    freeze S0, P3
    print "ok 1\n"
    thaw P5, S0
    print "ok 2\n"

    set S0, P1[P5]
    print S0
    end
CODE
ok
ok
ok 1
ok 2
ok
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "freeze/thaw a ResizableBooleanArray" );
.sub test :main
    .local pmc original_arr, thawed_arr
    .local string frozen_arr
    original_arr = new ['ResizableBooleanArray']
    set original_arr, 666
    original_arr[555] = 777

    # Dump some data before freezing
    print "Before freezing:\n"
    typeof $S10, original_arr   # type
    print $S10
    print "\n"
    set $I12, original_arr  # elements
    print $I12
    print "\n"
    $I12 = original_arr[554]
    print $I12
    print "\n"
    $I12 = original_arr[555]
    print $I12
    print "\n"
    $I12 = original_arr[556]
    print $I12
    print "\n"

    frozen_arr = freeze original_arr
    thawed_arr = thaw frozen_arr

    # Dump the same data after freeze/thaw
    print "\nAfter freeze/thaw:\n"
    typeof $S10, thawed_arr # type
    print $S10
    print "\n"
    set $I12, thawed_arr    # elements
    print $I12
    print "\n"
    $I12 = thawed_arr[554]
    print $I12
    print "\n"
    $I12 = thawed_arr[555]
    print $I12
    print "\n"
    $I12 = thawed_arr[556]
    print $I12
    print "\n"

.end
CODE
Before freezing:
ResizableBooleanArray
666
0
1
0

After freeze/thaw:
ResizableBooleanArray
666
0
1
0
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "freeze/thaw a ResizablePMCArray" );
.sub test :main
    .local pmc original_arr, thawed_arr
    .local string frozen_arr
    original_arr = new ['ResizablePMCArray']
    original_arr[0] = 1
    original_arr[1] = 2.72
    original_arr[2] = "three.14"

    # Dump some data before freezing
    print "Before freezing:\n"
    typeof $S10, original_arr    # type
    print $S10
    print "\n"
    set $I12, original_arr   # elements
    print $I12
    print "\n"
    $I12 = original_arr[0]
    print $I12
    print "\n"
    $N12 = original_arr[1]
    print $N12
    print "\n"
    $S12 = original_arr[2]
    print $S12
    print "\n"

    frozen_arr = freeze original_arr
    thawed_arr = thaw frozen_arr

    # Dump the same data after freeze/thaw
    print "\nAfter freeze/thaw:\n"
    typeof $S10, thawed_arr  # type
    print $S10
    print "\n"
    set $I12, thawed_arr # elements
    print $I12
    print "\n"
    $I12 = thawed_arr[0]
    print $I12
    print "\n"
    $N12 = thawed_arr[1]
    print $N12
    print "\n"
    $S12 = thawed_arr[2]
    print $S12
    print "\n"

.end
CODE
Before freezing:
ResizablePMCArray
3
1
2.72
three.14

After freeze/thaw:
ResizablePMCArray
3
1
2.72
three.14
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "freeze/thaw a Conure" );
.sub main :main
    .local pmc cl, o
    cl = newclass 'Conure'
    addattribute cl, 'temperature'
    o = new ['Conure']
    $S0 = freeze o
    $P1 = thaw $S0
    $P2 = getattribute $P1, 'temperature'
    say $P2
.end

.namespace ['Conure']
.sub init :method :vtable
    $P0 = new ['Integer']
    $P0 = 37
    setattribute self, 'temperature', $P0
.end
CODE
37
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "freeze/thaw obj of class w Hash attrs" );
.sub main :main
    .local pmc cl, o
    #cl = subclass 'Hash', 'OPTable'
    cl = newclass 'OPTable'
    addattribute cl, '%!key'
    addattribute cl, '%!klen'
    addattribute cl, '&!ws'
    o = new ['OPTable']
    o."init"()
    o."test"()
    $S0 = freeze o
    $P1 = thaw $S0
    $P1."test"()
.end

.namespace [ "OPTable" ]

.sub "init" :method :vtable
    .local pmc keytable, klentable
    keytable = new ['Hash']
    klentable = new ['Hash']
    setattribute self, '%!key', keytable
    setattribute self, '%!klen', klentable
.end

.sub "init" :method
    .local pmc keytable, klentable
    keytable = getattribute self, '%!key'
    keytable['bar'] = 1
    keytable['foobar'] = 2
    klentable = getattribute self, '%!klen'
    klentable['bar'] = 3
    klentable['foobar'] = 6
.end

.sub "test" :method
    .local pmc keytable, klentable
    keytable = getattribute self, "%!key"
    $I0 = keytable['bar']
    print $I0
    print ' '
    $I0 = keytable['foobar']
    print $I0
    print ' '
    klentable = getattribute self, "%!klen"
    $I0 = klentable['bar']
    print $I0
    print ' '
    $I0 = klentable['foobar']
    say $I0
.end
CODE
1 2 3 6
1 2 3 6
OUTPUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
