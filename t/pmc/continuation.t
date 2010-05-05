#!./parrot
# Copyright (C) 2006-2010, Parrot Foundation.
# $Id$

=head1 NAME

t/pmc/continuation.t - test Continuation PMC

=head1 SYNOPSIS

    % prove t/pmc/continuation.t

=head1 DESCRIPTION

Tests the Continuation PMC.

=cut

.sub main :main
    .include 'test_more.pir'
    plan(4)

    test_new()
    invoke_with_init()
    returns_tt1511()
    returns_tt1528()
.end

.sub test_new
    $P0 = new ['Continuation']
    ok(1, "new Continuation didn't explode")
.end

.sub invoke_with_init
    $P0 = new ['Continuation']
    set_addr $P0, L1
    $P0()
    ok(0, "didn't call continuation")
    goto end
L1:
    ok(1, "called continuation")
    goto end
end:
.end

.sub 'choose'
    .param int do_tailcall
    .param pmc options :slurpy
    .local pmc cc
    .local pmc chosen
    .local pmc paths

    if options goto got_options
    'fail'()
  got_options:
    chosen = shift options

    cc = new 'Continuation'
    set_addr cc, recurse
    paths = get_global '!paths'
    push paths, cc

    $P0  = get_global '!results'
    push $P0, chosen
    .return (chosen)

  recurse:
    if do_tailcall goto tail
    $P0 = 'choose'(do_tailcall, options :flat)
    .return($P0)
  tail:
    .tailcall 'choose'(do_tailcall, options :flat)
.end

.sub 'fail'
    .local pmc cc
    .local pmc paths

    paths = get_global '!paths'

    if paths goto got_paths
    cc = get_global '!topcc'
    goto call_cc
  got_paths:
    cc = shift paths

  call_cc:
    cc()
.end

.sub 'blob'
    .param int do_tailcall

    .local pmc city
    .local pmc store
    .local pmc bx
    .local pmc paths

    paths = new 'ResizablePMCArray'
    set_global '!paths', paths

    city = 'choose'(do_tailcall, "la", "ny", "bos")
    $P0  = get_global '!results'
    push $P0, city
    push $P0, ' '

    'fail'()
.end

.sub 'returns_tt1511'
    .local pmc cc

    # Install top-level cc in global.
    cc = new 'Continuation'
    set_addr cc, final_failure
    set_global '!topcc', cc

    $P0 = new 'ResizableStringArray'
    set_global '!results', $P0

    'blob'(1)
  final_failure:
    $S0 = join '', $P0
    is('lala nyny bosbos ', $S0, 'Results processed correctly')
.end

.sub 'returns_tt1528'
    .local pmc cc

    # Install top-level cc in global.
    cc = new 'Continuation'
    set_addr cc, final_failure
    set_global '!topcc', cc

    $P0 = new 'ResizableStringArray'
    set_global '!results', $P0

    'blob'(0)
  final_failure:
    $S0 = join '', $P0
    is('lala nyny bosbos ', $S0, 'Results processed correctly - without .tailcall')
.end

# end of tests.

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
