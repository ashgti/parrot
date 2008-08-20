## $Id$

=head1 NAME

src/classes/List.pir - Perl 6 List class and related functions

The List class implements a lazy list. It has two attributes, one that
contains the currently unevaluated parts of the list and one that contains
the fully evaluated parts. Both of these are Parrot ResizablePMCArrays.
When a list is created, we flatten out any sublists within it to get a
list of just values and iterators and put it into the unevaluated portion.
Then, if we index into a certain position in the list, we generate all
values up to that positioin if we have not done so already and hand them back.

For iterating, we do not save any of the generated values. This means that
things like:

  for =$fh1, =$fh2 -> $x { ... }

Will operate with constant memory consumption.

=head2 Object Methods

=over 4

=cut

.sub 'onload' :anon :load :init
    .local pmc p6meta, listproto
    p6meta = get_hll_global ['Perl6Object'], '$!P6META'
    $P0 = newclass 'List'
    addattribute $P0, '@!unevaluated'
    addattribute $P0, '@!evaluated'
    $P1 = new 'Hash'
    $P1['name'] = 'array'
    $P1 = new Role, $P1
    addrole $P0, $P1
    listproto = p6meta.'register'($P0, 'parent'=>'Any')

    $P0 = get_hll_namespace ['List']
    #'!EXPORT'('first grep keys kv map pairs reduce values', $P0)
.end


=item clone()    (vtable method)

Return a clone of this list.  (Clones its elements also.)

=cut

.namespace ['List']
.sub 'clone' :method
    .local pmc result, it, evaluated, unevaluated, new_evaluated, new_unevaluated

    # Clone evaluated.
    evaluated = getattribute self, '@!evaluated'
    new_evaluated = new 'ResizablePMCArray'
    it = iter evaluated
  iter_loop_ev:
    unless it goto iter_end_ev
    $P0 = shift it
    $P0 = clone $P0
    push new_evaluated, $P0
    goto iter_loop_ev
  iter_end_ev:

    # Clone unevaluated.
    unevaluated = getattribute self, '@!unevaluated'
    new_unevaluated = new 'ResizablePMCArray'
    it = iter unevaluated
  iter_loop_u:
    unless it goto iter_end_u
    $P0 = shift it
    $P0 = clone $P0
    push new_unevaluated, $P0
    goto iter_loop_u
  iter_end_u:

    # Build result.
    $P0 = typeof self
    result = new $P0
    setattribute result, '@!evaluated', new_evaluated
    setattribute result, '@!unevaluated', new_unevaluated
    .return (result)
.end

.sub '' :vtable('clone')
    $P0 = self.'clone'()
    .return ($P0)
.end


=item C<iterator> (vtable get_iter)

Gets an iterator for the list.

=cut

.sub 'iterator' :vtable('get_iter')
    $P0 = self.'clone'()
    .return ($P0)
.end


=item C<get_bool>

Returns true if there are items in the list.

=cut

.sub 'get_bool' :vtable
    .local pmc evaluated, unevaluated
    .local int elems

    # Check evaluated part.
    evaluated = getattribute self, "@!evaluated"
    elems = elements evaluated
    if elems goto true

    # Check unevaluated part; if we just have iterators, we need to make sure
    # they have values (that is, if we have a list of ten exhausted iterators,
    # we want to be sure that we return a false value here, since there are no
    # more values to be obtained).
  unevaluated_check:
    unevaluated = getattribute self, "@!unevaluated"
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


=item C<shift_pmc>

Gets the next value when we are iterating the List.

=cut

.sub 'shift_pmc' :vtable
    .local pmc value
    .local int elems_available
say "in shift_pmc"
    # See if we have anything in the evaluated part.
    .local pmc evaluated
    evaluated = getattribute self, "@!evaluated"
    elems_available = elements evaluated
    if elems_available == 0 goto use_unevaluated

    # We have things available in the evaluated portion. Get a value, then
    # increment our position, and we're done.
    value = shift evaluated
    .return (value)

    # Here we take values from the unevaluated part of the list.
  use_unevaluated:
    .local pmc unevaluated, try
    unevaluated = getattribute self, "@!unevaluated"
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



