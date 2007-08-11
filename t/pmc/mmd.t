#! perl
# Copyright (C) 2001-2007, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );

use Test::More;
use Parrot::Test tests => 44;

=head1 NAME

t/pmc/mmd.t - Multi-Method Dispatch

=head1 SYNOPSIS

    % prove t/pmc/mmd.t

=head1 DESCRIPTION

Tests the multi-method dispatch.

=cut

pir_output_is( <<'CODE', <<'OUTPUT', "Integer_divide_PerlInt  10 / 3 = 1003", todo => 'RT #41374' );

.sub 'test' :main

.include "pmctypes.pasm"
.include "mmd.pasm"

    .local  pmc lib_perl_group
    lib_perl_group    = loadlib 'perl_group'
    .local  int type_perl_int
    type_perl_int     = find_type 'PerlInt'

    .local pmc divide
    divide = global "Integer_divide_PerlInt"
    mmdvtregister .MMD_DIVIDE, .Integer, type_perl_int, divide

    $P0 = new type_perl_int
    $P1 = new Integer
    $P2 = new type_perl_int
    $P1 = 10
    $P2 = 3
    $P0 = $P1 / $P2
    print $P0
    print "\n"
.end

.sub Integer_divide_PerlInt
    .param pmc left
    .param pmc right
    .param pmc lhs
    $I0 = left
    $I1 = right
    $I2 = $I0/$I1     # don't call divide Integer/PerlInt here
    lhs = $I2         # '
    lhs += 1000  # prove that this function has been called
    .return(lhs)
.end
CODE
1003
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "1+1=3" );

.sub _main

.include "pmctypes.pasm"
.include "mmd.pasm"

    .local pmc add
    add = global "add"
    mmdvtregister .MMD_ADD, .Integer, .Integer, add

    $P0 = new Integer
    $P1 = new Integer
    $P2 = new Integer
    $P1 = 1
    $P2 = 1
    $P0 = $P1 + $P2
    print $P0
    print "\n"
.end

.sub add
    .param pmc left
    .param pmc right
    .param pmc lhs
    $I0 = left
    $I1 = right
    $I2 = $I0 + $I1
    inc $I2
    lhs = $I2
    .return (lhs)
.end
CODE
3
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "PASM divide - override builtin 10 / 3 = 42" );

.sub _main

.include "pmctypes.pasm"
.include "mmd.pasm"

    .local pmc divide
    divide = global "Integer_divide_Integer"
    mmdvtregister .MMD_DIVIDE, .Integer, .Integer, divide

    $P0 = new Integer
    $P1 = new Integer
    $P2 = new Integer
    $P1 = 10
    $P2 = 3
    $P0 = $P1 / $P2
    print $P0
    print "\n"
.end

.sub Integer_divide_Integer
    .param pmc left
    .param pmc right
    .param pmc lhs
    lhs = 42
    .return(lhs)
.end
CODE
42
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "INTVAL return numeq" );

.sub _main

.include "pmctypes.pasm"
.include "mmd.pasm"

    .local pmc comp
    comp = global "Float_cmp_Integer"
    mmdvtregister .MMD_CMP, .Float, .Integer, comp

    $P1 = new Float
    $P2 = new Integer
    $P1 = 47.11
    $P2 = 47
    $I0 = cmp $P1, $P2   # XXX cmp calls cmp_num
    print $I0
    print "\n"
.end

.sub Float_cmp_Integer
    .param pmc left
    .param pmc right
    .pcc_begin_return
    .return -42
    .pcc_end_return
.end
CODE
-42
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "mmdvtfind" );

.sub _main

.include "pmctypes.pasm"
.include "mmd.pasm"

    .local pmc comp
    comp = global "Float_cmp_Integer"
    mmdvtregister .MMD_NUMCMP, .Float, .Integer, comp
    $P0 = mmdvtfind .MMD_NUMCMP, .Float, .Integer
    if_null $P0, nok
    print "ok 1\n"
    ne_addr $P0, comp, nok
    print "ok 2\n"
    end
nok:
    print "not ok\n"
