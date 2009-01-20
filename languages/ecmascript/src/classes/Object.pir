## $Id$

=head1 TITLE

Object - JS Object class

=head1 DESCRIPTION

This file sets up the base classes and methods for JS's
object system.  Differences (and conflicts) between Parrot's
object model and the JS model means we have to do a little
name and method trickery here and there, and this file takes
care of much of that.

=cut

.namespace []
.sub '' :anon :init :load
    .local pmc jsmeta
    load_bytecode 'PCT.pbc'
    $P1 = get_root_global ['parrot'], 'Hash'
    $P0 = get_root_global ['parrot'], 'P6metaclass'
    $P0.'new_class'('JSObject', 'parent'=>$P1)
    jsmeta = $P0.'HOW'()
    set_hll_global ['JSObject'], '$!JSMETA', jsmeta
.end

=head2 Methods

=over 4

=item clone()

Returns a copy of the object.

NOTE: Don't copy what this method does; it's a tad inside-out. We should be
overriding the clone vtable method to call .clone() really. But if we do that,
we can't current get at the Object PMC's clone method, so for now we do it
like this.

=cut

.namespace ['JSObject']
.sub 'Set' :method
    .param pmc key
    .param pmc val
    self[key] = val
    .return(self)
.end

.namespace ['JSObject']
.sub 'Get' :method
    .param pmc key
    .local pmc val
    val = self[key]
    .return(val)
.end

.namespace ['JSObject']
.sub 'clone' :method
    .param pmc new_attrs :slurpy :named

    # Make a clone.
    .local pmc result
    $I0 = isa self, 'ObjectRef'
    unless $I0 goto do_clone
    self = deref self
  do_clone:
    result = clone self

    # Set any new attributes.
    .local pmc it
    it = iter new_attrs
  it_loop:
    unless it goto it_loop_end
    $S0 = shift it
    $P0 = new_attrs[$S0]
    $S0 = concat '!', $S0
    $P1 = result.$S0()
    'infix:='($P1, $P0)
    goto it_loop
  it_loop_end:

    .return (result)
.end


=item defined()

Return true if the object is defined.

=cut

.namespace ['JSObject']
.sub 'defined' :method
    $P0 = get_hll_global ['Bool'], 'True'
    .return ($P0)
.end


=item hash

Return invocant in hash context.

=cut

.namespace ['JSObject']
.sub 'hash' :method
    .tailcall self.'Hash'()
.end

.namespace []
.sub 'hash'
    .param pmc values :slurpy
    .tailcall values.'Hash'()
.end

=item item

Return invocant in item context.  Default is to return self.

=cut

.namespace ['JSObject']
.sub 'item' :method
    .return (self)
.end

.namespace []
.sub 'item'
    .param pmc x               :slurpy
    $I0 = elements x
    unless $I0 == 1 goto have_x
    x = shift x
  have_x:
    $I0 = can x, 'item'
    unless $I0 goto have_item
    x = x.'item'()
  have_item:
    .return (x)
.end


=item list

Return invocant in list context.  Default is to return a List containing self.

=cut

.namespace ['JSObject']
.sub 'list' :method
    $P0 = new 'List'
    push $P0, self
    .return ($P0)
.end

=item print()

Print the object.

=cut

.namespace ['JSObject']
.sub 'print' :method
    $P0 = get_hll_global 'print'
    .tailcall $P0(self)
.end

=item say()

Print the object, followed by a newline.

=cut

.namespace ['JSObject']
.sub 'say' :method
    $P0 = get_hll_global 'say'
    .tailcall $P0(self)
.end

=item true()

Boolean value of object -- defaults to C<.defined> (S02).

=cut

.namespace ['JSObject']
.sub 'true' :method
    .tailcall self.'defined'()
.end

=back

=head2 Coercion methods

=over 4

=item Array()

=cut

.namespace ['JSObject']
.sub 'Array' :method
    $P0 = new 'JSArray'
    $P0.'!STORE'(self)
    .return ($P0)
.end

=item Hash()

=cut

.namespace ['JSObject']
.sub 'Hash' :method
    $P0 = new 'JSHash'
    $P0.'!STORE'(self)
    .return ($P0)
.end

=item Iterator()

=cut

.sub 'Iterator' :method
    $P0 = self.'list'()
    .tailcall $P0.'Iterator'()
.end

=item Scalar()

Default Scalar() gives reference type semantics, returning
an object reference (unless the invocant already is one).

=cut

.namespace ['JSObject']
.sub '' :method('Scalar') :anon
    $I0 = isa self, 'ObjectRef'
    unless $I0 goto not_ref
    .return (self)
  not_ref:
    $P0 = new 'ObjectRef', self
    .return ($P0)
.end

