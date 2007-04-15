#! perl
# Copyright (C) 2001-2006, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 56;
use Parrot::Config;

=head1 NAME

t/pmc/namespace.t - test the NameSpace PMC as described in PDD21.

=head1 SYNOPSIS

    % prove t/pmc/namespace.t

=head1 DESCRIPTION

Test the NameSpace PMC as described in PDD21.

=cut

# L<PDD21/Namespace PMC API/>
pir_output_is( <<'CODE', <<'OUT', 'new' );
.sub 'test' :main
    new $P0, .NameSpace
    say 'ok 1 - $P0 = new .NameSpace'
.end
CODE
ok 1 - $P0 = new .NameSpace
OUT

# L<PDD21/Namespace PMC API/=head4 Untyped Interface>
pir_output_is( <<'CODE', <<'OUT', 'NameSpace does "hash"' );
.sub 'test' :main
    new $P0, .NameSpace
    $I0 = does $P0, 'hash'
    if $I0 goto ok_1
    print 'not '
  ok_1:
    say 'ok 1 - NameSpace does "hash"'
.end
CODE
ok 1 - NameSpace does "hash"
OUT

# L<PDD21//>
pir_output_is( <<'CODE', <<'OUTPUT', "find_global bar" );
.sub 'main' :main
    $P0 = find_global "bar"
    print "ok\n"
    $P0()
.end

.sub 'bar'
    print "bar\n"
.end
CODE
ok
bar
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "verify NameSpace type" );
.sub 'main' :main
    $P0 = find_global "Foo"
    typeof $S0, $P0
    print $S0
    print "\n"
.end

.namespace ["Foo"]
.sub 'bar'
    noop
.end
CODE
NameSpace
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "find_global Foo::bar" );
.sub 'main' :main
    $P0 = find_global "Foo", "bar"
    print "ok\n"
    $P0()
.end

.namespace ["Foo"]
.sub 'bar'
    print "bar\n"
.end
CODE
ok
bar
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "get_namespace Foo::bar" );
.sub 'main' :main
    $P0 = find_global "Foo", "bar"
    print "ok\n"
    $P1 = $P0."get_namespace"()
    print $P1
    print "\n"
.end

.namespace ["Foo"]
.sub 'bar'
.end
CODE
ok
Foo
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "find_global Foo::bar ns" );
.sub 'main' :main
    $P1 = find_global ["Foo"], "bar"
    print "ok\n"
    $P1()
.end

.namespace ["Foo"]
.sub 'bar'
    print "bar\n"
.end
CODE
ok
bar
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "find_global Foo::bar hash" );
.sub 'main' :main
    $P0 = find_global "Foo"
    $P1 = $P0["bar"]
    print "ok\n"
    $P1()
.end

.namespace ["Foo"]
.sub 'bar'
    print "bar\n"
.end
CODE
ok
bar
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "find_global Foo::Bar::baz" );
.sub 'main' :main
    $P2 = find_global ["Foo";"Bar"], "baz"
    print "ok\n"
    $P2()
.end

.namespace ["Foo" ; "Bar"]
.sub 'baz'
    print "baz\n"
.end
CODE
ok
baz
OUTPUT

pir_output_like( <<'CODE', <<'OUTPUT', "find_global Foo::bazz not found" );
.sub 'main' :main
    $P2 = find_global ["Foo"], "bazz"
    $P2()
    print "ok\n"
.end
CODE
/Null PMC access in invoke\(\)/
OUTPUT

# [this used to behave differently from the previous case.]
pir_output_like( <<'CODE', <<'OUTPUT', "find_global Foo::Bar::bazz not found" );
.sub 'main' :main
    $P2 = find_global ["Foo";"Bar"], "bazz"
    $P2()
    print "ok\n"
.end
CODE
/Null PMC access in invoke\(\)/
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "find_global Foo::Bar::baz hash" );
.sub 'main' :main
    $P0 = find_global "Foo"
    $P1 = $P0["Bar"]
    $P2 = $P1["baz"]
    print "ok\n"
    $P2()
.end

.namespace ["Foo"; "Bar"]
.sub 'baz'
    print "baz\n"
.end
CODE
ok
baz
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "find_global Foo::Bar::baz hash 2" );
.sub 'main' :main
    $P0 = find_global "Foo"
    $P1 = $P0["Bar" ; "baz"]
    print "ok\n"
    $P1()
.end

.namespace ["Foo"; "Bar"]
.sub 'baz'
    print "baz\n"
