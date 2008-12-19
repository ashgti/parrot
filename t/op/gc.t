#!perl
# Copyright (C) 2001-2005, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 20;

=head1 NAME

t/op/gc.t - Garbage Collection

=head1 SYNOPSIS

        % prove t/op/gc.t

=head1 DESCRIPTION

Tests garbage collection with the C<interpinfo> operation and various
DOD/GC related bugs.

=cut

TODO: {
    local $TODO = "GC_IT Doesn't like any of this";

pasm_output_is( <<'CODE', '1', "sweep 1" );
      interpinfo I1, 2   # How many DOD runs have we done already?
      sweep 1
      interpinfo I2, 2   # Should be one more now
      sub I3, I2, I1
      print I3
      end
CODE

pasm_output_is( <<'CODE', '0', "sweep 0" );
      interpinfo I1, 2   # How many DOD runs have we done already?
      sweep 0
      interpinfo I2, 2   # Should be same
      sub I3, I2, I1
      print I3
      end
CODE

pasm_output_is( <<'CODE', '1', "sweep 0, with object that need destroy" );
      new P0, 'Undef'
      interpinfo I1, 2   # How many DOD runs have we done already?
      needs_destroy P0
      sweep 0
      interpinfo I2, 2   # Should be one more now
      sub I3, I2, I1
      print I3
      end
CODE

pasm_output_is( <<'CODE', '10', "sweep 0, with object that need destroy/destroy" );
      new P0, 'Undef'
      needs_destroy P0
      interpinfo I1, 2   # How many DOD runs have we done already?
      new P0, 'Undef'    # kill object
      sweep 0
      interpinfo I2, 2   # Should be one more now
      sub I3, I2, I1
      sweep 0
      interpinfo I4, 2   # Should be same as last
      sub I5, I4, I2
      print I3           # These create PMCs that need early DOD, so we need
      print I5           # to put them after the second sweep op.
      end
CODE

pasm_output_is( <<'CODE', '1', "collect" );
      interpinfo I1, 3   # How many garbage collections have we done already?
      collect
      interpinfo I2, 3   # Should be one more now
      sub I3, I2, I1
      print I3
      end
CODE

pasm_output_is( <<'CODE', <<'OUTPUT', "collectoff/on" );
      interpinfo I1, 3
      collectoff
      collect
      interpinfo I2, 3
      sub I3, I2, I1
      print I3
      print "\n"

      collecton
      collect
      interpinfo I4, 3
      sub I6, I4, I2
      print I6
      print "\n"

      end
CODE
0
1
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "Nested collectoff/collecton" );
      interpinfo I1, 3
      collectoff
      collectoff
      collecton
      collect           # This shouldn't do anything...    #'
      interpinfo I2, 3
      sub I3, I2, I1
      print I3
      print "\n"

      collecton
      collect           # ... but this should
      interpinfo I4, 3
      sub I6, I4, I2
      print I6
      print "\n"

      end
CODE
0
1
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "vanishing slingleton PMC" );
_main:
    .const 'Sub' P0 = "_rand"
    set I16, 100
    set I17, 0
loop:
    sweep 1
    invokecc P0
    inc I17
    lt I17, I16, loop
    print "ok\n"
    end

.pcc_sub _rand:
    new P16, 'Random'
    set I5, P16[10]
    gt I5, 10, err
    lt I5, 0, err
    returncc
err:
    print "singleton destroyed .Random = ."
    new P16, 'Random'
    typeof S16, P16
    print S16
    print "\n"
    end
CODE
ok
OUTPUT

pir_output_is( <<'CODE', <<OUTPUT, "vanishing return continuation in method calls" );
.sub main :main
    .local pmc o, cl
    cl = newclass "Foo"

    new o, "Foo"
    print "ok\n"
    end
.end

.namespace ["Foo"]
.sub init :vtable :method
    print "init\n"
    sweep 1
    new $P6, 'String'
    set $P6, "hi"
    self."do_inc"()
    sweep 1
.end

.sub do_inc :method
    sweep 1
    inc self
    sweep 1
    print "back from _inc\n"
.end

.sub __increment :method
    print "inc\n"
    sweep 1
.end
CODE
init
inc
back from _inc
ok
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "failing if regsave is not marked" );
    newclass P9, "Source"
    newclass P10, "Source::Buffer"
    new P12, "Source"

    set S20, P12
    print S20
    set S20, P12
    print S20
    end

.namespace ["Source"]
.pcc_sub __get_string:  # buffer
    get_params "0", P2
    getprop P12, "buffer", P2
    sweep 1
    unless_null P12, buffer_ok
    new P12, "Source::Buffer"
    new P14, 'String'
    set P14, "hello\n"
    setprop P12, "buf", P14
    setprop P2, "buffer", P12
buffer_ok:
    set_returns "0", P12
    returncc

