#! parrot
# Copyright (C) 2001-2008, Parrot Foundation.
# $Id$

=head1 NAME

t/pmc/stringiterator.t - StringIterator.

=head1 SYNOPSIS

    % prove t/pmc/stringiterator.t

=head1 DESCRIPTION

Tests the C<StringIterator> PMC. Iterate over string in both directions.

=cut

.include 'iterator.pasm'

.sub main :main
    .include 'test_more.pir'

    plan(18)

    iterate_forward() # 10 tests
    iterate_backward() # 8 tests

.end

.sub 'iterate_forward'
    .local pmc s, it

    s = new ['String']

    it = iter s
    nok(it, "Iterator for empty string is empty")
    $I0 = isa it, 'Iterator'
    ok($I0, "Have proper type")

    s  = "bar"
    it = s

    it = iter s
    ok(it, "Iterator for 'bar' is not empty")
    $S0 = shift it
    ok(it, "Can shift 1st character")
    is($S0, 'b', "With correct value")

    $S0 = shift it
    ok(it, "Can shift 2nd character")
    is($S0, 'a', "With correct value")

    $P0 = shift it
    nok(it, "Iterator is finished after 3rd shift")
    is($P0, 'r', "3rd character has correct value as PMC")

    $I0 = 1
    push_eh fail
    $P0 = shift it
    $I0 = 0
  fail:
    pop_eh:
    ok($I0, "Shifting from finished iterator throws exception")

.end

.sub 'iterate_backward'
    .local pmc s, it

    s = new ['String']
    s = 'BAZ'

    it = iter s
    it = .ITERATE_FROM_END
    ok(it, "Iterator reset to backward iteration")

    $S0 = pop it
    ok(it, "Can shift 1st character")
    is($S0, 'Z', "With expected value")

    $S0 = pop it
    ok(it, "Can shift 2nd character")
    is($S0, 'A', "With expected value")

    $P0 = pop it
    nok(it, "Iterator is finished after third shift")
    is($P0, 'B', "3rd element has correct value as PMC")

    $I0 = 1
    push_eh fail
    $P0 = pop it
    $I0 = 0
  fail:
    pop_eh
    ok($I0, "Shifting from finished iterator throws exception")
.end

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 filetype=pir:
