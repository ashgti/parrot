#!perl
# Copyright (C) 2005-2007, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Parrot::Test tests => 6;

# these tests are run with -O2 by TestCompiler and show
# generated PASM code for various optimizations at level 2

##############################
pir_2_pasm_is( <<'CODE', <<'OUT', "used once lhs" );
.sub _main
    $I1 = 1
    $I2 = 2
    print $I2
    end
.end
CODE
# IMCC does produce b0rken PASM files
# see http://guest@rt.perl.org/rt3/Ticket/Display.html?id=32392
_main:
    print 2
    end
OUT

##############################
pir_2_pasm_is( <<'CODE', <<'OUT', "constant propogation and resulting dead code" );
.sub _main
       set $I0, 5
loop:
       set $I1, 2
       add $I0, $I1
       lt $I0, 20, loop
       print $I0
       end
.end
CODE
# IMCC does produce b0rken PASM files
# see http://guest@rt.perl.org/rt3/Ticket/Display.html?id=32392
_main:
  set I0, 5
loop:
  add I0, 2
  lt I0, 20, loop
  print I0
  end
OUT

##############################
pir_2_pasm_is( <<'CODE', <<'OUT', "don't move constant past a label" );
.sub _main
  set $I1, 10
  set $I0, 5
  lt $I1, 20, nxt
add:
  add $I0, $I1, $I1
  print $I0
nxt:
  set $I1, 20
  branch add
.end
CODE
# IMCC does produce b0rken PASM files
# see http://guest@rt.perl.org/rt3/Ticket/Display.html?id=32392
_main:
  set I1, 10
  set I0, 5
  branch nxt
add:
  add I0, I1, I1
  print I0
nxt:
  set I1, 20
  branch add
OUT

##############################
pir_2_pasm_is( <<'CODE', <<'OUT', "constant prop and null_i" );
.sub _main
  null $I0
  add $I1, $I0, 5
  print $I1
.end
CODE
# IMCC does produce b0rken PASM files
# see http://guest@rt.perl.org/rt3/Ticket/Display.html?id=32392
_main:
  print 5
  set_returns
  returncc
OUT

##############################
SKIP: {
    skip( "loop opt disabled for now", 1 );

    pir_2_pasm_is( <<'CODE', <<'OUT', "remove invariant from loop" );
.sub _main
       set $I0, 5
loop:
       set $I1, 2
       add $I0, $I1
       lt $I0, 20, loop
next:
       print $I0
       add $I0, $I1
       print $I0
       lt $I1, 4, next
       end
.end
CODE
# IMCC does produce b0rken PASM files
# see http://guest@rt.perl.org/rt3/Ticket/Display.html?id=32392
_main:
    set I0, 5
    set I1, 2
loop:
    add I0, 2
    lt I0, 20, loop
next:
    print I0
    add I0, I1
    print I0
    lt I1, 4, next
    end
OUT
}

##############################
pir_2_pasm_is( <<'CODE', <<'OUT', "constant prop repeated" );
.sub _main
  .local int a
  .local int b
  .local int sum
  a = 10
  b = 5
  sum = a + b
  print sum
  end
.end
CODE
# IMCC does produce b0rken PASM files
# see http://guest@rt.perl.org/rt3/Ticket/Display.html?id=32392
_main:
  print 15
  end
OUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
