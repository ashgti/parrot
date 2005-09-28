#! perl -w
# Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

t/pmc/exception.t - Exception Handling

=head1 SYNOPSIS

	% perl -Ilib t/pmc/exception.t

=head1 DESCRIPTION

Tests C<Exception> and C<Exception_Handler> PMCs.

=cut

use Parrot::Test tests => 25;
use Test::More;

output_is(<<'CODE', <<'OUTPUT', "push_eh - clear_eh");
    push_eh _handler
    print "ok 1\n"
    clear_eh
    print "ok 2\n"
    end
_handler:
    end
CODE
ok 1
ok 2
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "push_eh - throw");
    print "main\n"
    push_eh _handler
    new P30, .Exception
    throw P30
    print "not reached\n"
    end
_handler:
    print "caught it\n"
    end
CODE
main
caught it
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "push_eh - throw - message");
    print "main\n"
    push_eh _handler

    new P30, .Exception
    set P30["_message"], "something happend"
    throw P30
    print "not reached\n"
    end
_handler:
    print "caught it\n"
    set S0, P5["_message"]	# P5 is the exception object
    print S0
    print "\n"
    end
CODE
main
caught it
something happend
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "push_eh - throw - message, check P5");
    print "main\n"
    push_eh _handler

    new P30, .Exception
    set P30["_message"], "something happend"
    new P5, .Undef
    set P5, "a string\n"
    throw P30
    print "not reached\n"
    end
_handler:
    print "caught it\n"
    set S0, P5["_message"]	# P5 is the exception object
    print S0
    print "\n"
    set P0, P5["_P5"]	# original P5
    print P0
    end
CODE
main
caught it
something happend
a string
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "push_eh - throw - lexical");
    print "main\n"
    new_pad 0
    new P0, .Integer
    set P0, 42
    store_lex -1, "$a", P0
    push_eh _handler

    new P30, .Exception
    throw P30
    print "not reached\n"
    end
_handler:
    print "caught it\n"
    find_lex P0, "$a"
    print P0
    print "\n"
    end
CODE
main
caught it
42
OUTPUT

output_like(<<'CODE', <<'OUTPUT', "throw - no handler");
    new P0, .Exception
    set P0["_message"], "something happend"
    throw P0
    print "not reached\n"
    end
CODE
/something happend/
OUTPUT

output_like(<<'CODE', <<'OUTPUT', "throw - no handler, no message");
    newsub P20, .Exception_Handler, _handler
    set_eh P20
    new P0, .Exception
    clear_eh
    throw P0
    print "not reached\n"
    end
_handler:
    end
CODE
/No exception handler and no message/
OUTPUT

output_like(<<'CODE', <<'OUTPUT', "throw - no handler, no message");
    new P0, .Exception
    throw P0
    print "not reached\n"
    end
CODE
/No exception handler and no message/
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "2 exception handlers");
    print "main\n"
    push_eh _handler1
    push_eh _handler2

    new P30, .Exception
    set P30["_message"], "something happend"
    throw P30
    print "not reached\n"
    end
_handler1:
    print "caught it in 1\n"
    set S0, P5["_message"]
    print S0
    print "\n"
    end
_handler2:
    print "caught it in 2\n"
    set S0, P5["_message"]
    print S0
    print "\n"
    end
CODE
main
caught it in 2
something happend
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "2 exception handlers, throw next");
    print "main\n"
    push_eh _handler1
    push_eh _handler2

    new P30, .Exception
    set P30["_message"], "something happend"
    throw P30
    print "not reached\n"
    end
_handler1:
    print "caught it in 1\n"
    set S0, P5["_message"]
    print S0
    print "\n"
    end
_handler2:
    print "caught it in 2\n"
    set S0, P5["_message"]
    print S0
    print "\n"
    throw P5	# XXX rethrow?
    end
CODE
main
caught it in 2
something happend
caught it in 1
something happend
OUTPUT

output_is(<<'CODE', <<OUT, "die_hard");
    newsub P0, .Exception_Handler, _handler
    set_eh P0
    die_hard 3, 100
    print "not reached\n"
    end
_handler:
    print "caught it\n"
    end
CODE
caught it
OUT

output_is(<<'CODE', <<OUT, "die_hard, error, severity");
    newsub P0, .Exception_Handler, _handler
    set_eh P0
    die_hard 3, 100
    print "not reached\n"
    end
_handler:
    print "caught it\n"
    set I0, P5["_type"]
    print "error "
    print I0
    print "\n"
    set I0, P5["_severity"]
    print "severity "
    print I0
    print "\n"
    end
CODE
caught it
error 100
severity 3
OUT

