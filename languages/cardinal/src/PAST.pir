## $Id$

=head1 NAME

Cardinal::PAST - Abstract syntax tree nodes for Cardinal

=head1 DESCRIPTION

This file implements the various abstract syntax tree nodes
needed for Ruby 1.9.  The currently defined ast nodes:

    Cardinal::PAST::Node       - base class for all ast nodes

=head1 METHODS

=over 4

=cut

.include 'languages/cardinal/src/preamble'

.HLL 'Ruby', 'ruby_group'
.namespace [ 'Cardinal::PAST' ]

.sub '__onload' :load
    .local pmc base
    base = newclass 'Cardinal::PAST::Node'
    addattribute base, 'children'
    addattribute base, 'source'
    addattribute base, 'pos'
    addattribute base, 'name'

    $P0 = subclass base, 'Cardinal::PAST::Op'
    $P0 = subclass base, 'Cardinal::PAST::Exp'
    $P0 = subclass base, 'Cardinal::PAST::Stmt'
    $P0 = subclass base, 'Cardinal::PAST::Stmts'
    $P0 = subclass base, 'Cardinal::PAST::Sub'
    $P0 = subclass base, 'Cardinal::PAST::FunctionCall'
    $P0 = subclass base, 'Cardinal::PAST::MethodCall'
    $P0 = subclass base, 'Cardinal::PAST::WhileUntil'
    $P0 = subclass base, 'Cardinal::PAST::For'

    $P0 = subclass base, 'Cardinal::PAST::Val'
    addattribute $P0, 'valtype'

    $P0 = subclass base, 'Cardinal::PAST::Var'
    addattribute $P0, 'scope'

    $P1 = subclass $P0, 'Cardinal::PAST::PositionalParameter'
    addattribute $P1, 'type'
    $P1 = subclass $P0, 'Cardinal::PAST::OptionalParameter'
    addattribute $P1, 'default_value'
    $P1 = subclass $P0, 'Cardinal::PAST::RestParameter'
    $P1 = subclass $P0, 'Cardinal::PAST::BlockParameter'

    $P0 = subclass base, 'Cardinal::PAST::Block'
    addattribute $P0, 'outer'
    addattribute $P0, 'blocktype'
    addattribute $P0, 'vardecl'

    $P1 = subclass $P0, 'Cardinal::PAST::Module'
    addattribute $P1, 'class_path'

    $P2 = subclass $P1, 'Cardinal::PAST::Class'
    addattribute $P2, 'superclass'

    $P0 = subclass base, 'Cardinal::PAST::ClassPath'
    addattribute $P0, 'starting_colons'
    addattribute $P0, 'class_name'

    $P0 = subclass base, 'Cardinal::PAST::Rescue_Stmt'
    addattribute $P0, 'try_stmt'
    addattribute $P0, 'rescue_stmt'

    $P0 = new .Integer
    $P0 = 10
    set_hll_global ['Cardinal::PAST'], 'serial_number', $P0
    .return ()
.end


.namespace [ 'Cardinal::PAST::Node' ]

.sub 'attr' :method
    .param string attrname
    .param pmc value
    .param int setvalue
    if setvalue goto set
    value = getattribute self, attrname
    unless null value goto end
    value = new .Undef
  set:
    setattribute self, attrname, value
  end:
    .return (value)
.end


.sub 'init' :method
    .param pmc children        :slurpy
    .param pmc adverbs         :slurpy :named

    unless null children goto set_children
    children = new .ResizablePMCArray
  set_children:
    setattribute self, 'children', children

    if null adverbs goto end
    .local pmc iter
    iter = new .Iterator, adverbs
  iter_loop:
    unless iter goto iter_end
    $S0 = shift iter
    if $S0 == 'XXX' goto iter_loop
    $P0 = iter[$S0]
    $P1 = find_method self, $S0
    self.$P1($P0)
    goto iter_loop
  iter_end:
  end:
    .return ()
.end


.sub 'new' :method
    .param string class
    .param pmc children        :slurpy
    .param pmc adverbs         :slurpy :named

    $I0 = find_type class
    $P0 = new $I0
    $P0.'init'(children :flat, 'node'=>self, 'XXX'=>1, adverbs :flat :named)
    .return ($P0)
.end


.sub 'add_child' :method
    .param pmc child
    .local pmc array
    array = getattribute self, 'children'
    push array, child
    .return ()
  .end

.sub 'append_children' :method
    .param pmc children
    .local pmc array
    array = getattribute self, 'children'
    array.'append'(children)
    .return ()
.end


.sub 'add_child_new' :method
    .param string class
    .param pmc children        :slurpy
    .param pmc adverbs         :slurpy :named
    $P0 = self.'new'(class, children :flat, 'XXX'=>0, adverbs :flat :named)
    self.'add_child'($P0)
    .return ($P0)
.end

.gen_accessor('source')
.gen_accessor('pos')
.gen_accessor('name')
.gen_get_accessor('children')

