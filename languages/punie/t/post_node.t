#!/usr/bin/perl

use strict;
use lib qw(t . lib ../lib ../../lib ../../../lib);
use Parrot::Test tests => 6;

pir_output_is(<<'CODE', <<'OUT', 'load the library');
.sub _main
    load_bytecode 'languages/punie/lib/POST/Node.pir'
.end
CODE
OUT

pir_output_is(<<'CODE', <<'OUT', 'construct a node');
.sub _main
    load_bytecode 'languages/punie/lib/POST/Node.pir'
    .local pmc node
    node = new 'POST::Node'
    .return()
.end
CODE
OUT

pir_output_is(<<'CODE', <<'OUT', 'set attributes via method');
.sub _main
    load_bytecode 'languages/punie/lib/POST/Node.pir'
    .local pmc node
    node = new 'POST::Node'
    $P0 = new PerlString
    $P0 = 'bar'
    $P1 = new PerlArray
    push $P1, $P0
    node.set_node('foo', 42, $P1)
    $P1 = getattribute node, 'source'
    print $P1
    print "\n"
    $P1 = getattribute node, 'pos'
    print $P1
    print "\n"
    $P2 = getattribute node, 'children'
    $P3 = $P2[0]
    print $P3
    print "\n"
    .return ()
.end
CODE
foo
42
bar
OUT

pir_output_is(<<'CODE', <<'OUT', 'dump node structure in visual format');
.sub _main
    load_bytecode 'languages/punie/lib/POST/Node.pir'
    .local pmc node1
    .local pmc node2
    node1 = new 'POST::Node'
    node2 = new 'POST::Node'
    node2.set_node('b', 9, $P0)
    $P1 = new PerlArray
    push $P1, node2 
    node1.set_node('foo', 42, $P1)
    node1.dump()
    .return ()
.end
CODE
<POST::Node> => { 
    'source' => 'foo',
    'pos' => '42',
    'children' => [
        <POST::Node> => { 
            'source' => 'b',
            'pos' => '9',
            'children' => []
        }
    ]
}
OUT

pir_output_is(<<'CODE', <<'OUT', 'generate a label');
.sub _main
    load_bytecode 'languages/punie/lib/POST/Node.pir'
    .local pmc node
    node = new 'POST::Node'
    $S1 = node.generate_label('name')
    print $S1
    print "\n"
    $S2 = node.generate_label()
    print $S2
    print "\n"
    .return()
.end
CODE
name_label_1
label_2
OUT

pir_output_is(<<'CODE', <<'OUT', 'generate a temporary variable');
.sub _main
    load_bytecode 'languages/punie/lib/POST/Node.pir'
    .local pmc node
    node = new 'POST::Node'
    $S1 = node.generate_temp()
    print $S1
    print "\n"
    $S2 = node.generate_temp()
    print $S2
    print "\n"
    .return()
.end
CODE
$P1
$P2
OUT
