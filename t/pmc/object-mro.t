#! perl
# Copyright (C) 2001-2005, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 6;

=head1 NAME

t/pmc/object-mro.t - Object Methods Resolution Order

=head1 SYNOPSIS

    % prove t/pmc/object-mro.t

=head1 DESCRIPTION

These are tests for the C3 MRO order

=cut

pir_output_is( <<'CODE', <<'OUTPUT', "print mro diamond" );
#
# A   B A   E
#  \ /   \ /
#   C     D
#    \   /
#     \ /
#      F
.sub main :main
    .local pmc A, B, C, D, E, F, mro, p, it
    newclass A, "A"
    newclass B, "B"
    subclass C, A, "C"
    addparent C, B

    subclass D, A, "D"
    newclass E, "E"
    addparent D, E

    subclass F, C, "F"
    addparent F, D
    mro = F.'inspect'('all_parents')
    it = iter mro
    it = 0
loop:
    unless it goto ex
    p = shift it
    $S0 = p
    print $S0
    print ' '
    goto loop
ex:
    say 'G'
.end
CODE
F C D A B E G
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "print mro 1" );
#
# example take from: http://www.python.org/2.3/mro.html
#
# class O: pass
# class F(O): pass
# class E(O): pass
# class D(O): pass
# class C(D,F): pass
# class B(D,E): pass
# class A(B,C): pass
#
#                           6
#                          ---
# Level 3                 | O |                  (more general)
#                       /  ---  \
#                      /    |    \                      |
#                     /     |     \                     |
#                    /      |      \                    |
#                   ---    ---    ---                   |
# Level 2        3 | D | 4| E |  | F | 5                |
#                   ---    ---    ---                   |
#                    \  \ _ /       |                   |
#                     \    / \ _    |                   |
#                      \  /      \  |                   |
#                       ---      ---                    |
# Level 1            1 | B |    | C | 2                 |
#                       ---      ---                    |
#                         \      /                      |
#                          \    /                      \ /
#                            ---
# Level 0                 0 | A |                (more specialized)
#                            ---
#
.sub main :main
    .local pmc A, B, C, D, E, F, O
    newclass O, "O"
    subclass F, O, "F"
    subclass E, O, "E"
    subclass D, O, "D"

    subclass C, D, "C"
    addparent C, F

    subclass B, D, "B"
    addparent B, E

    subclass A, B, "A"
    addparent A, C

    .local pmc mro, it, p

    mro = A.'inspect'('all_parents')
    it = iter mro
    it = 0
loop:
    unless it goto ex
    p = shift it
    $S0 = p
    print $S0
    print ' '
    goto loop
ex:
    say 'G'
.end
CODE
A B C D E F O G
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "print mro 2" );
#
# example take from: http://www.python.org/2.3/mro.html
#
# class O: pass
# class F(O): pass
# class E(O): pass
# class D(O): pass
# class C(D,F): pass
# class B(E,D): pass
# class A(B,C): pass
#
#                            6
#                           ---
# Level 3                  | O |
#                        /  ---  \
#                       /    |    \
#                      /     |     \
#                     /      |      \
#                   ---     ---    ---
# Level 2        2 | E | 4 | D |  | F | 5
#                   ---     ---    ---
#                    \      / \     /
#                     \    /   \   /
#                      \  /     \ /
#                       ---     ---
# Level 1            1 | B |   | C | 3
#                       ---     ---
#                        \       /
#                         \     /
#                           ---
# Level 0                0 | A |
#                           ---
#

.sub main :main
    .local pmc A, B, C, D, E, F, O
    newclass O, "O"
    subclass F, O, "F"
    subclass E, O, "E"
    subclass D, O, "D"

    subclass C, D, "C"
    addparent C, F

    subclass B, E, "B"
    addparent B, D

    subclass A, B, "A"
    addparent A, C

    .local pmc mro, it, p

    mro = A.'inspect'('all_parents')
    it = iter mro
    it = 0
loop:
    unless it goto ex
    p = shift it
    $S0 = p
    print $S0
    print ' '
    goto loop
ex:
    say 'G'
.end
CODE
A B E C D F O G
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "print mro 3" );
#
#    C
#   / \
#  /   \
# A     B
#  \   /
#   \ /
#    D
#
.sub main :main
    .local pmc A, B, C, D
    newclass C, "C"
    subclass A, C, "A"
    subclass B, C, "B"
    subclass D, A, "D"
    addparent D, B

    .local pmc mro, it, p

    mro = D.'inspect'('all_parents')
    it = iter mro
    it = 0
loop:
    unless it goto ex
    p = shift it
    $S0 = p
    print $S0
    print ' '
    goto loop
ex:
    say 'G'
.end
CODE
D A B C G
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "print mro 4" );
#
#        TestObject
#            ^
#            |
#         LifeForm
#          ^    ^
#         /      \
#    Sentient    BiPedal
#       ^          ^
#       |          |
#  Intelligent  Humanoid
#        ^        ^
#         \      /
#          Vulcan
#
# example taken from: L<http://gauss.gwydiondylan.org/books/drm/drm_50.html>
#
#  define class <sentient> (<life-form>) end class;
#  define class <bipedal> (<life-form>) end class;
#  define class <intelligent> (<sentient>) end class;
#  define class <humanoid> (<bipedal>) end class;
#  define class <vulcan> (<intelligent>, <humanoid>) end class;
#
.sub main :main
    .local pmc TestObject, LifeForm, Sentient, BiPedal, Intelligent, Humanoid, Vulcan

    newclass TestObject, "TestObject"

    subclass LifeForm, TestObject, "LifeForm"

    subclass Sentient, LifeForm, "Sentient"
    subclass Intelligent, Sentient, "Intelligent"

    subclass BiPedal, LifeForm, "BiPedal"
    subclass Humanoid, BiPedal, "Humanoid"

    subclass Vulcan, Intelligent, "Vulcan"
    addparent Vulcan, Humanoid

    .local pmc mro, it, p

    mro = Vulcan.'inspect'('all_parents')
    it = iter mro
    it = 0
loop:
    unless it goto ex
    p = shift it
    $S0 = p
    print $S0
    print ' '
    goto loop
ex:
    say 'R'
.end
CODE
Vulcan Intelligent Sentient Humanoid BiPedal LifeForm TestObject R
OUTPUT

# ... now some tests which fail to compose the class

pir_error_output_like( <<'CODE', <<'OUTPUT', "mro error 1" );
#
# example take from: http://www.python.org/2.3/mro.html
#
# "Serious order disagreement" # From Guido
# class O: pass
# class X(O): pass
# class Y(O): pass
# class A(X,Y): pass
# class B(Y,X): pass
# try:
#     class Z(A,B): pass # creates Z(A,B) in Python 2.2
# except TypeError:
#     pass # Z(A,B) cannot be created in Python 2.3
#
.sub main :main
    .local pmc O, X, Y, A, B, Z

    newclass O, "O"
    subclass X, O, "X"
    subclass Y, O, "Y"

    subclass A, X, "A"
    addparent A, Y

    subclass B, Y, "B"
    addparent B, X

    subclass Z, A, "Z"
    addparent Z, B
.end
CODE
/ambiguous hierarchy/
OUTPUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
