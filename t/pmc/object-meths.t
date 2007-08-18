#! perl
# Copyright (C) 2001-2007, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 44;

=head1 NAME

t/pmc/object-meths.t - Object Methods

=head1 SYNOPSIS

    % prove t/pmc/object-meths.t

=head1 DESCRIPTION

Tests PMC object methods.

=cut

pasm_error_output_like( <<'CODE', <<'OUTPUT', "callmethod - unknown method" );
    newclass P2, "Foo"
    set S0, "nada"
    callmethodcc P2, S0
    print "nope\n"
    end
CODE
/Method 'nada' not found/
OUTPUT

pasm_error_output_like( <<'CODE', <<'OUTPUT', "callmethod (STR) - unknown method" );
    newclass P2, "Foo"
    set S1, "nada"
    callmethod P2, S1, P1
    print "nope\n"
    end
CODE
/Method 'nada' not found/
OUTPUT

pasm_error_output_like( <<'CODE', <<'OUTPUT', "callmethodcc - unknown method" );
    newclass P2, "Foo"
    set S0, "nada"
    callmethodcc P2, S0
    print "nope\n"
    end
CODE
/Method 'nada' not found/
OUTPUT

pasm_error_output_like( <<'CODE', <<'OUTPUT', "callmethodcc (STR) - unknown method" );
    newclass P2, "Foo"
    set S1, "nada"
    callmethodcc P2, S1
    print "nope\n"
    end
CODE
/Method 'nada' not found/
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "callmethod 1" );
    newclass P2, "Foo"
    set S0, "meth"

    print "main\n"
    callmethodcc P2, S0
    print "back\n"
    end

.namespace ["Foo"]
.pcc_sub meth:
    print "in meth\n"
    returncc
CODE
main
in meth
back
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "can class" );
    newclass P2, "Foo"
    set S0, "meth"

    new P3, 'Sub'
    # store the sub with the real name
    store_global "Foo", "meth", P3

    can I0, P2, "meth"
    print I0
    print "\n"
    can I0, P2, "no_such_meth"
    print I0
    print "\n"
    end
CODE
1
0
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "can object" );
    newclass P2, "Foo"
    find_type I0, "Foo"
    new P2, I0

    set S0, "meth"

    new P3, 'Sub'
    # store the sub with the real name
    store_global "Foo", "meth", P3

    can I0, P2, "meth"
    print I0
    print "\n"
    can I0, P2, "no_such_meth"
    print I0
    print "\n"
    end
CODE
1
0
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "constructor" );
    newclass P1, "Foo"
    find_type I1, "Foo"
    new P3, I1
    print "ok 2\n"
    end
.namespace ["Foo"]
.pcc_sub __init:
    get_params "(0)", P2
    print "ok 1\n"
    returncc
CODE
ok 1
ok 2
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "disabling the constructor" );
    newclass P1, "Foo"
    new P0, 'String'
    setprop P1, "BUILD", P0
    find_type I1, "Foo"
    new P3, I1
    print "ok 1\n"
    end
.namespace ["Foo"]
.pcc_sub __init:
    print "nok ok!\n"
    returncc
CODE
ok 1
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "specified constructor method does not exist" );
    newclass P1, "Foo"
    new P0, 'String'
    set P0, "bar"
    setprop P1, "BUILD", P0

    push_eh _handler

    find_type I1, "Foo"
    new P3, I1
    print "not ok 1\n"
    end

_handler:
    get_results "(0,0)", P0, S0
    print "catched it\n"
    print S0
    print "\n"
    end

.namespace ["Foo"]
.pcc_sub __init:
    print "nok ok 2!\n"
    returncc
CODE
catched it
Class BUILD method ('bar') not found
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "constructor - init attr" );
    newclass P1, "Foo"
    addattribute P1, ".i"
    find_type I1, "Foo"
    new P3, I1
    print "ok 2\n"
    print P3
    print "\n"
    end
