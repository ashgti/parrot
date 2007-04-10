# $Id$

=head1 NAME

POST - A(nother) low-level opcode syntax tree.

=head1 DESCRIPTION

Compilers progress through various levels of
tree representations of compilation of a source
code program.  POST (Parrot Opcode Syntax Tree) is
a low-level tree which closely corresponds to the
semantics of PIR/PASM.

The base class of POST is Cardinal::PAST::Node -- see C<lib/CPAST.pir>

=head1 METHODS

=over 4

=cut
.include 'languages/cardinal/src/preamble'

.namespace [ 'Cardinal::POST' ]

.sub '__onload' :load
    .local pmc base
    $P0 = getclass 'Cardinal::PAST::Node'
    base = subclass $P0, 'Cardinal::POST::Node'
    addattribute base, 'value'
    
    $P0 = subclass base, 'Cardinal::POST::Namespace'

    $P0 = subclass base, 'Cardinal::POST::Val'
    addattribute $P0, 'valtype'

    $P0 = subclass base, 'Cardinal::POST::Var'
    addattribute $P0, 'scope'
    addattribute $P0, 'islvalue'

    $P0 = subclass base, 'Cardinal::POST::Sub'
    addattribute $P0, 'outer'
    addattribute $P0, 'subtype'
    addattribute $P0, 'varhash'
    addattribute $P0, 'prologue'
    addattribute $P0, 'adverbs'
    addattribute $P0, 'subs'

    $P0 = subclass base, 'Cardinal::POST::Op'
    $P0 = subclass base, 'Cardinal::POST::Ops'
    $P0 = subclass base, 'Cardinal::POST::Label'
    $P0 = subclass base, 'Cardinal::POST::Assign'
    $P0 = subclass base, 'Cardinal::POST::Call'
    $P0 = subclass base, 'Cardinal::POST::Raw'
    addattribute $P0, 'raw'
    $P0 = subclass base, 'Cardinal::POST::NOP'
    $P0 = subclass base, 'Cardinal::POST::NamespaceLookup'
    addattribute $P0, 'item'

.end

.namespace [ 'Cardinal::POST::Node' ]

.sub '__init' :method
    $P0 = new .String
    setattribute self, 'name', $P0
    $P0 = new .String
    setattribute self, 'value', $P0
    .return ()
.end

.gen_accessor('name')
  
=item C<Cardinal::POST::Node::value()>

Set or return the invocant's value.  If no value has been
previously set for this node, then we generate a unique 
PMC register (uninitialized) and use that.

=cut

.sub value :method
    .param pmc value           :optional
    .param int has_value       :opt_flag

    value = self.'attr'('value', value, has_value)
    if value > '' goto end
    $S0 = self.'unique'('$P')
    assign value, $S0
  end:
    .return (value)
.end


.sub '__dumplist' :method
    .return ('name value children')
.end


.namespace [ 'Cardinal::POST::Ops' ]

.sub 'root_pir' :method
    ##   build the pir for this node and its children
    $P0 = self.'pir'(self)
    .return ($P0)
.end

.sub 'pir' :method
    .param pmc grammar
    .local pmc code, iter

    code = new 'PGE::CodeString'
    iter = self.'child_iter'()
  iter_loop:
    unless iter goto iter_end
    $P0 = shift iter
    $P1 = $P0.'pir'(grammar)
    code .= $P1
    goto iter_loop
  iter_end:
    .return (code)
.end


.namespace [ 'Cardinal::POST::Op' ]

.sub 'pir' :method
    .param pmc block
    .local pmc childvalues, iter
    childvalues = new .ResizablePMCArray
    iter = self.'child_iter'()
  iter_loop:
    unless iter goto iter_end
    $P0 = shift iter
    $I0 = isa $P0, 'Cardinal::POST::Node'
    if $I0 == 0 goto iter_loop_1
    $P0 = $P0.'value'()
  iter_loop_1:
    push childvalues, $P0
    goto iter_loop
  iter_end:

    .local pmc code
    code = new 'PGE::CodeString'
    $P0 = self.'name'()
    # $I0 = isa $P0, 'Cardinal::POST::Sub'
    # if $I0 goto get_sub_name
    $I0 = isa $P0, 'Cardinal::POST::Node'
    if $I0 goto get_sub_value
    .local string name
    name = $P0
    $S0 = substr name, 0, 1
    if $S0 == "'" goto sub_call
    code.'emit'('    %n %,', childvalues :flat, 'n'=>name)
    .return (code)
  get_sub_name:
    name = $P0.'name'()
    name = concat "'", name
    name = concat name, "'"
    goto sub_call
  get_sub_value:
    name = $P0.'value'()
  sub_call:
    $P0 = shift childvalues
    code.emit('    %r = %n(%,)', childvalues :flat, 'r'=>$P0, 'n'=>name)
    .return (code)
