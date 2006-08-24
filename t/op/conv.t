#!perl
# Copyright (C) 2001-2005, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 12;


=head1 NAME

t/op/conv.t - C# Ops

=head1 SYNOPSIS

	% prove t/op/conv.t

=head1 DESCRIPTION

Tests the additional opcodes for C# compilation. See F<ops/dotgnu.ops>.

=cut


pasm_output_is(<<'CODE', <<OUTPUT, "conv_i1_i");
    set I0, 127
    conv_i1 I0
    print I0
    print "\n"
    inc I0
    conv_i1 I0
    print I0
    print "\n"
    inc I0
    conv_i1 I0
    print I0
    print "\n"
    dec I0
    conv_i1 I0
    print I0
    print "\n"
    dec I0
    conv_i1 I0
    print I0
    print "\n"
    end
CODE
127
-128
-127
-128
127
OUTPUT

pasm_output_is(<<'CODE', <<OUTPUT, "conv_u1_i");
    set I0, 127
    conv_u1 I0
    print I0
    print "\n"
    inc I0
    conv_u1 I0
    print I0
    print "\n"
    set I0, 255
    conv_u1 I0
    print I0
    print "\n"
    inc I0
    conv_u1 I0
    print I0
    print "\n"
    dec I0
    conv_u1 I0
    print I0
    print "\n"
    end
CODE
127
128
255
0
255
OUTPUT

pasm_output_is(<<'CODE', <<OUTPUT, "conv_i2_i");
    set I0, 32767
    conv_i2 I0
    print I0
    print "\n"
    inc I0
    conv_i2 I0
    print I0
    print "\n"
    inc I0
    conv_i2 I0
    print I0
    print "\n"
    dec I0
    conv_i2 I0
    print I0
    print "\n"
    dec I0
    conv_i2 I0
    print I0
    print "\n"
    end
CODE
32767
-32768
-32767
-32768
32767
OUTPUT

pasm_output_is(<<'CODE', <<OUTPUT, "conv_u2_i");
    set I0, 32767
    conv_u2 I0
    print I0
    print "\n"
    inc I0
    conv_u2 I0
    print I0
    print "\n"
    set I0, 65535
    conv_u2 I0
    print I0
    print "\n"
    inc I0
    conv_u2 I0
    print I0
    print "\n"
    dec I0
    conv_u2 I0
    print I0
    print "\n"
    end
CODE
32767
32768
65535
0
65535
OUTPUT


pasm_output_is(<<'CODE', <<OUTPUT, "conv_i1_ofv_i, positive direction");
    set I0, 127
    conv_i1_ovf I0
    print I0
    print "\n"
    set I0, 128
    conv_i1_ovf I0
    print I0
    end
CODE
127
Overflow exception for conv_i1_ovf
OUTPUT

pasm_output_is(<<'CODE', <<OUTPUT, "conv_i1_ofv_i, negative direction");
    set I0, -128
    conv_i1_ovf I0
    print I0
    print "\n"
    dec I0
    conv_i1_ovf I0
    print I0
    end
CODE
-128
Overflow exception for conv_i1_ovf
OUTPUT

pasm_output_is(<<'CODE', <<OUTPUT, "conv_u1_ofv_i, positive direction");
    set I0, 255
    conv_u1_ovf I0
    print I0
    print "\n"
    set I0, 256
    conv_u1_ovf I0
    print I0
    end
CODE
255
Overflow exception for conv_u1_ovf
OUTPUT

pasm_output_is(<<'CODE', <<OUTPUT, "conv_u1_ofv_i, negative direction");
    set I0, 0
    conv_u1_ovf I0
    print I0
    print "\n"
    dec I0
    conv_u1_ovf I0
    print I0
    end
CODE
0
Overflow exception for conv_u1_ovf
OUTPUT

pasm_output_is(<<'CODE', <<OUTPUT, "conv_i2_ofv_i, positive direction");
    set I0, 32767
    conv_i2_ovf I0
    print I0
    print "\n"
    inc I0
    conv_i2_ovf I0
    print I0
    end
CODE
32767
Overflow exception for conv_i2_ovf
OUTPUT

pasm_output_is(<<'CODE', <<OUTPUT, "conv_i2_ofv_i, negative direction");
    set I0, -32768
    conv_i2_ovf I0
    print I0
    print "\n"
    dec I0
    conv_i2_ovf I0
    print I0
    end
CODE
-32768
Overflow exception for conv_i2_ovf
OUTPUT

pasm_output_is(<<'CODE', <<OUTPUT, "conv_u2_ofv_i, positive direction");
    set I0, 65535
    conv_u2_ovf I0
    print I0
    print "\n"
    inc I0
    conv_u2_ovf I0
    print I0
    print "\n"
    end
CODE
65535
Overflow exception for conv_u2_ovf
OUTPUT

pasm_output_is(<<'CODE', <<OUTPUT, "conv_u2_ofv_i, negative direction");
    set I0, 0
    conv_u2_ovf I0
    print I0
    print "\n"
    dec I0
    conv_u2_ovf I0
    print I0
    end
CODE
0
Overflow exception for conv_u2_ovf
OUTPUT