.namespace ["Foo"]
.pcc_sub __init:
    get_params "(0)", P2
    print "ok 1\n"
    new P10, 'Integer'
    set P10, 42
    classoffset I0, P2, "Foo"
    setattribute P2, I0, P10
    set_returns "()"
    returncc
.pcc_sub __get_string:
    get_params "(0)", P2
    classoffset I0, P2, "Foo"
    getattribute P10, P2, I0
    set_returns "(0)", P10
    returncc
CODE
ok 1
ok 2
42
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "constructor - parents" );
    newclass P1, "Foo"
    subclass P2, P1, "Bar"
    subclass P3, P2, "Baz"
    find_type I1, "Baz"
    new P3, I1
    find_type I1, "Bar"
    new P3, I1
    find_global P0, "_sub"
    invokecc P0
    print "done\n"
    end

    .namespace ["Foo"]
.pcc_sub __init:
    get_params "(0)", P2
    print "foo_init\n"
    classname S0, P2
    print S0
    print "\n"
    returncc

    .namespace ["Bar"]
.pcc_sub __init:
    get_params "(0)", P2
    print "bar_init\n"
    returncc

    .namespace ["Baz"]
.pcc_sub __init:
    get_params "(0)", P2
    print "baz_init\n"
    returncc

    .namespace	# main again
.pcc_sub _sub:
    print "in sub\n"
    returncc

CODE
foo_init
Baz
bar_init
baz_init
foo_init
Bar
bar_init
in sub
done
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "methods: self" );

.sub _main
    .local pmc A
    .local pmc B

    newclass A, "A"
    newclass B, "B"

    find_type I0, "A"
    find_type I1, "B"

    new A, I0
    new B, I1

    setprop A, "B", B

    A."foo"()
    B."foo"()

    end
.end

.namespace ["A"]

.sub foo :method
    .local pmc B

    print "A::foo\n"
    getprop B, "B", self

    self."blah"()
    B."blah"()
    self."blah"()

.end

.sub blah :method
    print "A::blah\n"
.end

.namespace ["B"]

.sub foo :method
    print "B::foo\n"
.end

.sub blah :method
    print "B::blah\n"
.end
CODE
A::foo
A::blah
B::blah
A::blah
B::foo
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "methods: self w arg" );

.sub _main
    .local pmc A
    .local pmc B

    newclass A, "A"
    newclass B, "B"

    find_type I0, "A"
    find_type I1, "B"

    new A, I0
    new B, I1

    A."foo"(B)
    B."foo"()

    end
.end

.namespace ["A"]

.sub foo :method
    .param pmc B

    print "A::foo\n"

    self."blah"()
    B."blah"()
    self."blah"()

.end

.sub blah :method
    print "A::blah\n"
.end

.namespace ["B"]

.sub foo :method
    print "B::foo\n"
.end

.sub blah :method
    print "B::blah\n"
.end
CODE
A::foo
A::blah
B::blah
A::blah
B::foo
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "methods: self w arg and ret" );

.sub _main
    .local pmc A
    .local pmc B

    newclass A, "A"
    newclass B, "B"

    find_type I0, "A"
    find_type I1, "B"

    new A, I0
    new B, I1

    .local pmc r
    r = A."foo"(B)
    r."foo"()

    end
.end

.namespace ["A"]

.sub foo :method
    .param pmc B

    print "A::foo\n"

    self."blah"()
    B."blah"()
    self."blah"()
    .pcc_begin_return
    .return B
    .pcc_end_return

.end

.sub blah :method
    print "A::blah\n"
.end

.namespace ["B"]

.sub foo :method
    print "B::foo\n"
.end

.sub blah :method
    print "B::blah\n"
.end
CODE
A::foo
A::blah
B::blah
A::blah
B::foo
OUTPUT