.sub 'children' :method
#    .param pmc value :optional
#    .param int has_value        :opt_flag
    null $P0
    .return self.'attr'('children', $P0, 0)
.end

.sub 'node' :method
    .param pmc node
    $I0 = isa node, 'Cardinal::PAST::Node'
    if $I0 goto clone_past
  clone_pge:
    $S0 = node
    self.'source'($S0)
    $I0 = node.'from'()
    self.'pos'($I0)
    .return ()
  clone_past:
    $S0 = node.'source'()
    self.'source'($S0)
    $I0 = node.'pos'()
    self.'pos'($I0)
    .return ()
.end


.sub 'child_iter' :method
    $P0 = getattribute self, 'children'
    $P1 = new .Iterator, $P0
    $P1 = 0
    .return ($P1)
.end


=item C<Cardinal::PAST::Node::unique([string fmt])>

Each call to C<unique> returns a unique number, or if a C<fmt>
parameter is given it returns a unique string beginning with
C<fmt>.  (This may eventually be generalized to allow
uniqueness anywhere in the string.)  The function starts
counting at 10 (so that the values 0..9 can be considered "safe").

=cut

.sub 'unique' :method
    .param string fmt          :optional
    .param int has_fmt         :opt_flag

    if has_fmt goto unique_1
    fmt = ''
  unique_1:
    $P0 = get_hll_global ['Cardinal::PAST'], 'serial_number'
    $S0 = $P0
    $S0 = concat fmt, $S0
    inc $P0
    .return ($S0)
.end


.sub '__elements' :method
    $P0 = getattribute self, 'children'
    $I0 = elements $P0
    .return ($I0)
.end


.sub '__get_pmc_keyed_int' :method
    .param int key
    $P0 = getattribute self, 'children'
    $P0 = $P0[key]
    .return ($P0)
.end


.sub '__set_pmc_keyed_int' :method
    .param int key
    .param pmc val
    $P0 = getattribute self, 'children'
    $P0[key] = val
    .return ()
.end

.sub '__get_string' :method
    $P0 = getattribute self, 'name'
    $S0 = $P0
    .return ($S0)
.end


.sub '__dumplist' :method
    .return ('pos name children')
.end


.sub '__dump' :method
    .param pmc dumper
    .param string label
    .local string indent, subindent

    (subindent, indent) = dumper.'newIndent'()
    print '=> { '
.local pmc attrlist, iter
    $S0 = self.'__dumplist'()
    attrlist = split ' ', $S0
    iter = new .Iterator, attrlist
  iter_loop:
    unless iter goto iter_end
    .local string attrname
    .local pmc val
    attrname = shift iter
    val = getattribute self, attrname
    print "\n"
    print subindent
    print attrname
    print ' => '
    dumper.'dump'(label, val)
    goto iter_loop
  iter_end:
    print "\n"
    print indent
    print '}'
    dumper.'deleteIndent'()
    .return ()
.end


.namespace [ 'Cardinal::PAST::Op' ]
.gen_dumplist('name children')

.namespace [ 'Cardinal::PAST::Val' ]
.gen_accessor('valtype')
.gen_dumplist('name valtype')

.namespace [ 'Cardinal::PAST::Var' ]
.gen_accessor('scope')
.gen_dumplist('name scope')

.namespace [ 'Cardinal::PAST::PositionalParameter' ]
.gen_accessor('type')
.gen_dumplist('name type')

.namespace [ 'Cardinal::PAST::OptionalParameter' ]
.gen_accessor('default_value')
.gen_dumplist('name default_value')

.namespace [ 'Cardinal::PAST::RestParameter' ]

.namespace [ 'Cardinal::PAST::BlockParameter' ]

.namespace [ 'Cardinal::PAST::Block' ]
.gen_accessor('outer')
.gen_accessor('blocktype')
.gen_dumplist('name outer blocktype children vardecl')

.sub '__init' :method
    null $P0
    setattribute self, 'outer', $P0
    $P0 = new .Hash
    setattribute self, 'vardecl', $P0
    .return ()
.end

.sub 'vardecl' :method
    .param pmc name            :optional
    .param int has_name        :opt_flag
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .local pmc vardecl
    vardecl = getattribute self, 'vardecl'
    if has_value goto with_value
    if has_name goto with_name
    .return (vardecl)
  with_name:
    value = vardecl[name]
    .return (value)
  with_value:
    vardecl[name] = value
    .return (value)
.end


.namespace [ 'Cardinal::PAST::Module' ]
.gen_accessor('class_path')

.namespace [ 'Cardinal::PAST::Class' ]

.namespace [ 'Cardinal::PAST::ClassPath' ]
.gen_accessor('class_name')
.gen_accessor('starting_colons')

.namespace [ 'Cardinal::PAST::Rescue_Stmt' ]
.gen_accessor('try_stmt')
.gen_accessor('rescue_stmt')
.gen_dumplist('name try_stmt rescue_stmt')


# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