.namespace []
.sub 'Scalar'
    .param pmc source
    $I0 = isa source, 'ObjectRef'
    if $I0 goto done
    $I0 = can source, 'Scalar'
    if $I0 goto can_scalar
    $I0 = does source, 'scalar'
    source = new 'ObjectRef', source
  done:
    .return (source)
  can_scalar:
    .tailcall source.'Scalar'()
.end

=item Str()

Return a string representation of the invocant.  Default is
the object's type and address.

=cut

.namespace ['JSObject']
.sub 'Str' :method
    $P0 = new 'ResizableStringArray'
    $P1 = self.'WHAT'()
    push $P0, $P1
    $I0 = get_addr self
    push $P0, $I0
    #$S0 = sprintf "[object Object %s 0x%x]", $P0
    $S0 = sprintf "[object Object]", $P0
    .return ($S0)
.end

=back

=head2 Special methods

=over 4

=item new()

Create a new object having the same class as the invocant.

=cut

.namespace ['JSObject']
.sub 'new' :method
    .param pmc init_parents :slurpy
    .param pmc init_this    :named :slurpy

    # Instantiate.
    .local pmc jsmeta
    jsmeta = get_hll_global ['JSObject'], '$!JSMETA'
    $P0 = jsmeta.'get_parrotclass'(self)
    $P1 = new $P0
    .return ($P1)
.end

.sub 'old_new' :method
    .param pmc init_parents :slurpy
    .param pmc init_this    :named :slurpy

    # Instantiate.
    .local pmc jsmeta
    jsmeta = get_hll_global ['JSObject'], '$!JSMETA'
    $P0 = jsmeta.'get_parrotclass'(self)
    $P1 = new $P0
    goto no_whence
    #.return ($P1)

    # If this proto object has a WHENCE auto-vivification, we should use
    # put any values it contains but that init_this does not into init_this.
    .local pmc whence
    whence = self.'WHENCE'()
    unless whence goto no_whence
    .local pmc this_whence_iter
    this_whence_iter = iter whence
  this_whence_iter_loop:
    unless this_whence_iter goto no_whence
    $S0 = shift this_whence_iter
    $I0 = exists init_this[$S0]
    if $I0 goto this_whence_iter_loop
    $P2 = whence[$S0]
    init_this[$S0] = $P2
    goto this_whence_iter_loop
  no_whence:

    # Now we will initialize each attribute in the class itself and it's
    # parents with an Undef or the specified initialization value. Note that
    # the all_parents list includes ourself.
    .local pmc all_parents, class_iter
    all_parents = inspect $P0, "all_parents"
    class_iter = iter all_parents
  class_iter_loop:
    unless class_iter goto class_iter_loop_end
    .local pmc cur_class
    cur_class = shift class_iter

    # If it's PMCProxy, then skip over it, since it's attribute is the delegate
    # instance of a parent PMC class, which we should not change to Undef.
    .local int is_pmc_proxy
    is_pmc_proxy = isa cur_class, "PMCProxy"
    if is_pmc_proxy goto class_iter_loop_end

    # If this the current class?
    .local pmc init_attribs
    eq_addr cur_class, $P0, current_class

    # If it's not the current class, need to see if we have any attributes.
    # Go through the provided init_parents to see if we have anything that
    # matches.
    .local pmc ip_iter, cur_ip
    ip_iter = iter init_parents
  ip_iter_loop:
    unless ip_iter goto ip_iter_loop_end
    cur_ip = shift ip_iter

    # We will check if their HOW matches.
    $P2 = jsmeta.'get_parrotclass'(cur_ip)
    eq_addr cur_class, $P2, found_parent_init

    goto found_init_attribs
  ip_iter_loop_end:

    # If we get here, found nothing.
    init_attribs = new 'Hash'
    goto parent_init_search_done

    # We found some parent init data, potentially.
  found_parent_init:
    init_attribs = cur_ip.'WHENCE'()
    $I0 = 'defined'(init_attribs)
    if $I0 goto parent_init_search_done
    init_attribs = new 'Hash'
  parent_init_search_done:
    goto found_init_attribs

    # If it's the current class, we will take the init_this hash.
  current_class:
    init_attribs = init_this
  found_init_attribs:

    # Now go through attributes of the current class and iternate over them.
    .local pmc attribs, it
    attribs = inspect cur_class, "attributes"
    it = iter attribs
  iter_loop:
    unless it goto iter_end
    $S0 = shift it

    # See if we have an init value; use Undef if not.
    .local int got_init_value
    $S1 = substr $S0, 2
    got_init_value = exists init_attribs[$S1]
    if got_init_value goto have_init_value
    $P2 = new 'Undef'
    goto init_done
  have_init_value:
    $P2 = init_attribs[$S1]
    delete init_attribs[$S1]
  init_done:

    # Is it a scalar? If so, want a scalar container with the type set on it.
    .local string sigil
    sigil = substr $S0, 0, 1
    if sigil != '$' goto no_scalar
    .local pmc attr_info, type
    attr_info = attribs[$S0]
    if null attr_info goto set_attrib
    type = attr_info['type']
    if null type goto set_attrib
    if got_init_value goto no_proto_init
    $I0 = isa type, 'P6protoobject'
    unless $I0 goto no_proto_init
    set $P2, type
  no_proto_init:
    $P2 = new 'JSScalar', $P2
    setprop $P2, 'type', type
    goto set_attrib
  no_scalar:

    # Is it an array? If so, initialize to JSArray.
    if sigil != '@' goto no_array
    $P3 = new 'JSArray'
    $I0 = defined $P2
    if $I0 goto have_array_value
    set $P2, $P3
    goto set_attrib
  have_array_value:
    'infix:='($P3, $P2)
    set $P2, $P3
    goto set_attrib
  no_array:

    # Is it a Hash? If so, initialize to JSHash.
    if sigil != '%' goto no_hash
    $P3 = new 'Hash'
    $I0 = defined $P2
    if $I0 goto have_hash_value
    set $P2, $P3
    goto set_attrib
  have_hash_value:
    'infix:='($P3, $P2)
    set $P2, $P3
    goto set_attrib
  no_hash:

  set_attrib:
    push_eh set_attrib_eh
    setattribute $P1, cur_class, $S0, $P2
  set_attrib_eh:
    pop_eh
    goto iter_loop
  iter_end:

    # Do we have anything left in the hash? If so, unknown.
    $I0 = elements init_attribs
    if $I0 == 0 goto init_attribs_ok
    'die'("You passed an initialization parameter that does not have a matching attribute.")
  init_attribs_ok:

    # Next class.
    goto class_iter_loop
  class_iter_loop_end:

    .return ($P1)