SKIP: {
    skip( "currently broken", 1 );
    pasm_output_is( <<'CODE', <<'OUTPUT', "exceptions and different runloops" );
_main:
    push_eh eh

    newclass P0, "Foo"

    print "new\n"
    find_type I0, "Foo"
    new P2, I0
eh:
    print "back in main\n"
    end

.namespace ["Foo"]
.pcc_sub __init:
    print "in __init\n"

    # raise an exception
    callmethodcc self, "qux"

    print "never\n"
    returncc
CODE
new
in __init
back in main
OUTPUT
}

pasm_output_is( <<'CODE', <<'OUTPUT', "find_method" );
    newclass P3, "Foo"
    find_type I0, "Foo"
    new P2, I0

    set S0, "meth"
    find_method P0, P2, S0
    print "main\n"
    callmethodcc P2, P0
    print "back\n"
    # check class
    find_method P0, P3, S0
    callmethodcc P3, P0
    print "back\n"
    end

.namespace ["Foo"]
.pcc_sub meth:
    print "in meth\n"
    returncc
CODE
main
in meth
back
in meth
back
OUTPUT

pasm_error_output_like( <<'CODE', <<'OUTPUT', "find_method - unknown method" );
    newclass P2, "Foo"
    set S0, "nada"
    find_method P0, P2, S0
    print "nope\n"
    end
CODE
/Method 'nada' not found/
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "constructor - diamond parents" );
#
# A   B A   E
#  \ /   \ /
#   C     D
#    \   /
#     \ /
#      F
    newclass P1, "A"
    newclass P2, "B"
    subclass P3, P1, "C"
    addparent P3, P2

    subclass P4, P1, "D"
    newclass P5, "E"
    addparent P4, P5

    subclass P6, P3, "F"
    addparent P6, P4
    bsr _check_isa

    print "new F\n"
    find_type I1, "F"
    new P16, I1
    print "done\n"
    end

_check_isa:
    print "F isa D "
    isa I0, P6, "D"
    print I0
    print "\n"
    print "D isa F "
    isa I0, P4, "F"
    print I0
    print "\n"
    print "F isa C "
    isa I0, P6, "C"
    print I0
    print "\n"
    print "C isa F "
    isa I0, P3, "F"
    print I0
    print "\n"
    print "F isa E "
    isa I0, P6, "E"
    print I0
    print "\n"
    print "E isa F "
    isa I0, P5, "F"
    print I0
    print "\n"
    print "F isa A "
    isa I0, P6, "A"
    print I0
    print "\n"
    print "A isa F "
    isa I0, P1, "F"
    print I0
    print "\n"
    print "F isa B "
    isa I0, P6, "B"
    print I0
    print "\n"
    print "B isa F "
    isa I0, P2, "F"
    print I0
    print "\n"

    print "C isa A "
    isa I0, P3, "A"
    print I0
    print "\n"
    print "A isa C "
    isa I0, P1, "C"
    print I0
    print "\n"
    print "D isa A "
    isa I0, P4, "A"
    print I0
    print "\n"
    print "A isa D "
    isa I0, P1, "D"
    print I0
    print "\n"
    ret


.namespace ["A"]
.pcc_sub __init:
    get_params "(0)", P2
    print "A init\n"
    returncc
.namespace ["B"]
.pcc_sub __init:
    get_params "(0)", P2
    print "B init\n"
    returncc
.namespace ["C"]
.pcc_sub __init:
    get_params "(0)", P2
    print "C init\n"
    returncc
.namespace ["D"]
.pcc_sub __init:
    get_params "(0)", P2
    print "D init\n"
    returncc
.namespace ["E"]
.pcc_sub __init:
    get_params "(0)", P2
    print "E init\n"
    returncc
.namespace ["F"]
.pcc_sub __init:
    get_params "(0)", P2
    print "F init\n"
    returncc
