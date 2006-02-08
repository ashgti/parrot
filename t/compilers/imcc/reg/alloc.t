#!perl -w
# Copyright: 2005 The Perl Foundation.  All Rights Reserved.
# $Id$

use strict;
use Parrot::Test tests => 3;

pir_output_is(<<'CODE', <<'OUT', "alligator");
# if the side-effect of set_addr/continuation isn't
# detected this program prints "Hi\nalligator\n"

.sub main :main
    $P0 = new .String
    $P0 = "Hi\n"
    $I0 = 2
lab:
    print $P0
    dec $I0
    unless $I0 goto ex
    new $P1, .Continuation
    set_addr $P1, lab
    $P2 = find_name "alligator"
    set_args "(0)", $P1
    invokecc $P2
ex:
.end
.sub alligator
    get_params "(0)", $P0
    invokecc $P0
.end
CODE
Hi
Hi
OUT

pir_output_is(<<'CODE', <<'OUT', "alligator 2 - r9629");
.sub xyz
    .local pmc args
    args = new .ResizablePMCArray
    push args, "abc"
    push args, "def"
    push args, "POPME"

    $S0 = args[-1]
    if $S0 != "POPME" goto start
    $P0 = pop args
  start:
    $I1 = elements args
    $I0 = 0
  loop:
    if $I0 >= $I1 goto end
    $S0 = args[$I0]
    print $S0
    print "\n"
    inc $I0
    goto loop
  end:
.end
CODE
abc
def
OUT

pir_2_pasm_is(<<'CODE', <<'OUT', ":unique_reg");
.sub main
    .param int i :unique_reg
    .local int j :unique_reg
    .local int k :unique_reg
    i = 5
    j = 2
    k = j * 2
.end
CODE
# IMCC does produce b0rken PASM files
# see http://guest@rt.perl.org/rt3/Ticket/Display.html?id=32392
main:
        get_params
        set I0, 5
        set I1, 2
        mul I2, I1, 2
        set_returns
        returncc
OUT
