=head1 NAME

PAST - Parrot abstract syntax tree

=head1 DESCRIPTION

This file implements the various abstract syntax tree nodes
for compiling programs in Parrot.

=cut

.namespace [ 'PAST' ]

.sub '__onload' :load :init
    ##   create the PAST::Node base class
    ##   XXX: Eventually we want this to be a subclass of
    ##   Capture, but as of now Capture isn't working.
    ##   So, we'll simulate it for now.
    .local pmc base, protomaker
    load_bytecode 'Parrot/Capture_PIR.pbc'
    protomaker = get_hll_global 'Protomaker'

    base = protomaker.'new_subclass'('Capture_PIR', 'PAST::Node')

    $P0 = protomaker.'new_subclass'(base, 'PAST::Op')
    $P0 = protomaker.'new_subclass'(base, 'PAST::Stmts')
    $P0 = protomaker.'new_subclass'(base, 'PAST::Val')
    $P0 = protomaker.'new_subclass'(base, 'PAST::Var')
    $P0 = protomaker.'new_subclass'(base, 'PAST::Block')
    $P0 = protomaker.'new_subclass'(base, 'PAST::VarList')

    $P0 = new 'Integer'
    $P0 = 10
    set_hll_global ['PAST::Node'], '$!serno', $P0

    .return ()
.end

=head1 PAST Node types

=head2 PAST::Node

C<PAST::Node> is the base class for all PAST (and POST) nodes.  It's
derived from class C<Capture>, so that it has both array and hash
components.  The array component is used to maintain a node's children,
while the hash component contains the attributes of the node.  In general
we provide and use accessor methods for a node's attributes, instead
of accessing the hash component directly.

Every PAST node predefines C<name>, C<source>, and C<pos> attributes.
The C<name> attribute is the node's name, if any, while C<source>
and C<pos> are used to identify the location in the original source
code for the node.  The C<source> and C<pos> values are generally
set by the C<node> method below.

Other node attributes are generally defined by subclasses of C<PAST::Node>.

=over 4

=item init([child1, child2, ..., ] [attr1=>val1, attr2=>val2, ... ])

Initialize a PAST node with the given children and attributes.
Adds each child to the node (using the C<push> method, below) and
calls the appropriate accessor method for each attribute.
And returns the node.

=cut

.namespace [ 'PAST::Node' ]

.sub 'init' :method
    .param pmc children        :slurpy
    .param pmc adverbs         :slurpy :named

    .local pmc iter
    iter = new 'Iterator', children
  children_loop:
    unless iter goto children_end
    $P0 = shift iter
    push self, $P0
    goto children_loop
  children_end:

    iter = new 'Iterator', adverbs
  adverbs_loop:
    unless iter goto adverbs_end
    $S0 = shift iter
    $P0 = iter[$S0]
    $P1 = find_method self, $S0
    self.$P1($P0)
    goto adverbs_loop
  adverbs_end:
  end:
    .return (self)
.end


=item new([child1, child2, ..., ] [attr1=>val1, attr2=>val2, ...])

Create a new PAST node of initialized with the given
children and attributes.  Returns the newly created node.

=cut

.sub 'new' :method
    .param pmc children        :slurpy
    .param pmc adverbs         :slurpy :named

    $P0 = typeof self
    $P1 = new $P0
    $P1.'init'(children :flat, adverbs :flat :named)
    .return ($P1)
.end


=item unshift(child)

Add C<child> to the beginning of the invocant's list of children.

=item push(child)

Add C<child> to the end of the invocant's list of children.

=cut

.sub 'unshift' :method
    .param pmc value
    unshift self, value
.end

.sub 'push' :method
    .param pmc value
    push self, value
.end


=item push_new(class, [child1, child2, ..., ] [attr1=>val1, attr2=>val2, ...])

Creates a new PAST node of type C<class>, initializes it with the
given children and attributes, and adds it to the end of the invocant's
array of children.  Returns the newly created PAST node.

=cut