.end
CODE
ok
baz
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "find_global Foo::Bar::baz alias" );
.sub 'main' :main
    $P0 = find_global "Foo"
    $P1 = $P0["Bar"]
    store_global "TopBar", $P1
    $P2 = find_global "TopBar", "baz"
    print "ok\n"
    $P2()
.end

.namespace ["Foo"; "Bar"]
.sub 'baz'
    print "baz\n"
.end
CODE
ok
baz
OUTPUT

pir_output_like( <<'CODE', <<'OUTPUT', "func() namespace resolution" );
.sub 'main' :main
    print "calling foo\n"
    foo()
    print "calling Foo::foo\n"
    $P0 = find_global "Foo", "foo"
    $P0()
    print "calling baz\n"
    baz()
.end

.sub 'foo'
    print "  foo\n"
    bar()
.end

.sub 'bar'
    print "  bar\n"
.end

.sub 'fie'
    print "  fie\n"
.end

.namespace ["Foo"]

.sub 'foo'
    print "  Foo::foo\n"
    bar()
    fie()
.end

.sub 'bar'
    print "  Foo::bar\n"
.end

.sub 'baz'
    print "  Foo::baz\n"
.end
CODE
/calling foo
  foo
  bar
calling Foo::foo
  Foo::foo
  Foo::bar
  fie
calling baz
Null PMC access in invoke\(\)/
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'get namespace of :anon .sub' );
.namespace ['lib']
.sub main :main :anon
    $P0 = get_namespace
    $P0 = $P0.'get_name'()
    $S0 = join "::", $P0
    say $S0
    end
.end
CODE
parrot::lib
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "get namespace in Foo::bar" );
.sub 'main' :main
    $P0 = find_global "Foo", "bar"
    print "ok\n"
    $P0()
.end

.namespace ["Foo"]
.sub 'bar'
    print "bar\n"
    .include "interpinfo.pasm"
    $P0 = interpinfo .INTERPINFO_CURRENT_SUB
    $P1 = $P0."get_namespace"()
    print $P1
    print "\n"
.end
CODE
ok
bar
Foo
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "get namespace in Foo::Bar::baz" );
.sub 'main' :main
    $P0 = find_global "Foo"
    $P1 = $P0["Bar"]
    $P2 = $P1["baz"]
    print "ok\n"
    $P2()
.end

.namespace ["Foo" ; "Bar"]
.sub 'baz'
    print "baz\n"
    .include "interpinfo.pasm"
    .include "pmctypes.pasm"
    $P0 = interpinfo .INTERPINFO_CURRENT_SUB
    $P1 = $P0."get_namespace"()
    $P2 = $P1.'get_name'()
    $S0 = join '::', $P2
    print $S0
    print "\n"
.end
CODE
ok
baz
parrot::Foo::Bar
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "segv in get_name" );
.namespace ['pugs';'main']
.sub 'main' :main
    $P0 = find_name "&say"
    $P0()
.end
.sub "&say"
    say "ok"
.end
CODE
ok
OUTPUT

pir_output_is( <<'CODE', <<'OUT', "latin1 namespace, global" );
.namespace [ iso-8859-1:"Fran�ois" ]

.sub '__init'
    print "latin1 namespaces are fun\n"
.end

.namespace

.sub 'main' :main
    $P0 = find_global iso-8859-1:"Fran�ois", '__init'
    $P0()
.end
CODE
latin1 namespaces are fun
OUT

pir_output_is( <<'CODE', <<'OUT', "unicode namespace, global" );
.namespace [ unicode:"Fran\xe7ois" ]

.sub '__init'
    print "unicode namespaces are fun\n"
.end

.namespace

.sub 'main' :main
    $P0 = find_global unicode:"Fran\xe7ois", '__init'
    $P0()
.end
CODE
unicode namespaces are fun
OUT

pir_output_is( <<'CODE', <<'OUTPUT', "verify root and parrot namespaces" );
# name may change though
.sub main :main
    # root namespace
    $P0 = get_root_namespace
    typeof $S0, $P0
    print $S0
    print "\n"
    print $P0
    print "\n"
    # parrot namespace
    $P1 = $P0["parrot"]
    print $P1
    print "\n"
    typeof $S0, $P1
    print $S0
    print "\n"
.end
CODE
NameSpace

