#!perl
# Copyright: 2001-2005 The Perl Foundation.  All Rights Reserved.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test;


=head1 NAME

t/op/calling.t - Parrot Calling Conventions

=head1 SYNOPSIS

	% prove t/op/calling.t

=head1 DESCRIPTION

Tests Parrot calling conventions.

=cut


output_is(<<'CODE', <<'OUTPUT', "set_args - parsing");
    noop
    set_args "(0b10, 0)", P0, I0
    print "Ok 1\n"
    set_args "()"
    print "Ok 2\n"
    end
CODE
Ok 1
Ok 2
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "var_args - parsing");
.pcc_sub main:
    print "Ok 1\n"
    set_args "(0b10, 0)", P0, I0
    get_results "(0)", I0
    find_name P1, "foo"
    print "Ok 2\n"
    invokecc P1
    print "Ok 5\n"
    end
.pcc_sub foo:
    get_params "(0b10, 0)", P0, I0
    print "Ok 3\n"
    set_returns "(0b100)", 42
    print "Ok 4\n"
    returncc
CODE
Ok 1
Ok 2
Ok 3
Ok 4
Ok 5
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "call - i, ic");
.pcc_sub main:
    set I16, 77
    set_args "(0b100, 0)", 42, I16
    find_name P1, "foo"
    invokecc P1
    print "back\n"
    end
.pcc_sub foo:
    get_params "(0, 0)", I16, I17
    print I16
    print "\n"
    print I17
    print "\n"
    returncc
CODE
42
77
back
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "call - i, ic, return i, ic");
.pcc_sub main:
    set I16, 77
    set_args "(0b100, 0)", 42, I16
    get_results "(0, 0)", I16, I17
    find_name P1, "foo"
    invokecc P1
    print I16
    print "\n"
    print I17
    print "\nback\n"
    end
.pcc_sub foo:
    get_params "(0, 0)", I16, I17
    print I16
    print "\n"
    print I17
    print "\n"
    set I16, 88
    set_returns "(4, 0)", 99, I16
    returncc
CODE
42
77
99
88
back
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "call - i, ic, return i, ic - adjust sig");
.pcc_sub main:
    set I16, 77
    set_args "(0, 0)", 42, I16
    get_results "(0, 0)", I16, I17
    find_name P1, "foo"
    invokecc P1
    print I16
    print "\n"
    print I17
    print "\nback\n"
    end
.pcc_sub foo:
    get_params "(0, 0)", I16, I17
    print I16
    print "\n"
    print I17
    print "\n"
    set I16, 88
    set_returns "(0, 0)", 99, I16
    returncc
CODE
42
77
99
88
back
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "all together now");
.pcc_sub main:
    set I16, 77
    set N16, 2.3
    set S16, "ok 1\n"
    new P16, .Integer
    set P16, 101
    set_args "(0, 0, 0, 0, 0, 0, 0)", 42, I16, 4.5, N16, S16, "ok 2\n", P16
    get_results "(0, 0, 0, 0)", I16, N16, S16, P16
    find_name P1, "foo"
    invokecc P1
    print I16
    print "\n"
    print N16
    print "\n"
    print S16
    print P16
    end
.pcc_sub foo:
    get_params "(0, 0, 0, 0, 0, 0, 0)", I16, I17, N16, N17, S16, S17, P16
    print I16
    print "\n"
    print I17
    print "\n"
    print N16
    print "\n"
    print N17
    print "\n"
    print S16
    print S17
    print P16
    print "\n"
    set I16, 88
    set N16, 5.5
    set S16, "ok 3\n"
    new P16, .String
    set P16, "ok 4\n"
    set_returns "(0, 0, 0, 0)", I16, N16, S16, P16
    returncc
CODE
42
77
4.500000
2.300000
ok 1
ok 2
101
88
5.500000
ok 3
ok 4
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "flatten arg");
.pcc_sub main:
    new P16, .String
    set P16, "ok 1\n"
    new P17, .ResizablePMCArray
    push P17, "ok 2\n"
    push P17, "ok 3\n"
    push P17, "ok 4\n"
    new P18, .String
    set P18, "ok 5\n"
    set_args "(0, 0x8, 0)", P16, P17, P18
    find_name P1, "foo"
    invokecc P1
    print "back\n"
    end
.pcc_sub foo:
    get_params "(0, 0, 0, 0, 0)", P1, P2, P3, P4, P5
    print P1
    print P2
    print P3
    print P4
    print P5
    returncc
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
back
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "slurpy param");
.pcc_sub main:
    new P16, .String
    set P16, "ok 1\n"
    new P17, .String
    set P17, "ok 2\n"
    new P18, .String
    set P18, "ok 3\n"
    set_args "(0, 0, 0)", P16, P17, P18
    find_name P1, "foo"
    invokecc P1
    print "back\n"
    end
.pcc_sub foo:
    get_params "(0, 0x8)", P1, P2
    print P1
    set I0, P2
    print I0
    print "\n"
    set S0, P2[0]
    print S0
    set S0, P2[1]
    print S0
    returncc
