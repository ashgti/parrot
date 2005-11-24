#! perl -w
# Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

t/pmc/coroutines.t - Coroutines

=head1 SYNOPSIS

	% perl -Ilib t/pmc/coroutines.t

=head1 DESCRIPTION

Tests the C<Coroutine> PMC.

=cut

use Parrot::Test tests => 12;
use Test::More;

output_is(<<'CODE', <<'OUTPUT', "Coroutine 1");
.include "interpinfo.pasm"
.pcc_sub _main:
    .const .Sub P0 = "_coro"
    new P10, .Integer
    set P10, 2
    store_global "i", P10
lp:
    invokecc P0
    print "back "
    print P10
    print "\n"
    if P10, lp
    print "done\n"
    end
.pcc_sub _coro:
loop:
    find_global P11, "i"
    dec P11
    yield
    branch loop
CODE
back 1
back 0
done
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "Coroutines and lexicals 1");
.include "interpinfo.pasm"
.pcc_sub main:
    new_pad 0

    new P20, .String
    set P20, "main\n"
    store_lex -1, "a", P20

    .const .Sub P0 = "co1"
    invokecc P0
    find_lex P21, "a"
    print P21
    invokecc P0
    print P21
    print "done\n"
    end

.pcc_sub co1:
    new_pad 1
    new P22, .String
    set P22, "coro\n"

    store_lex -1, "a", P22
    yield
    print P22
    yield
CODE
main
coro
main
done
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "Coroutines and lexicals 2");
.include "interpinfo.pasm"
.pcc_sub main:
    new_pad 0
    new_pad 1

    new P20, .Integer
    set P20, 20
    new P21, .Integer
    set P21, 21

    store_lex -1, "a", P20
    store_lex -2, "b", P21

    .const .Sub P5 = "co1"
    .const .Sub P6 = "co2"

    null I0
    set I3, 2
    set_args "(0,0)", P5, P6
    invokecc P5

    find_lex P10, "b"
    print P10
    print "\n"

    find_lex P10, "a"
    print P10
    print "\n"

    invokecc P5

    find_lex P10, "b"
    print P10
    print "\n"

    find_lex P10, "a"
    print P10
    print "\n"

    invokecc P5

    print "done\n"
    end

.pcc_sub co1:
    get_params "(0,0)", P5, P6
    new_pad 2

    new P22, .Integer
    set P22, 22

    store_lex -1, "b", P22  # hides
    store_lex "a", P22      # replaces

    # invoke c02
    invokecc P6

    # return
    yield

    find_lex P10, "b"
    print P10
    print "\n"

    find_lex P10, "a"
    print P10
    print "\n"

    # return again
    yield

    find_lex P10, "b"
    print P10
    print "\n"

    find_lex P10, "a"
    print P10
    print "\n"

    # return again
    yield

.pcc_sub co2:
    new_pad 1

    # return
    yield

CODE
21
22
22
22
21
22
22
22
done
OUTPUT


pir_output_is(<<'CODE', <<'OUTPUT', "Coroutines - M. Wallace yield example");

.sub __main__
    .local object return
    .local object counter
    .const .Sub itr = "_iterator"

    .local object zero
    zero = new Integer
    zero = 0

    new return, .Continuation
    set_addr return, return_here
    loop:
        .pcc_begin
            .pcc_call itr, return
            .result counter
        .pcc_end

        print counter
        print " "

        zero = 0
        print zero
        print "\n"
    goto loop
return_here:
    end
.end

.pcc_sub _iterator
    .local object x
    x = new Integer
    x = 0
    iloop:
        .pcc_begin_yield
        .return x
        .pcc_end_yield
        x = x + 1
    if x <= 10 goto iloop
    returncc
.end
CODE
0 0
1 0
2 0
3 0
4 0
5 0
6 0
7 0
8 0
9 0
10 0
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "Coroutine - exception in main");
.include "interpinfo.pasm"
_main:
    .const .Sub P0 = "_coro"
    push_eh _catchm
    new P16, .Integer
    set P16, 2
    store_global "i", P16
lp:
    invokecc P0
    print "back "
    print P16
    print "\n"
    find_global P17, "no_such"
    if P16, lp
    print "done\n"
    end
