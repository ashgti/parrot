#! perl -w
# Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

t/op/calling.t - Parrot Calling Conventions

=head1 SYNOPSIS

	% perl t/op/calling.t

=head1 DECSRIPTION

Tests Parrot calling conventions.

=cut

use Parrot::Test tests => 2;
use Test::More;

# Test calling convention operations
output_is(<<'CODE', <<OUTPUT, "foldup");
    new P19, .PerlString
    new P18, .PerlString
    new P17, .PerlString
    new P16, .PerlString
    new P21, .PerlString
    new P20, .PerlString
    new P22, .PerlString
    new P24, .PerlString
    new P28, .PerlString
    new P29, .PerlString
    new P30, .PerlString
    new P27, .PerlString
    new P26, .PerlString
    new P25, .PerlString
    new P23, .PerlString
    set P20,"Foobar!"
    set P23,"Baxman!"
    newsub P0, .Sub, _foo
    set P5,P19
    set P6,P18
    set P7,P17
    set P8,P16
    set P9,P21
    set P10,P20
    set P11,P22
    set P12,P24
    set P13,P28
    set P14,P29
    set P15,P30
    new P3, .SArray
    set P3, 4
    push P3,P27
    push P3,P26
    push P3,P25
    push P3,P23
    set I0,1
    set I1,0
    set I2,0
    set I3,11
    set I4,0
    savetop
    invokecc
    restoretop
    end
_foo:
    foldup P17
    set P16,P17[5]
    print P16
    print "\n"
    set P16,P17[14]
    print P16
    print "\n"
    set I0,1
    set I1,0
    set I2,0
    set I3,0
    set I4,0
    invoke P1

CODE
Foobar!
Baxman!
OUTPUT

output_is(<<'CODE', <<OUTPUT, "foldup_p_i w. skip");
    new P19, .PerlString
    new P18, .PerlString
    new P17, .PerlString
    new P16, .PerlString
    new P21, .PerlString
    new P20, .PerlString
    new P22, .PerlString
    new P24, .PerlString
    new P28, .PerlString
    new P29, .PerlString
    new P30, .PerlString
    new P27, .PerlString
    new P26, .PerlString
    new P25, .PerlString
    new P23, .PerlString
    set P20,"Foobar!"
    set P23,"Baxman!"
    newsub P0, .Sub, _foo
    set P5,P19
    set P6,P18
    set P7,P17
    set P8,P16
    set P9,P21
    set P10,P20
    set P11,P22
    set P12,P24
    set P13,P28
    set P14,P29
    set P15,P30
    new P3, .SArray
    set P3,15
    push P3,P27
    push P3,P26
    push P3,P25
    push P3,P23
    set I0,1
    set I1,4
    set I2,0
    set I3,11
    savetop
    invokecc
    restoretop
    end
_foo:
    foldup P17, 2
    set P16,P17[3]
    print P16
    print "\n"
    set P16,P17[12]
    print P16
    print "\n"
    set I0,1
    set I1,0
    set I2,0
    set I3,0
    set I4,0
    invoke P1

CODE
Foobar!
Baxman!
OUTPUT