CODE
ok 1
2
ok 2
ok 3
back
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "use it in PIR");
.sub main :main
    $P0 = new String
    $P0 = "hello\n"
    find_name $P1, "foo"
    # set_args and invoke must be adjacent
    set_args "(0)", $P0
    invokecc $P1
.end
.sub foo
    get_params "(0)", $P0
    print $P0
.end
CODE
hello
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "flatten + slurpy param");
.pcc_sub main:
    new P16, .String
    set P16, "ok 1\n"
    new P19, .ResizablePMCArray
    new P17, .String
    set P17, "ok 2\n"
    push P19, P17
    new P18, .String
    set P18, "ok 3\n"
    push P19, P18
    new P20, .ResizablePMCArray
    new P17, .String
    set P17, "ok 4\n"
    push P20, P17
    new P18, .String
    set P18, "ok 5\n"
    push P20, P18
    new P21, .String
    set P21, "ok 6\n"
    set_args "(0, 0x8, 0x8, 0)", P16, P19, P20, P21
    find_name P1, "foo"
    invokecc P1
    print "back\n"
    end
.pcc_sub foo:
    get_params "(0, 0x8)", P1, P2
    print P1
    set I0, P2
    print I0
    print "\n"
    set S0, P2[0]
    print S0
    set S0, P2[1]
    print S0
    set S0, P2[2]
    print S0
    set S0, P2[3]
    print S0
    set S0, P2[4]
    print S0
    set S0, P2[5]
    print S0
    returncc
CODE
ok 1
5
ok 2
ok 3
ok 4
ok 5
ok 6
back
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "use it in PIR");
.sub main :main
    $P0 = new String
    $P0 = "hello\n"
    find_name $P1, "foo"
    # set_args and invoke must be adjacent
    set_args "(0)", $P0
    invokecc $P1
.end
.sub foo
    get_params "(0)", $P0
    print $P0
.end
CODE
hello
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "type conversion - autobox");
.sub main :main
    $P0 = new String
    $P0 = "hello"
    find_name $P1, "foo"
    # set_args and invoke must be adjacent
    set_args "(0,0,0)", $P0, 42, "bar"
    invokecc $P1
.end
.sub foo
    get_params "(0x8)", $P0
    $S0 = $P0[0]
    print_item $S0
    $S0 = $P0[1]
    print_item $S0
    $S0 = $P0[2]
    print_item $S0
    print_newline
.end
CODE
hello 42 bar
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "type conversion - fetch");
.sub main :main
    $P0 = new .String
    $P0 = "hello"
    $P1 = new .Integer
    $P1 = 42
    $P2 = new .String
    $P2 = "again"
    $P3 = new .Float
    $P3 = 47.11
    find_name $P10, "foo"
    # set_args and invoke must be adjacent
    set_args "(0,0,0,0)", $P0, $P1, $P2, $P3
    invokecc $P10
.end
.sub foo
    get_params "(0,0,0,0)", $P0, $I0, $S0, $N0
    print_item $P0
    print_item $I0
    print_item $S0
    print_item $N0
    print_newline
    returncc
.end
CODE
hello 42 again 47.11
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "maybe flatten + slurpy param");
.pcc_sub main:
    new P16, .String
    set P16, "ok 1\n"
    new P19, .ResizablePMCArray
    new P17, .String
    set P17, "ok 2\n"
    push P19, P17
    new P18, .String
    set P18, "ok 3\n"
    push P19, P18
    new P20, .ResizablePMCArray
    new P17, .String
    set P17, "ok 4\n"
    push P20, P17
    new P18, .String
    set P18, "ok 5\n"
    push P20, P18
    new P21, .String
    set P21, "ok 6\n"
    set_args "(0x10, 0x10, 0x10, 0x10)", P16, P19, P20, P21
    find_name P1, "foo"
    invokecc P1
    print "back\n"
    end
.pcc_sub foo:
    get_params "(0, 0x8)", P1, P2
    print P1
    set I0, P2
    print I0
    print "\n"
    set S0, P2[0]
    print S0
    set S0, P2[1]
    print S0
    set S0, P2[2]
    print S0
    set S0, P2[3]
    print S0
    set S0, P2[4]
    print S0
    set S0, P2[5]
    print S0
    returncc
CODE
ok 1
5
ok 2
ok 3
ok 4
ok 5
ok 6
back
OUTPUT

pir_output_like(<<'CODE', <<'OUTPUT', "argc mismatch, too few");
.sub main :main
    .include "errors.pasm"
    errorson .PARROT_ERRORS_PARAM_COUNT_FLAG
    $P0 = new String
    $P0 = "hello\n"
    find_name $P1, "foo"
    set_args "(0)", $P0
    invokecc $P1
.end
.sub foo
    get_params "(0,0)", $P0, $P1
    print $P0
.end
CODE
/too few arguments passed/
OUTPUT

pir_output_like(<<'CODE', <<'OUTPUT', "argc mismatch, too many - no getparams", todo => 'no get_params at all');
.sub main :main
    .include "errors.pasm"
    errorson .PARROT_ERRORS_PARAM_COUNT_FLAG
    foo(5)
.end
.sub foo
    print "nada"
