#! perl -w

use Parrot::Test tests => 22;
use Test::More;

output_is(<<'CODE', <<'OUTPUT', "set_eh - clear_eh");
    newsub P20, .Exception_Handler, _handler
    set_eh P20
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

output_is(<<'CODE', <<'OUTPUT', "set_eh - throw");
    print "main\n"
    newsub P20, .Exception_Handler, _handler
    set_eh P20

    new P30, .Exception
    throw P30
    print "not reached\n"
    end
_handler:
    print "catched it\n"
    end
CODE
main
catched it
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "set_eh - throw - message");
    print "main\n"
    newsub P20, .Exception_Handler, _handler
    set_eh P20

    new P30, .Exception
    set P30["_message"], "something happend"
    throw P30
    print "not reached\n"
    end
_handler:
    print "catched it\n"
    set S0, P5["_message"]	# P5 is the exception object
    print S0
    print "\n"
    end
CODE
main
catched it
something happend
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "set_eh - throw - message, check P5");
    print "main\n"
    newsub P20, .Exception_Handler, _handler
    set_eh P20

    new P30, .Exception
    set P30["_message"], "something happend"
    new P5, .PerlUndef
    set P5, "a string\n"
    throw P30
    print "not reached\n"
    end
_handler:
    print "catched it\n"
    set S0, P5["_message"]	# P5 is the exception object
    print S0
    print "\n"
    save P0		# preserve reg
    set P0, P5["_P5"]	# original P5
    print P0
    restore P0
    end
CODE
main
catched it
something happend
a string
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "set_eh - throw - lexical");
    print "main\n"
    new_pad 0
    new P0, .PerlInt
    set P0, 42
    store_lex -1, "$a", P0
    newsub P20, .Exception_Handler, _handler
    set_eh P20

    new P30, .Exception
    throw P30
    print "not reached\n"
    end
_handler:
    print "catched it\n"
    find_lex P0, "$a"
    print P0
    print "\n"
    end
CODE
main
catched it
42
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "set_eh - throw - return");
    print "main\n"
    newsub P20, .Exception_Handler, _handler
    set_eh P20

    new P30, .Exception
    set P30["_message"], "something happend"
    throw P30
    print "back again\n"
    end
_handler:
    print "catched it\n"
    set S0, P5["_message"]	# P5 is the exception object
    print S0
    print "\n"
    set P2, P5["_invoke_cc"]	# the return continuation
    invoke P2
CODE
main
catched it
something happend
back again
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "set_eh - throw - return, change lexical");
    print "main\n"
    new_pad 0
    new P0, .PerlInt
    set P0, 42
    store_lex -1, "$a", P0
    newsub P20, .Exception_Handler, _handler
    set_eh P20

    new P30, .Exception
    set P30["_message"], "something happend"
    throw P30
    print "back again\n"
    find_lex P0, "$a"
    print P0
    print "\n"
    end
_handler:
    print "catched it\n"
    set S0, P5["_message"]	# P5 is the exception object
    print S0
    print "\n"
    find_lex P0, "$a"
    print P0
    print "\n"
    inc P0
    set P2, P5["_invoke_cc"]	# the return continuation
    invoke P2
CODE
main
catched it
something happend
42
back again
43
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "throw - no handler");
    new P0, .Exception
    set P0["_message"], "something happend"
    throw P0
    print "not reached\n"
    end
CODE
something happend
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "throw - no handler, no message");
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
No exception handler and no message
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "throw - no handler, no message");
    new P0, .Exception
    throw P0
    print "not reached\n"
    end
CODE
No exception handler and no message
OUTPUT
output_is(<<'CODE', <<'OUTPUT', "2 exception handlers");
    print "main\n"
    newsub P20, .Exception_Handler, _handler1
    set_eh P20
    newsub P21, .Exception_Handler, _handler2
    set_eh P21

    new P30, .Exception
    set P30["_message"], "something happend"
    throw P30
    print "not reached\n"
    end
_handler1:
    print "catched it in 1\n"
    set S0, P5["_message"]
    print S0
    print "\n"
    end
_handler2:
    print "catched it in 2\n"
    set S0, P5["_message"]
    print S0
    print "\n"
    end
CODE
main
catched it in 2
something happend
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "2 exception handlers, throw next");
    print "main\n"
    newsub P20, .Exception_Handler, _handler1
    set_eh P20
    newsub P21, .Exception_Handler, _handler2
    set_eh P21

    new P30, .Exception
    set P30["_message"], "something happend"
    throw P30
    print "not reached\n"
    end
_handler1:
    print "catched it in 1\n"
    set S0, P5["_message"]
    print S0
    print "\n"
    end
_handler2:
    print "catched it in 2\n"
    set S0, P5["_message"]
    print S0
    print "\n"
    throw P5	# XXX rethrow?
    end