CODE
F isa D 1
D isa F 0
F isa C 1
C isa F 0
F isa E 1
E isa F 0
F isa A 1
A isa F 0
F isa B 1
B isa F 0
C isa A 1
A isa C 0
D isa A 1
A isa D 0
new F
E init
B init
A init
D init
C init
F init
done
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "constructor - parents BUILD" );
    new P10, 'String'
    set P10, "_new"
    newclass P1, "Foo"
    setprop P1, "BUILD", P10
    subclass P2, P1, "Bar"
    setprop P2, "BUILD", P10
    subclass P3, P2, "Baz"
    setprop P3, "BUILD", P10
    find_type I1, "Baz"
    new P3, I1
    find_type I1, "Bar"
    new P3, I1
    find_global P0, "_sub"
    invokecc P0
    print "done\n"
    end

    .namespace ["Foo"]
.pcc_sub _new:
    get_params "(0)", P2
    print "foo_init\n"
    classname S0, P2
    print S0
    print "\n"
    returncc

    .namespace ["Bar"]
.pcc_sub _new:
    get_params "(0)", P2
    print "bar_init\n"
    returncc

    .namespace ["Baz"]
.pcc_sub _new:
    get_params "(0)", P2
    print "baz_init\n"
    returncc

    .namespace	# main again
.pcc_sub _sub:
    print "in sub\n"
    returncc

CODE
foo_init
Baz
bar_init
baz_init
foo_init
Bar
bar_init
in sub
done
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "constructor - vtable override" );
.sub main :main
  $P0 = newclass 'Foo'
  $P1 = subclass 'Foo', 'Bar'
  $P2 = new 'Bar'
.end

.namespace ['Foo']
.sub init :vtable :method
  print "foo init\n"
.end

.namespace ['Bar']
.sub init :vtable :method
  print "bar init\n"
.end

CODE
foo init
bar init
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "same method name in two namespaces" );

.namespace ["A"]
.sub foo :method
    .param int i

    .pcc_begin_return
    .pcc_end_return
.end

.namespace ["B"]
.sub foo :method
    .param int i

    .pcc_begin_return
    .pcc_end_return
.end

.namespace
.sub _main :main
    print "ok\n"
.end
CODE
ok
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "self - CURRENT_OBJECT" );

.sub _main
    .local pmc A

    newclass A, "A"
    find_type I0, "A"

    new A, I0
    A."foo"()
    end
.end

.namespace ["A"]

.sub foo :method
    .include "interpinfo.pasm"
    $P0 = interpinfo .INTERPINFO_CURRENT_OBJECT
    eq_addr self, $P0, ok
    print "not "
ok: print "ok\n"
.end
CODE
ok
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "Bug in method calling with nonconst keys" );

.sub _main
    newclass $P0, "Foo"

    find_type $I0, "Foo"
    new $P1, $I0

    $I1 = $P1["foo"]

    $S0 = "foo"
    $I1 = $P1[$S0]

    end
.end

.namespace ["Foo"]

.sub get_integer_keyed :vtable :method
    .param pmc key
    print "Key = "
    print key
    print "\n"
    .return(0)
.end
CODE
Key = foo
Key = foo
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "Bug in method calling with nonconst keys - clobber" );

.sub _main
    newclass $P0, "Foo"

    find_type $I0, "Foo"
    new $P1, $I0

    $I1 = $P1["foo"]

    $S0 = "foo"
    $I1 = $P1[$S0]

    end
.end

.namespace ["Foo"]

.sub get_integer_keyed :vtable :method
    .param pmc key
    $S0 = "bar"
    print "Key = "
    print key
    print "\n"
    print $S0
    print "\n"
    .return(0)
.end
CODE
Key = foo
bar
Key = foo
bar
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "method cache invalidation" );
.sub main :main
    .local pmc o, cl
    newclass cl, "Foo"
    subclass cl, cl, "Bar"
    $I0 = find_type "Bar"
    o = new $I0
    print o
    $P0 = global "ok2"
    store_global "Bar", "__get_string", $P0
    print o
