=head1 NAME

POST::Node - a base class for POST opcode syntax tree nodes

=head1 DESCRIPTION

All POST nodes subclass from this base type.

=cut

.namespace [ 'POST::Node' ]

.sub '__onload' :load
    .local pmc base
    newclass base, 'POST::Node'
    addattribute base, 'source'           # the original source code
    addattribute base, 'pos'              # offset position in source
    addattribute base, 'children'         # child nodes
    addattribute base, 'symbols'         # child nodes
    .return ()
.end


.sub init :vtable :method
    $P1 = new .Undef
    $P2 = new .Integer
    $P3 = new .ResizablePMCArray
	$P4 = new .Hash

    setattribute self, 'source', $P1
    setattribute self, 'pos', $P2
    setattribute self, 'children', $P3
    setattribute self, 'symbols', $P4
.end

.sub get_bool :vtable :method
	.return( 1 )
.end

.sub "set_node" :method
    .param string source
    .param int pos
    .param pmc children
    $P1 = getattribute self, "source"
    $P1 = source
    $P2 = getattribute self, "pos"
    $P2 = pos
    setattribute self, "children", children
    .return ()
.end

.sub source :method
    $P2 = getattribute self, "source"
    .return ($P2)
.end

.sub pos :method
    $P2 = getattribute self, "pos"
    .return ($P2)
.end

.sub children :method
	.param pmc children     :optional
	.param int got_children :opt_flag
	unless got_children goto get

	setattribute self, 'children', children
	.return()
  get:
    $P2 = getattribute self, "children"
    .return ($P2)
.end

.sub add_child :method
	.param pmc child

	.local pmc children
	children = getattribute self, 'children'
	push children, child
.end

.sub symbols :method
	.param pmc new_symbols  :optional
	.param int have_symbols :opt_flag

	unless have_symbols goto get
  set:
	setattribute self, 'symbols', new_symbols
	.return( new_symbols )
  get:
	.local pmc symbols
	symbols = getattribute self, 'symbols'
	.return( symbols )
.end

.sub "dump" :method
    .param int level :optional
    .local string indent
    indent = repeat "    ", level # tab is 4 spaces here
    level += 1 # set level for attributes
    $S0 = typeof self
    print indent
    print "<"
    print $S0
    print "> => { \n"

    # print source for this node
    self.dump_attribute("source", level)
    self.dump_attribute("pos", level)
    self.dump_children(level)

    # close off current node display
    print indent
    print "}\n"
    .return ()
.end

.sub "dump_attribute" :method
    .param string name
    .param int level :optional
    .local string indent
    indent = repeat "    ", level # tab is 4 spaces here
    # print value for this attribute
    print indent
    print "'"
    print name
    print "' => "
    $P1 = getattribute self, name
    $I0 = defined $P1
    unless $I0 goto attribute_undef
    print "'"
    print $P1
    print "'"
    goto attribute_def
  attribute_undef:
    print "undef"
  attribute_def:
    print ",\n"
    .return ()
.end

.sub "dump_children" :method
    .param int level :optional
    .local string indent
    indent = repeat "    ", level # tab is 4 spaces here
    level += 1 # set level to pass on to children
    # print children for this node
    print indent
    print "'children' => ["
    $P3 = getattribute self, "children"
    $I0 = defined $P3
    unless $I0 goto no_children
    print "\n"
    .local pmc iter
    iter = new Iterator, $P3 # loop over the array
    iter = 0 # start at the beginning
  loop_start:
    unless iter goto loop_end
    $P1 = shift iter
    $P1.dump(level)
    goto loop_start
  loop_end:
    print indent

  no_children:
    print "]\n"
    .return ()
.end

.sub generate_label :method
    .param string name :optional
    .param int got_name :opt_flag
    .local string label
     unless got_name goto no_name
       label = name . "_"
  no_name:
     label .= "label_"
     $I1 = _new_label_id()
     $S1 = $I1
     label .= $S1
     .return (label)
.end

# Autoincrementing id generator
.sub _new_label_id
    .local int id
    id = 0
loop:
    inc id 
    .yield(id)
    goto loop
.end

.sub generate_temp :method
    .local string temp
       temp = "$P"
     $I1 = _new_temp_id()
     $S1 = $I1
     temp .= $S1
     .return (temp)
.end

# Autoincrementing id generator
.sub _new_temp_id
    .local int id
    id = 0
loop:
    inc id 
    .yield(id)
    goto loop
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