parrot
NameSpace
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "ns.name()" );
.sub main :main
    .include "interpinfo.pasm"
    $P0 = get_root_namespace
    $P1 = $P0["parrot"]
    $P3 = new .NameSpace
    $P1["Foo"] = $P3
    $P2 = $P3.'get_name'()
    $I2 = elements $P2
    print $I2
    print "\n"
    $S0 = join '::', $P2
    print $S0
    print "\n"
.end
CODE
2
parrot::Foo
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "get_namespace_p_p, getnamespace_p_kc" );
.sub main :main
    .include "interpinfo.pasm"
    $P3 = new .NameSpace
    set_hll_global "Foo", $P3
    # fetch w array
    $P4 = new .FixedStringArray
    $P4 = 1
    $P4[0] = 'Foo'
    $P0 = get_hll_namespace $P4
    $P2 = $P0.'get_name'()
    $I2 = elements $P2
    print $I2
    print "\n"
    $S0 = join '::', $P2
    print $S0
    print "\n"
    # fetch w key
    $P2 = get_hll_namespace ["Foo"]
    $P2 = $P2.'get_name'()
    $I2 = elements $P2
    print $I2
    print "\n"
    $S0 = join '::', $P2
    print $S0
    print "\n"
.end
CODE
2
parrot::Foo
2
parrot::Foo
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "Sub.get_namespace, get_namespace" );
.sub 'main' :main
    $P0 = find_global "Foo", "bar"
    print "ok\n"
    $P1 = $P0."get_namespace"()
    $P2 = $P1.'get_name'()
    $S0 = join '::', $P2
    print $S0
    print "\n"
    $P0()
.end

.namespace ["Foo"]
.sub 'bar'
    $P1 = get_namespace
    print $P1
    print "\n"
.end
CODE
ok
parrot::Foo
Foo
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "check parrot ns" );
.sub 'main' :main
    $P0 = find_global ["String"], "lower"
    $S0 = $P0("OK\n")
    print $S0
.end
CODE
ok
OUTPUT

my $temp_a = "temp_a";
my $temp_b = "temp_b";

END {
    unlink( "$temp_a.pir", "$temp_a.pbc", "$temp_b.pir", "$temp_b.pbc" );
}

open my $S, '>', "$temp_a.pir" or die "Can't write $temp_a.pir";
print $S <<'EOF';
.HLL "Foo", ""
.namespace ["Foo_A"]
.sub loada :load
    $P0 = find_global "Foo_A", "A"
    print "ok 1\n"
    load_bytecode "temp_b.pbc"
.end

.sub A
.end
EOF
close $S;

open $S, '>', "$temp_b.pir" or die "Can't write $temp_b.pir";
print $S <<'EOF';
.namespace ["Foo_B"]
.sub loadb :load
    $P0 = find_global "Foo_B", "B"
    print "ok 2\n"
.end

.sub B
.end
EOF

close $S;

system(".$PConfig{slash}parrot$PConfig{exe} -o $temp_a.pbc $temp_a.pir");
system(".$PConfig{slash}parrot$PConfig{exe} -o $temp_b.pbc $temp_b.pir");

pir_output_is( <<'CODE', <<'OUTPUT', "HLL and load_bytecode - #38888" );
.sub main :main
    load_bytecode "temp_a.pbc"
    print "ok 3\n"
.end
CODE
ok 1
ok 2
ok 3
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "HLL and vars" );
# initial storage of _tcl global variable...

.HLL '_Tcl', ''

.sub huh 
  $P0 = new .Integer
  $P0 = 3.14
  store_global '$variable', $P0
.end

# start running HLL language
.HLL 'Tcl', ''

.sub foo :main
  huh()
  $P1 = get_root_namespace ['_tcl']
  $P2 = $P1['$variable'] 
  print $P2
  print "\n"
.end
CODE
3.14
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "HLL and namespace directives" );
.HLL '_Tcl', ''
.namespace ['Foo'; 'Bar']

.HLL 'Tcl', ''

.sub main :main
  $P0 = get_namespace
  $P1 = $P0.'get_name'()
  $S0 = join "::", $P1
  print $S0
  print "\n"
  end
.end
CODE
tcl
OUTPUT

{
    my $temp_a = "temp_a.pir";

    END {
        unlink($temp_a);
    }

    open $S, '>', $temp_a or die "Can't write $temp_a";
    print $S <<'EOF';
.HLL 'eek', ''

.sub foo :load :anon
  $P1 = new .String
  $P1 = "3.14\n"
  store_global '$whee', $P1
.end

.sub bark
  $P0 = find_global '$whee'
  print $P0
.end
EOF
    close $S;

    pir_output_is( <<'CODE', <<'OUTPUT', ":anon subs still get default namespace" );
.HLL 'cromulent', ''

.sub what
   load_bytecode 'temp_a.pir'
  .local pmc var
   var = get_root_namespace
   var = var['eek']
   var = var['bark']

    var()
.end
CODE
3.14
OUTPUT
}