=item C<list()>

Gets this value in list context (since it is already a List, just returns itself).

=cut

.sub 'list' :method
    .return (self)
.end


=item C<init>

On initilization, create evaluated and unevaluated parts just as normal Parrot
resizable arrays. This provides our storage.

=cut

.sub 'init' :vtable
    $P0 = new 'ResizablePMCArray'
    setattribute self, "@!unevaluated", $P0
    $P0 = new 'ResizablePMCArray'
    setattribute self, "@!evaluated", $P0
.end


=item get_pmc_keyed (vtable)

Gets an element at the specified index. (Also catch other vtables that want
to provide keys or get values in different forms and just delegate them to
the get_pmc_keyed_int).

=cut

.sub 'get_pmc_keyed_int' :vtable
    .param int index

    # Are we evaluated up to this point yet?
    .local pmc evaluated
    evaluated = getattribute self, "@!evaluated"
    $I0 = elements evaluated
    if $I0 <= index goto use_unevaluated
    $P0 = evaluated[index]
    .return ($P0)

    # Need to use the unevaluated portion of the list.
  use_unevaluated:
    self.'!evaluate_upto'(index)
    $P0 = evaluated[index]
    .return ($P0)
.end

.sub 'get_pmc_keyed' :vtable
    .param int index
    $P0 = self[index]
    .return ($P0)
.end

.sub 'get_pmc_keyed_str' :vtable
    .param int index
    $P0 = self[index]
    .return ($P0)
.end

.sub 'get_string_keyed' :vtable
    .param int index
    $P0 = self[index]
    .return ($P0)
.end

.sub 'get_string_keyed_int' :vtable
    .param int index
    $P0 = self[index]
    .return ($P0)
.end

.sub 'get_string_keyed_str' :vtable
    .param int index
    $P0 = self[index]
    .return ($P0)
.end

.sub 'get_integer_keyed' :vtable
    .param int index
    $P0 = self[index]
    .return ($P0)
.end

.sub 'get_integer_keyed_int' :vtable
    .param int index
    $P0 = self[index]
    .return ($P0)
.end

.sub 'get_integer_keyed_str' :vtable
    .param int index
    $P0 = self[index]
    .return ($P0)
.end


=item C<!evaluate_upto(index)>

Takes the PMC that has lazy parts and lazily evaluates it up to the required
index.

=cut

.sub '!evaluate_upto' :method
    .param int required
    
    # Get the parts of the list and see what we have and what's available.
    .local pmc evaluated, unevaluated
    .local int have, available
    evaluated = getattribute self, "@!evaluated"
    unevaluated = getattribute self, "@!unevaluated"
    have = elements evaluated
    available = elements unevaluated
    if available == 0 goto loop_end

    # Loop while we need more values.
  loop:
    if have > required goto loop_end
    .local pmc try
    try = unevaluated[0]
    $I0 = isa try, 'Perl6Iterator'
    if $I0 goto have_iter
    try = shift unevaluated
    push evaluated, try
    inc have
    goto loop

  have_iter:
    unless try goto iter_no_values
    try = shift try
    push evaluated, try
    goto loop

  iter_no_values:
    try = unshift unevaluated
    goto loop
  loop_end:
.end


=item elems (vtable elements)

Get the number of elements in the list. May cause evaluation of things that do
not know how many elements they have without evaluating themselves first.

=cut

.sub 'elems' :method
    .local pmc evaluated, unevaluated
    .local int total, uneval_count
    evaluated = getattribute self, "@!evaluated"
    unevaluated = getattribute self, "@!unevaluated"
    total = elements evaluated
    uneval_count = elements unevaluated
    if uneval_count == 0 goto done

    # We have lazy bits - need to evaluate them and work out how many elements
    # are in there.
    # XXX hack
    total += uneval_count

  done:
    .return (total)
.end

.sub '' :vtable('elements')
    $I0 = self.'elems'()
    .return ($I0)