.sub 'push_new' :method
    .param string class
    .param pmc children        :slurpy
    .param pmc adverbs         :slurpy :named
    $P0 = new class
    $P0.'init'(children :flat, adverbs :flat :named)
    push self, $P0
    .return ($P0)
.end


=item iterator( )

Returns a newly initialized iterator for the invocant's list of
children.

=cut

.sub 'iterator' :method
    .local pmc iter
    $P0 = self.'get_array'()
    iter = new 'Iterator', $P0
    iter = 0
    .return (iter)
.end


=item node([val])

Sets the invocant's C<source> and C<pos> attributes to those
of C<val>.  If C<val> is another PAST node, then C<source> and C<pos>
are simply copied from that node, otherwise C<val> is assumed to be
a C<Match> object and obtains source/position information from that.

=cut

.sub 'node' :method
    .param pmc node
    $I0 = isa node, 'PAST::Node'
    if $I0 goto clone_past
  clone_pge:
    $S0 = node
    $I0 = node.'from'()
    self['source'] = $S0
    self['pos'] = $I0
    .return ()
  clone_past:
    $P0 = node['source']
    $P1 = node['pos']
    self['source'] = $P0
    self['pos'] = $P1
    .return ()
.end


=item name([value])

Accessor method -- sets/returns the C<name> attribute of the invocant.

=cut

.sub 'name' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('name', value, has_value)
.end


=item returns([value])

Accessor method -- sets/returns the return type for the invocant.

=cut

.sub 'returns' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('returns', value, has_value)
.end


=item named([value])

Accessor method -- for named arguments, sets/returns the name to be
associated with the argument.

=cut

.sub 'named' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('named', value, has_value)
.end


=item flat([value]

Accessor method -- sets/returns the "flatten" flag on arguments.

=cut

.sub 'flat' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('flat', value, has_value)
.end


=item attr(STR attrname, PMC value, INT has_value)

Helper method for accessors.  If C<has_value> is true then set
the invocant's value of C<attrname> to C<value>.  Returns the
(resulting) value of C<attrname> in the invocant.

=cut

.sub 'attr' :method
    .param string attrname
    .param pmc value
    .param int has_value
    if has_value goto setattr
    value = self[attrname]
    unless null value goto getattr
    value = new 'Undef'
  getattr:
    .return (value)
  setattr:
    self[attrname] = value
    .return (value)
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


=back

=head2 PAST::Val

C<PAST::Val> nodes represent constant values in the abstract
syntax tree.  The C<name> attribute represents the value of the
node.

=over 4

=item value([value])

Get/set the constant value for this node.

=item returns([typename])

Get/set the type of PMC to be generated from this node.

=cut

.namespace [ 'PAST::Val' ]

.sub 'value' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('value', value, has_value)
.end

.sub 'returns' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('returns', value, has_value)
.end

=back

=head2 PAST::Var

C<PAST::Var> nodes represent variables within the abstract
syntax tree.  The variable name (if any) is given as the node's
C<name> attribute.

=over 4

=item scope([value])

Get/set the PAST::Var node's "scope" (i.e., how the variable
is accessed or set).  Allowable values include "package", "lexical",
"parameter", and "keyed", representing HLL global, lexical, block
parameter, and array/hash variables respectively.

=cut

.namespace [ 'PAST::Var' ]

.sub 'scope' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('scope', value, has_value)
.end


=item isdecl([flag])

Get/set the node's C<isdecl> attribute (for lexical variables) to C<flag>.
A true value of C<isdecl> indicates that the variable given by
this node is to be created within the current lexical scope.
Otherwise, the node refers to a lexical variable from an outer scope.

=cut

.sub 'isdecl' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('isdecl', value, has_value)
.end


=item lvalue([flag])

Get/set the C<lvalue> attribute, which indicates whether this
variable is being used in an lvalue context.

=cut

.sub 'lvalue' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('lvalue', value, has_value)
.end


=item namespace([namespace])

Get/set the variable's namespace attribute to the array of strings
given by C<namespace>.  Useful only for variables with a C<scope>
of 'package'.

=cut