.end
.sub ok2 :method
    .return("ok 2\n")
.end
.namespace [ "Foo" ]
.sub __get_string :method
    .return("ok 1\n")
.end
CODE
ok 1
ok 2
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "callmethod - method name" );
    newclass P2, "Foo"
    set S0, "meth"

    print "main\n"
    callmethodcc P2, S0
    print "back\n"
    end

.namespace ["Foo"]
.pcc_sub meth:
    print "in meth\n"
    getinterp P0
    set P1, P0["sub"]
    print P1
    print "\n"
    returncc
CODE
main
in meth
meth
back
OUTPUT

SKIP: {
    skip( "no bound NCI method", 1 );
    pir_output_is( <<'CODE', <<'OUTPUT', "bound NCI method" );
.sub main :main
    .local pmc s, l, f
    s = new 'String'
    s = "ABC\n"
    f = getattribute s, "lower"
    typeof $S0, f
    print $S0
    print "\n"
    l = f()
    print l
.end
CODE
Bound_NCI
abc
OUTPUT
}

pir_output_is( <<'CODE', <<'OUTPUT', "tailcallmeth" );
.sub main :main
    .local pmc cl, o, n
    cl = newclass "Foo"
    addattribute cl, "n"
    o = new "Foo"
    n = new 'Integer'
    n = 2000
    setattribute o, "Foo\0n", n
    o.go()
    n = getattribute o, "Foo\0n"
    print n
    print "\n"
.end

.namespace ["Foo"]
.sub go :method
    .local pmc n
    n = getattribute self, "Foo\0n"
    dec n
    unless n goto done
    .return self."go"()
done:
.end
CODE
0
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "kind of a super" );
.sub main :main
    .local pmc cl, o
    cl = subclass "String", "MyString"
    o = new "MyString"
    o = "foo"
    print o
    print "\n"
.end

.namespace ["MyString"]
.sub set_string_native :vtable :method
    .param string s
    classoffset $I0, self, "MyString"
    $P0 = getattribute self, $I0
    s .= s
    $P0 = s
.end
CODE
foofoo
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', ".Super - test dispatch with two classes" );
.sub main :main
    .local pmc o, cl
    cl = newclass 'Parent'
    cl = subclass cl, 'Child'
    o = new 'Child'
    o."foo"()
.end

.namespace ['Parent']
.sub foo :method
    print "Parent foo\n"
    self."bar"()
.end
.sub bar :method
    print "Parent bar\n"
.end

.namespace ['Child']
.sub foo :method
    print "Child foo\n"
    .local pmc s
    s = new 'Super', self
    s."foo"()
.end
CODE
Child foo
Parent foo
Parent bar
OUTPUT