.end

.sub '' :vtable('get_integer')
    $I0 = self.'elems'()
    .return ($I0)
.end

.sub '' :vtable('get_number')
    $N0 = self.'elems'()
    .return ($N0)
.end


=back

=head2 Methods added to ResizablePMCArray

=over 4

=item list()

Return the List as a list.

=cut

.namespace ['ResizablePMCArray']
.sub 'list' :method
    # Produces a new lazy list with the ResizablePMCArray set to the
    # unevaluated portion at this point.
    .local pmc list
    list = new 'List'
    self.'!flatten'()
    setattribute list, "@!unevaluated", self
    .return (list)
.end


=item !flatten()

Flatten the invocant, as in list context.  This doesn't make the list eager,
it just brings any nested Lists to the top layer. Since we want to return a
ResizablePMCArray, we splice in the evaluated and unevaluated portions of
any Lists encountered.

=cut

.sub '!flatten' :method
    .param int size            :optional
    .param int has_size        :opt_flag

    ##  we use the 'elements' opcode here because we want the true length
    .local int len, i
    len = elements self
    i = 0
  flat_loop:
    if i >= len goto flat_end
    unless has_size goto flat_loop_1
    if i >= size goto flat_end
  flat_loop_1:
    .local pmc elem
    elem = self[i]
    $I0 = defined elem
    unless $I0 goto flat_next
    $I0 = isa elem, 'Perl6Scalar'
    if $I0 goto flat_next
    $I0 = isa elem, 'List'
    unless $I0 goto check_array
    $P0 = getattribute elem, "@!unevaluated"
    splice self, $P0, i, 1
    $P0 = getattribute elem, "@!evaluated"
    splice self, $P0, i, 0
    goto flat_loop
  check_array:
    $I0 = does elem, 'array'
    unless $I0 goto flat_next
    splice self, elem, i, 1
    len = elements self
    goto flat_loop
  flat_next:
    inc i
    goto flat_loop
  flat_end:
    .return (self)
.end


=back

=head1 Functions

=over 4

=item C<list(...)>

Build a List from its arguments.

=cut

.namespace []
.sub 'list'
    .param pmc values          :slurpy
    values = values.'!flatten'()
    .local pmc list
    list = new 'List'
    setattribute list, "@!unevaluated", values
    .return (list)
.end


=item C<infix:,(...)>

Operator form for building a list from its arguments.

=cut

.sub 'infix:,'
    .param pmc args            :slurpy
    args = args.'!flatten'()
    .local pmc list
    list = new 'List'
    setattribute list, "@!unevaluated", args
    .return (list)
.end


################# Below here to review for lazy conversion. #################


=item get_string()    (vtable method)

Return the elements of the list joined by spaces.

=cut

.sub 'get_string' :vtable :method
    $S0 = join ' ', self
    .return ($S0)
.end


=item hash()

Return the List invocant as a Hash.

=cut

.sub 'hash' :method
    .local pmc result, iter
    result = new 'Perl6Hash'
    iter = self.'iterator'()
  iter_loop:
    unless iter goto iter_end
    .local pmc elem, key, value
    elem = shift iter
    $I0 = does elem, 'hash'
    if $I0 goto iter_hash
    $I0 = isa elem, 'Perl6Pair'
    if $I0 goto iter_pair
    unless iter goto err_odd_list
    value = shift iter
    value = clone value
    result[elem] = value
    goto iter_loop
  iter_hash:
    .local pmc hashiter
    hashiter = elem.'keys'()
  hashiter_loop:
    unless hashiter goto hashiter_end
    $S0 = shift hashiter
    value = elem[$S0]
    result[$S0] = value
    goto hashiter_loop
  hashiter_end:
    goto iter_loop
  iter_pair:
    key = elem.'key'()
    value = elem.'value'()
    result[key] = value
    goto iter_loop
  iter_end:
    .return (result)

  err_odd_list:
    die "Odd number of elements found where hash expected"
.end


=item item()

Return the List invocant in scalar context (i.e., an Array).

=cut

