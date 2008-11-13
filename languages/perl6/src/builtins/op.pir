## $Id$

=head1 NAME

src/builtins/op.pir - Perl6 builtin operators

=head1 Functions

=over 4

=cut

.namespace []

## This is used by integer computations, to upgrade the answer and return a
## Num if we overflow. We may want to return something like a BigInt in the
## future, but we don't have that yet and this gives something closer to the
## correct semantics than not upgrading an Int at all.
.sub '!upgrade_to_num_if_needed'
    .param num test
    if test > 2147483647.0 goto upgrade
    if test < -2147483648.0 goto upgrade
    $I0 = test
    .return ($I0)
  upgrade:
    .return (test)
.end


## autoincrement
.sub 'postfix:++' :multi(_)
    .param pmc a
    $P0 = clone a
    '!INIT_IF_PROTO'(a, 0)
    inc a
    .return ($P0)
.end

.sub 'postfix:--' :multi(_)
    .param pmc a
    $P0 = clone a
    '!INIT_IF_PROTO'(a, 0)
    dec a
    .return ($P0)
.end


.sub 'prefix:++' :multi(_)
    .param pmc a
    '!INIT_IF_PROTO'(a, 0)
    inc a
    .return (a)
.end


.sub 'prefix:--' :multi(_)
    .param pmc a
    '!INIT_IF_PROTO'(a, 0)
    dec a
    .return (a)
.end


## exponentiation
.sub 'infix:**' :multi(_,_)
    .param num base
    .param num exp
    $N0 = pow base, exp
    .return ($N0)
.end


.sub 'infix:**' :multi(Integer,Integer)
    .param num base
    .param num exp
    $N0 = pow base, exp
    .tailcall '!upgrade_to_num_if_needed'($N0)
.end


## symbolic unary
.sub 'prefix:!' :multi(_)
    .param pmc a
    if a goto a_true
    $P0 = get_hll_global ['Bool'], 'True'
    .return ($P0)
  a_true:
    $P0 = get_hll_global ['Bool'], 'False'
    .return ($P0)
.end


.sub 'prefix:^?' :multi(_)
    .param pmc a
    .tailcall 'prefix:!'(a)
.end


.sub 'prefix:+' :multi(_)
    .param num a
    .return (a)
.end


.sub 'prefix:+' :multi('Integer')
    .param num a
    .tailcall '!upgrade_to_num_if_needed'(a)
.end


.sub 'prefix:-' :multi(_)
    .param num a
    $N0 = neg a
    .return ($N0)
.end


.sub 'prefix:-' :multi('Integer')
    .param num a
    $N0 = neg a
    .tailcall '!upgrade_to_num_if_needed'($N0)
.end


.sub 'prefix:~' :multi(_)
    .param string a
    .return (a)
.end


.sub 'prefix:?' :multi(_)
    .param pmc a
    if a goto a_true
    $P0 = get_hll_global ['Bool'], 'False'
    .return ($P0)
  a_true:
    $P0 = get_hll_global ['Bool'], 'True'
    .return ($P0)
.end


## TODO: prefix:= prefix:* prefix:** prefix:~^ prefix:+^


## multiplicative
.sub 'infix:*' :multi(_,_)
    .param num a
    .param num b
    $N0 = a * b
    .return ($N0)
.end


.sub 'infix:*' :multi(Integer,Integer)
    .param num a
    .param num b
    $N0 = a * b
    .tailcall '!upgrade_to_num_if_needed'($N0)
.end


.sub 'infix:/' :multi(_,_)
    .param num a
    .param num b
    $N0 = a / b
    .return ($N0)
.end


.sub 'infix:/' :multi(Integer,Integer)
    .param num a
    .param num b
    $N0 = a / b
    $I0 = floor $N0
    $N1 = $N0 - $I0
    if $N1 != 0 goto upgrade
    .tailcall '!upgrade_to_num_if_needed'($N0)
  upgrade:
    .return ($N0)
.end


.sub 'infix:%' :multi(_,_)
    .param num a
    .param num b
    $N0 = mod a, b
    .return ($N0)
.end


.sub 'infix:%' :multi(Integer,Integer)
    .param num a
    .param num b
    $N0 = mod a, b
    .tailcall '!upgrade_to_num_if_needed'($N0)
.end


.sub 'infix:x' :multi(_,_)
    .param string str
    .param int count
    if count > 0 goto do_work
    $S0 = ""
    goto done
  do_work:
    $S0 = repeat str, count
  done:
    .return ($S0)
.end


.sub 'infix:xx' :multi(_,_)
    .param pmc a
    .param int n
    $P0 = 'list'()
  loop:
    unless n > 0 goto done
    push $P0, a
    dec n
    goto loop
  done:
    .return ($P0)
.end


.sub 'infix:+&' :multi(_,_)
    .param int a
    .param int b
    $I0 = band a, b
    .return ($I0)
.end


.sub 'infix:+<' :multi(_,_)
    .param int a
    .param int b
    $I0 = shl a, b
    .return ($I0)
.end


.sub 'infix:+>' :multi(_,_)
    .param int a
    .param int b
    $I0 = shr a, b
    .return ($I0)
