#! parrot
# Copyright (C) 2006-2008, Parrot Foundation.
# $Id$

=head1 NAME

t/pmc/callsignature.t - test CallSignature PMC

=head1 SYNOPSIS

    % prove t/pmc/callsignature.t

=head1 DESCRIPTION

Tests the CallSignature PMC.

=cut

.sub main :main
    .include 'test_more.pir'

    plan(8)

    test_instantiate()
    test_get_pmc()
    test_get_set_attrs()
.end


.sub test_instantiate
    $P0 = new ['CallSignature']
    isa_ok($P0,'CallSignature', 'Instantiated CallSignature')
.end

.sub test_get_pmc
    $P0 = new ['CallSignature']
    $P5 = new 'String'

    $P5 = 'foobar'
    setattribute $P0, 'returns', $P5
    $P5 = 'cheese'
    setattribute $P0, 'arg_flags', $P5

    $P2 = $P0
    isa_ok( $P2, 'CallSignature')
.end

.sub test_get_set_attrs
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
    is($P1,'moonbomb', 'got return_flags attribute')

    $P5 = 'cheese'
    setattribute $P0, 'arg_flags', $P5
    ok(1, 'set arg_flags attribute')
    getattribute $P1, $P0, 'arg_flags'
    is($P1,'cheese', 'got arg_flags attribute')
.end


# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