.sub 'item' :method
    $P0 = new 'Perl6Array'
    splice $P0, self, 0, 0
    .return ($P0)
.end


=item perl()

Returns a Perl representation of a List.

=cut

.sub 'perl' :method
    .local string result
    result = '['

    .local pmc iter
    iter = self.'iterator'()
    unless iter goto iter_done
  iter_loop:
    $P1 = shift iter
    $S1 = $P1.'perl'()
    result .= $S1
    unless iter goto iter_done
    result .= ', '
    goto iter_loop
  iter_done:
    result .= ']'
    .return (result)
.end


.namespace [ 'ResizablePMCArray' ]

=back


=item elems()

Return the number of elements in the list.

=cut

.sub 'elems' :method :multi('ResizablePMCArray') :vtable('get_number')
    self.'!flatten'()
    $I0 = elements self
    .return ($I0)
.end


=item first(...)

=cut

.sub 'first' :method :multi('ResizablePMCArray', 'Sub')
    .param pmc test
    .local pmc retv
    .local pmc iter
    .local pmc block_res
    .local pmc block_arg

    iter = self.'iterator'()
  loop:
    unless iter goto nomatch
    block_arg = shift iter
    block_res = test(block_arg)
    if block_res goto matched
    goto loop

  matched:
    retv = block_arg
    goto done

  nomatch:
    retv = new 'Failure'
    goto done

  done:
    .return(retv)
.end


.sub 'first' :multi('Sub')
    .param pmc test
    .param pmc values :slurpy

    .return values.'first'(test)
.end


=item grep(...)

=cut

.sub 'grep' :method :multi('ResizablePMCArray', 'Sub')
    .param pmc test
    .local pmc retv
    .local pmc iter
    .local pmc block_res
    .local pmc block_arg

    retv = new 'List'
    iter = self.'iterator'()
  loop:
    unless iter goto done
    block_arg = shift iter
    block_res = test(block_arg)

    unless block_res goto loop
    retv.'push'(block_arg)
    goto loop

  done:
    .return(retv)
.end

.sub 'grep' :multi('Sub')
    .param pmc test
    .param pmc values          :slurpy
    .return values.'grep'(test)
.end


=item iterator()

Returns an iterator for the list.

=cut

.sub 'iterator' :method
    self.'!flatten'()
    $P0 = iter self
    .return ($P0)
.end


=item keys()

Returns a List containing the keys of the invocant.

=cut

.sub 'keys' :method :multi(ResizablePMCArray)
    $I0 = self.'elems'()
    dec $I0
    $P0 = 'infix:..'(0, $I0)
    .return $P0.'list'()
.end

.sub 'keys' :multi()
    .param pmc values          :slurpy
    .return values.'keys'()
.end


=item kv()

Return items in invocant as 2-element (index, value) lists.

=cut

.sub 'kv' :method :multi(ResizablePMCArray)
    .local pmc result, iter
    .local int i

    result = new 'List'
    iter = self.'iterator'()
    i = 0
  iter_loop:
    unless iter goto iter_end
    $P0 = shift iter
    push result, i
    push result, $P0
    inc i
    goto iter_loop
  iter_end:
    .return (result)
.end

.sub 'kv' :multi()
    .param pmc values          :slurpy
    .return values.'kv'()
.end


=item map()

Map.

=cut

.sub 'map' :method :multi('ResizablePMCArray', 'Sub')
    .param pmc expression
    .local pmc res, elem, block, mapres, iter, args
    .local int i, arity

    arity = expression.'arity'()
    if arity > 0 goto body
    arity = 1
  body:
    res = new 'List'
    iter = self.'iterator'()
  map_loop:
    unless iter goto done

    # Creates arguments for closure
    args = new 'ResizablePMCArray'

    i = 0
  args_loop:
    if i == arity goto invoke
    unless iter goto elem_undef
    elem = shift iter
    goto push_elem
  elem_undef:
    elem = new 'Failure'
  push_elem:
    push args, elem
    inc i
    goto args_loop

  invoke:
    (mapres :slurpy) = expression(args :flat)
    unless mapres goto map_loop
    mapres.'!flatten'()
    $I0 = elements res
    splice res, mapres, $I0, 0
    goto map_loop

  done:
    .return(res)