.end


.sub 'infix:~&' :multi(_,_)
    .param string a
    .param string b
    $S0 = bands a, b
    .return ($S0)
.end


## TODO: infix:~< infix:~>


## additive
.sub 'infix:+' :multi(_,_)
    .param num a
    .param num b
    $N0 = a + b
    .return ($N0)
.end


.sub 'infix:+' :multi(Integer,Integer)
    .param num a
    .param num b
    $N0 = a + b
    .tailcall '!upgrade_to_num_if_needed'($N0)
.end


.sub 'infix:-' :multi(_,_)
    .param num a
    .param num b
    $N0 = a - b
    .return ($N0)
.end


.sub 'infix:-' :multi(Integer,Integer)
    .param num a
    .param num b
    $N0 = a - b
    .tailcall '!upgrade_to_num_if_needed'($N0)
.end


.sub 'infix:~' :multi(_,_)
    .param string a
    .param string b
    $S0 = concat a, b
    .return ($S0)
.end


.sub 'infix:+|'
    .param int a
    .param int b
    $I0 = bor a, b
    .return ($I0)
.end


.sub 'infix:+^'
    .param int a
    .param int b
    $I0 = bxor a, b
    .return ($I0)
.end


.sub 'infix:~|'
    .param string a
    .param string b
    $S0 = bors a, b
    .return ($S0)
.end


.sub 'infix:~^'
    .param string a
    .param string b
    $S0 = bxors a, b
    .return ($S0)
.end


.sub 'infix:?&'
    .param int a
    .param int b
    $I0 = band a, b
    $I0 = isne $I0, 0
    .return ($I0)
.end


.sub 'infix:?|'
    .param int a
    .param int b
    $I0 = bor a, b
    $I0 = isne $I0, 0
    .return ($I0)
.end


.sub 'infix:?^'
    .param int a
    .param int b
    $I0 = bxor a, b
    $I0 = isne $I0, 0
    .return ($I0)
.end


.sub 'true' :multi(_)
    .param pmc a
    .tailcall 'prefix:?'(a)
.end


.sub 'not' :multi(_)
    .param pmc a
    .tailcall 'prefix:!'(a)
.end


.sub 'infix:does'
    .param pmc var
    .param pmc role
    .param pmc init_value      :optional
    .param int have_init_value :opt_flag

    # Get the class of the variable we're adding roles to.
    .local pmc p6meta, parrot_class
    parrot_class = class var

    # Derive a new class that does the role(s) specified.
    .local pmc derived
    derived = new 'Class'
    addparent derived, parrot_class
    $I0 = isa role, 'Role'
    if $I0 goto one_role
    $I0 = isa role, 'List'
    if $I0 goto many_roles
    'die'("'does' expcts a role or a list of roles")

  one_role:
    '!keyword_does'(derived, role)
    goto added_roles

  many_roles:
    .local pmc role_it, cur_role
    role_it = iter role
  roles_loop:
    unless role_it goto roles_loop_end
    cur_role = shift role_it
    '!keyword_does'(derived, cur_role)
    goto roles_loop
  roles_loop_end:
  added_roles:

    # Register proto-object.
    .local pmc p6meta, proto
    p6meta = get_hll_global ['Perl6Object'], '$!P6META'
    proto = var.'WHAT'()
    p6meta.'register'(derived, 'protoobject'=>proto)

    # Instantiate the class to make it form itself.
    $P0 = new derived

    # Re-bless the object into the subclass.
    rebless_subclass var, derived

    # If we were given something to initialize with, do so.
    unless have_init_value goto no_init
    .local pmc attrs
    .local string attr_name
    attrs = inspect role, "attributes"
    attrs = attrs.'keys'()
    $I0 = elements attrs
    if $I0 != 1 goto attr_error
    attr_name = attrs[0]
    attr_name = substr attr_name, 2 # lop of sigil and twigil
    $P0 = var.attr_name()
    assign $P0, init_value
  no_init:

    # We're done - return.
    .return (var)

attr_error:
    'die'("Can only supply an initialization value to a role with one attribute")
.end


.sub 'infix:but'
    .param pmc var
    .param pmc role
    .param pmc value      :optional
    .param int have_value :opt_flag

    # First off, is the role actually a role?
    $I0 = isa role, 'Role'
    if $I0 goto have_role

    # If not, it may be an enum. If we don't have a value, get the class of
    # the thing passed as a role and find out.
    if have_value goto error
    .local pmc the_class, prop, role_list
    push_eh error
    the_class = class role
    prop = getprop 'enum', the_class
    if null prop goto error
    unless prop goto error

    # We have an enum; get the one role of the class and set the value.
    role_list = inspect the_class, 'roles'
    value = role
    role = role_list[0]
    pop_eh
    goto have_role

    # Did anything go wrong?
  error:
    'die'("The but operator can only be used with a role or enum value on the right hand side")

    # Now we have a role, copy the value and call does on the copy.
  have_role:
    var = clone var
    if null value goto no_value
    'infix:does'(var, role, value)
    goto return
  no_value:
    'infix:does'(var, role)
  return:
    .return (var)
.end

=back

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