# the test was skipped, the description says:
# find_global should find from .HLL namespace, not current .namespace
# but according to pdd21, {find,store}_global are relative to current

SKIP:
{
    skip( "immediate test, doesn't with -r (from .pbc)", 1 )
        if ( exists $ENV{TEST_PROG_ARGS} and $ENV{TEST_PROG_ARGS} =~ m/-r/ );

    pir_output_is( <<'CODE', <<'OUTPUT', "find_global in current" );
.HLL 'bork', ''
.namespace

.sub a :immediate
  $P1 = new .String
  $P1 = "ok\n"
  store_global 'sub_namespace', "eek", $P1
## store_global "eek", $P1
.end

.namespace [ 'sub_namespace' ]

.sub whee :main

$P1 = find_global 'eek'
print $P1
.end
CODE
ok
OUTPUT
}

open $S, '>', "$temp_b.pir" or die "Can't write $temp_b.pir";
print $S <<'EOF';
.HLL 'B', ''
.sub b_foo
    print "b_foo\n"
.end
EOF
close $S;

pir_output_like( <<'CODE', <<'OUTPUT', 'export_to() with null destination throws exception' );
.sub 'test' :main
    .local pmc nsa, nsb, ar

    ar = new .ResizableStringArray
    push ar, 'foo'
    nsa = new .Null
    nsb = get_namespace ['B']
    nsb.'export_to'(nsa, ar)
.end

.namespace ['B']
.sub 'foo' :anon
.end
CODE
/^destination namespace not specified\n/
OUTPUT

pir_output_like(
    <<'CODE', <<'OUTPUT', 'export_to() with null array exports default boject set !!!UNSPECIFIED!!!' );
.sub 'test' :main
    .local pmc nsa, nsb, ar

    ar = new .Null
    nsa = get_namespace
    nsb = get_namespace ['B']
    nsb.'export_to'(nsa, ar)
.end

.namespace ['B']
.sub 'foo'
.end
CODE
/^exporting default object set not yet specified\n/
OUTPUT

pir_output_like(
    <<'CODE', <<'OUTPUT', 'export_to() with empty array exports default boject set !!!UNSPECIFIED!!!' );
.sub 'test' :main
    .local pmc nsa, nsb, ar

    ar = new .ResizableStringArray
    nsa = get_namespace
    nsb = get_namespace ['B']
    nsb.'export_to'(nsa, ar)
.end

.namespace ['B']
.sub 'foo'
.end
CODE
/^exporting default object set not yet specified\n/
OUTPUT

pir_output_is( <<"CODE", <<'OUTPUT', "export_to -- success" );
.HLL 'A', ''
.sub main :main
    a_foo()
    load_bytecode "$temp_b.pir"
    .local pmc nsr, nsa, nsb, ar
    ar = new .ResizableStringArray
    push ar, "b_foo"
    nsr = get_root_namespace
    nsa = nsr['a']
    nsb = nsr['b']
    nsb."export_to"(nsa, ar)
    b_foo()
.end

.sub a_foo
    print "a_foo\\n"
.end
CODE
a_foo
b_foo
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "get_parent" );
.sub main :main
    .local pmc ns
    ns = get_hll_namespace ['Foo']
    ns = ns.'get_parent'()
    print ns
    print "\n"
.end
.namespace ['Foo']
.sub dummy
.end
CODE
parrot
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "find_global [''], \"print_ok\"" );
.namespace ['']

.sub print_ok
  print "ok\n"
  .return()
.end

.namespace ['foo']

.sub main :main
  $P0 = find_global [''], 'print_ok'
  $P0()
  end
.end
CODE
ok
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "find_global with array ('')" );
.namespace ['']

.sub print_ok
  print "ok\n"
  .return()
.end

.namespace ['foo']

.sub main :main
  $P0 = new .ResizableStringArray
  $P0[0] = ''
  $P0 = find_global $P0, 'print_ok'
  $P0()
  end
.end
CODE
ok
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "find_global with empty array" );
.namespace

.sub print_ok
  print "ok\n"
  .return()
.end

.namespace ['foo']

.sub main :main
  $P0 = new .ResizablePMCArray
  $P0 = find_global $P0, 'print_ok'
  $P0()
  end
