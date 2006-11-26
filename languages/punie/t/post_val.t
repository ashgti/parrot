#!perl

use strict;
use warnings;
use lib qw(t . lib ../lib ../../lib ../../../lib);
use Parrot::Test tests => 2;

pir_output_is( <<'CODE', <<'OUT', 'set attributes via method' );
.sub _main
    load_bytecode 'languages/punie/lib/POST.pir'
    .local pmc node
    node = new 'POST::Val'
    node.'source'('foo')
    node.'pos'(42)
    node.'value'('bar')
    $P1 = getattribute node, 'source'
    print $P1
    print "\n"
    $P1 = getattribute node, 'pos'
    print $P1
    print "\n"
    $P1 = getattribute node, 'value'
    print $P1
    print "\n"
    .return ()
.end
CODE
foo
42
bar
OUT

pir_output_is( <<'CODE', <<'OUT', 'dump node structure in visual format' );
.sub _main
    load_bytecode 'languages/punie/lib/POST.pir'
    .local pmc node
    node = new 'POST::Val'
    node.'source'('foo')
    node.'pos'(42)
    node.'value'('bar')
    node.dump()
    .return ()
.end
CODE
<POST::Val> => { 
    'source' => 'foo',
    'pos' => '42',
    'value' => 'bar',
    'valtype' => undef,
}
OUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