.end
CODE
/too many arguments passed/
OUTPUT

pir_output_like(<<'CODE', <<'OUTPUT', "argc mismatch, too many - force get_params");
.macro no_params
    get_params '()'
.endm
.sub main :main
    .include "errors.pasm"
    errorson .PARROT_ERRORS_PARAM_COUNT_FLAG
    foo(5)
.end
.sub foo
    .no_params
    print "nada"
.end
CODE
/too many arguments passed/
OUTPUT

pir_output_like(<<'CODE', <<'OUTPUT', "argc mismatch, too many");
.sub main :main
    .include "errors.pasm"
    errorson .PARROT_ERRORS_PARAM_COUNT_FLAG
    $P0 = new String
    $P0 = "hello\n"
    find_name $P1, "foo"
    set_args "(0,0)", $P0,77
    invokecc $P1
.end
.sub foo
    get_params "(0)", $P0
    print $P0
.end
CODE
/too many arguments passed/
OUTPUT

pir_output_like(<<'CODE', <<'OUTPUT', "argc mismatch, too many - catch exception");
.sub main :main
    .include "errors.pasm"
    errorson .PARROT_ERRORS_PARAM_COUNT_FLAG
    $P0 = new String
    $P0 = "hello\n"
    find_name $P1, "foo"
    set_args "(0,0)", $P0,77
    invokecc $P1
.end
.sub foo
    push_eh arg_handler
    get_params "(0)", $P0
    print $P0
    print "never\n"
arg_handler:
    get_results "(0,0)", $P1, $S0
    print "catched: "
    print $S0
.end
CODE
/^catched: too many arguments passed/
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "argc mismatch, optional");
.sub main :main
    .include "errors.pasm"
    errorson .PARROT_ERRORS_PARAM_COUNT_FLAG
    $P0 = new String
    $P0 = "hello\n"
    find_name $P1, "foo"
    set_args "(0)", $P0
    invokecc $P1
.end
.sub foo
    get_params "(0,0x20,0x40)", $P0, $P1, $I0
    print $P0
    if_null $P1, ok
    print "not "
ok:
    print "ok\n"
.end
CODE
hello
ok
OUTPUT

pir_output_like(<<'CODE', <<'OUTPUT', "argc mismatch, optional");
.sub main :main
    .include "errors.pasm"
    errorson .PARROT_ERRORS_PARAM_COUNT_FLAG
    .local pmc ar
    ar = new ResizableIntegerArray
    push ar, 1
    push ar, 2
    push ar, 3
    push ar, 4
    foo(ar :flat)
    print "never\n"
.end
.sub foo
    .param int i
    .param int j     :optional
    .param int got_j :opt_flag
    .param int k     :optional
    .param int got_k :opt_flag
.end
CODE
/too many arguments passed/
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "get_param later");
.pcc_sub main:
    set I16, 77
    set_args "(0b100, 0)", 42, I16
    get_results "(0, 0)", I16, I17
    find_name P1, "foo"
    invokecc P1
    print I16
    print "\n"
    print I17
    print "\nback\n"
    end
.pcc_sub foo:
    noop
    get_params "(0, 0)", I16, I17
    print I16
    print "\n"
    print I17
    print "\n"
    set I16, 88
    set_returns "(4, 0)", 99, I16
    returncc
CODE
42
77
99
88
back
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "tailcall 1");
.sub main :main
    .const .Sub f = "foo"
    print "main\n"
    get_results "(0)", $S0
    invokecc f
    print $S0
.end
.sub foo
    .const .Sub b = "bar"
    print "foo\n"
    tailcall b
.end
.sub bar
    print "bar\n"
    set_returns "(0)", "bar_ret\n"
    returncc
.end
CODE
main
foo
bar
bar_ret
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "tailcall 2 - pass arg");
.sub main :main
    .const .Sub f = "foo"
    print "main\n"
    get_results "(0)", $S0
    invokecc f
    print $S0
.end
.sub foo
    .const .Sub b = "bar"
    print "foo\n"
    set_args "(0)", "from_foo\n"
    tailcall b
.end
.sub bar
    get_params "(0)", $S0
    print "bar\n"
    print $S0
    set_returns "(0)", "bar_ret\n"
    returncc
.end
CODE
main
foo
bar
from_foo
bar_ret
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "tailcall 3 - pass arg");
.sub main :main
    .const .Sub f = "foo"
    print "main\n"
    get_results "(0)", $S0
    invokecc f
    print $S0
.end
.sub foo
    .const .Sub b = "bar"
    print "foo\n"
    set_args "(0)", "from_foo\n"
    tailcall b
.end
.sub bar
    get_params "(0)", $S0
    print "bar\n"
    print $S0
    set_returns "(0)", "bar_ret\n"
    returncc
.end
CODE
main
foo
bar
from_foo
bar_ret
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "get_params twice");
.pcc_sub main:
    new P16, .String
    set P16, "ok 1\n"
    new P17, .String
    set P17, "ok 2\n"
    new P18, .String
    set P18, "ok 3\n"
    set_args "(0, 0, 0)", P16, P17, P18
    find_name P1, "foo"
    invokecc P1
    print "back\n"
    end