CODE
main
catched it in 2
something happend
catched it in 1
something happend
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "2 exception handlers, throw next - return");
    print "main\n"
    newsub P20, .Exception_Handler, _handler1
    set_eh P20
    newsub P21, .Exception_Handler, _handler2
    set_eh P21

    new P30, .Exception
    set P30["_message"], "something happend"
    throw P30
    print "back in main\n"
    end
_handler1:
    print "catched it in 1\n"
    set S0, P5["_message"]
    print S0
    print "\n"
    set P2, P5["_invoke_cc"]	# the return continuation
    invoke P2
_handler2:
    print "catched it in 2\n"
    set S0, P5["_message"]
    print S0
    print "\n"
    throw P5
    print "back in 2\n"
    # XXX we cant return from here, the _return_cc in P5 is common
    # to both exception handlers
    end
CODE
main
catched it in 2
something happend
catched it in 1
something happend
back in 2
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "throw from a sub");
    print "main\n"
    newsub P20, .Exception_Handler, _handler
    set_eh P20
    newsub P0, .Sub, _sub
    invokecc
    print "back in main\n"
    end

_sub:
    new P30, .Exception
    set P30["_message"], "something happend"
    throw P30
    print "back in sub\n"
    invoke P1
_handler:
    print "catched it\n"
    set S0, P5["_message"]
    print S0
    print "\n"
    set P2, P5["_invoke_cc"]	# the return continuation
    invoke P2
CODE
main
catched it
something happend
back in sub
back in main
OUTPUT

output_is(<<'CODE', <<OUT, "die_hard");
    newsub P0, .Exception_Handler, _handler
    set_eh P0
    die_hard 3, 100
    print "not reached\n"
    end
_handler:
    print "catched it\n"
    end
CODE
catched it
OUT

output_is(<<'CODE', <<OUT, "die_hard, error, severity");
    newsub P0, .Exception_Handler, _handler
    set_eh P0
    die_hard 3, 100
    print "not reached\n"
    end
_handler:
    print "catched it\n"
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
catched it
error 100
severity 3
OUT

output_is(<<'CODE', <<OUT, "die_hard - no handler");
    die_hard 3, 100
    print "not reached\n"
    end
_handler:
    print "catched it\n"
    end
CODE
No exception handler and no message
OUT

output_like(<<'CODE', <<OUT, "find_lex");
    new_pad 0
    newsub P0, .Exception_Handler, _handler
    set_eh P0
    find_lex P1, "no_such_thing"
    print "resumed\n"
    end
_handler:
    print "catched it\n"
    set S0, P5["_message"]
    print S0
    print "\n"
    set P1, P5["_invoke_cc"]
    invoke P1
CODE
/^catched it
Lexical 'no_such_thing' not found
resumed
/
OUT

output_is(<<'CODE', <<'OUTPUT', "rethrow");
    print "main\n"
    newsub P20, .Exception_Handler, _handler1
    set_eh P20
    newsub P21, .Exception_Handler, _handler2
    set_eh P21

    new P30, .Exception
    set P30["_message"], "something happend"
    throw P30
    print "back in main\n"
    end
_handler1:
    print "catched it in 1\n"
    set S0, P5["_message"]
    print S0
    print "\n"
    set P2, P5["_invoke_cc"]	# the return continuation
    invoke P2
_handler2:
    print "catched it in 2\n"
    set S0, P5["_message"]
    print S0
    print "\n"
    rethrow P5
CODE
main
catched it in 2
something happend
catched it in 1
something happend
back in main
OUTPUT


output_is(<<'CODE', <<'OUTPUT', "clear_eh, set_eh again");
# bug reported by Jos Visser
	new_pad 0

	newsub P10, .Exception_Handler, _handler

	set I12, 1
	set_eh P10
	find_lex P13,"a"
	clear_eh
	new P13, .PerlNum
	store_lex -1,"a",P13
	set P13, I12
	find_lex P14,"a"
	print P14

	set I12, 2
	set_eh P10
	print "mark1\n"
	find_lex P13,"b"
	print "mark2\n"
	end

_handler:
        print "Hi from handler\n"
        set P2, P5["_invoke_cc"]
        invoke P2

CODE
Hi from handler
1mark1
Hi from handler
mark2
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "check that coroutines handler isnt run");
    print "main\n"
    newsub P0, .Coroutine, _sub
    invokecc
    print "back in main\n"
    find_lex P3, -1, "nix"
    end

_sub:
    print "in coro\n"
    newsub P20, .Exception_Handler, _handler
    set_eh P20
    invoke P1

_handler:
    print "catched it\n"
    set S0, P5["_message"]
    print S0
    print "\n"
    set P2, P5["_invoke_cc"]	# the return continuation
    invoke P2
CODE
main
in coro
back in main
Lexical 'nix' not found
OUTPUT

output_is(<<'CODE', '', "exit exception");
    noop
    exit 0
    print "not reached\n"
    end
CODE
1;

