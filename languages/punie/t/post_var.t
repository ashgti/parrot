#!perl

use strict;
use warnings;
use lib qw(t . lib ../lib ../../lib ../../../lib);
use Parrot::Test tests => 3;

pir_output_is( <<'CODE', <<'OUT', 'set attributes via method' );
.sub _main
    load_bytecode 'languages/punie/lib/POST.pir'
    .local pmc node
    node = new 'POST::Var'
    node.'source'('foo')
    node.'pos'(42)
    node.'varname'('bar')
    $P1 = getattribute node, 'source'
    print $P1
    print "\n"
    $P1 = getattribute node, 'pos'
    print $P1
    print "\n"
    $P1 = getattribute node, 'varname'
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
    node = new 'POST::Var'
    node.'source'('foo')
    node.'pos'(42)
    node.'varname'('bar')
    node.dump()
    .return ()
.end
CODE
<POST::Var> => { 
    'source' => 'foo',
    'pos' => '42',
    'varname' => 'bar',
    'hllname' => undef,
    'vartype' => undef,
    'scope' => undef,
}
OUT

pir_output_is( <<'CODE', <<'OUT', 'generate a temporary variable' );
.sub _main
    load_bytecode 'languages/punie/lib/POST.pir'
    .local pmc node
    node = new 'POST::Var'
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

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