.end

.sub Float_cmp_Integer
    .param pmc left
    .param pmc right
    .pcc_begin_return
    .return -42
    .pcc_end_return
.end
CODE
ok 1
ok 2
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "mmdvtfind - invoke it" );

.sub _main

.include "pmctypes.pasm"
.include "mmd.pasm"

    .local pmc comp
    comp = global "Float_cmp_Integer"
    mmdvtregister .MMD_NUMCMP, .Float, .Integer, comp
    $P0 = mmdvtfind .MMD_NUMCMP, .Float, .Integer
    if_null $P0, nok
    print "ok 1\n"
    ne_addr $P0, comp, nok
    print "ok 2\n"
    $P1 = new Float
    $P2 = new Integer
    $P1 = 47.11
    $P2 = 47
    $I0 = $P0($P1, $P2)
    print $I0
    print "\n"
    end
nok:
    print "not ok\n"
.end
.sub Float_cmp_Integer
    .param pmc left
    .param pmc right
    .pcc_begin_return
    .return -42
    .pcc_end_return
.end
CODE
ok 1
ok 2
-42
OUTPUT

my $temp = "temp.pir";
END { unlink $temp; }

open my $S, '>', "$temp" or die "Can't write $temp";
print $S <<'EOF';
.sub Integer_divide_Integer
    .param pmc left
    .param pmc right
    .param pmc lhs
    lhs = 42
    .return(lhs)
.end
EOF
close $S;

pir_output_is( <<'CODE', <<'OUTPUT', "PASM MMD divide - loaded sub" );

.sub _main

.include "pmctypes.pasm"
.include "mmd.pasm"

    .local pmc divide
    load_bytecode "temp.pir"
    divide = global "Integer_divide_Integer"
    mmdvtregister .MMD_DIVIDE, .Integer, .Integer, divide

    $P0 = new Integer
    $P1 = new Integer
    $P2 = new Integer
    $P1 = 10
    $P2 = 3
    $P0 = $P1 / $P2
    print $P0
    print "\n"
.end

CODE
42
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "PASM INTVAL - new result" );
.include "pmctypes.pasm"
.include "datatypes.pasm"
.include "mmd.pasm"
    find_global P10, "Integer_bxor_Intval"
    mmdvtregister .MMD_BXOR, .Integer, .DATATYPE_INTVAL, P10

    new P1, .Integer
    set P1, 3
    n_bxor P9, P1, 2    # create new result
    print P9
    print "\n"
    end
.pcc_sub Integer_bxor_Intval:
    get_params "(0,0)", P5, I5
    print "ok\n"
    set I10, P5
    bxor I11, I10, I5
    new P6, .Integer
    set P6, I11
    set_returns "(0)", P6
    returncc
CODE
ok
1
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "PASM INTVAL - existing result" );
.include "pmctypes.pasm"
.include "datatypes.pasm"
.include "mmd.pasm"
    find_global P10, "Integer_bxor_Intval"
    mmdvtregister .MMD_BXOR, .Integer, .DATATYPE_INTVAL, P10

    new P0, .Integer
    new P1, .Integer
    set P1, 3
    bxor P0, P1, 2    # use result
    print P0
    print "\n"
    end
.pcc_sub Integer_bxor_Intval:
    get_params "(0,0,0)", P5, I5, P6
    print "ok\n"
    set I10, P5
    bxor I11, I10, I5
    set P6, I11
    set_returns "(0)", P6
    returncc
CODE
ok
1
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "PASM INTVAL - mixed" );
.include "pmctypes.pasm"
.include "datatypes.pasm"
.include "mmd.pasm"
    find_global P10, "Integer_bxor_Intval"
    mmdvtregister .MMD_BXOR, .Integer, .DATATYPE_INTVAL, P10

    new P0, .Integer
    new P1, .Integer
    set P1, 3
    bxor P0, P1, 2      # reuse destination
    print P0
    print "\n"
    n_bxor P9, P1, 2    # create new result
    print P9
    print "\n"
    end