.end


.sub 'map' :multi('Sub')
    .param pmc expression
    .param pmc values          :slurpy
    .return values.'map'(expression)
.end


=item pairs()

Return a list of Pair(index, value) elements for the invocant.

=cut

.sub 'pairs' :method :multi(ResizablePMCArray)
    .local pmc result, iter
    .local int i

    result = new 'List'
    iter = self.'iterator'()
    i = 0
  iter_loop:
    unless iter goto iter_end
    $P0 = shift iter
    $P1 = 'infix:=>'(i, $P0)
    push result, $P1
    inc i
    goto iter_loop
  iter_end:
    .return (result)
.end

.sub 'pairs' :multi()
    .param pmc values          :slurpy
    .return values.'pairs'()
.end


=item reduce(...)

=cut

.sub 'reduce' :method :multi('ResizablePMCArray', 'Sub')
    .param pmc expression
    .local pmc retv
    .local pmc iter
    .local pmc elem
    .local pmc args
    .local int i, arity

    arity = expression.'arity'()
    if arity < 2 goto error

    iter = self.'iterator'()
    unless iter goto empty
    retv = shift iter
  loop:
    unless iter goto done

    # Create arguments for closure
    args = new 'ResizablePMCArray'
    # Start with 1. First argument is result of previous call
    i = 1

  args_loop:
    if i == arity goto invoke
    unless iter goto elem_undef
    elem = shift iter
    goto push_elem
  elem_undef:
    elem = new 'Failure'

  push_elem:
    push args, elem
    inc i
    goto args_loop

  invoke:
    retv = expression(retv, args :flat)
    goto loop

  empty:
    retv = new 'Undef'
    goto done

  error:
    'die'('Cannot reduce() using a unary or nullary function.')
    goto done

  done:
    .return(retv)
.end

.sub 'reduce' :multi('Sub')
    .param pmc expression
    .param pmc values          :slurpy
    .return values.'reduce'(expression)
.end


=item uniq(...)

=cut

# TODO Rewrite it. It's too naive.

.namespace ['List']
.sub uniq :method
    .local pmc ulist
    .local pmc key
    .local pmc val
    .local pmc uval
    .local int len
    .local int i
    .local int ulen
    .local int ui

    ulist = new 'List'
    len = self.'elems'()
    i = 0

  loop:
    if i == len goto done

    val = self[i]

    ui = 0
    ulen = ulist.'elems'()
    inner_loop:
        if ui == ulen goto inner_loop_done

        uval = ulist[ui]
        if uval == val goto found

        inc ui
        goto inner_loop
    inner_loop_done:

    ulist.'push'(val)

    found:

    inc i
    goto loop

  done:
    .return(ulist)
.end


=item values()

Returns a List containing the values of the invocant.

=cut

.sub 'values' :method :multi('ResizablePMCArray')
    self.'!flatten'()
    .return (self)
.end

.sub 'values' :multi()
    .param pmc values          :slurpy
    .return values.'!flatten'()
.end





=item C<infix:Z(...)>

The zip operator.

=cut

.sub 'infix:Z'
    .param pmc args :slurpy
    .local int num_args
    num_args = elements args

    # Empty list of no arguments.
    if num_args > 0 goto has_args
    $P0 = new 'List'
    .return($P0)
has_args:

    # Get minimum element count - what we'll zip to.
    .local int min_elem
    .local int i
    i = 0
    $P0 = args[0]
    min_elem = elements $P0
min_elems_loop:
    if i >= num_args goto min_elems_loop_end
    $P0 = args[i]
    $I0 = elements $P0
    unless $I0 < min_elem goto not_min
    min_elem = $I0
not_min:
    inc i
    goto min_elems_loop
min_elems_loop_end:

    # Now build result list of lists.
    .local pmc res
    res = new 'List'
    i = 0