.namespace ["Source::Buffer"]
.pcc_sub __get_string:
    get_params "0", P2
    sweep 1
    getprop P12, "buf", P2
    set S16, P12
    set_returns "0", S16
    returncc
CODE
hello
hello
OUTPUT

# this is a stripped down version of imcc/t/syn/pcc_16
# s. also src/pmc/retcontinuation.pmc
pasm_output_is( <<'CODE', <<OUTPUT, "coro context and invalid return continuations" );
.pcc_sub main:
    .const 'Sub' P0 = "co1"
    set I20, 0
l:
    get_results ''
    set_args ''
    invokecc P0
    inc I20
    lt I20, 3, l
    print "done\n"
    end
.pcc_sub co1:
    get_params ''
    set P17, P1
col:
    print "coro\n"
    sweep 1
    yield
    branch col

CODE
coro
coro
coro
done
OUTPUT

pir_output_is( <<'CODE', <<OUTPUT, "Recursion and exceptions" );

# this did segfault with GC_DEBUG

.sub main :main
    .local pmc n
    $P0 = getinterp
    $P0."recursion_limit"(10)
    newclass $P0, "b"
    $P0 = new "b"
    $P1 = new 'Integer'
    $P1 = 0
    n = $P0."b11"($P1)
    print "ok 1\n"
    print n
    print "\n"
.end
.namespace ["b"]
.sub b11 :method
    .param pmc n
    .local pmc n1
    # new_pad -1
    # store_lex -1, "n", n
    n1 = new 'Integer'
    n1 = n + 1
    push_eh catch
    n = self."b11"(n1)
    # store_lex -1, "n", n
    pop_eh
catch:
    # n = find_lex "n"
    .return(n)
.end
CODE
ok 1
9
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "write barrier 1" );
    null I2
    set I3, 100
lp3:
    null I0
    set I1, 1000
    new P1, 'ResizablePMCArray'
lp1:
    new P2, 'ResizablePMCArray'
    new P0, 'Integer'
    set P0, I0
    set P2[0], P0
    set P1[I0], P2
    if I0, not_0
    needs_destroy P0
    # force marking past P2[0]
    sweep 0
not_0:
    new P3, 'Undef'
    new P4, 'Undef'
    inc I0
    lt I0, I1, lp1

    null I0
    # trace 1
lp2:
    set P2, P1[I0]
    set P2, P2[0]
    eq P2, I0, ok
    print "nok\n"
    print "I0: "
    print I0
    print " P2: "
    print P2
    print " type: "
    typeof S0, P2
    print S0
    print " I2: "
    print I2
    print "\n"
    exit 1
ok:
    inc I0
    lt I0, I1, lp2
    inc I2
    lt I2, I3, lp3
    print "ok\n"
    end
CODE
ok
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "write barrier 2 - hash" );
    null I2
    set I3, 100
lp3:
    null I0
    set I1, 100
    new P1, 'Hash'
lp1:
    new P2, 'Hash'
    new P0, 'Integer'
    set P0, I0
    set S0, I0
    set P2["first"], P0
    set P1[S0], P2
    if I0, not_0
    new P0, 'Integer'
    needs_destroy P0
    null P0
    # force full sweep
    sweep 0
not_0:
    new P3, 'Undef'
    new P4, 'Undef'
    inc I0
    lt I0, I1, lp1

    null I0
    # trace 1
lp2:
    set S0, I0
    set P2, P1[S0]
    set P2, P2["first"]
    eq P2, I0, ok
    print "nok\n"
    print "I0: "
    print I0
    print " P2: "
    print P2
    print " type: "
    typeof S0, P2
    print S0
    print " I2: "
    print I2
    print "\n"
    exit 1
ok:
    inc I0
    lt I0, I1, lp2
    inc I2
    lt I2, I3, lp3
    print "ok\n"
    end
CODE
ok
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "write barrier 3 - ref" );
    null I2
    set I3, 10
lp3:
    null I0
    set I1, 100
    new P5, 'Ref'
    new P0, 'Integer'
    needs_destroy P0
    # force partial sweep
    # ref should now be black
    sweep 0
    # store white hash in ref - needs a barrier
    new P1, 'Hash'
    setref P5, P1
    null P1
    new P0, 'Integer'
    needs_destroy P0
    null P0
    # force full sweep
    sweep 0
    deref P1, P5
lp1:
    new P0, 'Integer'
    new P2, 'Ref', P0
    set P0, I0
    set S0, I0
    set P1[S0], P2
    if I0, not_0
    new P0, 'Integer'
not_0:
    new P3, 'Undef'
    new P4, 'Undef'
    inc I0
    lt I0, I1, lp1

    null I0
    deref P1, P5
    # trace 1
lp2:
    set S0, I0
    set P2, P1[S0]
    deref P2, P2
    eq P2, I0, ok
    print "nok\n"
    print "I0: "
    print I0
    print " P2: "
    print P2
    print " type: "
    typeof S0, P2
    print S0
    print " I2: "
    print I2
    print "\n"
    exit 1