.pcc_sub Integer_bxor_Intval:
    # the destination is optional, depending on the infix op used
    get_params "(0,0,0x80,0x100)", P5, I5, P6, I7
    print "ok\n"
    set I10, P5
    bxor I11, I10, I5
    if I7, has_dest
    new P6, .Integer
has_dest:
    set P6, I11
    set_returns "(0)", P6
    returncc

CODE
ok
1
ok
1
OUTPUT

pir_output_is( <<'CODE', <<'OUT', "first dynamic MMD call" );

.sub main :main
    .local pmc F, B, f, b, m, s
    newclass F, "Foo"
    f = new "Foo"
    newclass B, "Bar"
    b = new "Bar"
    # create a multi the hard way
    ## m = new MultiSub
    ## s = find_global "Foo", "foo"
    ## push m, s
    ## s = find_global "Bar", "foo"
    ## push m, s
    ## global "foo" = m
    print "calling foo(f, b)\n"
    foo(f, b)
    print "calling foo(b, f)\n"
    foo(b, f)
.end

.sub foo :multi(Foo, Bar)
    .param pmc x
    .param pmc y
    print "  Foo::foo\n"
.end

.sub foo :multi(Bar, Foo)
    .param pmc x
    .param pmc y
    print "  Bar::foo\n"
.end
CODE
calling foo(f, b)
  Foo::foo
calling foo(b, f)
  Bar::foo
OUT

pir_output_is( <<'CODE', <<'OUT', "MMD second arg int/float dispatch" );
.sub foo :multi(_, Integer)
    .param pmc first
    .param pmc second
    print "(_, Int) method:  "
    print first
    print ', '
    print second
    print "\n"
.end
.sub foo :multi(_, Float)
    .param pmc first
    .param pmc second
    print "(_, Float) method:  "
    print first
    print ', '
    print second
    print "\n"
.end
.sub main :main
    $P0 = new .Float
    $P0 = 9.5
    foo(1, $P0)
    $P1 = new .Integer
    $P1 = 3
    foo(1, $P1)
.end
CODE
(_, Float) method:  1, 9.5
(_, Int) method:  1, 3
OUT

pir_error_output_like( <<'CODE', <<'OUT', "MMD single method, dispatch failure" );
## Compare this to the previous example.
.sub foo :multi(_, Float)
    .param pmc first
    .param pmc second
    print "(_, Float) method:  "
    print first
    print ', '
    print second
    print "\n"
.end
.sub main :main
    $P0 = new .Float
    $P0 = 9.5
    foo(1, $P0)
    $P1 = new .Integer
    $P1 = 3
    foo(1, $P1)
.end
CODE
/\A\(_, Float\) method:  1, 9\.5
No applicable methods/
OUT

pir_output_is( <<'CODE', <<'OUT', "MMD on argument count" );
.sub main :main
    p("ok 1\n")
    p("-twice", "ok 2\n")
.end

.sub p :multi(string)
    .param string s
    print s
.end

.sub p :multi(string, string)
    .param string opt
    .param string s
    if opt != '-twice' goto no_twice
    print s
    print s
    .return()
no_twice:
    print s
.end
CODE
ok 1
ok 2
ok 2
OUT

pir_output_is( <<'CODE', <<'OUT', "MMD on mative types" );
.sub main :main
    p("ok 1\n")
    p(42)
.end

.sub p :multi(string)
    .param string s
    print s
.end

.sub p :multi(int)
    .param int i
    print i
    print "\n"
.end
CODE
ok 1
42
OUT

pir_output_is( <<'CODE', <<'OUT', 'MMD on PMC types', todo => 'RT #41374' );
.sub 'test' :main
    .local  pmc  lib_perl_group
    lib_perl_group    = loadlib 'perl_group'
    .local  int  type_perl_string
    type_perl_string  = find_type 'PerlInt'

    $P0 = new String
    $P0 = "ok 1\n"
    $P1 = new type_perl_string
    $P1 = "ok 2\n"
    p($P0)
    p($P1)
    $P0 = subclass "PerlString", "Xstring"
    $P0 = new "Xstring"
    $P0 = "ok 3\n"
    $P1 = subclass "String", "Ystring"
    $P1 = new "Ystring"
    $P1 = "ok 4\n"
    p($P0)
    p($P1)
