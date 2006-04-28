#!/usr/bin/perl

use strict;
use lib qw(t . lib ../lib ../../lib ../../../lib);
use Parrot::Test tests => 2;

pir_output_is(<<'CODE', <<'OUT', 'set attributes via method');
.sub _main
    load_bytecode 'languages/punie/lib/PAST.pir'
    .local pmc node
    node = new 'PAST::Op'
    node.'source'('foo')
    node.'pos'(42)
    node.'op'('bar')
    $P1 = getattribute node, 'source'
    print $P1
    print "\n"
    $P1 = getattribute node, 'pos'
    print $P1
    print "\n"
    $P1 = getattribute node, 'op'
    print $P1
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
    load_bytecode 'languages/punie/lib/PAST.pir'
    .local pmc node
    node = new 'PAST::Op'
    node.'source'('foo')
    node.'pos'(42)
    node.'op'('bar')
    node.dump()
    .return ()
.end
CODE
<PAST::Op> => { 
    'source' => 'foo',
    'pos' => '42',
    'op' => 'bar',
    'children' => []
}
OUT
