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

    plan(15)

    test_instantiate()
    test_get_set_attrs()
    test_push_pop_indexed_access()
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

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