.end

.sub p :multi(String)
    .param pmc p
    print "String "
    print p
.end

.sub p :multi(PerlString)
    .param pmc p
    print "PerlSt "
    print p
.end
CODE
String ok 1
PerlSt ok 2
PerlSt ok 3
String ok 4
OUT

pir_output_is( <<'CODE', <<'OUT', "MMD on PMC types quoted", todo => 'RT #41374' );
.sub main :main
    .local  pmc  lib_perl_group
    lib_perl_group    = loadlib 'perl_group'
    .local  int  type_perl_string
    type_perl_string  = find_type 'PerlInt'

    $P0 = new String
    $P0 = "ok 1\n"
    $P1 = new type_perl_string
    $P1 = "ok 2\n"
    p($P0)
    p($P1)
    $P0 = subclass "PerlString", "Xstring"
    $P0 = new "Xstring"
    $P0 = "ok 3\n"
    $P1 = subclass "String", "Ystring"
    $P1 = new "Ystring"
    $P1 = "ok 4\n"
    p($P0)
    p($P1)
.end

.sub p :multi("String")
    .param pmc p
    print "String "
    print p
.end

.sub p :multi("PerlString")
    .param pmc p
    print "PerlSt "
    print p
.end
CODE
String ok 1
PerlSt ok 2
PerlSt ok 3
String ok 4
OUT

pir_output_like( <<'CODE', <<'OUT', "MMD on PMC types, invalid", todo => 'RT #41374' );
.sub main :main
    .local  pmc  lib_perl_group
    lib_perl_group    = loadlib 'perl_group'
    .local  int  type_perl_string
    type_perl_string  = find_type 'PerlInt'

    $P0 = new String
    $P0 = "ok 1\n"
    $P1 = new type_perl_string
    $P1 = "ok 2\n"
    p($P0)
    p($P1)
    $P0 = subclass "PerlString", "Xstring"
    $P0 = new "Xstring"
    $P0 = "ok 3\n"
    $P1 = subclass "String", "Ystring"
    $P1 = new "Ystring"
    $P1 = "ok 4\n"
    p($P0)
    p($P1)
    $P0 = new Integer
    p($P0)
.end

.sub p :multi(String)
    .param pmc p
    print "String "
    print p
.end

.sub p :multi(PerlString)
    .param pmc p
    print "PerlSt "
    print p
.end
CODE
/String ok 1
PerlSt ok 2
PerlSt ok 3
String ok 4
No applicable methods/
OUT

pir_output_is( <<'CODE', <<'OUT', "MMD on PMC types 3", todo => 'RT #41374' );
.sub main :main
    .local  pmc  lib_perl_group
    lib_perl_group    = loadlib 'perl_group'
    .local  int  type_perl_string
    type_perl_string  = find_type 'PerlInt'
    .local  int  type_perl_int
    type_perl_int     = find_type 'PerlInt'

    $P0 = new String
    $P0 = "ok 1\n"
    $P1 = new type_perl_string
    $P1 = "ok 2\n"
    p($P0)
    p($P1)
    $P0 = subclass "PerlString", "Xstring"
    $P0 = new "Xstring"
    $P0 = "ok 3\n"
    $P1 = subclass "String", "Ystring"
    $P1 = new "Ystring"
    $P1 = "ok 4\n"
    p($P0)
    p($P1)
    $P0 = new type_perl_int
    $P0 = 42
    p($P0)
.end

.sub p :multi(String)
    .param pmc p
    print "String "
    print p
.end

.sub p :multi(PerlString)
    .param pmc p
    print "PerlSt "
    print p
.end

.sub p :multi(Integer)
    .param pmc p
    print "Intege "
    print p
    print "\n"
.end

CODE
String ok 1
PerlSt ok 2
PerlSt ok 3
String ok 4
Intege 42
OUT