.end


.namespace [ 'Cardinal::POST::Label' ]

=item C<Cardinal::POST::Label::value()>

Returns the value for this label.  If one hasn't already been
set, a new unique label is generated from the invocant's name
and that is returned.

=cut

.sub 'value' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    value = self.'attr'('value', value, has_value)
    if value > '' goto value_end
    .local pmc name
    name = self.'name'()
    $S0 = self.'unique'(name)
    assign value, $S0
  value_end:
    .return (value)
.end


.sub 'pir' :method
    .param pmc block
    .local string code
    .local string value
    value = self.'value'()
    code = '  '
    code .= value
    code .= ":\n"
    .return (code)
.end


.namespace [ 'Cardinal::POST::Sub' ]
.gen_accessor('outer')
.gen_accessor('subtype')
.gen_accessor('varhash')
.gen_accessor('prologue')

.macro gen_adverb_accessor(x)
.sub .x :method
    .param pmc value
    .return self.'add_to_adverbs'(.x)
.end
.endm


.sub 'add_to_adverbs' :method
    .param pmc adverb
    $P0 = self.'adverbs'()
    $I0 = defined $P0
    if $I0 goto exists
    $P0 = new .String
    goto append
  exists:
    $P0 .= " "
  append:
    $P0 .= adverb
.end

.gen_adverb_accessor(':load')
.gen_adverb_accessor(':main')
.gen_adverb_accessor(':method')

.sub 'adverbs' :method
    .param pmc subtype         :optional
    .param int has_subtype     :opt_flag
    .return self.'attr'('adverbs', subtype, has_subtype)
.end

.sub 'subs' :method
    $P0 = self.'push_sub'()
    .return ($P0)
.end

.sub 'append_subs' :method
    .param pmc arg
    $P0 = self.'push_sub'()
    $P0.'append'(arg)
    .return ()
.end

.sub 'root_pir' :method
    ##   build the pir for this node and its children
    ($P0, $P1) = self.'pir'(self)
    .return ($P0)
.end

.sub 'pir_regex' :method
    .local pmc p6regex, regexast, regexpir
    .local string name, value
    name = self.'name'()
    value = self.'value'()
    p6regex = compreg 'PGE::P6Regex'
    regexast = self[0]
    regexpir = p6regex(regexast, 'name'=>name, 'grammar'=>'', 'target'=>'PIR')

    .local pmc code
    code = new 'PGE::CodeString'
    code.'emit'("    %0 = find_name '%1'", value, name)
    .return (regexpir, code)
.end

.sub 'push_sub'     :method
    .param pmc value      :optional
    .param int has_value  :opt_flag
    .local pmc stack 
    stack = self.'attr'('subs', 0, 0)
    $I0 = defined stack
    if $I0 goto test_value
    stack = new .ResizablePMCArray
    stack = self.'attr'('subs', stack, 1)
  test_value:
    unless has_value goto end
    push stack, value
  end:
    .return (stack)
.end

.sub '__dumplist' :method
    .return ('name subtype outer value children')
.end

.namespace [ 'Cardinal::POST::Val' ]
.gen_accessor('value')
.gen_accessor('valtype')
.gen_dumplist('name value valtype')


.namespace [ 'Cardinal::POST::Var' ]
.gen_accessor('scope')
.gen_accessor('islvalue')
.gen_dumplist('name scope value')

.sub 'paramname' :method
    .local string name
    name = self.'name'()
    name = clone name
    $I0 = 0
  scalar_loop:
    $I0 = index name, '$', $I0
    if $I0 < 0 goto scalar_end
    substr name, $I0, 1, 'dollar_'
    goto scalar_loop
  scalar_end:
    .return (name)
.end