.end
CODE
ok
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "Namespace.get_global() with array ('')" );
.namespace ['']

.sub print_ok
  print "ok\n"
  .return()
.end

.namespace ['foo']

.sub main :main
  $P1 = new .ResizableStringArray
  $P1[0] = ''
  $P1 = get_hll_global $P1, 'print_ok'
  $P1()
  end
.end
CODE
ok
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "Namespace introspection" );
.sub main :main
    .local pmc f
    f = get_hll_global ['Foo'], 'dummy'
    f()
.end
.namespace ['Foo']
.sub dummy
    .local pmc interp, ns_caller
    interp = getinterp
    ns_caller = interp['namespace'; 1]
    print ns_caller
    print "\n"
.end
CODE
parrot
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "Nested namespace introspection" );
.sub main :main
    .local string no_symbol

    .local pmc foo_ns
    foo_ns = get_hll_namespace [ 'Foo' ]
    $S0    = foo_ns
    print "Found namespace: "
    print $S0
    print "\n"

    .local pmc bar_ns
    bar_ns = foo_ns.find_namespace( 'Bar' )
    $S0    = bar_ns
    print "Found nested namespace: "
    print $S0
    print "\n"

    .local pmc baz_ns
    baz_ns    = bar_ns.find_namespace( 'Baz' )
    no_symbol = 'Baz'

    .local int is_defined
    is_defined = defined baz_ns
    if is_defined goto oops
    goto find_symbols

  oops:
    print "Found non-null '"
    print no_symbol
    print "'\n"
    .return()

  find_symbols:
    .local pmc a_sub
    a_sub = bar_ns.find_sub( 'a_sub' )
    $S0   = a_sub
    a_sub()
    print "Found sub: "
    print $S0
    print "\n"

    .local pmc some_sub
    some_sub  = bar_ns.find_sub( 'some_sub' )
    no_symbol = 'some_sub'

    is_defined = defined some_sub
    if is_defined goto oops

    .local pmc a_var
    a_var    = bar_ns.find_var( 'a_var' )
    print "Found var: "
    print a_var
    print "\n"

    .local pmc some_var
    some_var    = bar_ns.find_var( 'some_var' )
    no_symbol = 'some_var'

    is_defined = defined some_var
    if is_defined goto oops

.end

.namespace ['Foo']

.sub some_sub
.end

.namespace [ 'Foo'; 'Bar' ]

.sub a_sub
    .local pmc some_var
    some_var = new .String
    some_var = 'a string PMC'
    store_global [ 'Foo'; 'Bar' ], 'a_var', some_var
.end
CODE
Found namespace: Foo
Found nested namespace: Bar
Found sub: a_sub
Found var: a string PMC
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'get_root_namespace' );
.sub main :main
    .local pmc root_ns
    root_ns = get_root_namespace
    .local int is_defined
    is_defined = defined root_ns
    unless is_defined goto NO_NAMESPACE_FOUND
        print "Found root namespace.\n"
    NO_NAMESPACE_FOUND:
.end
CODE
Found root namespace.
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'get_root_namespace "Foo"' );
.sub main :main
    .local pmc foo_ns
    foo_ns = get_root_namespace [ "foo" ]
    .local int is_defined
    is_defined = defined foo_ns
    unless is_defined goto NO_NAMESPACE_FOUND
        print "Found root namespace 'foo'.\n"
    NO_NAMESPACE_FOUND:
.end
.HLL 'Foo', ''
.sub dummy
.end
CODE
Found root namespace 'foo'.
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'get_root_namespace "Foo", not there' );
.sub main :main
    .local pmc foo_ns
    foo_ns = get_root_namespace [ "Foo" ]
    .local int is_defined
    is_defined = defined foo_ns
    if is_defined goto NAMESPACE_FOUND
        print "Didn't find root namespace 'Foo'.\n"
    NAMESPACE_FOUND:
.end

.namespace [ "NotFoo" ]
CODE
Didn't find root namespace 'Foo'.
OUTPUT

my $create_nested_key = <<'CREATE_NESTED_KEY';
.sub create_nested_key
    .param string name
    .param pmc other_names :slurpy

    .local pmc key
    key = new .Key
    key = name

    .local int elem
    elem = other_names

    if elem goto nested
    .return( key )

  nested:
    .local pmc tail
    tail = create_nested_key(other_names :flat)
    push key, tail

    .return( key )
.end
CREATE_NESTED_KEY