pir_output_is( <<'CODE', <<'OUT', "MMD on PMC types, global namespace", todo => 'RT #41374' );
.sub main :main
    .local  pmc  lib_perl_group
    lib_perl_group    = loadlib 'perl_group'
    .local  int  type_perl_string
    type_perl_string  = find_type 'PerlInt'
    .local  int  type_perl_int
    type_perl_int     = find_type 'PerlInt'

    $P0 = new String
    $P0 = "ok 1\n"
    $P1 = new type_perl_string
    $P1 = "ok 2\n"
    p($P0)
    p($P1)
    $P0 = subclass "PerlString", "Xstring"
    $P0 = new "Xstring"
    $P0 = "ok 3\n"
    $P1 = subclass "String", "Ystring"
    $P1 = new "Ystring"
    $P1 = "ok 4\n"
    p($P0)
    p($P1)
.end

.sub p :multi(String)
    .param pmc p
    print "String "
    print p
.end

.sub p :multi(PerlString)
    .param pmc p
    print "PerlSt "
    print p
.end
CODE
String ok 1
PerlSt ok 2
PerlSt ok 3
String ok 4
OUT

pir_output_is( <<'CODE', <<'OUT', "MMD on PMC types, package namespace", todo => 'RT #41374' );

.namespace ["Some"]

.sub main :main
    .local  pmc  lib_perl_group
    lib_perl_group    = loadlib 'perl_group'
    .local  int  type_perl_string
    type_perl_string  = find_type 'PerlInt'
    .local  int  type_perl_int
    type_perl_int     = find_type 'PerlInt'

    $P0 = new String
    $P0 = "ok 1\n"
    $P1 = new type_perl_string
    $P1 = "ok 2\n"
    p($P0)
    p($P1)
    $P0 = subclass "PerlString", "Xstring"
    $P0 = new "Xstring"
    $P0 = "ok 3\n"
    $P1 = subclass "String", "Ystring"
    $P1 = new "Ystring"
    $P1 = "ok 4\n"
    p($P0)
    p($P1)
.end

.sub p :multi(String)
    .param pmc p
    print "String "
    print p
.end

.sub p :multi(PerlString)
    .param pmc p
    print "PerlSt "
    print p
.end
CODE
String ok 1
PerlSt ok 2
PerlSt ok 3
String ok 4
OUT

pir_output_is( <<'CODE', <<'OUT', "MMD on PMC types - Any", todo => 'RT #41374' );

.sub main :main
    .local  pmc  lib_perl_group
    lib_perl_group    = loadlib 'perl_group'
    .local  int  type_perl_string
    type_perl_string  = find_type 'PerlInt'
    .local  int  type_perl_int
    type_perl_int     = find_type 'PerlInt'

    $P0 = new String
    $P0 = "ok 1\n"
    $P1 = new type_perl_string
    $P1 = "ok 2\n"
    p($P0)
    p($P1)
    $P0 = new type_perl_int
    $P0 = 42
    p($P0)
    $P0 = new type_perl_int
    $P0 = 43
    q($P0)
.end

.namespace

.sub p :multi(String)
    .param pmc p
    print "String "
    print p
.end

.sub p :multi(PerlString)
    .param pmc p
    print "PerlSt "
    print p
.end

.sub p :multi(_)
    .param pmc p
    print "Any    "
    print p
    print "\n"
.end

.sub q :multi(pmc)
    .param pmc p
    print "Any    "
    print p
    print "\n"
.end

CODE
String ok 1
PerlSt ok 2
Any    42
Any    43
OUT

pir_output_is( <<'CODE', <<'OUTPUT', "__add as function - Int, Float" );
.sub main :main
    .local pmc d, l, r, a
    d = new Integer
    l = new Integer
    r = new Float
    l = 3
    r = 39.42
    a = get_root_global ["__parrot_core"], "__add"
    a(l, r, d)
    print d
    print "\n"
    end
.end
CODE
42.42
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "__add as method" );
.sub main :main
    .local pmc d, l, r
    l = new Integer
    r = new Integer
    l = 3
    r = 39
    d = l."__add"(r)
    print d
    print "\n"
    end
