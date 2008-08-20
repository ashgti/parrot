## $Id$

=head1 NAME

src/classes/Iterator.pir - Perl 6 Iterator class

XXX Needs cleanup and probably fixes/changes before moving to trunk.

=head2 Object Methods

=over 4

=cut

.sub 'onload' :anon :load :init
    .local pmc p6meta
    p6meta = get_hll_global ['Perl6Object'], '$!P6META'
    p6meta.'new_class'('Perl6Iterator', 'name'=>'Iterator', 'parent'=>'Any', 'attributes'=>'@!list $!position')
.end


.sub 'get_bool' :vtable
    .local pmc list, evaluated, unevaluated
    .local int elems

    # Check evaluated part.
    list = getattribute self, "@!list"
    evaluated = getattribute list, "@!evaluated"
    elems = elements evaluated
    if elems goto true

    # Check unevaluated part; if we just have iterators, we need to make sure
    # they have values (that is, if we have a list of ten exhausted iterators,
    # we want to be sure that we return a false value here, since there are no
    # more values to be obtained).
    unevaluated = getattribute list, "@!unevaluated"
    elems = elements unevaluated
    unless elems goto false
    .local int cur_check
    cur_check = 0
  try_loop:
    $P0 = unevaluated[cur_check]
    $I0 = isa $P0, 'Perl6Iterator'
    unless $I0 goto true
    if $P0 goto true
    inc cur_check
    if cur_check >= elems goto false
    goto try_loop

  true:
    .return (1)
  false:
    .return (0)
.end


.sub 'shift_pmc' :vtable
    .local pmc position, value, list
    .local int elems_available

    position = getattribute self, "$!position"
    list = getattribute self, "@!list"
    if position < 0 goto use_unevaluated

    # If we're here, we're getting from the evaluated part - or at least,
    # trying to.
    .local pmc evaluated
    evaluated = getattribute list, "@!evaluated"
    elems_available = elements evaluated
    $I0 = position
    if elems_available <= $I0 goto out_of_evaluated

    # We have things available in the evaluated portion. Get a value, then
    # increment our position, and we're done.
    value = evaluated[position]
    inc position
    .return (value)

    # Here we find we have depleted our supply of evaluated values. Set the
    # position to -1 to indicate this and fall through to using the unevaluated
    # part of the list to service this request.
  out_of_evaluated:
    position = -1

    # Here we take values from the unevaluated part of the list.
  use_unevaluated:
    .local pmc unevaluated, try
    unevaluated = getattribute list, "@!unevaluated"
  try_loop:
    elems_available = elements unevaluated
    if elems_available == 0 goto failure

    # See if it's an iterator at the head of the list or not.
    try = unevaluated[0]
    $I0 = isa try, 'Perl6Iterator'
    if $I0 goto get_from_iter
    
    # Not an iterator, so just pull this value off the head of the unevaluated
    # part and hand it back.
    value = unshift unevaluated
    .return (value)

    # We have an iterator. Make sure it's not exhausted, and if not get a value
    # from it. If it is, then we need to remove it and try the next iterator.
  get_from_iter:
    unless try goto empty_iter
    value = shift try
    .return (value)
  empty_iter:
    $P0 = unshift unevaluated
    goto try_loop

    # If there's no elements available, we fail.
  failure:
    value = new 'Undef'
    .return (value)
.end
