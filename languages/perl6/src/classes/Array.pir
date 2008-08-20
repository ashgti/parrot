## $Id$

=head1 NAME

src/classes/Array.pir - Perl 6 Array class and related functions

=head2 Object Methods

=cut

.sub 'onload' :anon :load :init
    .local pmc p6meta, arrayproto
    p6meta = get_hll_global ['Perl6Object'], '$!P6META'
    arrayproto = p6meta.'new_class'('Perl6Array', 'parent'=>'List', 'name'=>'Array')

    $P0 = get_hll_namespace ['Perl6Array']
    '!EXPORT'('delete exists pop push shift unshift', 'from'=>$P0)
.end


.namespace ['Perl6Array']
.sub 'infix:=' :method
    .param pmc source
    $P0 = get_hll_global 'list'
    $P0 = $P0(source)
    setattribute $P0, "@!unevaluated", $P0
    .return (self)
.end


.namespace []
.sub 'circumfix:[ ]'
    .param pmc values          :slurpy
    values.'!flatten'()
    $P0 = new 'Perl6Array'
    setattribute $P0, "@!unevaluated", values
    $P1 = new 'Perl6Scalar'
    assign $P1, $P0
    .return ($P1)
.end


=head2 Array methods

=over 4

=item set_pmc_keyed_int (vtable)

Sets the element at the specified index. (Also catch other vtables that want
to provide keys or set values in different forms and just delegate them to
the set_pmc_keyed_int).

=cut

.namespace ['Perl6Array']
.sub 'set_pmc_keyed_int' :vtable
    .param int index
    .param pmc value

    # Are we evaluated up to the point before this element yet?
    .local pmc evaluated
    evaluated = getattribute self, "@!evaluated"
    $I0 = elements evaluated
    if $I0 < index goto use_unevaluated
    evaluated[index] = $P0
    .return ()

    # Need to use the unevaluated portion of the list.
  use_unevaluated:
    .local int upto
    upto = index - 1
    self.'!evaluate_upto'(upto)
    $P0 = evaluated[index]
    .return ($P0)
.end


=item item()

Return Array in item context (i.e., self)

=cut

.sub 'item' :method
    .return (self)
.end


=item push(args :slurpy)

Add C<args> to the end of the Array.

=cut

.sub 'push' :method :multi(Perl6Array)
    .param pmc args :slurpy
    args.'!flatten'()
    $P0 = getattribute self, "@!unevaluated"
    $I0 = elements $P0
    splice $P0, args, $I0, 0
    .return self.'elems'()
.end

.sub '' :vtable('push_pmc')
    .param pmc arg
    self.'push'(arg)
.end

.sub '' :vtable('push_string')
    .param pmc arg
    self.'push'(arg)
.end

.sub '' :vtable('push_integer')
    .param pmc arg
    self.'push'(arg)
.end


=item shift()

Shift the first item off the array and return it.

=cut

.sub 'shift' :method :multi(Perl6Array)
    # First, try and shift from the evaluated part.
    .local pmc evaluated
    evaluated = getattribute self, "@!evaluated"
    $I0 = elements evaluated
    unless $I0 goto use_unevaluated
    $P0 = shift evaluated
    .return ($P0)

    # Failed, try the unevaluated part.
  use_unevaluated:
    .local pmc unevaluated
    unevaluated = getattribute self, "@!unevaluated"
  try_loop:
    $I0 = elements unevaluated
    unless $I0 goto empty
    .local pmc try
    try = unevaluated[0]
    $I0 = isa try, 'Perl6Iterator'
    if $I0 goto is_iter
    try = shift unevaluated
    .return (try)
  is_iter:
    unless try goto iter_out_of_values
    try = shift try
    .return (try)
  iter_out_of_values:
    try = shift unevaluated
    goto try_loop

  empty:
    $P0 = new 'Failure'
    .return ($P0)
.end

.sub '' :vtable('shift_pmc')
    $P0 = self.'shift'()
    .return ($P0)
.end

.sub '' :vtable('shift_string')
    $S0 = self.'shift'()
    .return ($S0)
.end

.sub '' :vtable('shift_integer')
    $I0 = self.'shift'()
    .return ($I0)
.end

.sub '' :vtable('shift_float')
    $N0 = self.'shift'()
    .return ($N0)
.end


.sub '' :vtable('delete_keyed_int')
    .param int index
    
    # Make sure we have evaluated up to this index, then delete from the
    # evaluated part.
    self.'!evaluate_upto'(index)
    .local pmc evaluated
    evaluated = getattribute self, "@!evaluated"
    delete evaluated[index]
.end

.sub '' :vtable('delete_keyed')
    .param int index
    delete self[index]
.end

.sub '' :vtable('delete_keyed_str')
    .param int index
    delete self[index]
.end


=item unshift(args :slurpy)

Adds C<args> to the beginning of the Array.

=cut

.sub 'unshift' :method :multi(Perl6Array)
    .param pmc args :slurpy
    args.'!flatten'()
    $P0 = getattribute self, "@!evaluated"
    splice $P0, args, 0, 0
    .return self.'elems'()
.end

.sub '' :vtable('unshift_pmc')
    .param pmc arg
    self.'unshift'(arg)
.end

.sub '' :vtable('unshift_string')
    .param pmc arg
    self.'unshift'(arg)
.end

.sub '' :vtable('unshift_integer')
    .param pmc arg
    self.'unshift'(arg)
.end

.sub '' :vtable('unshift_float')
    .param pmc arg
    self.'unshift'(arg)
.end


############### Below here still to review after lazy changes. ###############

=item delete(indices :slurpy)

Delete the elements specified by C<indices> from the array
(i.e., replace them with null).  We also shorten the array
to the length of the last non-null (existing) element.

=cut

.sub 'delete' :method :multi(Perl6Array)
    .param pmc indices :slurpy
    .local pmc result
    result = new 'List'
    null $P99

    indices.'!flatten'()
  indices_loop:
    unless indices goto indices_end
    $I0 = shift indices
    $P0 = self[$I0]
    push result, $P0
    self[$I0] = $P99

  shorten:
    $I0 = self.'elems'()
    dec $I0
  shorten_loop:
    if $I0 < 0 goto shorten_end
    $P0 = self[$I0]
    unless null $P0 goto shorten_end
    delete self[$I0]
    dec $I0
    goto shorten_loop
  shorten_end:
    goto indices_loop

  indices_end:
    .return (result)
.end


=item exists(indices :slurpy)

Return true if the elements at C<indices> have been assigned to.

=cut

.sub 'exists' :method :multi(Perl6Array)
    .param pmc indices :slurpy
    .local int test

    test = 0
  indices_loop:
    unless indices goto indices_end
    $I0 = shift indices
    test = exists self[$I0]
    if test goto indices_loop
  indices_end:
    .return 'prefix:?'(test)
.end


=item pop()

Remove the last item from the array and return it.

=cut

.sub 'pop' :method :multi(Perl6Array)
    .local pmc x
    unless self goto empty
    x = pop self
    goto done
  empty:
    x = new 'Failure'
  done:
    .return (x)
.end


=item values()

Return the values of the Array as a List.

=cut

.sub 'values' :method
    $P0 = new 'List'
    splice $P0, self, 0, 0
    .return ($P0)
.end


# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