_catchm:
    print "catch main\n"
    end

.pcc_sub _coro:
    push_eh _catchc
corolp:
    find_global P17, "i"
    dec P17
    yield
    branch corolp
_catchc:
    print "catch coro\n"
    end
CODE
back 1
catch main
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "Coroutine - exception in coro");
.include "interpinfo.pasm"
_main:
    .const .Sub P0 = "_coro"
    push_eh _catchm
    new P16, .Integer
    set P16, 2
    store_global "i", P16
lp:
    invokecc P0
    print "back "
    print P16
    print "\n"
    if P16, lp
    print "done\n"
    end
_catchm:
    print "catch main\n"
    end

.pcc_sub _coro:
    push_eh _catchc
corolp:
    find_global P17, "i"
    dec P17
    yield
    find_global P17, "no_such"
    branch corolp
_catchc:
    print "catch coro\n"
    end
CODE
back 1
catch coro
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "Coroutine - exception in coro no handler");
.include "interpinfo.pasm"
_main:
    .const .Sub P0 = "_coro"
    push_eh _catchm
    new P16, .Integer
    set P16, 2
    store_global "i", P16
lp:
    invokecc P0
    print "back "
    print P16
    print "\n"
    if P16, lp
    print "done\n"
    end
_catchm:
    print "catch main\n"
    end
.pcc_sub _coro:
corolp:
    find_global P17, "i"
    dec P17
    yield
    find_global P17, "no_such"
    branch corolp
_catchc:
    print "catch coro\n"
    end
CODE
back 1
catch main
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "Coroutine - exception in coro rethrow");
.include "interpinfo.pasm"
_main:
    .const .Sub P0 = "_coro"
    push_eh _catchm
    new P16, .Integer
    set P16, 2
    store_global "i", P16
lp:
    invokecc P0
    print "back "
    print P16
    print "\n"
    if P16, lp
    print "done\n"
    end
_catchm:
    print "catch main\n"
    end

.pcc_sub _coro:
    push_eh _catchc
corolp:
    find_global P17, "i"
    dec P17
    yield
    find_global P17, "no_such"
    branch corolp
_catchc:
    print "catch coro\n"
    rethrow P5
    end
CODE
back 1
catch coro
catch main
OUTPUT


pir_output_is(<<'CODE', 'Coroutine', "Coro new - type");

.sub main :main
    .local pmc c
    c = global "coro"
    typeof S0, c
    print S0
.end
.sub coro
    .local pmc x
    x = new Integer
    x = 0
    iloop:
        .yield (x)
        x = x + 1
    if x <= 4 goto iloop
.end
CODE

pir_output_is(<<'CODE', '01234', "Coro new - yield");

.sub main :main
    .local pmc c
    c = global "coro"
loop:
    .pcc_begin
    .pcc_call c
    .result   $P0 :optional
    .result   $I0 :opt_flag
    .pcc_end
    unless $I0,  ex
    print $P0
    goto loop
ex:
.end
.sub coro
    .local pmc x
    x = new Integer
    x = 0
    iloop:
        .yield (x)
        x = x + 1
    if x <= 4 goto iloop
.end
CODE

pir_output_is(<< 'CODE', << 'OUTPUT', "check whether interface is done");

.sub _main
    .local pmc pmc1
    pmc1 = new Coroutine
    .local int bool1
    does bool1, pmc1, "scalar"      # XXX WTF
    print bool1
    print "\n"
    does bool1, pmc1, "no_interface"
    print bool1
    print "\n"
    end
.end
CODE
1
0
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "re-entering coro from another sub");

.sub main :main
    .local int z
    .const .Sub corou = "_coroufn"
    corou("from main")
    z = 0
  loop:
    unless z < 4 goto end
    met(corou)
    inc z
    goto loop
  end:
.end

.sub met
    .param pmc corou
    corou()
.end

.sub _coroufn
    .param string x
    .local int j
    print "coroutine: first call "
    print x
    print "\n"
    j = 0
  coroufn_1:
    inc j
    print "yield #"
    print j
    print "\n"
    .yield()
    goto coroufn_1
.end
CODE
coroutine: first call from main
yield #1
yield #2
yield #3
yield #4
yield #5
OUTPUT


