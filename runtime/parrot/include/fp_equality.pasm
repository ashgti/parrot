# Copyright (C) 2004-2009, Parrot Foundation.
# $Id$

=head1 NAME

fp_equality.pasm - floating point equivalency macros

=head1 DESCRIPTION

This file provides PIR macros to determine if a pair of floating point numbers are equivalent.
The same macros are also provided for PASM.

=cut

.macro fp_eq ( J, K, L )
    set $N10, .J
    set $N11, .K
    sub $N12, $N11, $N10
    abs $N12, $N12
    gt  $N12, 0.000001, .$FPEQNOK

    branch  .L
.label $FPEQNOK:
.endm

.macro fp_eq_ok (  J, K, L )
    macro_local num $J, $K, $L
    macro_local int $i
    set $J, .J
    set $K, .K
    sub $K, $K, $J
    abs $K, $K

    set $I, 0
    gt  $K, 0.000001, .$FPEQNOK
    set $I, 1
.label $FPEQNOK:
    ok( i, .$L )
.endm

.macro fp_ne ( J, K, L )
    set $N10, .J
    set $N11, .K
    sub $N12, $N11, $N10
    abs $N12, $N12
    lt  $N12, 0.000001, .$FPNENOK

    branch  .L
.label $FPNENOK:
.endm

.macro fp_eq_pasm ( J, K, L )
    set N10, .J
    set N11, .K
    sub N12, N11, N10
    abs N12, N12
    gt  N12, 0.000001, .$FPEQNOK

    branch  .L
.label $FPEQNOK:
.endm


.macro fp_ne_pasm ( J, K, L )
    set N10, .J
    set N11, .K
    sub N12, N11, N10
    abs N12, N12
    lt  N12, 0.000001, .$FPNENOK

    branch  .L
.label $FPNENOK:
.endm

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
