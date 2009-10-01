#! parrot
# Copyright (C) 2009, Patrick R. Michaud
# $Id$

=head1 NAME

t/compilers/pct/regex/03-symtoken.t - tests for PCT::Regex symtokens

=head1 SYNOPSIS

    % prove t/compilers/pct/regex/03-symtoken.t

=cut

.sub 'main' :main
    load_bytecode 'Test/More.pbc'

    .local pmc exports, curr_namespace, test_namespace
    curr_namespace = get_namespace
    test_namespace = get_namespace ['Test';'More']
    exports        = split ' ', 'plan diag ok nok is todo'
    test_namespace.'export_to'(curr_namespace, exports)

    load_bytecode 'PCT/Regex.pbc'

    # create a grammar for us to test
    .local pmc p6meta, xyz
    p6meta = get_hll_global 'P6metaclass'
    xyz = p6meta.'new_class'('XYZ', 'parent'=>'Regex::Cursor')

    # add some symtokens
    xyz.'!symtoken_add'('term:sym<abc>', 'abc')
    xyz.'!symtoken_add'('infix:sym<+>', '+')
    xyz.'!symtoken_add'('infix:sym<->', '-')
    xyz.'!symtoken_add'('infix:sym<++>', '++')
    xyz.'!symtoken_add'('infix:sym<+->', '+-')

    # create a cursor for matching
    .local pmc cur
    cur = xyz.'!cursor_init'('abc++-')

    $I0 = p6meta.'isa'(cur, xyz)
    ok($I0, 'match cursor isa XYZ')

    .local pmc abc, match
    abc = cur.'term:sym<abc>'()
    ok(abc, 'Matched initial abc')
    match = abc.'MATCH'()
    ok(match, '?$/')
    $I0 = match.'from'()
    is($I0, 0, "$/.from")
    $I0 = match.'to'()
    is($I0, 3, "$/.to")
    $S0 = match['sym']
    is($S0, 'abc', "$<sym>")

    .local pmc abc2
    abc2 = abc.'term:sym<abc>'()
    nok(abc2, 'No second match for abc')

    .local pmc plus
    plus = abc.'infix:sym<+>'()
    ok(plus, 'Matched plus following abc')
    match = plus.'MATCH'()
    ok(match, '?$/')
    $I0 = match.'from'()
    is($I0, 3, "$/.from")
    $I0 = match.'to'()
    is($I0, 4, "$/.to")
    $S0 = match['sym']
    is($S0, '+', "$<sym>")
    
.end
