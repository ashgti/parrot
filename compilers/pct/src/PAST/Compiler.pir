=head1 NAME

PAST::Compiler - PAST Compiler

=head1 DESCRIPTION

PAST::Compiler implements a basic compiler for PAST nodes.
By default PAST::Compiler transforms a PAST tree into POST.

=head2 Signature Flags

Throughout the compiler PAST uses a number of 1-character
"flags" to indicate allowable register types and conversions.
This helps the compiler generate more efficient code and know
what sorts of conversions are allowed (or desired).  The
basic flags are:

    P,S,I,N   PMC, string, int, or num register
    s         string register or constant
    i         int register or constant
    n         num register or constant
    r         any register result
    v         void (no result)
    *         any result type except void
    +         PMC, int register, num register, or numeric constant
    ~         PMC, string register, or string constant
    :         argument (same as '*'), possibly with :named or :flat

These flags are used to describe signatures and desired return
types for various operations.  For example, if an opcode is
specified with a signature of C<I~P*>, then the opcode places
its result in an int register, its first child is coerced into
some sort of string value, its second child is coerced into a
PMC register, and the third and subsequent children can return
any value type.

=cut

.include "cclass.pasm"
.include "except_types.pasm"

.namespace [ 'PAST::Compiler' ]

.sub 'onload' :anon :load :init
    load_bytecode 'PCT/HLLCompiler.pbc'
    .local pmc p6meta, cproto
    p6meta = new 'P6metaclass'
    cproto = p6meta.'new_class'('PAST::Compiler', 'parent'=>'PCT::HLLCompiler', 'attr'=>'%!symtable')
    cproto.'language'('PAST')
    $P1 = split ' ', 'post pir evalpmc'
    cproto.'stages'($P1)

    ##  %piropsig is a table of common opcode signatures
    .local pmc piropsig
    piropsig = new 'Hash'
    piropsig['isa']      = 'IP~'
    piropsig['isfalse']  = 'IP'
    piropsig['issame']   = 'IPP'
    piropsig['istrue']   = 'IP'
    piropsig['n_abs']    = 'PP'
    piropsig['n_add']    = 'PP+'
    piropsig['n_band']   = 'PPP'
    piropsig['n_bnot']   = 'PP'
    piropsig['n_bor']    = 'PPP'
    piropsig['n_concat'] = 'PP~'
    piropsig['n_div']    = 'PP+'
    piropsig['n_fdiv']   = 'PP+'
    piropsig['n_mod']    = 'PP+'
    piropsig['n_mul']    = 'PP+'
    piropsig['n_neg']    = 'PP'
    piropsig['n_not']    = 'PP'
    piropsig['n_shl']    = 'PP+'
    piropsig['n_shr']    = 'PP+'
    piropsig['n_sub']    = 'PP+'
    piropsig['pow']      = 'NN+'
    piropsig['print']    = 'v*'
    piropsig['set']      = 'PP'
    set_global '%piropsig', piropsig

    ##  %valflags specifies when PAST::Val nodes are allowed to
    ##  be used as a constant.  The 'e' flag indicates that the
    ##  value must be quoted+escaped in PIR code.
    .local pmc valflags
    valflags = new 'Hash'
    valflags['String']   = 's~*:e'
    valflags['Integer']  = 'i+*:'
    valflags['Float']    = 'n+*:'
    set_global '%valflags', valflags

    $P0 = new 'CodeString'
    set_global '%!codestring', $P0

    $P0 = new 'Integer'
    $P0 = 11
    set_global '$!serno', $P0

    .return ()
.end

=head2 Compiler methods

=over 4

=item to_post(node [, 'option'=>option, ...])

Compile the abstract syntax tree given by C<past> into POST.

=cut

.sub 'to_post' :method
    .param pmc past
    .param pmc options         :slurpy :named

    .local pmc symtable
    symtable = new 'Hash'
    setattribute self, '%!symtable', symtable

    .local pmc blockpast
    blockpast = get_global '@?BLOCK'
    unless null blockpast goto have_blockpast
    blockpast = new 'ResizablePMCArray'
    set_global '@?BLOCK', blockpast
  have_blockpast:
    null $P0
    set_global '$?SUB', $P0                                # see RT#49758
    .return self.'as_post'(past, 'rtype'=>'v')
.end

=item escape(str)

Return C<str> as a PIR constant string.

=cut

.sub 'escape' :method
    .param string str
    $P0 = get_global '%!codestring'
    str = $P0.'escape'(str)
    .return (str)
.end

=item unique([STR fmt])

Generate a unique number that can be used as an identifier.
If C<fmt> is provided, then it will be used as a prefix to the
unique number.

=cut

.sub 'unique' :method
    .param string fmt          :optional
    .param int has_fmt         :opt_flag

    if has_fmt goto unique_1
    fmt = ''
  unique_1:
    $P0 = get_global '$!serno'
    $S0 = $P0
    $S0 = concat fmt, $S0
    inc $P0
    .return ($S0)
.end

=item uniquereg(rtype)

Generate a unique register based on C<rtype>, where C<rtype>
is one of the signature flags described above.

=cut

.sub 'uniquereg' :method
    .param string rtype
    unless rtype goto err_nortype
    if rtype == 'v' goto reg_void
    .local string reg
    reg = 'P'
    $I0 = index 'Ss~Nn+Ii', rtype
    if $I0 < 0 goto reg_psin
    reg = substr 'SSSNNNII', $I0, 1
  reg_psin:
    reg = concat '$', reg
    .return self.'unique'(reg)
  reg_void:
    .return ('')
  err_nortype:
    self.panic('rtype not set')
.end

=item coerce(post, rtype)

Return a POST tree that coerces the result of C<post> to have a
return value compatible with C<rtype>.  C<rtype> can also be
a specific register, in which case the result of C<post> is
forced into that register (with conversions as needed).

=cut

