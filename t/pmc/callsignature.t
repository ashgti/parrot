#! parrot
# Copyright (C) 2006-2009, Parrot Foundation.
# $Id$

=head1 NAME

t/pmc/callsignature.t - test CallSignature PMC

=head1 SYNOPSIS

    % prove t/pmc/callsignature.t

=head1 DESCRIPTION

Tests the CallSignature PMC.

=cut

.sub 'main' :main
    .include 'test_more.pir'

    plan(34)

    test_instantiate()
    test_get_set_attrs()
    test_push_pop_indexed_access()
    test_shift_unshift_indexed_access()
    test_indexed_access()
.end

.sub 'test_instantiate'
    $P0 = new ['CallSignature']
    ok(1, 'Instantiated CallSignature')
.end

.sub 'test_get_set_attrs'
    $P0 = new ['CallSignature']
    $P5 = new 'String'

    $P5 = 'foobar'
    setattribute $P0, 'returns', $P5
    ok(1, 'set returns attribute')
    getattribute $P1, $P0, 'returns'
    is($P1,'foobar', 'got returns attribute')

    $P5 = 'moonbomb'
    setattribute $P0, 'return_flags', $P5
    ok(1, 'set return_flags attribute')
    getattribute $P1, $P0, 'return_flags'
    is($P5,'moonbomb', 'got return_flags attribute')

    $P5 = 'cheese'
    setattribute $P0, 'arg_flags', $P5
    ok(1, 'set arg_flags attribute')
    getattribute $P1, $P0, 'arg_flags'
    is($P5,'cheese', 'got arg_flags attribute')
.end

.sub 'test_push_pop_indexed_access'
    $P0 = new [ 'CallSignature' ]
    $P1 = new [ 'Integer' ]
    $P1 = 100

    push $P0, $P1
    $P2 = $P0[0]
    is( $P2, 100, 'push_pmc/get_pmc_keyed_int pair' )
    $P2 = pop $P0
    is( $P2, 100, 'push_pmc/pop_pmc pair' )

    push $P0, 200
    $I0 = $P0[0]
    is( $I0, 200, 'push_integer/get_integer_keyed_int pair' )
    $I0 = pop $P0
    is( $I0, 200, 'push_integer/pop_integer pair' )

    push $P0, 3.03
    $N0 = $P0[0]
    is( $N0, 3.03, 'push_number/get_number_keyed_int pair' )
    $N0 = pop $P0
    is( $N0, 3.03, 'push_number/pop_number pair' )

    push $P0, 'hello'
    $S0 = $P0[0]
    is( $S0, 'hello', 'push_string/get_string_keyed_int pair' )
    $S0 = pop $P0
    is( $S0, 'hello', 'push_string/pop_string pair' )
.end

.sub 'test_shift_unshift_indexed_access'
    $P0 = new [ 'CallSignature' ]
    $P1 = new [ 'Integer' ]
    $P1 = 100

    unshift $P0, $P1
    $P2 = $P0[0]
    is( $P2, 100, 'unshift_pmc/get_pmc_keyed_int pair' )
    $P2 = shift $P0
    is( $P2, 100, 'unshift_pmc/shift_pmc pair' )

    unshift $P0, 200
    $I0 = $P0[0]
    is( $I0, 200, 'unshift_integer/get_integer_keyed_int pair' )
    $I0 = shift $P0
    is( $I0, 200, 'unshift_integer/shift_integer pair' )

    unshift $P0, 3.03
    $N0 = $P0[0]
    is( $N0, 3.03, 'unshift_number/get_number_keyed_int pair' )
    $N0 = shift $P0
    is( $N0, 3.03, 'unshift_number/shift_number pair' )

    unshift $P0, 'hello'
    $S0 = $P0[0]
    is( $S0, 'hello', 'unshift_string/get_string_keyed_int pair' )
    $S0 = shift $P0
    is( $S0, 'hello', 'unshift_string/shift_string pair' )
.end

.sub 'test_indexed_access'
    $P0    = new [ 'CallSignature' ]
    $P0[0] = 100
    $P0[1] = 1.11

    $S0    = '2.22'
    $P0[2] = $S0

    $P1    = new [ 'Float' ]
    $P1    = 3.33
    $P0[3] = $P1

    $I1    = $P0[0]
    is( $I1, 100, 'set_integer_indexed_int/get_integer_indexed_int pair' )

    $N1    = $P0[1]
    is( $N1, 1.11, 'set_number_indexed_int/get_number_indexed_int pair' )

    $S1    = $P0[2]
    is( $S1, '2.22', 'set_string_indexed_int/get_string_indexed_int pair' )

    $P1    = $P0[3]
    is( $P1, 3.33, 'set_pmc_indexed_int/get_pmc_indexed_int pair' )

    $I1 = shift $P0
    is( $I1, 100, 'set_integer_indexed_int/shift_integer pair' )

    $N1 = $P0[0]
    is( $N1, 1.11, 'shift_* should remove elements from array' )

    $N1 = shift $P0
    is( $N1, 1.11, 'set_number_indexed_int/shift_number pair' )

    $S1 = $P0[0]
    is( $S1, '2.22', 'shift_* should remove elements from array' )

    $S1 = shift $P0
    is( $S1, '2.22', 'set_string_indexed_int/shift_string pair' )

    $P1 = $P0[0]
    is( $P1, 3.33, 'shift_* should remove elements from array' )

    $P1 = shift $P0
    is( $P1, 3.33, 'set_pmc_indexed_int/shift_pmc pair' )
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