TODO: {
    local $TODO = "3 class interitance, 4-class diamond inheritance";
    pir_output_is( <<'CODE', <<'OUTPUT', ".Super - dispatch on addparent-established heirarchy" );
.sub main :main
    .local pmc o, p, c
    p = newclass 'Parent'
    c = newclass 'Child'
    addparent c, p
    o = new 'Child'
    o."foo"()
.end

.namespace ['Parent']
.sub foo :method
    print "Parent foo\n"
    self."bar"()
.end
.sub bar :method
    print "Parent bar\n"
.end

.namespace ['Child']
.sub foo :method
    print "Child foo\n"
    .local pmc s
    s = new 'Super', self
    s."foo"()
.end
CODE
Child foo
Parent foo
Parent bar
OUTPUT

    pir_output_is( <<'CODE', <<'OUTPUT', ".Super - subclass established, three levels deep" );
.sub main :main
    .local pmc o, p, c, g
    p = newclass 'Parent'
    c = subclass p, 'Child'
    g = subclass c, 'GrandChild'
    o = new 'GrandChild'
    o."foo"()
.end

.namespace ['Parent']
.sub foo :method
    print "Parent foo\n"
    self."foo"()
.end

.namespace ['Child']
.sub foo :method
    print "Child foo\n"
    .local pmc s
    s = new 'Super', self
    s."foo"()
.end

.namespace ['GrandChild']
.sub foo :method
    print "GrandChild foo\n"
    .local pmc s
    s = new 'Super', self
    s."foo"()
.end

CODE
GrandChild foo
Child foo
Parent foo
OUTPUT

    pir_output_is( <<'CODE', <<'OUTPUT', ".Super - simple 4-class diamond C3 dispatch" );
.sub main :main
    .local pmc o, p, c1, c2, i
    p = newclass 'Parent'
    c1 = subclass p, 'Child1'
    c2 = subclass p, 'Child2'
    i = subclass c1, 'Inbred'
    addparent i, c2
    o = new 'Inbred'
    o."foo"()
.end

.namespace ['Parent']
.sub foo :method
    print "Parent foo\n"
    self."foo"()
.end

.namespace ['Child1']
.sub foo :method
    print "Child1 foo\n"
    .local pmc s
    s = new 'Super', self
    s."foo"()
.end

.namespace ['Child2']
.sub foo :method
    print "Child2 foo\n"
    .local pmc s
    s = new 'Super', self
    s."foo"()
.end

.namespace ['Inbred']
.sub foo :method
    print "Inbred foo\n"
    .local pmc s
    s = new 'Super', self
    s."foo"()
.end

CODE
Inbred foo
Child1 foo
Child2 foo
Parent foo
OUTPUT
}

pir_output_is( <<'CODE', <<'OUTPUT', "delegate keyed_int" );
.sub main :main
    .local pmc cl, o
    cl = newclass "MyClass"
    o = new "MyClass"
    $I0 = 5
    $S0 = "foo"
    o[$I0] = 42
    o[$S0] = 42
    delete o[$I0]
    delete o[$S0]
    $I1 = defined o[$I0]
    $I1 = defined o[$S0]
    $I1 = exists o[$I0]
    $I1 = exists o[$S0]
.end

.namespace ["MyClass"]

.sub set_integer_keyed_int :vtable :method
    .param int key
    .param int val
    print "ikey\n"
.end

.sub set_integer_keyed :vtable :method
    .param string key
    .param int val
    print "skey\n"
.end

.sub delete_keyed_int :vtable :method
    .param int key
    print "del_ikey\n"
.end

.sub delete_keyed :vtable :method
    .param string key
    print "del_skey\n"
.end

.sub defined_keyed_int :vtable :method
    .param int key
    print "def_ikey\n"
    .return (0)
.end

.sub defined_keyed :vtable :method
    .param string key
    print "def_skey\n"
    .return (0)
.end

.sub exists_keyed_int :vtable :method
    .param int key
    print "exists_ikey\n"
    .return (0)
.end

.sub exists_keyed :vtable :method
    .param string key
    print "exists_skey\n"
    .return (0)
.end

CODE
ikey
skey
del_ikey
del_skey
def_ikey
def_skey
exists_ikey
exists_skey
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "delegate keyed_int PMC derived" );
.sub main :main
    .local pmc cl, o
    cl = subclass "ResizablePMCArray", "MyClass"
    o = new "MyClass"
    $I0 = 5
    o[$I0] = 42
    $I1 = o[$I0]
    print $I1
    print "\n"
.end

.namespace ["MyClass"]

.sub get_integer_keyed_int :vtable :method
    .param int key
    print "ikey\n"
    .local pmc ar
    ar = getattribute self, "__value"
    $I0 = ar[key]
    .return ($I0)
.end

.sub set_integer_keyed_int :vtable :method
    .param int key
    .param int val
    print "pkey\n"
    .local pmc ar
    ar = getattribute self, "__value"
    ar[key] = val