.sub 'coerce' :method
    .param pmc post
    .param string rtype

    unless rtype goto err_nortype

    .local string pmctype, result, rrtype
    null pmctype
    null result

    ##  if rtype is a register, then set result and use the register
    ##  type as rtype
    $S0 = substr rtype, 0, 1
    unless $S0 == '$' goto have_rtype
    result = rtype
    rtype = substr result, 1, 1
  have_rtype:

    ##  these rtypes allow any return value, so no coercion needed.
    $I0 = index 'v*:', rtype
    if $I0 >= 0 goto end

    ##  figure out what type of result we already have
    .local string source
    source = post.'result'()
    $S0 = substr source, 0, 1
    if $S0 == '$' goto source_reg
    if $S0 == '"' goto source_str
    if $S0 == '.' goto source_int_or_num
    if $S0 == '-' goto source_int_or_num
    $I0 = is_cclass .CCLASS_NUMERIC, source, 0
    if $I0 goto source_int_or_num
    $S0 = substr source, 0, 8
    if $S0 == 'unicode:' goto source_str
    ##  assume that whatever is left acts like a PMC
    goto source_pmc

  source_reg:
    ##  source is some sort of register
    ##  if a register is all we need, we're done
    if rtype == 'r' goto end
    $S0 = substr source, 1, 1
    ##  if we have the correct register type already, we're done
    if $S0 != rtype goto source_reg_1
    unless result goto end
    goto coerce_reg
  source_reg_1:
    $S0 = downcase $S0
    if $S0 == rtype goto end
    ##  figure it out based on the register type
    if $S0 == 's' goto source_str
    if rtype == '+' goto end
    if $S0 == 'i' goto source_int
    if $S0 == 'n' goto source_num
  source_pmc:
    $I0 = index 'SINsin', rtype
    if $I0 < 0 goto end
    goto coerce_reg

  source_str:
    if rtype == '~' goto end
    if rtype == 's' goto end
    rrtype = 'S'
    pmctype = "'String'"
    goto coerce_reg

  source_int_or_num:
    if rtype == '+' goto end
    ##  existence of an 'e' or '.' implies num
    $I0 = index source, '.'
    if $I0 >= 0 goto source_num
    $I0 = index source, 'E'
    if $I0 >= 0 goto source_num

  source_int:
    if rtype == 'i' goto end
    rrtype = 'I'
    pmctype = "'Integer'"
    goto coerce_reg

  source_num:
    if rtype == 'n' goto end
    rrtype = 'N'
    pmctype = "'Float'"

  coerce_reg:
    ##  okay, we know we have to do a coercion.
    ##  If we just need the value in a register (rtype == 'r'),
    ##  then create result based on the preferred register type (rrtype).
    if rtype != 'r' goto coerce_reg_1
    result = self.'uniquereg'(rrtype)
  coerce_reg_1:
    ##  if we haven't set the result target yet, then generate one
    ##  based on rtype.  (The case of rtype == 'r' was handled above.)
    if result goto coerce_reg_2
    result = self.'uniquereg'(rtype)
  coerce_reg_2:
    ##  create a new ops node to hold the coercion, put C<post> in it.
    $P0 = get_hll_global ['POST'], 'Ops'
    post = $P0.'new'(post, 'result'=>result)
    ##  if we need a new pmc (rtype == 'P' && pmctype defined), create it
    if rtype != 'P' goto have_result
    unless pmctype goto have_result
    post.'push_pirop'('new', result, pmctype)
  have_result:
    ##  store the value into the target register
    post.'push_pirop'('set', result, source)

  end:
    .return (post)

  err_nortype:
    self.panic('rtype not set')
.end


=item post_children(node [, 'signature'=>signature] )

Return the POST representation of evaluating all of C<node>'s
children in sequence.  The C<signature> option is a string of
flags as described in "Signature Flags" above.  Since we're
just evaluating children nodes, the first character of
C<signature> (return value type) is ignored.  Thus a C<signature>
of C<v~P*> says that the first child needs to be something
in string context, the second child should be a PMC, and the
third and subsequent children can be any value they wish.

=cut

.sub 'post_children' :method
    .param pmc node
    .param pmc options         :slurpy :named

    .local pmc ops
    $P0 = get_hll_global ['POST'], 'Ops'
    ops = $P0.'new'('node'=>node)

    ##  get any conversion types
    .local string signature
    signature = options['signature']
    if signature goto have_signature
    signature = '**'
  have_signature:
    .local int sigmax, sigidx
    sigmax = length signature
    dec sigmax

    ##  if the signature contains a ':', then we're doing
    ##  flagged arguments (:flat, :named)
    .local pmc posargs, namedargs
    posargs = new 'ResizableStringArray'
    null namedargs
    $I0 = index signature, ':'
    if $I0 < 0 goto nocolon
    namedargs = new 'ResizableStringArray'
  nocolon:

    .local pmc iter
    .local string rtype
    .local int sigidx
    iter = node.'iterator'()
    sigidx = 1
    rtype = substr signature, sigidx, 1
  iter_loop:
    unless iter goto iter_end
    .local pmc cpast, cpost
    cpast = shift iter
    cpost = self.'as_post'(cpast, 'rtype'=>rtype)
    cpost = self.'coerce'(cpost, rtype)
    ops.'push'(cpost)
    .local pmc isflat
    isflat = cpast.'flat'()
    if rtype != ':' goto iter_pos
    .local pmc npast, npost
    npast = cpast.'named'()
    unless npast goto iter_pos
    $S0 = cpost
    if isflat goto flat_named
    npost = self.'as_post'(npast, 'rtype'=>'~')
    $S1 = npost
    ops.'push'(npost)
    concat $S0, ' :named('
    concat $S0, $S1
    concat $S0, ')'
    goto named_done
  flat_named:
    concat $S0, ' :named :flat'
  named_done:
    push namedargs, $S0
    goto iter_rtype
  iter_pos:
    if isflat goto flat_pos
    push posargs, cpost
    goto iter_rtype
  flat_pos:
    $S0 = cpost
    concat $S0, ' :flat'
    push posargs, $S0
  iter_rtype:
    unless sigidx < sigmax goto iter_loop
    inc sigidx
    rtype = substr signature, sigidx, 1
    goto iter_loop
  iter_end:
    .return (ops, posargs, namedargs)
.end

=back

=head2 Methods on C<PAST::Node> arguments

The methods below are used to transform PAST nodes into their
POST equivalents.

=head3 Defaults

=over 4

=item as_post(node) (General)

Return a POST representation of C<node>.  Note that C<post> is
a multimethod based on the type of its first argument, this is
the method that is called when no other methods match.

=item as_post(Any)

This is the "fallback" method for any unrecognized node types.
We use this to throw a more useful exception in case any non-PAST
nodes make it into the tree.

=cut

.sub 'as_post' :method :multi(_, _)
    .param pmc node
    .param pmc options         :slurpy :named
    $S0 = typeof node
    self.panic("PAST::Compiler can't compile node of type ", $S0)
.end

=item as_post(Undef)

Return an empty POST node that can be used to hold a (PMC) result.

=cut

.sub 'as_post' :method :multi(_, Undef)
    .param pmc node
    .param pmc options         :slurpy :named
    .local string result
    $P0 = get_hll_global ['POST'], 'Ops'
    result = self.'uniquereg'('P')
    .return $P0.'new'('result'=>result)
.end

=item as_post(String class)

Generate POST to create a new object of type C<class>.  This
is typically invoked by the various vivification methods below
(e.g., in a PAST::Var node to default a variable to a given type).

=cut

.sub 'as_post' :method :multi(_, String)
    .param pmc node
    .param pmc options         :slurpy :named

    .local string result
    $P0 = get_hll_global ['POST'], 'Op'
    result = self.'uniquereg'('P')
    $S0 = self.'escape'(node)
    .return $P0.'new'(result, $S0, 'pirop'=>'new', 'result'=>result)
.end

=item as_post(PAST::Node node)

Return the POST representation of executing C<node>'s children in
sequence.  The result of the final child is used as the result
of this node.

N.B.:  This method is also the one that is invoked for converting
nodes of type C<PAST::Stmts>.

=cut

.sub 'as_post' :method :multi(_, PAST::Node)
    .param pmc node
    .param pmc options         :slurpy :named

    .local pmc ops
    .local string rtype
    rtype = options['rtype']
    $P0 = node.'list'()
    $I0 = elements $P0
    $S0 = repeat 'v', $I0
    concat $S0, rtype
    ops = self.'post_children'(node, 'signature'=>$S0)
    $P0 = ops[-1]
    ops.'result'($P0)
    .return (ops)
.end

=back

=head3 C<PAST::Block>

=over 4

=item as_post(PAST::Block node)

Return the POST representation of a C<PAST::Block>.

=cut

