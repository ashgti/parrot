#! parrot
# Copyright (C) 2009, Patrick R. Michaud
# $Id$

=head1 NAME

t/compilers/pct/regex/05-p6regex.t - basic p6regex tests

=head1 SYNOPSIS

    % prove t/compilers/pct/regex/05-p6regext.t

=cut

.sub 'main' :main
    load_bytecode 'Test/More.pbc'

    .local pmc exports, curr_namespace, test_namespace
    curr_namespace = get_namespace
    test_namespace = get_namespace ['Test';'More']
    exports        = split ' ', 'plan diag ok nok is todo'
    test_namespace.'export_to'(curr_namespace, exports)

    plan(4)

    load_bytecode 'PCT/Regex.pbc'

    .local pmc p6regex, cur
    p6regex = get_hll_global ['Regex'], 'P6Regex'
    cur = p6regex.'!cursor_init'('abcd')

    .local pmc abc, match
    abc = cur.'atom'()
    ok(abc, 'Matched <atom> abc')
    match = abc.'MATCH'()
    $S0 = match
    is($S0, 'abc', 'Matched <atom> abc')

    .local pmc d
    d = abc.'atom'()
    ok(abc, 'Matched <atom> d')
    match = d.'MATCH'()
    $S0 = match
    is($S0, 'd', 'Matched <atom> d')
.end