.end

CODE
pkey
ikey
42
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "delegate keyed_int PMC derived - inherit" );
.sub main :main
    .local pmc cl, o
    cl = subclass "ResizablePMCArray", "MyClass"
    o = new "MyClass"
    $I0 = 5
    o[$I0] = 42
    $I1 = o[$I0]
    print $I1
    print "\n"
.end

.namespace ["MyClass"]

.sub get_integer_keyed_int :vtable :method
    .param int key
    print "ikey\n"
    .local pmc ar
    ar = getattribute self, "__value"
    $I0 = ar[key]
    .return ($I0)
.end

CODE
ikey
42
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "addmethod op" );
.sub main :main
    .local pmc c
    c = newclass ['whatever']
    .const .Sub foo = "whatever_foo"
    addmethod c, "foo", foo
    $P0 = new ['whatever']
    $P0.'foo'()
.end

.sub whatever_foo :anon :method
    print "Foo!\n"
.end
CODE
Foo!
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "inherit a PMC METHOD" );
.sub main :main
    .local pmc cl, o
    cl = subclass 'Integer', 'MyInt'
    o = new 'MyInt'
    o = 10
    $S0 = o.'get_as_base'(16)
    print $S0
    print "\n"
.end
CODE
a
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "init calls" );
.sub main :main
    .local pmc cl, o
    cl = newclass 'MyClass'
    o = new 'MyClass'
    o = new 'MyClass', $P0
.end

.namespace ['MyClass']

.sub init :method :vtable
    .param pmc initializer :optional
    print "init was called\n"
.end

.sub init_pmc :method :vtable
    .param pmc initializer
    print "init_pmc was called\n"
.end
CODE
init was called
init_pmc was called
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "overloading find_method vtable" );
.sub main :main
    .local pmc cl, o
    cl = newclass 'MyClass'
    o = new 'MyClass'
    o.'foo'()
.end

.namespace ['MyClass']

.sub find_method :method :vtable
    .param string methodname
    print "find_method was called\n"
    $P0 = find_global "MyClass", methodname
    .return($P0)
.end

.sub foo
  print "foo was called\n"
.end

CODE
find_method was called
foo was called
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "overloading attribute accessor vtable" );
.sub main :main
    .local pmc cl, o
    cl = newclass 'MyClass'
    o = new 'MyClass'
    $P2 = new String
    $P2 = "blue"
    setattribute o, 0, $P2
    setattribute o, "blue", $P2
    $P1 = getattribute o, 0
    $P1 = getattribute o, "blue"
.end

.namespace ['MyClass']

.sub get_attr :method :vtable
    .param int offset
    print "get_attr was called\n"
.end
.sub get_attr_str :method :vtable
    .param string attrname
    print "get_attr_str was called\n"
.end
.sub set_attr :method :vtable
    .param int offset
    .param pmc val
    print "set_attr was called\n"
.end
.sub set_attr_str :method :vtable
    .param string attrname
    .param pmc val
    print "set_attr_str was called\n"
.end
CODE
set_attr was called
set_attr_str was called
get_attr was called
get_attr_str was called
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "overloading get_class vtable" );
.sub main :main
    .local pmc cl, o, cl2
    cl = newclass 'MyClass'
    o = new 'MyClass'
    cl2 = class o
.end

.namespace ['MyClass']

.sub get_class :method :vtable
    print "get_class was called\n"
.end

CODE
get_class was called
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', ":flat args to NCI methods" );
## The point of this is to make sure that we can call File.exists (which is
## an NCI method) with a :flat arg,  Fixes RT#44247.
.sub _ :main
    .local pmc arr, file
    arr = new 'ResizablePMCArray'
    push arr, 'no such file.txt'
    file = new 'File'
    $I0 = file.exists(arr :flat)
    print "exists:  "
    print $I0
    print "\n"
.end
CODE
exists:  0
OUTPUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