.pcc_sub foo:
    get_params "(0, 0x8)", P1, P2
    print P1
    set I0, P2
    print I0
    print "\n"
    set S0, P2[0]
    print S0
    set S0, P2[1]
    print S0
    get_params "(0, 0, 0)", P1, P2, P3
    print P1
    print P2
    print P3
    returncc
CODE
ok 1
2
ok 2
ok 3
ok 1
ok 2
ok 3
back
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "empty args");
.sub main :main
    $P0 = new String
    $P0 = "hello\n"
    find_name $P1, "foo"
    set_args "()"
    invokecc $P1
.end
.sub foo
    get_params "(0x20, 0x40)", $P1, $I0
    if_null $P1, ok
    print "not "
ok:
    print "ok\n"
.end
CODE
ok
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "optional args");
.sub main :main
    $P0 = new String
    $P0 = "hello\n"
    find_name $P1, "foo"
    set_args "(0x0)", $P0
    invokecc $P1
.end
.sub foo
    get_params "(0x20, 0x40)", $P1, $I0
    unless_null $P1, ok
    print "not "
ok:
    print "ok\n"
.end
CODE
ok
OUTPUT


pir_output_is(<<'CODE', <<'OUTPUT', "pir uses no ops");
.sub main :main
    $I0 = 77
    foo(42, $I0)
    print "back\n"
.end

.emit
.pcc_sub foo:
    get_params "(0, 0)", I16, I17
    print I16
    print "\n"
    print I17
    print "\n"
    set_returns "()"
    returncc
.eom
CODE
42
77
back
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "pir call evaled code");
.sub main :main
    .local string s
    s  = ".sub foo\n"
    s .= ".param int i\n"
    s .= ".param int j\n"
    s .= "print_item i\n"
    s .= "print_item j\n"
    s .= "print_newline\n"
    s .= ".return(99)\n"
    s .= ".end\n"
    .local pmc comp
    comp = compreg "PIR"
    $P0 = comp(s)
    $I0 = 77
    $I0 = foo(42, $I0)
    print $I0
    print "\n"
.end

CODE
42 77
99
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "tailcall 4 - pir calls");
.sub main :main
    .const .Sub f = "foo"
    print "main\n"
    $S0 = f()
    print $S0
.end
.sub foo
    .const .Sub b = "bar"
    print "foo\n"
    .return b("from_foo\n")
.end
.sub bar
    .param string s
    print "bar\n"
    print s
    .return ("bar_ret\n")
.end
CODE
main
foo
bar
from_foo
bar_ret
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "type conversion - native");
.sub main :main
    foo(42, "42", 42.20)
.end
.sub foo
    get_params "(0,0,0)", $I0, $I1, $I2
    print_item $I0
    print_item $I1
    print_item $I2
    print_newline
    # yeah fetch args again
    get_params "(0,0,0)", $N0, $N1, $N2
    print_item $N0
    print_item $N1
    print_item $N2
    print_newline
    get_params "(0,0,0)", $S0, $S1, $S2
    print_item $S0
    print_item $S1
    print_item $S2
    print_newline
.end
CODE
42 42 42
42.0 42.0 42.2
42 42 42.2
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "type conversion - PIR const");
.const int MYCONST = -2
.sub main :main
    $P0 = new PerlString
    "foo"(MYCONST)
.end
.sub "foo"
    .param string str1 :optional
    .param int has_s   :opt_flag
    print str1
    print "\n"
.end
CODE
-2
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "optional args, :opt_flag");
.sub main :main
    $P0 = new String
    $P0 = "hello\n"
    foo($P0)
    foo()
.end
.sub foo
    .param pmc p1  :optional
    .param int i1  :opt_flag

    if_null p1, skip
    print p1
skip:
    print i1
    print "\n"
.end
CODE
hello
1
0
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "optional multiple :opt_flag");
.sub main :main
    $P0 = new String
    $P0 = "ok 1\n"
    foo($P0, "ok 2\n", "ok 3\n")
.end
.sub foo
    .param pmc p1  :optional
    .param int i1  :opt_flag
    .param pmc p2  :optional
    .param int i2  :opt_flag
    .param pmc p3  :optional
    .param int i3  :opt_flag
    .param pmc p4  :optional
    .param int i4  :opt_flag

    print p1
    print p2
    print p3
    if_null p4, ok
    print "not "
ok:
    print "ok 4\n"
    print_item i1
    print_item i2
    print_item i3
    print_item i4
    print_newline
.end


CODE
ok 1
ok 2
ok 3
ok 4
1 1 1 0
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "optional returns, void ret");
.sub main :main
    .local pmc f
    $I0 = 99
    f = global "foo"
    .pcc_begin
    .pcc_call f
    .result   $P0 :optional
    .result   $I0 :opt_flag
    .pcc_end
    unless $I0,  ex
    print "not "
ex:
    print "ok 1\n"
.end
.sub foo
.end
CODE
ok 1
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "tailcall 5 - arg/param conflict");
.sub main :main
    .local pmc a, b
    a = new Integer
    a = 1
    b = new Integer
    b = 2
    .local pmc c, d
    (c, d) = foo(a, b)
    eq_addr a, c, ok1
    print "not "
