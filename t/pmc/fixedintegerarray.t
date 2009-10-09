#! parrot
# Copyright (C) 2001-2008, Parrot Foundation.
# $Id$

=head1 NAME

t/pmc/fixedintegerarray.t - FixedIntegerArray PMC

=head1 SYNOPSIS

    % prove t/pmc/fixedintegerarray.t

=head1 DESCRIPTION

Tests C<FixedIntegerArray> PMC. Checks size, sets various elements, including
out-of-bounds test. Checks INT and PMC keys.

=cut

.sub 'main' :main
    .include 'test_more.pir'
    plan(29)

    'test_set_size'()       # 2 tests
    'test_reset_size'()     # 1 test
    'test_set_first'()      # 3 tests
    'test_set_second'()     # 3 tests
    'test_out_of_bounds'()  # 4 tests
    'test_set_via_pmc'()    # 3 tests
    'test_get_via_pmc'()    # 4 tests
    'test_interface_done'() # 4 tests
    'test_get_iter'()       # 1 test
    'test_equality'()       # 5 tests
.end

.sub 'test_set_size'
    $P0 = new ['FixedIntegerArray']

    $I0 = $P0
    is($I0, 0, "Fresh array has 0 elements")

    $P0 = 42
    $I0 = $P0
    is($I0, 42, "Size was set correctly")
.end

.sub 'test_reset_size'
    $P0 = new ['FixedIntegerArray']

    $I0 = 1
    $P0 = 1
    push_eh handled
    $P0 = 2
    $I0 = 0
  handled:
    pop_eh

    ok($I0, "Can't resize")
.end

.sub 'test_set_first'
    $P0 = new ['FixedIntegerArray']
    $P0 = 1

    $P0[0] = -7
    $I0 = $P0[0]
    is($I0, -7, "First element set to integer properly")

    $P0[0] = 3.7
    $I0 = $P0[0]
    is($I0, 3, "First element set to number properly")

    $P0[0] = "17"
    $I0 = $P0[0]
    is($I0, 17, "First element set to string properly")
.end

.sub 'test_set_second'
    $P0 = new ['FixedIntegerArray']
    $P0 = 2

    $P0[1] = -7
    $I0 = $P0[1]
    is($I0, -7, "Second element set to integer properly")

    $P0[1] = 3.7
    $I0 = $P0[1]
    is($I0, 3, "Second element set to number properly")

    $P0[1] = "17"
    $I0 = $P0[1]
    is($I0, 17, "Second element set to string properly")
.end


.sub 'test_out_of_bounds'
    $P0 = new ['FixedIntegerArray']
    $P0 = 1

    $I0 = 1
    push_eh handle_set
    $P0[2] = 7
    $I0 = 0
  handle_set:
    ok($I0, "Can't set out-of-bounds element")

    $I0 = 1
    push_eh handle_set_negative
    $P0[-42] = 7
    $I0 = 0
  handle_set_negative:
    ok($I0, "Can't set element on negative index")

    $I0 = 1
    push_eh handle_get
    $I1 = $P0[2]
    $I0 = 0
  handle_get:
    ok($I0, "Can't get out-of-bounds element")

    $I0 = 1
    push_eh handle_get_negative
    $I1 = $P0[-1]
    $I0 = 0
  handle_get_negative:
    ok($I0, "Can't get element with negative index")

.end

# Set via PMC keys, access via INTs
.sub 'test_set_via_pmc'
    $P0 = new ['FixedIntegerArray']
    $P0 = 3

    $P1 = new ['Key']

    $P1 = 0
    $P0[$P1] = 25
    $I0 = $P0[0]
    is($I0, 25, "Set INTVAL via PMC Key works")

    $P1 = 1
    $P0[$P1] = 2.5
    $I0 = $P0[1]
    is($I0, 2, "Set FLOATVAL via PMC Key works")

    $P1 = 2
    $P0[$P1] = "17"
    $I0 = $P0[2]
    is($I0, 17, "Set STRING via PMC Key works")
.end

# Set via INTs, access via PMC Keys
.sub 'test_get_via_pmc'
    $P0 = new ['FixedIntegerArray']
    $P0 = 1024

    $P0[25]   = 125
    $P0[128]  = 10.2
    $P0[513]  = "17"

    $P1 = new ['Integer']
    $P1 = 123456
    $P0[1023] = $P1

    $P2 = new ['Key']
    
    $P2 = 25
    $I0 = $P0[$P2]
    is($I0, 125, "Get INTVAL via Key works")

    $P2 = 128
    $N0 = $P0[$P2]
    is($N0, 10.0, "Get FLOATVAL via Key works")

    $P2 = 513
    $S0 = $P0[$P2]
    is($S0, "17", "Get STRING via Key works")

    $P2 = 1023
    $I0 = $P0[$P2]
    is($I0, 123456, "Get INTVAL for stored PMC via Key works")

.end

.sub 'test_interface_done'
    .local pmc pmc1
    pmc1 = new ['FixedIntegerArray']
    .local int bool1
    does bool1, pmc1, "scalar"
    nok(bool1, "Does not scalar")
    does bool1, pmc1, "array"
    ok(bool1, "Does array")
    does bool1, pmc1, "no_interface"
    nok(bool1, "Does not no_interface")
.end

.sub 'test_get_iter'
    $P0 = new ['FixedIntegerArray']
    $P0 = 3
    $P0[0] = 42
    $P0[1] = 43
    $P0[2] = 44
    $S0 = ""
    $P1 = iter $P0
  loop:
    unless $P1 goto loop_end
    $S2 = shift $P1
    concat $S0, $S2
    goto loop
  loop_end:
    is($S0, "424344", "Iteration works")
.end

.sub 'test_equality'
    .local pmc a1, a2
    a1 = new ['FixedIntegerArray']
    a2 = new ['FixedIntegerArray']

    is(a1, a2, "Empty arrays are equal")
    
    a1 = 3
    isnt(a1, a2, "Different size arrays aren't equal")

    a2 = 3

    a1[0] = 42
    a2[0] = 42
    is(a1, a2, "Equal with first element set")

    a1[1] = 84
    isnt(a1, a2, "Not equal when second element differ")
    
    a2[1] = 84
    is(a1, a2, "Equal when second element same")
.end


# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
