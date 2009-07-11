#! perl
# Copyright (C) 2001-2007, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 18;

=head1 NAME

t/pmc/resizableintegerarray.t - ResizableIntegerArray PMC

=head1 SYNOPSIS

    % prove t/pmc/ResizableIntegerArray.t

=head1 DESCRIPTION

Tests C<ResizableIntegerArray> PMC. Checks size, sets various elements, including
out-of-bounds test. Checks INT and PMC keys.

=cut

pasm_output_is( <<'CODE', <<'OUTPUT', "Setting array size" );
    new P0, ['ResizableIntegerArray']

    set I0,P0
    eq I0,0,OK_1
    print "not "
OK_1:    print "ok 1\n"

    set P0,1
    set I0,P0
    eq I0,1,OK_2
    print "not "
OK_2:    print "ok 2\n"

    set P0,5
    set I0,P0
    eq I0,5,OK_3
    print "not "
OK_3:    print "ok 3\n"

    set P0,9
    set I0,P0
    eq I0,9,OK_4
    print "not "
OK_4:    print "ok 4\n"

    set P0,7
    set I0,P0
    eq I0,7,OK_5
    print "not "
OK_5:    print "ok 5\n"

        end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "Setting first element" );
    new P0, ['ResizableIntegerArray']
    set P0, 1

    set P0[0],-7
    set I0,P0[0]
    eq I0,-7,OK_1
    print "not "
OK_1:    print "ok 1\n"

    set P0[0],3.7
    set N0,P0[0]
    eq N0,3.0,OK_2
    print "not "
OK_2:    print "ok 2\n"

    set P0[0],"17"
    set S0,P0[0]
    eq S0,"17",OK_3
    print "not "
OK_3:    print "ok 3\n"

    end
CODE
ok 1
ok 2
ok 3
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "Setting second element" );
    new P0, ['ResizableIntegerArray']

    set P0[1], -7
    set I0, P0[1]
    eq I0,-7,OK_1
    print "not "
OK_1:    print "ok 1\n"

    set P0[1], 3.7
    set N0, P0[1]
    eq N0,3.0,OK_2
    print "not "
OK_2:    print "ok 2\n"

    set P0[1],"17"
    set S0, P0[1]
    eq S0,"17",OK_3
    print "not "
OK_3:    print "ok 3\n"

    end
CODE
ok 1
ok 2
ok 3
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "Setting negatively indexed elements" );
    new P0, ['ResizableIntegerArray']
    set P0, 1

    push_eh eh
    set P0[-1], -7
    pop_eh
    print "no ex\n"
    end
eh:
    say "got an ex"
    end
CODE
got an ex
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "Getting negatively indexed elements" );
    new P0, ['ResizableIntegerArray']
    set P0, 1

    push_eh eh
    set I0, P0[-1]
    pop_eh
    print "no ex\n"
    end
eh:
    say "got an ex"
    end
CODE
got an ex
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "Setting out-of-bounds elements" );
    new P0, ['ResizableIntegerArray']
    set P0, 1

    set P0[1], -7
    print "ok 1\n"

    end
CODE
ok 1
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "Getting out-of-bounds elements" );
    new P0, ['ResizableIntegerArray']
    set P0, 1

    set I0, P0[1]
    print "ok 1\n"
    end
CODE
ok 1
OUTPUT

pasm_output_is( <<"CODE", <<'OUTPUT', "Set via PMC keys, access via INTs" );
     .include 'fp_equality.pasm'
     new P0, ['ResizableIntegerArray']
     new P1, ['Key']

     set P1, 0
     set P0[P1], 25

     set P1, 1
     set P0[P1], 2.5

     set P1, 2
     set P0[P1], "17"

     set I0, P0[0]
     eq I0, 25, OK1
     print "not "
OK1: print "ok 1\\n"

     set N0, P0[1]
     .fp_eq_pasm(N0, 2.0, OK2)
     print "not "
OK2: print "ok 2\\n"

     set S0, P0[2]
     eq S0, "17", OK3
     print "not "
OK3: print "ok 3\\n"

     end
CODE
ok 1
ok 2
ok 3
OUTPUT