ok1:
    print "ok 1\n"
    eq_addr b, d, ok2
    print "not "
ok2:
    print "ok 2\n"
.end

.sub foo
    .param pmc a
    .param pmc b
    $P0 = new Integer
    $P0 = 3
    .return bar($P0, a, b)
.end

.sub bar
    .param pmc x
    .param pmc a
    .param pmc b
    .return (a, b)
.end
CODE
ok 1
ok 2
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "OO argument passing");
.sub main :main
    .local pmc cl, o, f
    cl = newclass "Foo"
    o = new "Foo"
    o."bar"("ok 1\n")
    f = find_global "Foo", "bar"
    f(o, "ok 2\n")
    o."baz"("ok 3\n")
    f = find_global "Foo", "baz"
    f(o, "ok 4\n")
.end
.namespace ["Foo"]
.sub bar method
    .param string s
    print self
    print " "
    print s
.end
.sub baz
    .param pmc self
    .param string s
    print self
    print " "
    print s
.end
.sub __get_string method
    $S0 = typeof self
    .return ($S0)
.end
CODE
Foo ok 1
Foo ok 2
Foo ok 3
Foo ok 4
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "OO argument passing - 2");
.sub main :main
    .local pmc cl, o, f
    cl = newclass "Foo"
    o = new "Foo"
    $S0 = o
    print $S0
    $S1 = o[2]
    print $S1
    print $S0
.end
.namespace ["Foo"]
.sub __get_string :method
    $S0 = typeof self
    print $S0
    print " "
    .return ("ok 1\n")
.end
.sub __get_string_keyed_int :method
    .param int key
    $S0 = "ok "
    $S1 = key
    $S0 .= $S1
    $S0 .= "\n"
    .return ($S0)
.end
CODE
Foo ok 1
ok 2
ok 1
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "OO argument passing - 3");
.sub main :main
    .local pmc cl, o, f
    cl = newclass "Foo"
    o = new "Foo"
    $S0 = foo(o)
    print $S0
.end
.sub foo
    .param pmc arg
    .return (arg) # force conversion to string
.end
.namespace ["Foo"]
.sub __get_string :method
    $S0 = typeof self
    print $S0
    print " "
    .return ("ok 1\n")
.end

CODE
Foo ok 1
OUTPUT
# see also tcl in leo-ctx5 by Coke; Date 28.08.2005
pir_output_is(<<'CODE', <<'OUTPUT', "bug - :slurpy promotes to :flatten");
.sub main :main
    $P0 = new String
    $P0 = "ok 1\n"
    $P1 = new String
    $P1 = "ok 2\n"
    $P0 = foo($P0, $P1)
    print $P0
.end
.sub foo
    .param pmc p :slurpy
    .return bar(p)
.end
.sub bar
    .param pmc p
    .local pmc q
    q = p[0]
    print q
    q = p[1]
    .return (q)
.end
CODE
ok 1
ok 2
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "call :slurpy with :flat");
.sub _fn1
	.param pmc rest_arg :slurpy
	print "[in _fn1]\n"
	print rest_arg
	print "\n"
.end
.sub main :main
	$P34 = new Array
	$P34 = 0
	## normal flattening direct call, non-slurpy returns
	$P35 = _fn1($P34 :flat)
.end
CODE
[in _fn1]
0
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "call with :flat in the middle");
.sub _fn1
    .param int arg1
    .param int arg2
    .param int arg3
    .param int arg4
    print arg1
    print ' '
    print arg2
    print ' '
    print arg3
    print ' '
    print arg4
    print "\n"
.end
.sub main :main
    $P30 = new Integer
    $P30 = 2
    $P31 = new Integer
    $P31 = 3
    $P34 = new Array
    $P34 = 2
    $P34[0] = $P30
    $P34[1] = $P31
    $I4 = 4
    $P35 = _fn1(1, $P34 :flat, $I4)
.end
CODE
1 2 3 4
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "right number of args via :flat");
.sub _fn1
    .param int arg1
    .param int arg2
    .param int arg3
    .param int arg4
    print arg1
    print ' '
    print arg2
    print ' '
    print arg3
    print ' '
    print arg4
    print "\n"
.end
.sub main :main
    .include "errors.pasm"
    errorson .PARROT_ERRORS_PARAM_COUNT_FLAG
    $P30 = new Integer
    $P30 = 2
    $P31 = new Integer
    $P31 = 3
    $P34 = new Array
    $P34 = 3
    $P34[0] = $P30
    $P34[1] = $P31
    $P34[2] = $P30
    $P35 = _fn1(1, $P34 :flat)
.end
CODE
1 2 3 2
OUTPUT

pir_output_like(<<'CODE', <<'OUTPUT', "too many args via :flat");
.sub _fn1
    .param int arg1
    .param int arg2
    .param int arg3
    .param int arg4
    print arg1
    print ' '
    print arg2
    print ' '
    print arg3
    print ' '
    print arg4
    print "\n"