zip_loop:
    if i >= min_elem goto zip_loop_end
    .local pmc cur_list
    cur_list = new 'List'
    .local int j
    j = 0
zip_elem_loop:
    if j >= num_args goto zip_elem_loop_end
    $P0 = args[j]
    $P0 = $P0[i]
    cur_list[j] = $P0
    inc j
    goto zip_elem_loop
zip_elem_loop_end:
    res[i] = cur_list
    inc i
    goto zip_loop
zip_loop_end:

    .return(res)
.end


=item C<infix:X(...)>

The non-hyper cross operator.

=cut

.sub 'infix:X'
    .param pmc args            :slurpy
    .local pmc res
    res = new 'List'

    # Algorithm: we'll maintain a list of counters for each list, incrementing
    # the counter for the right-most list and, when it we reach its final
    # element, roll over the counter to the next list to the left as we go.
    .local pmc counters
    .local pmc list_elements
    .local int num_args
    counters = new 'FixedIntegerArray'
    list_elements = new 'FixedIntegerArray'
    num_args = elements args
    counters = num_args
    list_elements = num_args

    # Get element count for each list.
    .local int i
    .local pmc cur_list
    i = 0
elem_get_loop:
    if i >= num_args goto elem_get_loop_end
    cur_list = args[i]
    $I0 = elements cur_list
    list_elements[i] = $I0
    inc i
    goto elem_get_loop
elem_get_loop_end:

    # Now we'll start to produce them.
    .local int res_count
    res_count = 0
produce_next:

    # Start out by building list at current counters.
    .local pmc new_list
    new_list = new 'List'
    i = 0
cur_perm_loop:
    if i >= num_args goto cur_perm_loop_end
    $I0 = counters[i]
    $P0 = args[i]
    $P1 = $P0[$I0]
    new_list[i] = $P1
    inc i
    goto cur_perm_loop
cur_perm_loop_end:
    res[res_count] = new_list
    inc res_count

    # Now increment counters.
    i = num_args - 1
inc_counter_loop:
    $I0 = counters[i]
    $I1 = list_elements[i]
    inc $I0
    counters[i] = $I0

    # In simple case, we just increment this and we're done.
    if $I0 < $I1 goto inc_counter_loop_end

    # Otherwise we have to carry.
    counters[i] = 0

    # If we're on the first element, all done.
    if i == 0 goto all_done

    # Otherwise, loop.
    dec i
    goto inc_counter_loop
inc_counter_loop_end:
    goto produce_next

all_done:
    .return(res)
.end


=item C<infix:min(...)>

The min operator.

=cut

.sub 'infix:min'
    .param pmc args :slurpy

    # If we have no arguments, undefined.
    .local int elems
    elems = elements args
    if elems > 0 goto have_args
    $P0 = new 'Undef'
    .return($P0)
have_args:

    # Find minimum.
    .local pmc cur_min
    .local int i
    cur_min = args[0]
    i = 1
find_min_loop:
    if i >= elems goto find_min_loop_end
    $P0 = args[i]
    $I0 = 'infix:cmp'($P0, cur_min)
    if $I0 != -1 goto not_min
    set cur_min, $P0
not_min:
    inc i
    goto find_min_loop
find_min_loop_end:

    .return(cur_min)
.end


=item C<infix:max(...)>

The max operator.

=cut

.sub 'infix:max'
    .param pmc args :slurpy

    # If we have no arguments, undefined.
    .local int elems
    elems = elements args
    if elems > 0 goto have_args
    $P0 = new 'Undef'
    .return($P0)
have_args:

    # Find maximum.
    .local pmc cur_max
    .local int i
    cur_max = args[0]
    i = 1
find_max_loop:
    if i >= elems goto find_max_loop_end
    $P0 = args[i]
    $I0 = 'infix:cmp'($P0, cur_max)
    if $I0 != 1 goto not_max
    set cur_max, $P0
not_max:
    inc i
    goto find_max_loop
find_max_loop_end:

    .return(cur_max)
.end

## TODO: zip


=back

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