ok:
    inc I0
    lt I0, I1, lp2
    inc I2
    lt I2, I3, lp3
    print "ok\n"
    end

CODE
ok
OUTPUT

pasm_output_is( <<'CODE', <<OUTPUT, "write barrier 4 - tqueue" );
    null I2
    set I3, 100
lp3:
    null I0
    set I1, 10
    new P5, 'TQueue'
    new P0, 'Integer'
    needs_destroy P0
    # force partial sweep
    # P5 should now be black
    sweep 0
    # store white queue P1 in black P5 - needs a barrier
    new P1, 'TQueue'
    push P5, P1
    null P1
    new P0, 'Integer'
    needs_destroy P0
    # force  sweep
    sweep 0
    shift P1, P5
    push P5, P1
lp1:
    new P0, 'Integer'
    needs_destroy P0
    # force  sweep
    sweep 0
    set P0, I0
    new P2, 'TQueue'
    push P2, P0
    push P1, P2
    new P3, 'Undef'
    new P4, 'Undef'
    inc I0
    lt I0, I1, lp1

    null I0
    shift P1, P5
lp2:
    shift P2, P1
    shift P2, P2
    eq P2, I0, ok
    print "nok\n"
    print "I0: "
    print I0
    print " P2: "
    print P2
    print " type: "
    typeof S0, P2
    print S0
    print " I2: "
    print I2
    print "\n"
    exit 1
ok:
    inc I0
    lt I0, I1, lp2
    inc I2
    lt I2, I3, lp3
    print "ok\n"
    end

CODE
ok
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "verify deleg_pmc object marking" );
.sub main :main
    .local pmc cl, s, t
    cl = subclass "String", "X"
    addattribute cl, "o3"
    addattribute cl, "o4"
    s = new "X"
    $P0 = new 'String'
    $S0 = "ok" . " 3\n"
    $P0 = $S0
    setattribute s, "o3", $P0
    $P0 = new 'String'
    $S0 = "ok" . " 4\n"
    $P0 = $S0
    setattribute s, "o4", $P0
    null $P0
    null $S0
    null cl
    sweep 1
    s = "ok 1\n"
    print s
    .local int i
    i = 0
lp:
    t = new "X"
    inc i
    if i < 1000 goto lp
    t = "ok 2\n"
    print s
    print t
    $P0 = getattribute s, "o3"
    print $P0
    $P0 = getattribute s, "o4"
    print $P0
.end
CODE
ok 1
ok 1
ok 2
ok 3
ok 4
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "AddrRegistry 1" );
.sub main :main
    .local pmc a, reg, nil
    reg = new 'AddrRegistry'
    a = new 'String'
    null nil
    $I0 = reg[a]
    if $I0 == 0 goto ok1
    print "not "
ok1:
    print "ok 1\n"
    reg[a] = nil
    $I0 = reg[a]
    if $I0 == 1 goto ok2
    print "not "
ok2:
    print "ok 2\n"
    reg[a] = nil
    $I0 = reg[a]
    if $I0 == 2 goto ok3
    print "not "
ok3:
    print "ok 3\n"

    delete reg[a]
    $I0 = reg[a]
    if $I0 == 1 goto ok4
    print "not "
ok4:
    print "ok 4\n"
    delete reg[a]
    $I0 = reg[a]
    if $I0 == 0 goto ok5
    print "not "
ok5:
    print "ok 5\n"
.end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "AddrRegistry 2" );
.sub main :main
    .local pmc a, b, reg, nil
    null nil
    reg = new 'AddrRegistry'
    a = new 'String'
    b = new 'String'
    $I0 = elements reg
    print $I0
    reg[a] = nil
    $I0 = elements reg
    print $I0
    reg[a] = nil
    $I0 = elements reg
    print $I0
    reg[b] = nil
    $I0 = elements reg
    print $I0
    print "\n"
.end
CODE
0112
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "AddrRegistry 2" );
.sub main :main
    .local pmc a, b, c, reg, nil, it
    null nil
    reg = new 'AddrRegistry'
    a = new 'String'
    a = "k1"
    b = new 'String'
    b = "k2"
    c = new 'String'
    c = "k3"
    reg[a] = nil
    reg[b] = nil
    reg[c] = nil

    it = iter reg
loop:
    unless it goto done
    $P0 = shift it
    print $P0
    goto loop
done:
    print "\n"
.end
# the current hash implementation returns entries in order
# for a few keys, and if there were no deletes
CODE
k1k2k3
OUTPUT

}

=head1 SEE ALSO

F<examples/benchmarks/primes.c>,
F<examples/benchmarks/primes.pasm>,
F<examples/benchmarks/primes.pl>,
F<examples/benchmarks/primes2_i.pasm>,
F<examples/benchmarks/primes2.c>,
F<examples/benchmarks/primes2.py>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