.end

=item 'PARROT'

Report the object's true nature.

=cut

.sub 'PARROT' :method
    .local pmc obj
    .local string result
    obj = self
    result = ''
    $I0 = isa obj, 'ObjectRef'
    unless $I0 goto have_obj
    result = 'ObjectRef->'
    obj = deref obj
  have_obj:
    $P0 = typeof obj
    $S0 = $P0
    result .= $S0
    .return (result)
.end


=item REJECTS(topic)

Define REJECTS methods for objects (this would normally
be part of the Pattern role, but we put it here for now
until we get roles).

=cut

.sub 'REJECTS' :method
    .param pmc topic
    $P0 = self.'ACCEPTS'(topic)
    $P1 = not $P0
    .return ($P1)
.end


=item WHENCE()

Return the invocant's auto-vivification closure.

=cut

.sub 'WHENCE' :method
    $P0 = self.'WHAT'()
    $P1 = $P0.'WHENCE'()
    .return ($P1)
.end


=item WHERE

Gets the memory address of the object.

=cut

.sub 'WHERE' :method
    $I0 = get_addr self
    .return ($I0)
.end


=item WHICH

Gets the object's identity value

=cut

.sub 'WHICH' :method
    # For normal objects, this can just be the memory address.
    .tailcall self.'WHERE'()
.end

=back

=head2 Private methods

=over 4

=item !cloneattr(attrlist)

Create a clone of self, also cloning the attributes given by attrlist.

=cut

.namespace ['JSObject']
.sub '!cloneattr' :method
    .param string attrlist
    .local pmc jsmeta, result
    jsmeta = get_hll_global ['JSObject'], '$!JSMETA'
    $P0 = jsmeta.'get_parrotclass'(self)
    result = new $P0

    .local pmc attr_it
    attr_it = split ' ', attrlist
  attr_loop:
    unless attr_it goto attr_end
    $S0 = shift attr_it
    unless $S0 goto attr_loop
    $P1 = getattribute self, $S0
    if null $P1 goto null_attr
    $P1 = clone $P1
  null_attr:
    setattribute result, $S0, $P1
    goto attr_loop
  attr_end:
    .return (result)
.end

=item !.?

Helper method for implementing the .? operator. Calls at most one matching
method, and returns undef if there are none.

=cut

.sub '!.?' :method
    .param string method_name
    .param pmc pos_args     :slurpy
    .param pmc named_args   :slurpy :named

    # Get all possible methods.
    .local pmc methods
    methods = self.'!MANY_DISPATCH_HELPER'(method_name, pos_args, named_args)

    # Do we have any?
    $I0 = elements methods
    if $I0 goto invoke
    .tailcall '!FAIL'('Undefined value returned by invocation of undefined method')

    # If we do have a method, call it.
  invoke:
    $P0 = methods[0]
    .tailcall self.$P0(pos_args :flat, named_args :named :flat)