pir_output_is( <<"CODE", <<'OUTPUT', 'get_name()' );
$create_nested_key

.sub main :main
    .local pmc key
    key = create_nested_key( 'SingleName' )
    print_namespace( key )

    key = create_nested_key( 'Nested', 'Name', 'Space' )
    print_namespace( key )

    key = get_namespace

    .local pmc ns
    ns = key.'get_name'()

    .local string ns_name
    ns_name = join ';', ns
    print ns_name
    print "\\n"
.end

.sub 'print_namespace'
    .param pmc key

    .local pmc get_ns
    get_ns = find_global key, 'get_namespace'

    .local pmc ns
    ns = get_ns()

    .local pmc name_array
    name_array = ns.'get_name'()

    .local string name
    name = join ';', name_array

    print name
    print "\\n"
.end

.sub get_namespace
    .local pmc ns
    ns = get_namespace
    .return( ns )
.end

.namespace [ 'SingleName' ]

.sub get_namespace
    .local pmc ns
    ns = get_namespace
    .return( ns )
.end

.namespace [ 'Nested'; 'Name'; 'Space' ]

.sub get_namespace
    .local pmc ns
    ns = get_namespace
    .return( ns )
.end

CODE
parrot;SingleName
parrot;Nested;Name;Space
parrot
OUTPUT

pir_output_is( <<"CODE", <<'OUTPUT', 'add_namespace()' );
$create_nested_key

.sub main :main
    .local pmc root_ns
    root_ns = get_namespace

    .local pmc child_ns
    child_ns = new .NameSpace
    root_ns.'add_namespace'( 'Nested', child_ns )

    .local pmc grandchild_ns
    grandchild_ns = new .NameSpace
    child_ns.'add_namespace'( 'Grandkid', grandchild_ns )

    .local pmc great_grandchild_ns
    great_grandchild_ns = new .NameSpace
    grandchild_ns.'add_namespace'( 'Greatgrandkid', great_grandchild_ns )

    .local pmc parent
    parent = great_grandchild_ns.'get_parent'()
    print_ns_name( parent )

    parent = parent.'get_parent'()
    print_ns_name( parent )

    parent = parent.'get_parent'()
    print_ns_name( parent )
.end

.sub print_ns_name
    .param pmc namespace

    .local pmc ns
    ns = namespace.'get_name'()

    .local string ns_name
    ns_name = join ';', ns
    print ns_name
    print "\\n"
.end
CODE
parrot;Nested;Grandkid
parrot;Nested
parrot
OUTPUT

pir_output_like( <<'CODE', <<'OUTPUT', 'add_namespace() with error' );
.sub main :main
    .local pmc ns_child
    ns_child = subclass 'NameSpace', 'NSChild'

    .local pmc child
    child = new 'NSChild'

    .local pmc root_ns
    root_ns = get_namespace

    root_ns.'add_namespace'( 'Really nested', child )

    .local pmc not_a_ns
    not_a_ns = new .Integer

    push_eh _invalid_ns
    root_ns.'add_namespace'( 'Nested', not_a_ns )
    end

_invalid_ns:
    .local pmc exception
    .local string message
    .get_results( exception, message )

    print message
    print "\n"
.end
CODE
/Invalid type \d+ in add_namespace\(\)/
OUTPUT

pir_output_is( <<"CODE", <<'OUTPUT', 'add_sub()' );
$create_nested_key

.sub 'main' :main
    .local pmc report_ns
    report_ns = find_global 'report_namespace'

    .local pmc key
    key = create_nested_key( 'Parent' )

    .local pmc parent_ns
    parent_ns = get_namespace key
    parent_ns.'add_sub'( 'report_ns', report_ns )

    key = create_nested_key( 'Parent', 'Child' )

    .local pmc child_ns
    child_ns = get_namespace key
    child_ns.'add_sub'( 'report_ns', report_ns )

    .local pmc report_namespace
    report_namespace = find_global [ 'Parent' ], 'report_ns'
    report_namespace()

    report_namespace = find_global [ 'Parent'; 'Child' ], 'report_ns'
    report_namespace()
.end

.sub 'report_namespace'
    .local pmc namespace
    namespace = get_namespace

    .local pmc ns
    ns = namespace.'get_name'()

    .local string ns_name
    ns_name = join ';', ns
    print ns_name
    print "\\n"
.end

.namespace [ 'Parent' ]

.sub dummy
.end

.namespace [ 'Parent'; 'Child' ]