.end
.sub main :main
    .include "errors.pasm"
    errorson .PARROT_ERRORS_PARAM_COUNT_FLAG
    $P30 = new Integer
    $P30 = 2
    $P31 = new Integer
    $P31 = 3
    $P34 = new Array
    $P34 = 4
    $P34[0] = $P30
    $P34[1] = $P31
    $P34[2] = $P30
    $P34[3] = $P31
    $P35 = _fn1(1, $P34 :flat)
.end
CODE
/too many arguments passed \(5\) - 4 params expected/
OUTPUT

pir_output_like(<<'CODE', <<'OUTPUT', "too few args via :flat");
.sub _fn1
    .param int arg1
    .param int arg2
    .param int arg3
    .param int arg4
    print arg1
    print ' '
    print arg2
    print ' '
    print arg3
    print ' '
    print arg4
    print "\n"
.end
.sub main :main
    .include "errors.pasm"
    errorson .PARROT_ERRORS_PARAM_COUNT_FLAG
    $P30 = new Integer
    $P30 = 2
    $P31 = new Integer
    $P31 = 3
    $P34 = new Array
    $P34 = 2
    $P34[0] = $P30
    $P34[1] = $P31
    $P35 = _fn1(1, $P34 :flat)
.end
CODE
/too few arguments passed \(3\) - 4 params expected/
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "tailcall to NCI");
.sub main :main
    .local pmc s
    s = new .String
    s = "OK 1\n"
    $S0 = s."lower"()
    print $S0
    s = "OK 2\n"
    $S1 = foo(s)
    print $S1
.end
.sub foo
    .param pmc s
    .return s."lower"()
.end
CODE
ok 1
ok 2
OUTPUT


# bug - repeated calls to eval'd sub crashes (pmichaud, 2005.10.27)
pir_output_is(<<'CODE', <<'OUTPUT', "repeated calls to eval'd sub");
.sub main :main
    .local string s
    .local pmc outer
    s =  ".namespace [ \"XYZ\" ]\n"
    s .= ".sub outer\n"
    s .= "    .param int n\n"
    s .= "    $I0 = n % 1000\n"
    s .= "    if $I0 goto end\n"
    s .= "    print n\n"
    s .= "    print \"\\n\"\n"
    s .= "  end:\n"
    s .= ".end\n"
    $P0 = compreg "PIR"
    $P0(s)
    outer = find_global "XYZ", "outer"
    $I1 = 0
  loop:
    inc $I1
    if $I1 > 10000 goto end
    outer($I1)
    goto loop
  end:
.end
CODE
1000
2000
3000
4000
5000
6000
7000
8000
9000
10000
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "tailcall - overlapping Ix");
.sub main :main
    ($I0, $I1) = foo()
    print $I0
    print $I1
    print "\n"
.end
.sub foo
    .const .Sub b = "bar"
    I0 = 10
    I1 = 20
    set_args "(0,0)", I1, I0
    tailcall b
.end
.sub bar
    get_params "(0,0)", I0, I1
    .return (I0, I1)
.end
CODE
2010
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "tailcall - optional not set");
.sub main :main
    foo()
    print "ok\n"
.end
.sub foo
    .const .Sub b = "bar"
    .return b(10, 20)
.end
.sub bar
    .param int a
    .param int b
    .param int c :optional
    .param int has_c :opt_flag
    print_item a
    print_item b
    unless has_c goto no_c
    print_item c
    goto got_c
no_c:
    print_item 'no'
got_c:
    print_newline
.end
CODE
10 20 no
ok
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "tailcall - optional set");
.sub main :main
    foo()
    print "ok\n"
.end
.sub foo
    .const .Sub b = "bar"
    .return b(10, 20, 30)
.end
.sub bar
    .param int a
    .param int b
    .param int c :optional
    .param int has_c :opt_flag
    print_item a
    print_item b
    unless has_c goto no_c
    print_item c
    goto got_c
no_c:
    print_item 'no'
got_c:
    print_newline
.end
CODE
10 20 30
ok
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "clone_key_arg");
.sub main :main
    foo()
    print "ok\n"
.end

.sub foo
    .local pmc cl, o
    cl = newclass "MyClass"
    o = new "MyClass"
    $S0 = "key"
    $I0 = 3
    o[$S0;$I0] = 42
.end

.namespace ["MyClass"]

# key arguments in register have to be expanded into their
# values because in the called sub frame the original refered
# registers just don't exist              #' for vim

.sub __set_integer_keyed :method
    .param pmc key
    .param int val
    print_item key        # print first key
    key = shift key       # get next key
    print_item key
    print_item val
    print_newline
.end
CODE
key 3 42
ok
OUTPUT

# result_info op

pir_output_is(<<'CODE', <<'OUTPUT', "result_info op");
.sub main :main
    test()
    $I0 = test()
    ($I1, $I2, $I3) = test()
.end

.sub test
    $P0 = result_info
    $I0 = elements $P0
    print $I0
    print "\n"
    .return (0)
.end
CODE
0
1
3
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "result_info op with eval");
.sub main :main
    $S0 = <<"TESTSUB"