.sub 'namespace' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('namespace', value, has_value)
.end


=item slurpy([flag])

Get/set the node's C<slurpy> attribute (for parameter variables) to C<flag>.
A true value of C<slurpy> indicates that the parameter variable given by this
node is to be created as a slurpy parameter (consuming all remaining arguments
passed in).

=cut

.sub 'slurpy' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('slurpy', value, has_value)
.end


=item viviself([type])

If the variable needs to be instantiated, then C<type> indicates
either the type of the value to create for the node or (future
implementation) a PAST tree to create the value.

=cut

.sub 'viviself' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('viviself', value, has_value)
.end


=item vivibase([type])

For keyed nodes, C<type> indicates the type of aggregate to
create for the base if the base doesn't specify its own 'viviself'
attribute.

=cut

.sub 'vivibase' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('vivibase', value, has_value)
.end


=back

=head2 PAST::Op

C<PAST::Op> nodes represent the operations in an abstract syntax
tree.  The primary function of the node is given by its C<pasttype>,
secondary functions may be given by the node's C<name>, C<pirop>,
or other attributes.

=over 4

=item pasttype([type])

A C<PAST::Op> node's C<pasttype> determines the type of operation to
be performed.  Predefined values of C<pasttype> are:

assign     - Copy the value of the node's second child into
             the variable expression given by its first child.

bind       - Bind the variable given by the node's first child
             to the value given by its second child.

if         - Evaluate the first child; if the first child is
             true then evaluate the second child (if any)
             otherwise evaluate the third child (if any).
             If either the second or third child are missing,
             then they evaluate as the result of the first child.

unless     - Same as 'if' above, but reverse the evaluation
             of the second and third children nodes.

while      - Evaluate the first child, if the result is
             true then evaluate the second child and repeat.

until      - Evaluate the first child, if the result is
             false then evaluate the second child and repeat.

for        - Iterate over the first child. For each element, 
             invoke the sub in the second child, passing the
             element as the only parameter.

call       - Call a subroutine, passing the results of any
             child nodes as arguments.  The subroutine to be
             called is given by the node's C<name> attribute,
             if the node has no C<name> attribute then the
             first child is assumed to evaluate to a callable
             sub.

pirop      - Execute the named PIR opcode, passing the results
             of any children nodes as arguments.

inline     - Execute the sequence of PIR statements given
             by the node's C<inline> attribute (a string).
             See the C<inline> method below for details.

callmethod - Invokes a method on an object, using children
             nodes as arguments.  If the node has a C<name>
             attribute, then the first child is the invocant
             and any remaining children are arguments.  If
             the node doesn't have a C<name> attribute, then
             the first child evaluates to the method to be
             called, the second child is the invocant, and
             the remaining children are arguments to the method call.

try        - (preliminary) Execute the code given by the first
             child, and if any exceptions occur then handle them
             using the code given by the second child.

If a node doesn't have a value set for C<pasttype>, then it
assumes "pirop" if its C<pirop> attribute is set, otherwise it
assumes "call".

=cut

.namespace [ 'PAST::Op' ]

.sub 'pasttype' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('pasttype', value, has_value)
.end


=item pirop([opcode])