.sub dummy
.end
CODE
parrot
parrot
OUTPUT

pir_output_like( <<'CODE', <<'OUTPUT', 'add_sub() with error' );
.sub main :main
    .local pmc s_child
    s_child = subclass 'Sub', 'SubChild'

    .local pmc e_child
    e_child = subclass 'Closure', 'ClosureChild'

    .local pmc child
    child = new 'SubChild'

    .local pmc root_ns
    root_ns = get_namespace

    root_ns.'add_sub'( 'child', child )
    print "Added sub child\n"

    child = new 'Closure'
    root_ns.'add_sub'( 'closure', child )
    print "Added closure\n"

    child = new 'Coroutine'
    root_ns.'add_sub'( 'coroutine', child )
    print "Added coroutine\n"

    child = new 'Eval'
    root_ns.'add_sub'( 'eval', child )
    print "Added eval\n"

    child = new 'ClosureChild'
    root_ns.'add_sub'( 'closure_child', child )
    print "Added closure child\n"

    .local pmc not_a_sub
    not_a_sub = new .Integer

    push_eh _invalid_sub
    root_ns.'add_sub'( 'Nested', not_a_sub )
    end

_invalid_sub:
    .local pmc exception
    .local string message
    .get_results( exception, message )

    print message
    print "\n"
.end
CODE
/Added sub child
Added closure
Added coroutine
Added eval
Added closure child
Invalid type \d+ in add_sub\(\)/
OUTPUT

pir_output_is( <<"CODE", <<'OUTPUT', 'add_var()' );
$create_nested_key

.sub 'main' :main
    .local pmc foo
    foo = new .String
    foo = 'Foo'

    .local pmc bar
    bar = new .String
    bar = 'Bar'

    .local pmc key
    key = create_nested_key( 'Parent' )

    .local pmc parent_ns
    parent_ns = get_namespace key
    parent_ns.'add_var'( 'foo', foo )

    key = create_nested_key( 'Parent', 'Child' )

    .local pmc child_ns
    child_ns = get_namespace key
    child_ns.'add_var'( 'bar', bar )

    .local pmc my_var
    my_var = find_global [ 'Parent' ], 'foo'
    print "Foo: "
    print my_var
    print "\\n"

    my_var = find_global [ 'Parent'; 'Child' ], 'bar'
    print "Bar: "
    print my_var
    print "\\n"
.end

.namespace [ 'Parent' ]

.sub dummy
.end

.namespace [ 'Parent'; 'Child' ]

.sub dummy
.end
CODE
Foo: Foo
Bar: Bar
OUTPUT

pir_output_is( <<"CODE", <<'OUTPUT', 'del_namespace()' );
$create_nested_key

.sub 'main' :main
    .local pmc root_ns
    root_ns = get_namespace

    .local pmc key
    key      = create_nested_key( 'Parent' )

    .local pmc child_ns
    child_ns = root_ns.'find_namespace'( key )

    key      = create_nested_key( 'Child' )

    .local pmc grandchild_ns
    grandchild_ns = child_ns.'find_namespace'( key )

    child_ns.'del_namespace'( 'Child' ) 

    key      = create_nested_key( 'Child' )

    .local pmc grandchild_ns
    grandchild_ns = child_ns.'find_namespace'( key )
    if_null grandchild_ns, CHECK_SIBLING
    print "Grandchild still exists\\n"

  CHECK_SIBLING:
    key      = create_nested_key( 'Sibling' )
    grandchild_ns = child_ns.'find_namespace'( key )
    if_null grandchild_ns, DELETE_PARENT
    print "Sibling not deleted\\n"

  DELETE_PARENT:
    key      = create_nested_key( 'Parent' )
    root_ns.'del_namespace'( 'Parent' )
    child_ns = root_ns.'find_namespace'( key )
    if_null child_ns, CHECK_UNCLE
    print "Child still exists\\n"

  CHECK_UNCLE:
    key      = create_nested_key( 'FunUncle' )
    grandchild_ns = root_ns.'find_namespace'( key )
    if_null grandchild_ns, DELETE_PARENT
    print "Fun uncle stuck around\\n"

  ALL_DONE:
.end

.namespace [ 'FunUncle' ]

.sub dummy
.end

.namespace [ 'Parent' ]

.sub dummy
.end

.namespace [ 'Parent'; 'Child' ]

.sub dummy
.end

.namespace [ 'Parent'; 'Sibling' ]

.sub dummy
.end
CODE
Sibling not deleted
Fun uncle stuck around
OUTPUT