.sub test
    $P0 = result_info
    $I0 = elements $P0
    print $I0
    print "\\n"
.end
TESTSUB
    $P0 = compreg "PIR"
    $P1 = $P0($S0)
    test()
    $I0 = test()
    ($I1, $I2, $I3) = test()
.end
CODE
0
1
3
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', ":slurpy result");
.sub main :main
   ($P0 :slurpy) = foo()
   $S0 = $P0[0]
   print $S0
   $S0 = $P0[1]
   print $S0
.end
.sub foo
   .return("ok 1\n", "ok 2\n")
.end
CODE
ok 1
ok 2
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', ":optional result");
.sub main :main
   ($S0 :optional, $I1 :opt_flag) = foo()
   unless $I1 goto no_ret
   print "ok 1\n"
   print $S0
   end
no_ret:
   print "not ok 1\n"
.end
.sub foo
   .return("ok 2\n")
.end
CODE
ok 1
ok 2
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', ":optional result");
.sub main :main
   ($S0 :optional, $I1 :opt_flag) = foo()
   if $I1 goto has_ret
   print "ok 1\n"
   end
has_ret:
   print "not ok 1\n"
.end
.sub foo
   .return()
.end
CODE
ok 1
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "set_args via continuation -> results");
.sub main :main
    .local string result
    result = foo("ok 1\n")
    print result
.end

.sub foo
    .param string s
    .local pmc cc
    .include 'interpinfo.pasm'
    cc = interpinfo .INTERPINFO_CURRENT_CONT
    bar(cc, s)
.end

.sub bar
    .param pmc cc
    .param string s
    print s
    cc("ok 2\n")
.end
CODE
ok 1
ok 2
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "call evaled vtable code");
.sub main :main
    .local string s
    .local pmc cl, o
    cl = newclass "Foo"
    s  = ".namespace ['Foo']\n"
    s .= ".sub __get_integer_keyed_int :method\n"
    s .= ".param int i\n"
    s .= "i += 5\n"
    s .= ".return(i)\n"
    s .= ".end\n"
    .local pmc comp
    comp = compreg "PIR"
    $P0 = comp(s)
    o = new 'Foo'
    $I0 = o[12]
    print $I0
    print "\n"
.end
CODE
17
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "named - 1");
.pcc_sub main:
    set_args "(0x80, 0, 0x80, 0)", "b", 10, "a", 20
    get_results "()"
    find_name P1, "foo"
    invokecc P1
    print "ok\n"
    end
.pcc_sub foo:
    get_params "(0x80, 0, 0x80, 0)", "a", I0, "b", I1
    print_item I1
    print_item I0
    print_newline
    returncc
CODE
10 20
ok
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "named - 2 flatten");
.pcc_sub main:
    new P0, .Hash
    set P0['a'], 20
    set P0['b'], 10
    set_args "(0x88)", P0            # :flatten :named
    get_results "()"
    find_name P1, "foo"
    invokecc P1
    print "ok\n"
    end
.pcc_sub foo:
    get_params "(0x80, 0, 0x80, 0)", "a", I0, "b", I1
    print_item I1
    print_item I0
    print_newline
    returncc
CODE
10 20
ok
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "named - 3 slurpy hash");
.pcc_sub main:
    set_args "(0x80, 0, 0x80, 0,0x80, 0)", "a", 10, "b", 20, 'c', 30
    get_results "()"
    find_name P1, "foo"
    invokecc P1
    print "ok\n"
    end
.pcc_sub foo:
    get_params "(0x80, 0, 0x88)", "a", I0, P0
    print_item I0
    elements I1, P0
    print_item I1
    typeof S0, P0
    print_item S0
    set I2, P0['b']
    print_item I2
    set I2, P0['c']
    print_item I2
    print_newline
    returncc

CODE
10 2 Hash 20 30
ok
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "named - 4 positional -> named");
.pcc_sub main:
    set_args  "(0, 0, 0)", 10, 20, 30
    get_results "()"
    find_name P1, "foo"
    invokecc P1
    print "ok\n"
    end
.pcc_sub foo:
    get_params "(0x80, 0, 0x80, 0, 0x80, 0)", "a", I0, "b", I1, 'c', I2
    print_item I0
    print_item I1
    print_item I2
    print_newline
    returncc
CODE
10 20 30
ok
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "named - 5 slurpy array -> named");
.pcc_sub main:
    set_args  "(0, 0, 0, 0x80, 0, 0x80, 0)", 10, 20, 30, 'a', 40, 'b', 50
    get_results "()"
    find_name P1, "foo"
    invokecc P1
    print "ok\n"
    end
.pcc_sub foo:
    get_params "(0, 0x08, 0x80, 0, 0x80, 0)", I0, P0, "b", I1, "a", I2
    print_item I0
    set I0, P0[0]
    print_item I0
    set I0, P0[1]
    print_item I0
    print_item I1
    print_item I2
    print_newline
    returncc
CODE
10 20 30 50 40
ok
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', ":optional followed by :slurpy (empty)");
.sub main :main
	_write_thing(3)