output_like(<<'CODE', <<OUT, "die_hard - no handler");
    die_hard 3, 100
    print "not reached\n"
    end
_handler:
    print "caught it\n"
    end
CODE
/No exception handler and no message/
OUT

output_like(<<'CODE', <<OUT, "find_lex");
    new_pad 0
    push_eh _handler
    find_lex P1, "no_such_thing"
    clear_eh
ok:
    print "resumed\n"
    end
_handler:
    print "caught it\n"
    set S0, P5["_message"]
    print S0
    print "\n"
    branch ok
CODE
/^caught it
Lexical 'no_such_thing' not found
resumed
/
OUT

output_is(<<'CODE', '', "exit exception");
    noop
    exit 0
    print "not reached\n"
    end
CODE

pir_output_is(<<'CODE', <<'OUTPUT', "set recursion limit, method call ");

# see also t/op/gc_14.imc

.sub main @MAIN
    .local pmc n
    new_pad 0
    $P0 = getinterp
    $P0."recursion_limit"(50)
    newclass $P0, "b"
    $I0 = find_type "b"
    $P0 = new $I0
    $P1 = new Integer
    $P1 = 0
    n = $P0."b11"($P1)
    print "ok 1\n"
    print n
    print "\n"
.end
.namespace ["b"]
.sub b11 method
    .param pmc n
    .local pmc n1
    new_pad -1
    store_lex -1, "n", n
    n1 = new Integer
    n1 = n + 1
    push_eh catch
    n = self."b11"(n1)
    store_lex -1, "n", n
    clear_eh
catch:
    n = find_lex "n"
    .return(n)
.end
CODE
ok 1
49
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "push_eh - throw");
    print "main\n"
    push_eh handler
    print "ok\n"
    new P30, .Exception
    throw P30
    print "not reached\n"
    end
handler:
    print "caught it\n"
    end
CODE
main
ok
caught it
OUTPUT
1;

output_is(<<'CODE', <<'OUTPUT', "pushmark");
    pushmark 10
    print "ok 1\n"
    popmark 10
    print "ok 2\n"
    end
CODE
ok 1
ok 2
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "pushmark nested");
    pushmark 10
    pushmark 11
    print "ok 1\n"
    popmark 11
    popmark 10
    print "ok 2\n"
    end
CODE
ok 1
ok 2
OUTPUT

output_like(<<'CODE', <<'OUTPUT', "pushmark - pop wrong one");
    pushmark 10
    print "ok 1\n"
    popmark 500
    print "never\n"
    end
CODE
/mark not found/
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "pushaction");
    pushmark 10
    print "ok 1\n"
    .const .Sub P10 = "action"
    pushaction P10
    print "ok 2\n"
    popmark 10
    print "ok 3\n"
    end
.pcc_sub action:
    get_params "(0)", I5
    print "in action I5 = "
    print I5
    print "\n"
    returncc
CODE
ok 1
ok 2
in action I5 = 0
ok 3
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "pushaction, throw");
    push_eh handler
    print "ok 1\n"
    .const .Sub P10 = "action"
    pushaction P10
    print "ok 2\n"
    new P10, .Exception
    throw P10
    print "never\n"
handler:
    print "ok 3\n"
    end
.pcc_sub action:
    get_params "(0)", I5
    print "in action I5 = "
    print I5
    print "\n"
    returncc
CODE
ok 1
ok 2
in action I5 = 1
ok 3
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "pushaction, sub exit");
.sub main
    print "main\n"
    foo()
    print "back\n"
.end

.sub foo
   .const .Sub ac = "action"
    pushaction ac
    print "foo\n"
.end

.sub action
    .param int i
    print "in action I5 = "
    print i
    print "\n"
.end
CODE
main
foo
in action I5 = 0
back
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "pushaction, sub exit - capture CC");
.sub main
    print "main\n"
    foo()
    print "back\n"
.end

.sub foo
   .const .Sub ac = "action"
    pushaction ac
    .include "interpinfo.pasm"
    .local pmc cc
    cc = interpinfo .INTERPINFO_CURRENT_CONT
    print "foo\n"
.end

.sub action
    print "never\n"
.end
CODE
main
foo
back
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "pushaction, sub exit - capture CC, ret");
.sub main
    print "main\n"
    foo()
    print "back\n"
.end

.sub foo
   .const .Sub ac = "action"
    pushaction ac
    .include "interpinfo.pasm"
    .local pmc cc
    cc = interpinfo .INTERPINFO_CURRENT_CONT
    print "foo\n"
    invokecc cc
.end

.sub action
    print "never\n"
.end
CODE
main
foo
back
OUTPUT