.sub 'pir' :method
    .param pmc block
    .local string name, scope, value
    .local pmc code
    .local int islvalue
    name = self.'name'()
    scope = self.'scope'()
    value = self.'value'()
    islvalue = self.'islvalue'()
    code = new 'PGE::CodeString'
    if scope == 'lexical' goto generate_lexical
    if scope == 'instance' goto generate_instance
    if scope == 'class' goto generate_class
    if scope == 'parameter' goto generate_parameter
    if scope == 'global' goto generate_global
    goto generate_find
  generate_parameter:
    .local pmc varhash
    varhash = block.'varhash'()
    ##   if we already generated the code for this
    ##   variable, we generate nothing here.
    $I0 = exists varhash[name]
    if $I0 goto generate_find
    .local pmc prologue
    .local string pname
    pname = self.'paramname'()
    prologue = block.'prologue'()
    prologue.'emit'("    .param pmc %0", pname)
    code.'emit'("    .lex '%0', %1", name, value)
    code.'emit'("    %0 = %1", value, pname)
    varhash[name] = self
    goto end
  generate_lexical:
    .local pmc varhash
    varhash = block.'varhash'()
    $I0 = exists varhash[name]
    if $I0 goto generate_find
    ##    This is the first time to see a lexical, generate its .lex
    code.'emit'("    .lex '%0', %1", name, value)
    varhash[name] = self
    goto end
  generate_global:
    if islvalue goto end
    code.'emit'("    %0 = get_hll_global '%1'", value, name)
    goto end
  generate_instance:
    if islvalue goto end
    code.'emit'("    %0 = get_hll_global '%1'", value, name)
    goto end
  generate_class:
    if islvalue goto end
    code.'emit'("    %0 = get_hll_global '%1'", value, name)
    goto end
  generate_find:
    if islvalue goto end
    code.'emit'("    %0 = find_name '%1'", value, name)
  end:
    .return (code)
.end


.sub 'assignpir' :method
    .param pmc block
    .param pmc x
    .local pmc name, value, scope, xvalue, code, varhash
    name = self.'name'()
    value = self.'value'()
    scope = self.'scope'()
    xvalue = x.'value'()
    code = new 'PGE::CodeString'
    varhash = block.'varhash'()
    $I0 = exists varhash[name]
    if $I0 goto with_varhash_name
    varhash[name] = self
  with_varhash_name:
    if scope == 'instance' goto store_instance
    if scope == 'class' goto store_class
    if scope == 'global' goto store_global
  store_lexical:
    code.'emit'("    store_lex '%0', %1", name, xvalue)
    .return (code)
  store_instance:
    code.'emit'("    set_hll_global '%0', %1", name, xvalue)
    .return (code)
  store_class:
    code.'emit'("    set_hll_global '%0', %1", name, xvalue)
    .return (code)
  store_global:
    code.'emit'("    set_hll_global '%0', %1", name, xvalue)
    .return (code)
.end
    

.namespace [ 'Cardinal::POST::Assign' ]
.sub 'value' :method
    ##   return the value of our left hand side as our value
    $P0 = self[0]
    $P0 = $P0.'value'()
    .return ($P0)
.end

.namespace [ 'Cardinal::POST::Call' ]
.sub 'value' :method
    .return ("")
.end

.namespace [ 'Cardinal::POST::Raw' ]
.gen_accessor('raw')

.namespace [ 'Cardinal::POST::Namespace' ]

.sub 'get_pir_name' :method
    .local pmc name
    .local pmc iter
    .local string code
    name = self.'name'()
    iter = new Iterator, name
    .local int second_flag 
    second_flag = 0
  iter_loop1:
    unless iter goto iter_end1
    $P0 = shift iter
    $S0 = $P0 
    unless second_flag goto first
    code .= "; "
    goto emit
  first:
    second_flag = 1
  emit:
#    code.'emit'("'%0'; ", $S0)
    code .= "'"
    code .= $S0
    code .= "' "
    goto iter_loop1
  iter_end1:
  .return (code)
.end

.namespace [ 'Cardinal::POST::NamespaceLookup' ]
.sub 'pir' :method
    .param pmc block
    .local pmc item
    .local pmc name
    .local pmc value
    item = self.'item'()
    name = self.'name'()
    value = self.'value'()

    .local pmc code
    code = new 'PGE::CodeString'
    $S0 = "'"
    $S1 = item
    $S0 .= $S1
    $S0 .= "'"
    #code.'emit'('    %r = get_hll_namespace [ %n ; %i ]', 'r'=>value, 'n'=>name, 'i'=>$S0)
    code.'emit'('    %r = get_hll_namespace [ %n]', 'r'=>value, 'n'=>name)
    code.'emit'('    %r = %r[ %i ]', 'r'=>value, 'i'=>$S0)
    .return (code)
.end

.gen_accessor('item')
.gen_dumplist('name value item')

.sub 'namespace' :method
    .param pmc namespace
    $S0 = namespace.'get_pir_name'()
    self.'name'($S0)
    .return ()
.end


# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