pir_output_like( <<'CODE', <<'OUTPUT', 'del_namespace() with error' );
.sub dummy
.end

.sub main :main
    .local pmc not_a_ns
    not_a_ns = new .Array

    store_global 'Not_A_NS', not_a_ns

    .local pmc root_ns
    root_ns = get_namespace
    delete_namespace( root_ns, 'dummy' )
    delete_namespace( root_ns, 'Not_A_NS' )
.end

.sub delete_namespace
    .param pmc    root_ns
    .param string name
    push_eh _invalid_ns
    root_ns.'del_namespace'( name )

_invalid_ns:
    .local pmc exception
    .local string message
    .get_results( exception, message )

    print message
    print "\n"
    .return()
.end
CODE
/Invalid type \d+ for 'dummy' in del_namespace\(\)
Invalid type \d+ for 'Not_A_NS' in del_namespace\(\)/
OUTPUT

pir_output_is( <<"CODE", <<'OUTPUT', 'del_sub()' );
.sub 'main' :main
    .local pmc root_ns
    root_ns = get_namespace

    .local pmc parent_ns
    parent_ns = root_ns.'find_namespace'( 'Parent' )
    parent_ns.'del_sub'( 'dummy' )

    .local pmc my_sub   
    my_sub = find_global [ 'Parent' ], 'dummy'
    if_null my_sub, PARENT_NO_DUMMY
    print "Parent did not delete dummy\\n"

  PARENT_NO_DUMMY:
    my_sub = find_global [ 'Parent' ], 'no_dummy'
    my_sub()

    .local pmc child_ns
    child_ns = parent_ns.'find_namespace'( 'Child' )
    child_ns.'del_sub'( 'dummy' )

    .local pmc my_sub   
    my_sub = find_global [ 'Parent'; 'Child' ], 'dummy'
    if_null my_sub, CHILD_NO_DUMMY
    print "Child did not delete dummy\\n"
    my_sub()

  CHILD_NO_DUMMY:
    my_sub = find_global [ 'Parent'; 'Child' ], 'no_dummy'
    my_sub()
.end

.namespace [ 'Parent' ]

.sub dummy
.end

.sub no_dummy
    print "Parent is no dummy\\n"
.end

.namespace [ 'Parent'; 'Child' ]

.sub dummy
    print "Dummy sub!\\n"
.end

.sub no_dummy
    print "Child is no dummy\\n"
.end

CODE
Parent is no dummy
Child is no dummy
OUTPUT

pir_output_like( <<'CODE', <<'OUTPUT', 'del_sub() with error' );
.sub main :main
    .local pmc not_a_ns
    not_a_ns = new .Array

    store_global 'Not_A_Sub', not_a_ns

    .local pmc root_ns
    root_ns = get_namespace

    push_eh _invalid_sub
    root_ns.'del_sub'( 'Not_A_Sub' )

_invalid_sub:
    .local pmc exception
    .local string message
    .get_results( exception, message )

    print message
    print "\n"
    .return()
.end
CODE
/Invalid type \d+ for 'Not_A_Sub' in del_sub\(\)/
OUTPUT

pir_output_is( <<"CODE", <<'OUTPUT', 'del_var()' );
.sub 'main' :main
    .local pmc foo
    foo = new .String
    foo = 'Foo'

    .local pmc bar
    bar = new .String
    bar = 'Bar'

    store_global [ 'Parent' ],          'Foo', foo
    store_global [ 'Parent'; 'Child' ], 'Bar', bar

    .local pmc root_ns
    root_ns = get_namespace

    .local pmc parent_ns
    parent_ns = root_ns.'find_namespace'( 'Parent' )
    parent_ns.'del_var'( 'Foo' )

    .local pmc child_ns
    child_ns = parent_ns.'find_namespace'( 'Child' )
    child_ns.'del_var'( 'Bar' )

    .local pmc my_var
    my_var = find_global [ 'Parent' ], 'Foo'
    if_null my_var, TEST_CHILD_VAR
    print "Parent Foo exists: "
    print my_var
    print "\\n"

  TEST_CHILD_VAR:
    my_var = find_global [ 'Parent'; 'Child' ], 'Bar'
    if_null my_var, ALL_DONE
    print "Child Bar exists: "
    print my_var
    print "\\n"

  ALL_DONE:
.end

.namespace [ 'Parent' ]

.sub dummy
.end

.namespace [ 'Parent'; 'Child' ]

CODE
OUTPUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