.end
.sub _write_thing
	.param pmc arg1 :optional
	.param pmc rest_arg :slurpy
	print arg1
	print ' '
	print rest_arg
	print "\n"
.end
CODE
3 0
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', ":optional followed by :slurpy (used)");
.sub main :main
	_write_thing(3, 4, 5)
.end
.sub _write_thing
	.param pmc arg1 :optional
	.param pmc rest_arg :slurpy
	print arg1
	print ' '
	print rest_arg
	print "\n"
.end
CODE
3 2
OUTPUT

## Named
pir_output_is(<<'CODE', <<'OUTPUT', ":named(\"...\") syntax for .param and sub call");
.sub main :main
        foo( 10 :named("b"), 20 :named("a"))
        print "ok\n"
        end
.end

.sub foo
        .param int c :named("a")
        .param int d :named("b")

        print_item d
        print_item c
        print_newline
        .return()
.end
CODE
10 20
ok
OUTPUT


## Named
pir_output_is(<<'CODE', <<'OUTPUT', ":named(\"...\") syntax for the 4 kind");
.sub main :main
        ($I0 :named("b"), $I1 :named("a")) = foo( 10 :named("b"), 20 :named("a"))
        print_item $I0
        print_item $I1
        print_newline
        print "ok\n"

        end
.end

.sub foo
        .param int c :named("a")
        .param int d :named("b")

        print_item d
        print_item c
        print_newline

        .return ( 10 :named("a"), 20 :named("b"))
.end
CODE
10 20
20 10
ok
OUTPUT



## Named
pir_output_is(<<'CODE', <<'OUTPUT', " 'foo' => 10 syntax for function call");
.sub main :main
        foo ('a'=>20,'b'=>10)
        print "ok\n"

        end
.end

.sub foo
        .param int c :named("a")
        .param int d :named("b")

        print_item d
        print_item c
        print_newline

        .return ()
.end
CODE
10 20
ok
OUTPUT


## Named
pir_output_is(<<'CODE', <<'OUTPUT', " 'foo' => d syntax for parameters");
.sub main :main
        foo ('a'=>20,'b'=>10)
        print "ok\n"

        end
.end

.sub foo
        .param int "b" => d
        .param int "a" => c

        print_item d
        print_item c
        print_newline

        .return ()
.end
CODE
10 20
ok
OUTPUT


## Named
pir_output_is(<<'CODE', <<'OUTPUT', " 'foo' => d syntax for target list");
.sub main :main
        ("b" => $I0 , "a" => $I1) = foo( "b" => 10 , "a" => 20)
        print_item $I0
        print_item $I1
        print_newline
        print "ok\n"

        end
.end

.sub foo
        .param int "a" => c
        .param int "b" => d

        print_item d
        print_item c
        print_newline

        .return ( 10 :named("a"), 20 :named("b"))
.end
CODE
10 20
20 10
ok
OUTPUT


## Named
pir_output_is(<<'CODE', <<'OUTPUT', " 'foo' => d syntax for return");
.sub main :main
        ("b" => $I0 , "a" => $I1) = foo( "b" => 10 , "a" => 20)
        print_item $I0
        print_item $I1
        print_newline
        print "ok\n"

        end
.end

.sub foo
        .param int "a" => c
        .param int "b" => d

        print_item d
        print_item c
        print_newline

        .return ( "a" => 10, "b" => 20 )
.end
CODE
10 20
20 10
ok
OUTPUT

pir_output_like(<<'CODE', <<'OUTPUT', "named => pos passing");
.sub main :main
        foo( "b" => 10 , "a" => 20)
        print "never\n"
        end
.end

.sub foo
        .param int a
        .param int b
.end
CODE
/many named arguments/
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "named optional - set");
.sub main :main
        foo ('a'=>20,'b'=>10)
        print "ok\n"
.end

.sub foo
        .param int d :named('b')
	.param int c :named('a') :optional
        print_item d
        print_item c
        print_newline
.end
CODE
10 20
ok
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "named optional - set", todo => 'imcc.y support');
.sub main :main
        foo ('a'=>20,'b'=>10)
        print "ok\n"
.end

.sub foo
        .param int 'b' => d
	.param int 'a' => c  :optional
        print_item d
        print_item c
        print_newline
.end
CODE
10 20
ok
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "named optional - set, :opt_flag");
.sub main :main
        foo ('a'=>20,'b'=>10)
        print "ok\n"
.end

.sub foo
        .param int d :named('b') :optional
	.param int has_d :opt_flag
	.param int c :named('a') :optional
	.param int has_c :opt_flag
        print_item d
        print_item has_d
        print_item c
        print_item has_c
        print_newline
.end
CODE
10 1 20 1
ok
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "named flat/slurpy");
.sub main :main
        .local pmc h
        h = new .Hash
        h['a'] = 20
        h['b'] = 10
        foo( h :named :flat )
        print "ok\n"
        end
.end

.sub foo
        .param pmc h :named :slurpy
        $I0 = h['a']
        $I1 = h['b']
        print_item $I0
        print_item $I1
        print_newline
.end
CODE
20 10
ok
OUTPUT


## remember to change the number of tests :-)
BEGIN { plan tests => 77 }

