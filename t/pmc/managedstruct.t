#! perl
# Copyright: 2001-2005 The Perl Foundation.  All Rights Reserved.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 6;
use Parrot::Config;

=head1 NAME

t/pmc/managedstruct.t - Managed C Structure

=head1 SYNOPSIS

	% prove t/pmc/managedstruct.t

=head1 DESCRIPTION

Tests the ManagedStruct PMC. Checks element access and memory allocation.

=cut

pasm_output_is(<<'CODE', <<'OUTPUT', "Setting ManagedStruct size");
	new P0,.ManagedStruct
	set I0,P0
	eq I0,0,OK_1
	print "not "
OK_1:	print "ok 1\n"
	set P0,1
	set I0,P0
	eq I0,1,OK_2
	print "not "
OK_2:	print "ok 2\n"
	set P0,2
	set I0,P0
	eq I0,2,OK_3
	print "not "
OK_3:	print "ok 3\n"
        end
CODE
ok 1
ok 2
ok 3
OUTPUT


pasm_output_is(<<'CODE', <<'OUTPUT', "element access - float, double");
    new P2, .ResizablePMCArray
    .include "datatypes.pasm"
    push P2, .DATATYPE_FLOAT
    push P2, 2	# 2 elem array
    push P2, 0
    push P2, .DATATYPE_DOUBLE
    push P2, 0
    push P2, 0
    new P0, .ManagedStruct, P2
    # must have set size automatically
    # this is hopefully 2*4+8 everywhere
    set I0, P0
    print I0
    print "\n"
    set P0[0;0], 14.1
    set N0, P0[0;0]
    set P0[0;1], 14.2
    set P0[1], 14.3
    set N0, P0[0;0]
    print N0
    print "\n"
    set N0, P0[0;1]
    print N0
    print "\n"
    set N0, P0[1]
    print N0
    print "\n"
    end
CODE
16
14.100000
14.200000
14.300000
OUTPUT

pasm_output_like(<<'CODE', <<'OUTPUT', "element access - char, short");
    new P2, .ResizablePMCArray
    .include "datatypes.pasm"
    push P2, .DATATYPE_CHAR
    push P2, 2	# 2 elem char array
    push P2, 0
    new P0, .ManagedStruct, P2
    set I0, P0
    ge I0, 2, ok1
    print "not "
ok1:
    print "ok "
    set P0[0;0], 1
    set P0[0;1], 258	# must be truncated to 2
    set I0, P0[0;0]
    print I0
    set I0, P0[0;1]
    print I0
    print " "
    # now acces that as a short
    new P2, .ResizablePMCArray
    push P2, .DATATYPE_SHORT
    push P2, 1
    push P2, 0
    assign P0, P2
    # should be 2*256+1 or 1*256+2
    set I0, P0[0]
    print I0
    end
CODE
/^ok 12 (513|258)$/
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "named element access int16");

.include "datatypes.pasm"

.sub _main
	new $P1, .OrderedHash
	set  $P1['x'], .DATATYPE_INT16
	push $P1, 0
	push $P1, 0

	set $P1['y'], .DATATYPE_INT16
	push $P1, 0
	push $P1, 0

	# need a ManagedStruct to allocate data memory
	new $P2, .ManagedStruct, $P1

	# calc allocation size
	set $I0, 0
	sizeof $I1, .DATATYPE_INT16
	add $I0, $I1
	add $I0, $I1
	# set size
	set $P2, $I0

	# set struct values by name
	set $I0, 2
	set $P2["x"], $I0

	set $I1, 16
	set $S0, "y"
	set $P2[$S0], $I1

	# get struct values by struct item idx
	set $I2, $P2[0]
	set $I3, $P2[1]

	print "x: "
	print $I2

	print "\ny: "
	print $I3

	# get struct values by name
	set $I2, $P2["x"]
	set $I3, $P2["y"]

	print "\nx: "
	print $I2

	print "\ny: "
	print $I3
	print "\n"
	end
.end
CODE
x: 2
y: 16
x: 2
y: 16
OUTPUT

pasm_output_is(<<'CODE', <<'OUTPUT', "nested struct offsets");
  # the nested structure
  .include "datatypes.pasm"
  new P3, .ResizablePMCArray
  push P3, .DATATYPE_INT
  push P3, 0
  push P3, 0
  push P3, .DATATYPE_INT
  push P3, 0
  push P3, 0
  new P4, .UnManagedStruct, P3
  # outer structure
  new P2, .ResizablePMCArray
  push P2, .DATATYPE_INT
  push P2, 0
  push P2, 0
  push P2, .DATATYPE_STRUCT
  # attach the unmanged struct as property
  set P1, P2[-1]
  setprop P1, "_struct", P4
  push P2, 0
  push P2, 0
  push P2, .DATATYPE_INT
  push P2, 0
  push P2, 0
  # attach struct initializer
  new P5, .UnManagedStruct, P2

  # now check offsets
  set I0, P2[2]
  print I0
  print "\n"
  set I0, P2[5]
  print I0
  print "\n"
  set I0, P2[8]
  print I0
  print "\n"
  # nested
  set I0, P3[2]
  print I0
  print "\n"
  set I0, P3[5]
  print I0
  print "\n"

  # check struct size
  set I0, P5
  print I0
  print "\n"
  # nested
  set I0, P4
  print I0
  print "\n"
  end
CODE
0
4
12
0
4
16
8
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "check whether interface is done");

.sub _main
    .local pmc pmc1
    pmc1 = new ManagedStruct
    .local int bool1
    does bool1, pmc1, "scalar"
    print bool1
    print "\n"
    does bool1, pmc1, "no_interface"
    print bool1
    print "\n"
    end
.end
CODE
1
0
OUTPUT