.end

=item !.*

Helper method for implementing the .* operator. Calls one or more matching
methods.

=cut

.sub '!.*' :method
    .param string method_name
    .param pmc pos_args     :slurpy
    .param pmc named_args   :slurpy :named

    # Get all possible methods.
    .local pmc methods
    methods = self.'!MANY_DISPATCH_HELPER'(method_name, pos_args, named_args)

    # Build result capture list.
    .local pmc pos_res, named_res, cap, result_list, it, cur_meth
    $P0 = get_hll_global 'list'
    result_list = $P0()
    it = iter methods
  it_loop:
    unless it goto it_loop_end
    cur_meth = shift it
    (pos_res :slurpy, named_res :named :slurpy) = cur_meth(self, pos_args :flat, named_args :named :flat)
    cap = 'prefix:\\'(pos_res :flat, named_res :flat :named)
    push result_list, cap
    goto it_loop
  it_loop_end:

    .return (result_list)
.end


=item !.+

Helper method for implementing the .+ operator. Calls one or more matching
methods, dies if there are none.

=cut

.sub '!.+' :method
    .param string method_name
    .param pmc pos_args     :slurpy
    .param pmc named_args   :slurpy :named

    # Use !.* to produce a (possibly empty) list of result captures.
    .local pmc result_list
    result_list = self.'!.*'(method_name, pos_args :flat, named_args :flat :named)

    # If we got no elements at this point, we must die.
    $I0 = elements result_list
    if $I0 == 0 goto failure
    .return (result_list)
  failure:
    $S0 = "Could not invoke method '"
    concat $S0, method_name
    concat $S0, "' on invocant of type '"
    $S1 = self.'WHAT'()
    concat $S0, $S1
    concat $S0, "'"
    'die'($S0)
.end


=item !MANY_DISPATCH_HELPER

This is a helper for implementing .+, .? and .*. In the future, it may well be
the basis of WALK also. It returns all methods we could possible call.

=cut

.sub '!MANY_DISPATCH_HELPER' :method
    .param string method_name
    .param pmc pos_args
    .param pmc named_args

    # We need to find all methods we could call with the right name.
    .local pmc jsmeta, result_list, class, mro, it
    $P0 = get_hll_global 'list'
    result_list = $P0()
    jsmeta = get_hll_global ['JSObject'], '$!JSMETA'
    class = self.'WHAT'()
    class = jsmeta.'get_parrotclass'(class)
    mro = inspect class, 'all_parents'
    it = iter mro
  mro_loop:
    unless it goto mro_loop_end
    .local pmc cur_class, meths, cur_meth
    cur_class = shift it
    meths = inspect cur_class, 'methods'
    cur_meth = meths[method_name]
    if null cur_meth goto mro_loop

    # If we're here, found a method. But is it a multi?
    $I0 = isa cur_meth, "JSMultiSub"
    if $I0 goto multi_dispatch

    # Single dispatch - add to the result list.
    push result_list, cur_meth
    goto mro_loop

    # Multiple dispatch; get all applicable candidates.
  multi_dispatch:
    .local pmc possibles, possibles_it
    possibles = cur_meth.'find_possible_candidates'(self, pos_args :flat)
    possibles_it = iter possibles
  possibles_it_loop:
    unless possibles_it goto possibles_it_loop_end
    cur_meth = shift possibles_it
    push result_list, cur_meth
    goto possibles_it_loop
  possibles_it_loop_end:
    goto mro_loop
  mro_loop_end:

    .return (result_list)
.end

=item !.^

Helper for doing calls on the metaclass.

=cut

.sub '!.^' :method
    .param string method_name
    .param pmc pos_args     :slurpy
    .param pmc named_args   :slurpy :named

    # Get the HOW or the object and do the call on that.
    .local pmc how
    how = self.'HOW'()
    .tailcall how.method_name(self, pos_args :flat, named_args :flat :named)
.end

=back

=head2 Vtable functions

=cut

.namespace ['JSObject']
.sub '' :vtable('decrement') :method
    $P0 = self.'pred'()
    'infix:='(self, $P0)
    .return(self)
.end

.sub '' :vtable('defined') :method
    $I0 = self.'defined'()
    .return ($I0)
.end

.sub '' :vtable('get_bool') :method
    $I0 = self.'true'()
    .return ($I0)
.end

.sub '' :vtable('get_iter') :method
    .tailcall self.'Iterator'()
.end

.sub '' :vtable('get_string') :method
    $S0 = self.'Str'()
    .return ($S0)
.end

.sub '' :vtable('increment') :method
    $P0 = self.'succ'()
    'infix:='(self, $P0)
    .return(self)
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
