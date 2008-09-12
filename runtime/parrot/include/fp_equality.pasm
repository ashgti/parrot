# Copyright (C) 2004-2007, The Perl Foundation.
# $Id$

=head1 NAME

fp_equality.pasm - floating point equivalency macros

=head1 DESCRIPTION

This file provides two macros to determine if a pair of PIR nums are equivalent.

=cut

.macro fp_eq (  J, K, L )
    set N10, .J
    set N11, .K
    sub N12, N11, N10
    abs N12, N12
    gt  N12, 0.000001, .$FPEQNOK

    branch  .L
.label $FPEQNOK:
.endm


.macro fp_ne(   J,K,L)
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
