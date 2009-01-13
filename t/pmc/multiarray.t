#! perl
# Copyright (C) 2001-2005, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 5;

=head1 NAME

t/pmc/multiarray.t - MultiArray PMC

=head1 SYNOPSIS

    % prove t/pmc/multiarray.t

=head1 DESCRIPTION

Tests the C<MultiArray> PMC. Checks element access in 2 and 3 dimensions,
and cloning.

=cut

pasm_output_is( <<'CODE', <<'OUTPUT', "multiarray creation" );
    new P1, ['ResizablePMCArray']
    # intial size
    set P1[0], 0
    set P1[1], 50
    # dimensions 10x5 2D array
    # XXX can this be done w/o push/append a key?
    new P2, ['Key']
    set P2, 10
    new P3, ['Key']
    set P3, 5
    push P2, P3
    set P1[2], 1
    set P1[3], P2
    new P0, ['MultiArray'], P1
    print "ok\n"
    end
CODE
ok
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "multiarray access 2d" );
    new P1, ['ResizablePMCArray']
    # intial size
    set P1[0], 0
    set P1[1], 100
    # dimensions
    new P2, ['Key']
    set P2, 10
    new P3, ['Key']
    set P3, 5
    push P2, P3
    set P1[2], 1
    set P1[3], P2
    new P0, ['MultiArray'], P1
    # array size, multiarray doesnt autogrow
    set P0, 200

    set I10, 10
    set I11, 20
    set I0, 0
lp2:
    set I1, 0
lp1:
    mul I2, I0, I1
    set P0[I0;I1], I2
    inc I1
    lt I1, I11, lp1
    inc I0
    lt I0, I10, lp2
    print "ok 1\n"

    # read values
    set I0, 0
lp4:
    set I1, 0
lp3:
    mul I2, I0, I1
    set I3, P0[I0;I1]
    ne I2, I3, nok1
    inc I1
    lt I1, I11, lp3
    inc I0
    lt I0, I10, lp4
    print "ok 2\n"
    end
nok1:
    print "nok1: got "
    print I3
    print " wanted "
    print I2
    print "\n"
    end
CODE
ok 1
ok 2
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "multiarray access 3d" );
    new P1, ['ResizablePMCArray']
    # initial size
    set P1[0], 0
    set P1[1], 200
    # dimensions 10x5x4
    new P2, ['Key']
    set P2, 10
    new P3, ['Key']
    set P3, 5
    push P2, P3
    new P3, ['Key']
    set P3, 4
    push P2, P3
    set P1[2], 1
    set P1[3], P2
    new P0, ['MultiArray'], P1
    set P0, 200

    set I10, 10
    set I11, 5
    set I12, 4
    set I0, 0
lp0:
    set I1, 0
lp1:
    set I2, 0
lp2:
    mul I9, I0, I1
    mul I9, I9, I2
    set P0[I0;I1;I2], I9
    inc I2
    lt I2, I12, lp2
    inc I1
    lt I1, I11, lp1
    inc I0
    lt I0, I10, lp0
    print "ok 1\n"

    # read values

    set I0, 0
lp10:
    set I1, 0
lp11:
    set I2, 0
lp12:
    mul I9, I0, I1
    mul I9, I9, I2
    set I8, P0[I0;I1;I2]
    ne I8, I9, nok1
    inc I2
    lt I2, I12, lp12
    inc I1
    lt I1, I11, lp11
    inc I0
    lt I0, I10, lp10
    print "ok 2\n"

    # clone then read values
    clone P10, P0

    set I0, 0
lp20:
    set I1, 0
lp21:
    set I2, 0
lp22:
    mul I9, I0, I1
    mul I9, I9, I2
    set I8, P10[I0;I1;I2]
    ne I8, I9, nok2
    inc I2
    lt I2, I12, lp22
    inc I1
    lt I1, I11, lp21
    inc I0
    lt I0, I10, lp20
    print "ok 3\n"
    end
nok2:
    print "cloned read failed "
nok1:
    print "got "
    print I8
    print " wanted "
    print I9
    print "\n"
    end
CODE
ok 1
ok 2
ok 3
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', "check whether interface is done" );

.sub _main
    .local pmc pmc1
    pmc1 = new ['MultiArray']
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

pir_output_is( << 'CODE', << 'OUTPUT', 'Verify Basic Iterator support' );
  .sub test :main
     $P1 = new ['ResizablePMCArray']

     $P1[0] = 0
     $P1[1] = 4

     $P2 = new ['Key']
     $P2 = 2
     $P3 = new ['Key']
     $P3 = 2
     push $P2, $P3

     $P1[2] = 1
     $P1[3] = $P2

     $P0 = new ['MultiArray'], $P1

     $P0[0;0] = 1
     $P0[1;0] = 2
     $P0[0;1] = 3
     $P0[1;1] = 4

     $P9 = new ['Iterator'], $P0
loop:
    unless $P9 goto end_loop
    $P10 = shift $P9
    say $P10
    goto loop
end_loop:
  .end
CODE
1
2
3
4
OUTPUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