Get/set the PIR opcode to be executed for this node.  The PAST
implementation knows about the argument types for many of the
PIR opcodes, and will try to automatically convert the results
of any children nodes into the correct types if needed.  (In
general, the implementation tries to convert things to PMCs
whenever it isn't certain what else to do.)  The table of
PIR opcodes that PAST "knows" about is in F<POST.pir>.

=cut

.sub 'pirop' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('pirop', value, has_value)
.end


=item lvalue([flag])

Get/set whether this node is an lvalue, or treats its first
child as an lvalue (e.g., for assignment).

=cut

.sub 'lvalue' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('lvalue', value, has_value)
.end

=item inline([STRING code])

Get/set the code to be used for inline PIR when C<pasttype> is
"inline".  The C<code> argument is PIR text to be inserted in
the final generated code sequence.  Sequences of "%0", "%1",
"%2", ... "%9" in C<code> are replaced with the evaluated
results of the first, second, third, ..., tenth children nodes.
(If you need more than ten arguments to your inline PIR, consider
making it a subroutine call instead.)

The register to hold the result of the inline PIR operation is
given by "%r", "%t", or "%u" in the C<code> string:

  %r   - Generate a unique PMC register for the result.
  %t   - Generate a unique PMC register for the result,
         and initialize it with an object of type C<returns>
         before the execution of the inline PIR.
  %u   - Re-use the first child's PMC (%0) if it's a temporary
         result, otherwise same as %t above.
  %v   - (void) Re-use the first child's PMC (%0) as the result
         of this operation.

=cut

.sub 'inline' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('inline', value, has_value)
.end

=back

=head2 PAST::Stmts

C<PAST::Stmts> is a container of C<PAST::Node> without any specific methods.

=head2 PAST::Block

C<PAST::Block> nodes represent lexical scopes within an abstract
syntax tree, and roughly translate to individual Parrot subroutines.
A C<PAST::Block> node nested within another C<PAST::Block> node
acts like a nested lexical scope.

If the block has a C<name> attribute, that becomes the name of
the resulting Parrot sub, otherwise a unique name is automatically
generated for the block.

=over 4

=item blocktype([STRING type])

Get/set the type of the block.  The currently understood values
are 'declaration' and 'immediate'.  'Declaration' indicates
that a block is simply being defined at this point, while
'immediate' indicates a block that is to be immediately
executed when it is evaluated in the AST (e.g., the immediate
blocks in Perl6 C<if>, C<while>, and other similar statements).

=cut

.namespace [ 'PAST::Block' ]

.sub 'blocktype' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('blocktype', value, has_value)
.end


=item namespace([namespace])

Get/set the namespace for this block.  The C<namespace> argument
can be either a string or an array of strings.

=cut

.sub 'namespace' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('namespace', value, has_value)
.end


=item symbol(name, [attr1 => val, attr2 => val2, ...])

If called with named arguments, sets the symbol hash corresponding
to C<name> in the current block.  The HLL is free to select
any symbol attributes desired, although the 'scope' attribute
is typically used to assist with lexical scoping of PAST::Var
nodes.

If no named arguments are given, returns the current
attribute hash for symbol C<name>.

=cut

.sub 'symbol' :method
    .param string name
    .param pmc attr            :slurpy :named
    .local pmc symtable
    symtable = self['symtable']
    unless null symtable goto have_symtable
    symtable = new 'Hash'
    self['symtable'] = symtable
  have_symtable:
    if attr goto set_symbol
  get_symbol:
    $P0 = symtable[name]
    if null $P0 goto set_symbol
    .return ($P0)
  set_symbol:
    symtable[name] = attr
    .return (attr)
.end


=item symtable([value])

Get/set the symbol table for the block.  May be deprecated in
favor of the C<symbol> method above.

=cut

.sub 'symtable' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('symtable', value, has_value)
.end


=item compiler([name])

Indicate that the children nodes of this block are to be
compiled using compiler C<name> instead of the standard
PAST compiler.

=cut

.sub 'compiler' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('compiler', value, has_value)
.end


=item pragma([pragma])

Get/set any pragmas (PIR) for this block.

=cut

.sub 'pragma' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('pragma', value, has_value)
.end


.namespace [ 'PAST::VarList' ]

.sub 'bindvalue' :method
    .param pmc value           :optional
    .param int has_value       :opt_flag
    .return self.'attr'('bindvalue', value, has_value)
.end


=back

=head1 AUTHOR

Patrick Michaud <pmichaud@pobox.com> is the author and maintainer.
Please send patches and suggestions to the Parrot porters or
Perl 6 compilers mailing lists.

=head1 HISTORY

2006-11-20  Patrick Michaud added first draft of POD documentation.
2007-11-21  Re-implementation with pdd26 compliance, compiler toolkit

=head1 COPYRIGHT

Copyright (C) 2006-2007, The Perl Foundation.

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