.sub 'as_post' :method :multi(_, ['PAST::Block'])
    .param pmc node
    .param pmc options         :slurpy :named

    ##  add current block node to @?BLOCK
    .local pmc blockpast
    blockpast = get_global '@?BLOCK'
    unshift blockpast, node

    .local string name
    name = node.'name'()
    if name goto have_name
    name = self.'unique'('_block')
  have_name:

    ##  create a POST::Sub node for this block
    .local string blocktype
    blocktype = node.'blocktype'()
    .local pmc ns, pirflags
    ns = node.'namespace'()
    pirflags = node.'pirflags'()
    .local pmc bpost
    $P0 = get_hll_global ['POST'], 'Sub'
    bpost = $P0.'new'('node'=>node, 'name'=>name, 'blocktype'=>blocktype, 'namespace'=>ns, 'pirflags'=>pirflags)

    ##  determine the outer POST::Sub for the new one
    .local pmc outerpost
    outerpost = get_global '$?SUB'
    $P0 = node.'lexical'()
    if $P0 goto outer_block
    null $P0
    set_global '$?SUB', $P0
    goto outer_done
  outer_block:
    bpost.'outer'(outerpost)
    set_global '$?SUB', bpost
  outer_done:

    ##  merge the node's symtable with the master
    .local pmc outersym, symtable
    outersym = getattribute self, '%!symtable'
    symtable = outersym
    ##  if the Block doesn't have a symtable, re-use the existing one
    $P0 = node.'symtable'()
    unless $P0 goto have_symtable
    ##  if the Block has a default ('') entry, use the Block's symtable as-is
    symtable = $P0
    $I0 = defined symtable['']
    if $I0 goto have_symtable
    ##  merge the Block's symtable with outersym
    symtable = clone symtable
  symtable_merge:
    .local pmc iter
    iter = new 'Iterator', outersym
  symtable_merge_loop:
    unless iter goto have_symtable
    $S0 = shift iter
    $I0 = exists symtable[$S0]
    if $I0 goto symtable_merge_loop
    $P0 = iter[$S0]
    symtable[$S0] = $P0
    goto symtable_merge_loop
  have_symtable:
    setattribute self, '%!symtable', symtable

    .local pmc compiler
    compiler = node.'compiler'()
    if compiler goto children_compiler

    ##  control exception handler
    .local pmc ctrlpast, ctrllabel, rethrowlabel
    ctrlpast = node.'control'()
    unless ctrlpast goto children_past
    $P0 = get_hll_global ['POST'], 'Label'
    $S0 = self.'unique'('control_')
    ctrllabel = $P0.'new'('result'=>$S0)
    $S0 = concat $S0, '_rethrow'
    rethrowlabel = $P0.'new'('result'=>$S0)
    bpost.'push_pirop'('push_eh', ctrllabel)

  children_past:
    ##  all children but last are void context, last returns anything
    $P0 = node.'list'()
    $I0 = elements $P0
    $S0 = repeat 'v', $I0
    concat $S0, '*'
    ##  convert children to post
    .local pmc ops
    ops = self.'post_children'(node, 'signature'=>$S0)
    bpost.'push'(ops)
    ##  result of last child is return from block
    $P0 = ops[-1]
    bpost.'push_pirop'('return', $P0)

    unless ctrlpast goto sub_done
    bpost.'push'(ctrllabel)
    bpost.'push_pirop'('.local pmc exception')
    bpost.'push_pirop'('.get_results (exception, $S10)')
    $I0 = isa ctrlpast, 'PAST::Node'
    if $I0 goto control_past
    if ctrlpast == 'return_pir' goto control_return
    self.panic("Unrecognized control handler '", ctrlpast, "'")
  control_return:
    ##  handle 'return' exceptions
    $S0 = self.'uniquereg'('P')
    bpost.'push_pirop'('getattribute', $S0, 'exception', '"type"')
    bpost.'push_pirop'('if_null', $S0, rethrowlabel)
    bpost.'push_pirop'('ne', $S0, .CONTROL_RETURN, rethrowlabel)
    bpost.'push_pirop'('getattribute', $S0, 'exception', '"payload"')
    bpost.'push_pirop'('return', $S0)
    bpost.'push'(rethrowlabel)
    bpost.'push_pirop'('throw', 'exception')
    goto sub_done
  control_past:
    $P0 = self.'as_post'(ctrlpast, 'rtype'=>'*')
    bpost.'push'($P0)
    goto sub_done

  children_compiler:
    ##  set the compiler to use for the POST::Sub node, pass on
    ##  and compiler arguments and add this block's child to it.
    bpost.'compiler'(compiler)
    $P0 = node.'compiler_args'()
    bpost.'compiler_args'($P0)
    $P0 = node[0]
    bpost.'push'($P0)

  sub_done:
    ##  restore previous outer scope and symtable
    set_global '$?SUB', outerpost
    setattribute self, '%!symtable', outersym

    ##  determine name and namespace
    name = self.'escape'(name)
    $I0 = defined ns
    unless $I0 goto have_ns_key
    $P0 = get_global '%!codestring'
    ns = $P0.'key'(ns)
  have_ns_key:

    .local string rtype, result
    rtype = options['rtype']

    if blocktype == 'immediate' goto block_immediate
    if rtype == 'v' goto block_done
    result = self.'uniquereg'('P')
    $P0 = get_hll_global ['POST'], 'Ops'
    bpost = $P0.'new'(bpost, 'node'=>node, 'result'=>result)
    if ns goto block_decl_ns
    bpost.'push_pirop'('get_global', result, name)
    goto block_done
  block_decl_ns:
    bpost.'push_pirop'('get_hll_global', result, ns, name)
    goto block_done

  block_immediate:
    result = self.'uniquereg'(rtype)
    $P0 = get_hll_global ['POST'], 'Ops'
    bpost = $P0.'new'(bpost, 'node'=>node, 'result'=>result)
    if ns goto block_immediate_ns
    bpost.'push_pirop'('call', name, 'result'=>result)
    goto block_done
  block_immediate_ns:
    $S0 = '$P10'
    bpost.'push_pirop'('get_hll_global', $S0, ns, name, 'result'=>$S0)
    bpost.'push_pirop'('call', $S0, 'result'=>result)

  block_done:
    ##  remove current block from @?BLOCK
    $P99 = shift blockpast
    .return (bpost)
.end


=back

=head3 C<PAST::Op>

=over 4

=item as_post(PAST::Op node)

Return the POST representation of a C<PAST::Op> node.  Normally
this is handled by redispatching to a method corresponding to
the node's "pasttype" attribute.

=cut