.end
CODE
42
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "__add as method - inherited", todo => 'RT #41374' );
.sub main :main
    .local  pmc  lib_perl_group
    lib_perl_group    = loadlib 'perl_group'
    .local  int  type_perl_string
    type_perl_string  = find_type 'PerlInt'
    .local  int  type_perl_int
    type_perl_int     = find_type 'PerlInt'

    .local pmc d, l, r
    l = new type_perl_int
    r = new type_perl_int
    l = 3
    r = 39
    d = l."__add"(r)
    print d
    print "\n"
.end
CODE
42
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "__add as method - Int, Float" );
.sub main :main
    .local pmc d, l, r
    l = new Integer
    r = new Float
    l = 3
    r = 39.42
    d = l."__add"(r)
    print d
    print "\n"
    end
.end
CODE
42.42
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "bound __add method" );
.sub main :main
    .local pmc d, l, r, m
    d = new Integer
    l = new Integer
    r = new Float
    l = 3
    r = 39.42
    m = find_global 'Float', "__add"
    d = m(r, l)
    print d
    print "\n"
    r = new Integer
    r = 39
    m = find_global 'Integer', "__add"
    d = m(r, l)
    print d
    print "\n"
    end
.end
CODE
42.42
42
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "Integer subclasses" );
.sub main :main
    .local pmc d, l, r, cl
    cl = subclass "Integer", "AInt"
    d = new "AInt"
    l = new "AInt"
    r = new "AInt"
    l = 4
    r = 38
    print l
    print "\n"
    print r
    print "\n"
    # dispatches to Parrot_Integer_add_Integer
    add d, l, r
    print d
    print "\n"
    # dispatches to Parrot_Integer_add_Integer
    d = l."__add"(r)
    print d
    print "\n"
    l."__i_add"(r)
    print l
    print "\n"
.end

CODE
4
38
42
42
42
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "Integer subclasses, n_add" );
.sub main :main
    $P0 = subclass "Integer", "AInt"
    $P0 = new "AInt"
    $P1 = new Integer
    set $P0, 6
    set $P1, 2

    $P2 = n_add  $P0, $P1
    print $P2
    print "\n"
.end
.namespace ["AInt"]
.sub __add :multi(AInt, Integer)
    .param pmc l
    .param pmc r
    print l
    print r
    print "\n"
    $P0 = new Integer
    $P0 = 2
    .return($P0)
.end
CODE
62
2
OUTPUT

## my $temp = "temp.pir";
## END { unlink $temp; };

open my $P, '>', "$temp" or die "can't write $temp";
print $P <<'EOF';
.sub __add :multi(Integer, Integer)
    .param pmc l
    .param pmc r
    print l
    print r
    print "\n"
    $P0 = new Integer
    $P0 = 2
    .return($P0)
.end
EOF
close $P;

pir_output_is( <<'CODE', <<'OUTPUT', "override builtin n_add" );
.sub main
    load_bytecode "temp.pir"
    $P0 = new Integer
    $P1 = new Integer
    set $P0, 6
    set $P1, 2

    $P2 = n_add  $P0, $P1
    print $P2
    print "\n"
.end
CODE
62
2
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "mmd bug reported by Jeff" );
.namespace ['Foo']

.sub bar :method, :multi(Foo, string)
    .param string arg
    print "string\n"
.end

.sub bar :method, :multi(Foo, pmc)
    .param pmc arg
    print "PMC\n"
.end

.sub bar :method, :multi(Foo)
    print "nothing\n"
.end

.namespace

.sub main :main
    newclass $P0, 'Foo'

    $I0 = find_type 'Foo'
    $P0 = new $I0

    $P0.'bar'('Bar!')

    $P1 = new .String
    $P1 = "Bar!"
    $P0.'bar'($P1)

    $P0.'bar'()