pasm_output_is( <<"CODE", <<'OUTPUT', "Set via INTs, access via PMC Keys" );
     .include 'fp_equality.pasm'
     new P0, ['ResizableIntegerArray']
     set P0, 1

     set P0[25], 125
     set P0[128], 10.2
     set P0[513], "17"
     new P1, ['Integer']
     set P1, 123456
     set P0[1023], P1

     new P2, ['Key']
     set P2, 25
     set I0, P0[P2]
     eq I0, 125, OK1
     print "not "
OK1: print "ok 1\\n"

     set P2, 128
     set N0, P0[P2]
     .fp_eq_pasm(N0, 10.0, OK2)
     print "not "
OK2: print "ok 2\\n"

     set P2, 513
     set S0, P0[P2]
     eq S0, "17", OK3
     print "not "
OK3: print "ok 3\\n"

     set P2, 1023
     set P3, P0[P2]
     set I1, P3
     eq I1, 123456, OK4
     print "not "
OK4: print "ok 4\\n"

     end
CODE
ok 1
ok 2
ok 3
ok 4
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', "check whether interface is done" );

.sub test :main
    .local pmc pmc1
    pmc1 = new ['ResizableIntegerArray']
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

pir_output_is( << 'CODE', << 'OUTPUT', "push integer" );

.sub test :main
    .local pmc pmc1
    pmc1 = new ['ResizableIntegerArray']
    pmc1[9999] = 0
    push pmc1, 10001
    .local int elements
    elements = pmc1
    print elements
    print "\n"
    .local string last
    last = pmc1[10000]
    print last
    print "\n"
    end
.end
CODE
10001
10001
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', 'basic pop' );
     new P0, ['ResizableIntegerArray']
     set P0[0], 4
     set P0[1], 8
     set P0[2], 16
     pop I0, P0
     eq I0, 16, OK1
     print "not "
     print I0
OK1: print "ok 1\n"

     pop I0, P0
     eq I0, 8, OK2
     print "not "
     print I0
OK2: print "ok 2\n"

     pop I0, P0
     eq I0, 4, OK3
     print "not "
     print I0
OK3: print "ok 3\n"
     end
CODE
ok 1
ok 2
ok 3
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', 'pop many values' );
     new P0, ['ResizableIntegerArray']
     set I0, 0
L1:  set P0[I0], I0
     inc I0
     lt I0, 100000, L1

L2:  dec I0
     pop I1, P0
     eq I0, I1, OK
     branch NOT_OK
OK:  gt I0, 0, L2
     print "ok\n"
     end

NOT_OK:
     print I0
     print "\n"
     print I1
     print "\n"
     end
CODE
ok
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', 'push/pop' );
     new P0, ['ResizableIntegerArray']
     push P0, 2
     push P0, 4
     push P0, 6
     pop I0, P0
     eq I0, 6, OK1
     print "not "
OK1: print "ok 1\n"
     end
CODE
ok 1
OUTPUT

pasm_error_output_like( <<'CODE', <<'OUTPUT', 'pop from empty array' );
     new P0, ['ResizableIntegerArray']
     pop I0, P0
     end
CODE
/ResizableIntegerArray: Can't pop from an empty array!/
OUTPUT

#'
pir_output_is( << 'CODE', << 'OUTPUT', "shift integer" );
.sub test :main
    .local pmc ar
    ar = new ['ResizableIntegerArray']
    ar[0] = 10
    ar[1] = 20
    $I0 = elements ar
    print $I0
    print ' '
    $I0 = shift ar
    print $I0
    print ' '
    $I0 = elements ar
    print $I0
    print ' '
    $I0 = shift ar
    print $I0
    print ' '
    $I0 = elements ar
    say $I0
.end
CODE
2 10 1 20 0
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', "unshift integer" );
.sub test :main
    .local pmc ar
    ar = new ['ResizableIntegerArray']
    unshift ar, 10
    unshift ar, 20
    $I0 = elements ar
    print $I0
    print ' '
    $I0 = ar[0]
    print $I0
    print ' '
    $I0 = ar[1]
    say $I0
.end
CODE
2 20 10
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "get_iter" );
.sub 'main' :main
    $P0 = new ['ResizableIntegerArray']
    $P0[0] = 42
    $P0[1] = 43
    $P0[2] = 44
    push $P0, 999
    $P1 = iter $P0
loop:
    unless $P1 goto loop_end
    $S2 = shift $P1
    say $S2
    goto loop
loop_end:
.end
CODE
42
43
44
999
OUTPUT


# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