.sub 'as_post' :method :multi(_, ['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named

    ##  see if we set first child's lvalue
    $I0 = node.'lvalue'()
    unless $I0 goto have_lvalue
    $P0 = node[0]
    if null $P0 goto have_lvalue
    $P0.'lvalue'($I0)
  have_lvalue:

    .local string pasttype
    pasttype = node.'pasttype'()
    unless pasttype goto post_pirop
    $P0 = find_method self, pasttype
    .return self.$P0(node, options :flat :named)

  post_pirop:
    .local pmc pirop
    pirop = node.'pirop'()
    unless pirop goto post_inline
    .return self.'pirop'(node, options :flat :named)

  post_inline:
    .local pmc inline
    inline = node.'inline'()
    unless inline goto post_call
    .return self.'inline'(node, options :flat :named)

  post_call:
    .return self.'call'(node, options :flat :named)
.end


=item pirop(PAST::Op node)

Return the POST representation of a C<PAST::Op> node with
a 'pasttype' of 'pirop'.

=cut

.sub 'pirop' :method :multi(_, ['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named

    .local string pirop, signature
    pirop = node.'pirop'()
    ##  see if pirop is of form "pirop signature"
    $I0 = index pirop, ' '
    if $I0 < 0 goto pirop_1
    $I1 = $I0 + 1
    signature = substr pirop, $I1
    pirop = substr pirop, 0, $I0
    goto have_signature
  pirop_1:
    $P0 = get_global '%piropsig'
    signature = $P0[pirop]
    if signature goto have_signature
    signature = 'vP'
  have_signature:

    .local pmc ops
    ops = self.'post_children'(node, 'signature'=>signature)

    .local pmc arglist
    arglist = ops.'list'()

    $S0 = substr signature, 0, 1
    if $S0 == 'v' goto pirop_void
  pirop_reg:
    .local string result
    result = self.'uniquereg'($S0)
    ops.'result'(result)
    ops.'push_pirop'(pirop, result, arglist :flat)
    .return (ops)
  pirop_void:
    ops.'push_pirop'(pirop, arglist :flat)
    .return (ops)
.end


=item call(PAST::Op node)

Return the POST representation of a C<PAST::Op> node
for calling a sub.

=cut

.sub 'call' :method :multi(_, ['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named
    .local string pasttype
    pasttype = node.'pasttype'()
    if pasttype goto have_pasttype
    pasttype = 'call'
  have_pasttype:

    .local string signature
    signature = 'v:'
    ## for callmethod, the invocant (child) must be a PMC
    if pasttype != 'callmethod' goto have_signature
    signature = 'vP:'
  have_signature:

    .local pmc name, ops, posargs, namedargs
    name = node.'name'()
    if name goto call_by_name
    ##  our first child is the thing to be invoked, so make sure it's a PMC
    substr signature, 1, 0, 'P'
    (ops, posargs, namedargs) = self.'post_children'(node, 'signature'=>signature)
    goto children_done
  call_by_name:
    (ops, posargs, namedargs) = self.'post_children'(node, 'signature'=>signature)
    $I0 = isa name, 'PAST::Node'
    if $I0 goto call_by_name_past
    $S0 = self.'escape'(name)
    unshift posargs, $S0
    goto children_done
  call_by_name_past:
    .local pmc name_post
    name_post = self.'as_post'(name, 'rtype'=>'s')
    name_post = self.'coerce'(name_post, 's')
    ops.'push'(name_post)
    unshift posargs, name_post
  children_done:

    ##  generate the call itself
    .local string result, rtype
    rtype = options['rtype']
    result = self.'uniquereg'(rtype)
    ops.'push_pirop'(pasttype, posargs :flat, namedargs :flat, 'result'=>result)
    ops.'result'(result)
    .return (ops)
.end


=item callmethod(PAST::Op node)

Return the POST representation of a C<PAST::Op> node
to invoke a method on a PMC.

=cut

.sub 'callmethod' :method :multi(_, ['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named
    .return self.'call'(node, options :flat :named)
.end


=item if(PAST::Op node)

=item unless(PAST::Op node)

Return the POST representation of C<PAST::Op> nodes with
a 'pasttype' of if/unless.

=cut

.sub 'if' :method :multi(_,['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named

    .local pmc opsclass, ops
    opsclass = get_hll_global ['POST'], 'Ops'
    ops = opsclass.'new'('node'=>node)

    .local string rtype, result
    rtype = options['rtype']
    result = self.'uniquereg'(rtype)
    ops.'result'(result)

    .local string pasttype
    pasttype = node.'pasttype'()

    .local pmc exprpast, thenpast, elsepast, childpast
    .local pmc exprpost, thenpost, elsepost, childpost
    exprpast = node[0]
    thenpast = node[1]
    elsepast = node[2]

    .local pmc thenlabel, endlabel
    $P0 = get_hll_global ['POST'], 'Label'
    $S0 = concat pasttype, '_'
    $S0 = self.'unique'($S0)
    thenlabel = $P0.'new'('result'=>$S0)
    $S0 = concat $S0, '_end'
    endlabel = $P0.'new'('result'=>$S0)

    .local string exprrtype, childrtype
    exprrtype = 'r'
    if rtype != 'v' goto have_exprrtype
    exprrtype = '*'
  have_exprrtype:
    childrtype = rtype
    $I0 = index '*:', rtype
    if $I0 < 0 goto have_childrtype
    childrtype = 'P'
  have_childrtype:

    exprpost = self.'as_post'(exprpast, 'rtype'=>exprrtype)

    childpast = thenpast
    bsr make_childpost
    thenpost = childpost
    childpast = elsepast
    bsr make_childpost
    elsepost = childpost

    if null elsepost goto no_elsepost

    ops.'push'(exprpost)
    ops.'push_pirop'(pasttype, exprpost, thenlabel)
    if null elsepost goto else_done
    ops.'push'(elsepost)
  else_done:
    ops.'push_pirop'('goto', endlabel)
    ops.'push'(thenlabel)
    if null thenpost goto then_done
    ops.'push'(thenpost)
  then_done:
    ops.'push'(endlabel)
    .return (ops)

  no_elsepost:
    $S0 = 'if'
    unless pasttype == $S0 goto no_elsepost_1
    $S0 = 'unless'
  no_elsepost_1:
    ops.'push'(exprpost)
    ops.'push_pirop'($S0, exprpost, endlabel)
    if null thenpost goto no_elsepost_2
    ops.'push'(thenpost)
  no_elsepost_2:
    ops.'push'(endlabel)
    .return (ops)

  make_childpost:
    null childpost
    $I0 = defined childpast
    unless $I0 goto no_childpast
    childpost = self.'as_post'(childpast, 'rtype'=>childrtype)
    goto childpost_coerce
  no_childpast:
    if rtype == 'v' goto ret_childpost
    childpost = opsclass.'new'('result'=>exprpost)
  childpost_coerce:
    unless result goto ret_childpost
    childpost = self.'coerce'(childpost, result)
  ret_childpost:
    ret
.end

.sub 'unless' :method :multi(_, ['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named
    .return self.'if'(node, options :flat :named)
.end


=item while(PAST::Op node)

=item until(PAST::Op node)

Return the POST representation of a C<while> or C<until> loop.

=cut

.sub 'while' :method :multi(_, ['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named

    .local string pasttype
    pasttype = node.'pasttype'()

    .local pmc ops
    $P0 = get_hll_global ['POST'], 'Ops'
    ops = $P0.'new'('node'=>node)

    .local pmc exprpast, exprpost
    .local pmc bodypast, bodypost
    exprpast = node[0]
    bodypast = node[1]

    .local pmc looplabel, endlabel
    $P0 = get_hll_global ['POST'], 'Label'
    $S0 = concat pasttype, '_'
    $S0 = self.'unique'($S0)
    looplabel = $P0.'new'('result'=>$S0)
    $S0 = concat $S0, '_end'
    endlabel = $P0.'new'('result'=>$S0)

    ##  determine if we need an 'if' or an 'unless'
    ##  on the conditional (while => if, until => unless)
    .local string iftype
    iftype = 'if'
    if pasttype == 'until' goto have_iftype
    iftype = 'unless'
  have_iftype:

    .local string rtype, exprrtype
    rtype = options['rtype']
    exprrtype = 'r'
    if rtype != 'v' goto have_exprrtype
    exprrtype = '*'
  have_exprrtype:

    ops.'push'(looplabel)
    exprpost = self.'as_post'(exprpast, 'rtype'=>exprrtype)
    ops.'push'(exprpost)
    ops.'push_pirop'(iftype, exprpost, endlabel)
    bodypost = self.'as_post'(bodypast, 'rtype'=>'v')
    ops.'push'(bodypost)
    ops.'push_pirop'('goto', looplabel)
    ops.'push'(endlabel)
    ops.'result'(exprpost)
    .return (ops)
.end

.sub 'until' :method :multi(_, ['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named
    .return self.'while'(node, options :flat :named)
.end

=item repeat_while(PAST::Op node)

=item repeat_until(PAST::Op node)

Return the POST representation of a C<repeat_while> or C<repeat_until> loop.

=cut

.sub 'repeat_while' :method :multi(_, ['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named

    .local string pasttype
    pasttype = node.'pasttype'()

    .local pmc ops
    $P0 = get_hll_global ['POST'], 'Ops'
    ops = $P0.'new'('node'=>node)

    .local pmc exprpast, exprpost
    .local pmc bodypast, bodypost
    exprpast = node[0]
    bodypast = node[1]

    .local pmc looplabel
    $P0 = get_hll_global ['POST'], 'Label'
    $S0 = concat pasttype, '_'
    looplabel = $P0.'new'('name'=>$S0)

    ##  determine if we need an 'if' or an 'unless'
    ##  on the conditional (repeat_while => if, repeat_until => unless)
    .local string iftype
    iftype = 'if'
    if pasttype != 'repeat_until' goto have_iftype
    iftype = 'unless'
  have_iftype:

    .local string rtype, exprrtype
    rtype = options['rtype']
    exprrtype = 'r'
    if rtype != 'v' goto have_exprrtype
    exprrtype = '*'
  have_exprrtype:

    ops.'push'(looplabel)
    bodypost = self.'as_post'(bodypast, 'rtype'=>'v')
    ops.'push'(bodypost)
    exprpost = self.'as_post'(exprpast, 'rtype'=>exprrtype)
    ops.'push'(exprpost)
    ops.'push_pirop'(iftype, exprpost, looplabel)
    ops.'result'(exprpost)
    .return (ops)
.end

.sub 'repeat_until' :method :multi(_, ['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named
    .return self.'repeat_while'(node, options :flat :named)
.end


=item for(PAST::Op node)

Return the POST representation of the C<for> loop given
by C<node>.

=cut

.sub 'for' :method :multi(_, ['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named

    .local pmc ops
    $P0 = get_hll_global ['POST'], 'Ops'
    ops = $P0.'new'('node'=>node)

    .local pmc looplabel, endlabel
    $P0 = get_hll_global ['POST'], 'Label'
    $S0 = self.'unique'('for_')
    looplabel = $P0.'new'('result'=>$S0)
    $S0 = concat $S0, '_end'
    endlabel = $P0.'new'('result'=>$S0)

    .local pmc collpast, collpost
    collpast = node[0]
    collpost = self.'as_post'(collpast, 'rtype'=>'P')
    ops.'push'(collpost)

    .local string iter
    iter = self.'uniquereg'('P')
    ops.'result'(iter)
    $S0 = self.'uniquereg'('I')
    ops.'push_pirop'('defined', $S0, collpost)
    ops.'push_pirop'('unless', $S0, endlabel)
    ops.'push_pirop'('iter', iter, collpost)
    ops.'push'(looplabel)
    ops.'push_pirop'('unless', iter, endlabel)


    .local pmc subpast
    subpast = node[1]

    ##  determine the number of elements to take at each iteration
    .local int arity
    arity = 1
    $P0 = node.'arity'()
    $I0 = defined $P0
    unless $I0 goto arity_child
    arity = $P0
    goto have_arity
  arity_child:
    $P0 = subpast.'arity'()
    $I0 = defined $P0
    unless $I0 goto have_arity
    arity = $P0
  have_arity:

    .local pmc arglist
    arglist = new 'ResizablePMCArray'
  arity_loop:
    .local string nextval
    nextval = self.'uniquereg'('P')
    ops.'push_pirop'('shift', nextval, iter)
    if arity < 1 goto arity_end
    push arglist, nextval
    dec arity
    if arity > 0 goto arity_loop
  arity_end:

    .local pmc subpost
    subpost = self.'as_post'(subpast, 'rtype'=>'P')
    ops.'push'(subpost)
    ops.'push_pirop'('call', subpost, arglist :flat)
    ops.'push_pirop'('goto', looplabel)
    ops.'push'(endlabel)
    .return (ops)
.end


=item list(PAST::Op node)

Build a list from the children.  The type of list constructed
is determined by the C<returns> attribute, which defaults
to C<ResizablePMCArray> if not set.

=cut

.sub 'list' :method :multi(_, ['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named

    .local pmc ops, posargs
    (ops, posargs) = self.'post_children'(node, 'signature'=>'v*')

    .local pmc returns
    returns = node.'returns'()
    if returns goto have_returns
    returns = new 'String'
    returns = 'ResizablePMCArray'
  have_returns:

    .local pmc listpost, iter
    listpost = self.'as_post'(returns, 'rtype'=>'P')
    ops.'result'(listpost)
    ops.'push'(listpost)
    iter = new 'Iterator', posargs
  iter_loop:
    unless iter goto iter_end
    $S0 = shift iter
    ops.'push_pirop'('push', listpost, $S0)
    goto iter_loop
  iter_end:
    .return (ops)
.end


=item return(PAST::Op node)

Generate a return exception, using the first child (if any) as
a return value.

=cut

.sub 'return' :method :multi(_, ['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named

    .local pmc ops
    $P0 = get_hll_global ['POST'], 'Ops'
    ops = $P0.'new'('node'=>node)

    .local string exreg, extype
    exreg = self.'uniquereg'('P')
    extype = concat exreg, "['_type']"
    ops.'push_pirop'('new', exreg, '"Exception"')
    ops.'push_pirop'('set', extype, .CONTROL_RETURN)

    .local pmc cpast, cpost
    cpast = node[0]
    unless cpast goto cpast_done
    cpost = self.'as_post'(cpast, 'rtype'=>'P')
    cpost = self.'coerce'(cpost, 'P')
    ops.'push'(cpost)
    ops.'push_pirop'('setattribute', exreg, "'payload'", cpost)
  cpast_done:
    ops.'push_pirop'('throw', exreg)
    .return (ops)
.end


=item try(PAST::Op node)

Return the POST representation of a C<PAST::Op>
node with a 'pasttype' of bind.  The first child
is the code to be surrounded by an exception handler,
the second child (if any) is the code to process the
handler.

=cut

.sub 'try' :method :multi(_, ['PAST::Op'])
    .param pmc node
    .param pmc options       :slurpy :named

    .local pmc ops
    $P0 = get_hll_global ['POST'], 'Ops'
    ops = $P0.'new'('node'=>node)

    .local pmc catchlabel, endlabel
    $P0 = get_hll_global ['POST'], 'Label'
    $S0 = self.'unique'('catch_')
    catchlabel = $P0.'new'('result'=>$S0)
    $S0 = concat $S0, '_end'
    endlabel = $P0.'new'('result'=>$S0)

    .local string rtype
    rtype = options['rtype']

    .local pmc trypast, trypost
    trypast = node[0]
    trypost = self.'as_post'(trypast, 'rtype'=>rtype)
    ops.'push_pirop'('push_eh', catchlabel)
    ops.'push'(trypost)
    ops.'push_pirop'('pop_eh')
    .local pmc elsepast, elsepost
    elsepast = node[2]
    if null elsepast goto else_done
    elsepost = self.'as_post'(elsepast, 'rtype'=>'v')
    ops.'push'(elsepost)
  else_done:
    ops.'push_pirop'('goto', endlabel)
    ops.'push'(catchlabel)
    .local pmc catchpast, catchpost
    catchpast = node[1]
    if null catchpast goto catch_done
    catchpost = self.'as_post'(catchpast, 'rtype'=>'v')
    ops.'push'(catchpost)
  catch_done:
    ops.'push'(endlabel)
    ops.'result'(trypost)
    .return (ops)
.end


=item chain(PAST::Op node)

A short-circuiting chain of operations.  In a sequence of nodes
with pasttype 'chain', the right operand of a node serves as
the left operand of its parent.  Each node is evaluated only
once, and the first false result short-circuits the chain.
In other words,  C<<  $x < $y < $z >>  is true only if
$x < $y and $y < $z, but $y only gets evaluated once.

=cut

.sub 'chain' :method :multi(_, ['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named
    .local pmc clist, cpast

    ##  first, we build up the list of nodes in the chain
    clist = new 'ResizablePMCArray'
    cpast = node
  chain_loop:
    $I0 = isa cpast, 'PAST::Op'
    if $I0 == 0 goto chain_end
    .local string pasttype
    pasttype = cpast.'pasttype'()
    if pasttype != 'chain' goto chain_end
    push clist, cpast
    cpast = cpast[0]
    goto chain_loop
  chain_end:

    .local pmc ops, endlabel
    $P0 = get_hll_global ['POST'], 'Ops'
    ops = $P0.'new'('node'=>node)
    $S0 = self.'unique'('$P')
    ops.'result'($S0)
    $P0 = get_hll_global ['POST'], 'Label'
    endlabel = $P0.'new'('name'=>'chain_end_')

    .local pmc apast, apost
    cpast = pop clist
    apast = cpast[0]
    apost = self.'as_post'(apast, 'rtype'=>'P')
    ops.'push'(apost)

  clist_loop:
    .local pmc bpast, bpost
    bpast = cpast[1]
    bpost = self.'as_post'(bpast, 'rtype'=>'P')
    ops.'push'(bpost)
    .local string name
    name = cpast.'name'()
    name = self.'escape'(name)
    ops.'push_pirop'('call', name, apost, bpost, 'result'=>ops)
    unless clist goto clist_end
    ops.'push_pirop'('unless', ops, endlabel)
    cpast = pop clist
    apost = bpost
    goto clist_loop
  clist_end:
    ops.'push'(endlabel)
    .return (ops)
.end


=item def_or(PAST::Op node)

The short-circuiting default operator (e.g., Perl 6's C<< infix:<//> >>).
Returns its first child if its defined, otherwise it evaluates and returns
the second child.  (N.B.: This particular pasttype is a candidate for
being refactored out using thunks of some sort.)

=cut

.sub 'def_or' :method :multi(_, ['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named

    .local pmc ops
    $P0 = get_hll_global ['POST'], 'Ops'
    $S0 = self.'unique'('$P')
    ops = $P0.'new'('node'=>node, 'result'=>$S0)

    .local pmc lpast, lpost
    lpast = node[0]
    lpost = self.'as_post'(lpast, 'rtype'=>'P')
    ops.'push'(lpost)
    ops.'push_pirop'('set', ops, lpost)

    .local pmc endlabel
    $P0 = get_hll_global ['POST'], 'Label'
    $S0 = self.'unique'('default_')
    endlabel = $P0.'new'('result'=>$S0)

    $S0 = self.'unique'('$I')
    ops.'push_pirop'('defined', $S0, ops)
    ops.'push_pirop'('if', $S0, endlabel)
    .local pmc rpast, rpost
    rpast = node[1]
    rpost = self.'as_post'(rpast, 'rtype'=>'P')
    ops.'push'(rpost)
    ops.'push_pirop'('set', ops, rpost)
    ops.'push'(endlabel)
    .return (ops)
.end


=item xor(PAST::Op node)

A short-circuiting exclusive-or operation.  Each child is evaluated,
if exactly one child evaluates to true then its value is returned,
otherwise return Undef.  Short-circuits with Undef as soon as
a second child is found that evaluates as true.

=cut

.sub 'xor' :method :multi(_,['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named

    .local pmc ops
    $P0 = get_hll_global ['POST'], 'Ops'
    ops = $P0.'new'('node'=>node)
    $S0 = self.'unique'('$P')
    ops.'result'($S0)

    .local pmc labelproto, endlabel, falselabel
    labelproto = get_hll_global ['POST'], 'Label'
    falselabel = labelproto.'new'('name'=>'xor_false')
    endlabel = labelproto.'new'('name'=>'xor_end')

    .local pmc iter, apast, apost, i, t, u
    i = self.'unique'('$I')
    t = self.'unique'('$I')
    u = self.'unique'('$I')
    iter = node.'iterator'()
    apast = shift iter
    apost = self.'as_post'(apast, 'rtype'=>'P')
    ops.'push'(apost)
    ops.'push_pirop'('set', ops, apost)
    ops.'push_pirop'('istrue', t, apost)
  middle_child:
    .local pmc bpast, bpost
    bpast = shift iter
    bpost = self.'as_post'(bpast, 'rtype'=>'P')
    ops.'push'(bpost)
    ops.'push_pirop'('istrue', u, bpost)
    ops.'push_pirop'('and', i, t, u)
    ops.'push_pirop'('if', i, falselabel)
    unless iter goto last_child
    .local pmc truelabel
    truelabel = labelproto.'new'('name'=>'xor_true')
    ops.'push_pirop'('if', t, truelabel)
    ops.'push_pirop'('set', ops, bpost)
    ops.'push_pirop'('set', t, u)
    ops.'push'(truelabel)
    goto middle_child
  last_child:
    ops.'push_pirop'('if', t, endlabel)
    ops.'push_pirop'('set', ops, bpost)
    ops.'push_pirop'('goto', endlabel)
    ops.'push'(falselabel)
    ops.'push_pirop'('new', ops, '"Undef"')
    ops.'push'(endlabel)
    .return (ops)
.end


=item bind(PAST::Op node)

Return the POST representation of a C<PAST::Op>
node with a 'pasttype' of bind.

=cut

.sub 'bind' :method :multi(_, ['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named

    .local pmc ops, lpast, rpast, lpost, rpost
    lpast = node[0]
    rpast = node[1]

    $P0 = get_hll_global ['POST'], 'Ops'
    ops = $P0.'new'('node'=>node)
    rpost = self.'as_post'(rpast, 'rtype'=>'P')
    rpost = self.'coerce'(rpost, 'P')
    ops.'push'(rpost)

    lpast.lvalue(1)
    lpost = self.'as_post'(lpast, 'bindpost'=>rpost)
    ops.'push'(lpost)
    ops.'result'(lpost)
    .return (ops)
.end


=item copy(PAST::Op node)

Implement a 'copy' assignment (at least until we get the 'copy'
opcode -- see RT#47828).

=cut

.sub 'copy' :method :multi(_, ['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named
    .local pmc rpast, rpost, lpast, lpost
    rpast = node[1]
    lpast = node[0]
    rpost = self.'as_post'(rpast, 'rtype'=>'P')
    lpost = self.'as_post'(lpast, 'rtype'=>'P')
    .local pmc ops, alabel
    $P0 = get_hll_global ['POST'], 'Ops'
    ops = $P0.'new'(rpost, lpost, 'node'=>node, 'result'=>lpost)
    ops.'push_pirop'('copy', lpost, rpost)
    .return (ops)
.end


=item inline(PAST::Op node)

Return the POST representation of a C<PAST::Op>
node with a 'pasttype' of inline.

=cut

.sub 'inline' :method :multi(_, ['PAST::Op'])
    .param pmc node
    .param pmc options         :slurpy :named

    .local pmc ops
    ops = self.'post_children'(node, 'signature'=>'vP')

    .local string inline
    inline = node.'inline'()

    .local string result
    result = ''
    $I0 = index inline, '%t'
    if $I0 >= 0 goto result_new
    $I0 = index inline, '%r'
    unless $I0 >= 0 goto have_result
    result = self.'unique'('$P')
    ops.'result'(result)
    goto have_result
  result_new:
    result = self.'unique'('$P')
    ops.'push_pirop'('new', result, "'Undef'")
    ops.'result'(result)
  have_result:

    .local pmc arglist
    arglist = ops.'list'()
    ops.'push_pirop'('inline', arglist :flat, 'inline'=>inline, 'result'=>result)
    $S0 = options['rtype']
    .return (ops)
.end


=back

=head3 C<PAST::Var>

=over 4

=item as_post(PAST::Block node)

Return the POST representation of a C<PAST::Var>.  Generally we
redispatch to an appropriate handler based on the node's 'scope'
attribute.

=cut

.sub 'as_post' :method :multi(_, ['PAST::Var'])
    .param pmc node
    .param pmc options         :slurpy :named

    ##  set 'bindpost'
    .local pmc bindpost
    bindpost = options['bindpost']
    unless null bindpost goto have_bindpost
    bindpost = new 'Undef'
  have_bindpost:

    ## determine the node's scope.  First, check the node itself
    .local string scope
    scope = node.'scope'()
    if scope goto have_scope
    ## otherwise, check the current symbol table under the variable's name
    .local string name
    name = node.'name'()
    .local pmc symtable
    symtable = getattribute self, '%!symtable'
    $P0 = symtable[name]
    if null $P0 goto default_scope
    scope = $P0['scope']
    if scope goto have_scope
  default_scope:
    ##  see if an outer block has set a default scope
    $P0 = symtable['']
    if null $P0 goto scope_error
    scope = $P0['scope']
    unless scope goto scope_error
  have_scope:
    push_eh scope_error
    $P0 = find_method self, scope
    pop_eh
    .return self.$P0(node, bindpost)
  scope_error:
    unless scope goto scope_error_1
    scope = concat " '", scope
    scope = concat scope, "'"
  scope_error_1:
    .return self.'panic'("Scope", scope, " not found for PAST::Var '", name, "'")
.end


.sub 'vivify' :method
    .param pmc node
    .param pmc ops
    .param pmc fetchop
    .param pmc storeop

    .local pmc viviself, vivipost, vivilabel
    viviself = node.'viviself'()
    vivipost = self.'as_post'(viviself, 'rtype'=>'P')
    ops.'result'(vivipost)
    ops.'push'(fetchop)
    unless viviself goto vivipost_done
    $P0 = get_hll_global ['POST'], 'Label'
    vivilabel = $P0.'new'('name'=>'vivify_')
    ops.'push_pirop'('unless_null', ops, vivilabel)
    ops.'push'(vivipost)
    $I0 = node.'lvalue'()
    unless $I0 goto vivipost_stored
    ops.'push'(storeop)
  vivipost_stored:
    ops.'push'(vivilabel)
  vivipost_done:
    .return (ops)
.end


.sub 'parameter' :method :multi(_, ['PAST::Var'])
    .param pmc node
    .param pmc bindpost

    ##  get the current sub
    .local pmc subpost
    subpost = get_global '$?SUB'

    ##  determine lexical, register, and parameter names
    .local string name, named, pname, has_pname
    name = node.'name'()
    named = node.'named'()
    pname = self.'unique'('param_')
    has_pname = concat 'has_', pname

    ##  returned post node
    .local pmc ops
    $P0 = get_hll_global ['POST'], 'Ops'
    ops = $P0.'new'('node'=>node, 'result'=>pname)

    ##  handle optional params
    .local pmc viviself, vivipost, vivilabel
    viviself = node.'viviself'()
    unless viviself goto param_required
    vivipost = self.'as_post'(viviself, 'rtype'=>'P')
    $P0 = get_hll_global ['POST'], 'Label'
    vivilabel = $P0.'new'('name'=>'optparam_')
    subpost.'add_param'(pname, 'named'=>named, 'optional'=>1)
    ops.'push_pirop'('if', has_pname, vivilabel)
    ops.'push'(vivipost)
    ops.'push_pirop'('set', ops, vivipost)
    ops.'push'(vivilabel)
    goto param_done

  param_required:
    .local int slurpy
    slurpy = node.'slurpy'()
    subpost.'add_param'(pname, 'named'=>named, 'slurpy'=>slurpy)

  param_done:
    name = self.'escape'(name)
    ops.'push_pirop'('.lex', name, ops)
    .return (ops)
.end


.sub 'package' :method :multi(_, ['PAST::Var'])
    .param pmc node
    .param pmc bindpost

    .local pmc ops, fetchop, storeop
    $P0 = get_hll_global ['POST'], 'Ops'
    ops = $P0.'new'('node'=>node)

    .local string name
    name = node.'name'()
    name = self.'escape'(name)

    $P0 = get_hll_global ['POST'], 'Op'
    .local pmc ns
    ns = node.'namespace'()
    $I0 = defined ns
    if $I0 goto package_hll
    if bindpost goto package_bind
    fetchop = $P0.'new'(ops, name, 'pirop'=>'get_global')
    storeop = $P0.'new'(name, ops, 'pirop'=>'set_global')
    .return self.'vivify'(node, ops, fetchop, storeop)
  package_bind:
    .return $P0.'new'(name, bindpost, 'pirop'=>'set_global', 'result'=>bindpost)

  package_hll:
    if ns goto package_ns
    if bindpost goto package_hll_bind
    fetchop = $P0.'new'(ops, name, 'pirop'=>'get_hll_global')
    storeop = $P0.'new'(name, ops, 'pirop'=>'set_hll_global')
    .return self.'vivify'(node, ops, fetchop, storeop)
  package_hll_bind:
    .return $P0.'new'(name, bindpost, 'pirop'=>'set_hll_global', 'result'=>bindpost)

  package_ns:
    $P1 = new 'CodeString'
    ns = $P1.'key'(ns)
    if bindpost goto package_ns_bind
    fetchop = $P0.'new'(ops, ns, name, 'pirop'=>'get_hll_global')
    storeop = $P0.'new'(ns, name, ops, 'pirop'=>'set_hll_global')
    .return self.'vivify'(node, ops, fetchop, storeop)
  package_ns_bind:
    .return $P0.'new'(ns, name, bindpost, 'pirop'=>'set_hll_global', 'result'=>bindpost)
.end


.sub 'lexical' :method :multi(_, ['PAST::Var'])
    .param pmc node
    .param pmc bindpost

    .local string name
    $P0 = get_hll_global ['POST'], 'Ops'
    name = node.'name'()
    name = self.'escape'(name)

    .local int isdecl
    isdecl = node.'isdecl'()

    if bindpost goto lexical_bind

  lexical_post:
    if isdecl goto lexical_decl
    .local pmc ops, fetchop, storeop
    ops = $P0.'new'('node'=>node)
    $P0 = get_hll_global ['POST'], 'Op'
    fetchop = $P0.'new'(ops, name, 'pirop'=>'find_lex')
    storeop = $P0.'new'(name, ops, 'pirop'=>'store_lex')
    .return self.'vivify'(node, ops, fetchop, storeop)

  lexical_decl:
    ops = $P0.'new'('node'=>node)
    .local pmc viviself, vivipost
    viviself = node.'viviself'()
    vivipost = self.'as_post'(viviself, 'rtype'=>'P')
    ops.'push'(vivipost)
    ops.'push_pirop'('.lex', name, vivipost)
    ops.'result'(vivipost)
    .return (ops)

  lexical_bind:
    $P0 = get_hll_global ['POST'], 'Op'
    if isdecl goto lexical_bind_decl
    .return $P0.'new'(name, bindpost, 'pirop'=>'store_lex', 'result'=>bindpost)
  lexical_bind_decl:
    .return $P0.'new'(name, bindpost, 'pirop'=>'.lex', 'result'=>bindpost)
.end


.sub 'keyed' :method :multi(_, ['PAST::Var'])
    .param pmc node
    .param pmc bindpost
    .param string keyrtype     :optional
    .param int has_keyrtype    :opt_flag

    .local pmc ops
    $P0 = get_hll_global ['POST'], 'Ops'
    ops = $P0.'new'('node'=>node)

    if has_keyrtype goto have_keyrtype
    keyrtype = '*'
  have_keyrtype:

    .local pmc keypast, keypost
    keypast = node[1]
    keypost = self.'as_post'(keypast, 'rtype'=>keyrtype)
    keypost = self.'coerce'(keypost, keyrtype)
    ops.'push'(keypost)

    .local pmc basepast, basepost
    basepast = node[0]

    $P0 = node.'vivibase'()
    unless $P0 goto have_vivibase
    $I0 = can basepast, 'viviself'
    unless $I0 goto have_vivibase
    $P1 = basepast.'viviself'()
    unless $P1 goto vivibase_1
    if $P1 != 'Undef' goto have_vivibase
  vivibase_1:
    basepast.'viviself'($P0)
  have_vivibase:

    #  if the keyed node is an lvalue, its base is an lvalue also
    $I0 = node.'lvalue'()
    unless $I0 goto have_lvalue
    basepast.lvalue($I0)
  have_lvalue:

    basepost = self.'as_post'(basepast, 'rtype'=>'P')
    ops.'push'(basepost)
    .local string name
    $S0 = basepost.'result'()
    name = concat $S0, '['
    $S0 = keypost.'result'()
    concat name, $S0
    concat name, ']'
    .local pmc fetchop, storeop
    $P0 = get_hll_global ['POST'], 'Op'
    if bindpost goto keyed_bind
    fetchop = $P0.'new'(ops, name, 'pirop'=>'set')
    storeop = $P0.'new'(name, ops, 'pirop'=>'set')
    .return self.'vivify'(node, ops, fetchop, storeop)
  keyed_bind:
    ops.'result'(bindpost)
    ops.'push_pirop'('set', name, ops)
    .return (ops)
.end


.sub 'keyed_int' :method :multi(_, ['PAST::Var'])
    .param pmc node
    .param pmc bindpost
    .return self.'keyed'(node, bindpost, 'i')
.end


.sub 'attribute' :method :multi(_, ['PAST::Var'])
    .param pmc node
    .param pmc bindpost

    .local string name
    $P0 = get_hll_global ['POST'], 'Ops'
    name = node.'name'()
    name = self.'escape'(name)

    .local int isdecl
    isdecl = node.'isdecl'()

    if bindpost goto attribute_bind

  attribute_post:
    if isdecl goto attribute_decl
    .local pmc ops, fetchop, storeop
    ops = $P0.'new'('node'=>node)
    $P0 = get_hll_global ['POST'], 'Op'
    fetchop = $P0.'new'(ops, 'self', name, 'pirop'=>'getattribute')
    storeop = $P0.'new'('self', name, ops, 'pirop'=>'setattribute')
    .return self.'vivify'(node, ops, fetchop, storeop)

  attribute_decl:
    .return $P0.'new'('node'=>node)

  attribute_bind:
    $P0 = get_hll_global ['POST'], 'Op'
    if isdecl goto attribute_bind_decl
    .return $P0.'new'('self', name, bindpost, 'pirop'=>'setattribute', 'result'=>bindpost)
  attribute_bind_decl:
    .return $P0.'new'('self', name, bindpost, 'pirop'=>'setattribute', 'result'=>bindpost)
.end


=back

=head3 C<PAST::Val>

=over 4

=item as_post(PAST::Val node [, 'rtype'=>rtype])

Return the POST representation of the constant value given
by C<node>.  The C<rtype> parameter advises the method whether
the value may be returned directly as a PIR constant or needs
to have a PMC generated containing the constant value.

=cut

.sub 'as_post' :method :multi(_, ['PAST::Val'])
    .param pmc node
    .param pmc options         :slurpy :named

    .local pmc ops
    $P0 = get_hll_global ['POST'], 'Ops'
    ops = $P0.'new'('node'=>node)

    .local pmc value, returns
    value = node.'value'()
    returns = node.'returns'()
    if returns goto have_returns
    $S0 = typeof value
    returns = $S0
  have_returns:

    .local string valflags
    $P0 = get_global '%valflags'
    valflags = $P0[returns]

    $I0 = index valflags, 'e'
    if $I0 < 0 goto escape_done
    value = self.'escape'(value)
  escape_done:

    .local string rtype
    rtype = options['rtype']
    $I0 = index valflags, rtype
    if $I0 < 0 goto result_pmc
    ops.'result'(value)
    .return (ops)

  result_pmc:
    .local string result
    result = self.'unique'('$P')
    returns = self.'escape'(returns)
    ops.'push_pirop'('new', result, returns)
    ops.'push_pirop'('assign', result, value)
    ops.'result'(result)
    .return (ops)
.end


=back

=head1 AUTHOR

Patrick Michaud <pmichaud@pobox.com> is the author and maintainer.
Please send patches and suggestions to the Parrot porters or
Perl 6 compilers mailing lists.

=head1 HISTORY

2006-11-20  Patrick Michaud added first draft of POD documentation.
2006-11-27  Significant refactor into separate modules.

=head1 COPYRIGHT

Copyright (C) 2006-2008, The Perl Foundation.

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