.end
CODE
string
PMC
nothing
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "use a core func for an object" );
.sub main :main
    .local pmc d, l, r, cl
    cl = newclass "AInt"
    addattribute cl, ".i"
    d = new "AInt"
    l = new "AInt"
    r = new "AInt"
    .local pmc func
    .local int typ
    .include "mmd.pasm"
    func = mmdvtfind .MMD_ADD, .Float, .Float
    typ = typeof l
    mmdvtregister .MMD_ADD, typ, typ, func
    l = 4
    r = 38
    print l
    print "\n"
    print r
    print "\n"
    add d, l, r
    print d
    print "\n"
.end
.namespace ["AInt"]
.sub init :vtable :method
    $P0 = new Integer
    setattribute self, ".i", $P0
.end
.sub set_integer_native :vtable :method
    .param int i
    $P0 = getattribute self, ".i"
    $P0 = i
.end
.sub set_number_native :vtable :method
    .param float f
    $P0 = getattribute self, ".i"
    $P0 = f
.end
.sub get_string :vtable :method
    $P0 = getattribute self, ".i"
    $S0 = $P0
    .return ($S0)
.end
.sub get_number :vtable :method
    $P0 = getattribute self, ".i"
    $N0 = $P0
    .return ($N0)
.end
CODE
4
38
42
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "multisub vs find_name" );
.sub main :main
    $P0 = find_name "foo"
    $S0 = classname $P0
    print $S0
    print "\n"
.end
.sub foo :method, :multi(string)
    .param pmc x
    print "  foo\n"
.end
.sub foo :method, :multi(pmc)
    .param pmc x
    print "  foo\n"
.end
CODE
MultiSub
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "multisub w void" );
.sub main :main
    foo('xx')
    foo()
    foo('xx')
.end
.sub foo :multi(string)
    .param pmc x
    print "foo string\n"
.end
.sub foo :multi()
    print "foo\n"
.end
CODE
foo string
foo
foo string
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "multisub w/o .HLL" );
.sub main :main
    $P0 = new .Integer
    $P0 = 3
    $P9 = 'foo'($P0)

    $P0 = new .ResizablePMCArray
    push $P0, 4
    $P1 = new .String
    $P1 = 'hello'
    $P9 = 'foo'($P0, $P1)
.end

.sub 'foo' :multi(Integer)
    print "foo(Integer)\n"
    .return (0)
.end

.sub 'foo' :multi(ResizablePMCArray, _)
    print "foo(ResizablePMCArray,_)\n"
    .return (0)
.end
CODE
foo(Integer)
foo(ResizablePMCArray,_)
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "multisub w/ .HLL, rt #39161" );
.HLL 'Perl6', ''
.sub main :main
    $P0 = new .Integer
    $P0 = 3
    $P9 = 'foo'($P0)

    $P0 = new .ResizablePMCArray
    push $P0, 4
    $P1 = new .String
    $P1 = 'hello'
    $P9 = 'foo'($P0, $P1)
.end

.sub 'foo' :multi(Integer)
    print "foo(Integer)\n"
    .return (0)
.end

.sub 'foo' :multi(ResizablePMCArray, _)
    print "foo(ResizablePMCArray,_)\n"
    .return (0)
.end
CODE
foo(Integer)
foo(ResizablePMCArray,_)
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "multisub w/ flatten" );
# see also 'rt #39173
.sub main :main
    .local pmc int_pmc
    int_pmc = new .Integer
    int_pmc = 3

    .local pmc args
    args = new .ResizablePMCArray
    push args, int_pmc
    'foo'( args :flat )

    .local pmc string_pmc
    string_pmc = new .String
    string_pmc = 'hello'

    args = new .ResizablePMCArray
    push args, string_pmc
    'foo'( args :flat )
    end
.end

.sub 'foo' :multi(Integer)
    print "foo(Integer)\n"
.end

.sub 'foo' :multi(String)
    print "foo(String)\n"
.end
CODE
foo(Integer)
foo(String)
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "keyed class name and multi" );
.sub main :main
    .local pmc class
    newclass class, [ 'Some'; 'Class' ]

    .local int class_type
    class_type = find_type [ 'Some'; 'Class' ]

    .local pmc instance
    instance = new class_type

    .local string name
    name = typeof instance

    print "Type: "
    print name
    print "\n"
    end
.end
CODE
Type: Some;Class
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "keyed class name and multi" );
.sub main :main
    .local pmc class
    newclass class, [ 'Some'; 'Class' ]

    .local int class_type
    class_type = find_type [ 'Some'; 'Class' ]

    .local pmc instance
    instance = new class_type

    foo( instance )
    end
.end

.sub 'foo' :multi( [ 'Some'; 'Class' ])
    print "Called multi for class\n"
.end

.sub 'foo' :multi(_)
    print "Called wrong multi\n"
.end
CODE
Called multi for class
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "unicode sub names and multi (RT#39254)" );
.sub unicode:"\u7777" :multi(string)
  .param pmc arg
  print 'String:'
  say arg
.end
.sub unicode:"\u7777" :multi(int)
  .param pmc arg
  print 'Int:'
  say arg
.end

.sub main :main
  unicode:"\u7777"('what')
  unicode:"\u7777"(23)
.end
CODE
String:what
Int:23
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "autoboxing on multis" );
.sub box_me_up :multi(string)
    .param string first
    .param pmc    second

    .local string promoted_type
    promoted_type = typeof second
    print "BMU autobox type: "
    print promoted_type
    print "\n"
.end

.sub box_me_up :multi()
    print "BMU no autobox, so sad\n"
.end

.sub box_me_up :multi(int, int)
    print "BMU inty, so bad\n"
.end

.sub main :main
    box_me_up( 'foo', 'bar' )
.end
CODE
BMU autobox type: String
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', '_ matches native types' );
.sub main :main
  .local pmc asub
  asub = get_global 'main'

  foo('world', asub) # should call :multi(_, Sub)
.end

.sub foo :multi(_, Sub)
  .param pmc x
  .param pmc y
  print x
  print " "
  say ":multi(_, Sub)"
.end

.sub foo :multi(Integer, Sub)
  .param int x
  .param pmc y
  print x
  print " "
  say ":multi(int, Sub)"
.end
CODE
world :multi(_, Sub)
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'type mix with _' );
.sub main :main
    $P0 = new .Integer
    $P0 = 3
    'foo'($P0)
    'foo'(2)
    'foo'("1")
    $P0 = new .String
    $P0 = "0"
    'foo'($P0)
    $P0 = new .Hash
    'foo'($P0)
.end

.sub 'foo' :multi(Integer)
    .param pmc i
    print "foo(Integer)\n"
.end

.sub 'foo' :multi(_)
    .param pmc i
    print "foo(_)\n"
.end

.sub 'foo' :multi(int)
    .param int i
    print "foo(int)\n"
.end

.sub 'foo' :multi(String)
    .param pmc i
    print "foo(String)\n"
.end

.sub 'foo' :multi(string)
    .param string i
    print "foo(string)\n"
.end
CODE
foo(Integer)
foo(int)
foo(string)
foo(String)
foo(_)
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', ':multi with :outer' );
.sub main :main
    new $P0, .String
    assign $P0, 'arg0'
    new $P1, .String
    assign $P1, 'arg1'

    $P99 = "foo"($P0)

    $P99 = "foo"($P0, $P1)

    $P99 = "bar"($P0)

    $P99 = "bar"($P0, $P1)
.end


.sub "foo" :multi(_)
    .param pmc x
    print "foo(_)  : "
    say x
    .return (x)
.end

.sub "foo" :multi(_,_)
    .param pmc x
    .param pmc y
    print "foo(_,_): "
    print x
    print " "
    say y
    .return (y)
.end

.sub "bar" :outer("main") :multi(_)
    .param pmc x
    print "bar(_)  : "
    say x
    .return (x)
.end

.sub "bar" :outer("main") :multi(_,_)
    .param pmc x
    .param pmc y
    print "bar(_,_): "
    print x
    print " "
    say y
    .return (y)
.end
CODE
foo(_)  : arg0
foo(_,_): arg0 arg1
bar(_)  : arg0
bar(_,_): arg0 arg1
OUTPUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
