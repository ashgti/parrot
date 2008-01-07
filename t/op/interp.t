#!perl
# Copyright (C) 2001-2006, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );

use Test::More;
use Parrot::Test tests => 9;

=head1 NAME

t/op/interp.t - Running the Interpreter

=head1 SYNOPSIS

    % prove t/op/interp.t

=head1 DESCRIPTION

Tests the old and new styles of running the Parrot interpreter and the
C<interpinfo> opcode.

=cut

SKIP: {
    skip( "we really shouldn't run just a label - use a sub", 1 );

    pasm_output_is( <<'CODE', <<'OUTPUT', "runinterp - new style" );
    new P0, 'ParrotInterpreter'
    print "calling\n"
    # set_addr/invoke ?
    runinterp P0, foo
    print "ending\n"
    end
    print "bad things!\n"
  foo:
    print "In 2\n"
    end
CODE
calling
In 2
ending
OUTPUT
}

pir_output_is( <<'CODE', <<'OUTPUT', 'runinterp - works without printing' );
.sub 'test' :main
    .local string actual
    .local pmc test_interp
               test_interp = new 'ParrotInterpreter'
    .local pmc stdout
               stdout = getstdout
    push stdout, 'string'

    print "uno\n"
    runinterp test_interp, pasm
    print "dos\n"

  get_stdout:
    $S0 = readline stdout
    actual .= $S0
    if $S0 goto get_stdout
    $S0 = pop stdout

    print actual
    goto pasm_end

  pasm:
    noop
    end
  pasm_end:
.end
CODE
uno
dos
OUTPUT

pir_output_is(
    <<'CODE', <<'OUTPUT', 'runinterp - works with printing', todo => 'something funky here' );
.sub 'test' :main
    .local string actual
    .local pmc test_interp
               test_interp = new 'ParrotInterpreter'
    .local pmc stdout
               stdout = getstdout
    push stdout, 'string'

    print "uno\n"
    runinterp test_interp, pasm
    print "dos\n"

  get_stdout:
    $S0 = readline stdout
    actual .= $S0
    if $S0 goto get_stdout
    $S0 = pop stdout

    print actual
    goto pasm_end

  pasm:
    print "y uno es igual a\n"
    end
  pasm_end:
.end
CODE
uno
y uno es igual a
dos
OUTPUT

# Need to disable DOD while trace is on, as there's a non-zero chance that a
# DOD sweep would occur, causing a bonus "DOD" line in the output, which makes
# the test fail.
pasm_output_like(
    <<'CODE', <<'OUTPUT', "restart trace", todo => "lines are out of order due to buffering" );
    printerr "ok 1\n"
    sweepoff
    set I0, 1
    trace I0
    printerr "ok 2\n"
    dec I0
    trace I0
    sweepon
    printerr "ok 3\n"
    end
CODE
/^ok\s1\n
(?:\s+8.*)?\n
ok\s2\n
(?:\s+10.*)?\n
(?:\s+12.*)?\n
ok\s3\n$/x
OUTPUT

# This is the behavior as of Parrot 0.4.3
# RT#46819 Should there be a warning?
pasm_output_is( <<'CODE', 'nada:', 'interp - warnings' );
    new P0, 'Undef'
    set I0, P0
    printerr "nada:"
    warningson 1
    new P1, 'Undef'
    set I0, P1
    end
CODE

pasm_output_is( <<'CODE', <<'OUTPUT', "getinterp" );
    .include "interpinfo.pasm"
    getinterp P0
    print "ok 1\n"
    set I0, P0[.INTERPINFO_ACTIVE_PMCS]
    interpinfo I1, .INTERPINFO_ACTIVE_PMCS
    eq I0, I1, ok2
    print "not "
  ok2:
    print "ok 2\n"
    end
CODE
ok 1
ok 2
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "access argv" );
    get_params "0", P5
    .include "iglobals.pasm"
    getinterp P1
    set P2, P1[.IGLOBALS_ARGV_LIST]
    set I0, P5
    set I1, P2
    eq I0, I1, ok1
    print "not "
  ok1:
    print "ok 1\n"
    set S0, P5[0]
    set S1, P2[0]
    eq S0, S1, ok2
    print "not "
  ok2:
    print "ok 2\n"
    end
CODE
ok 1
ok 2
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "check_events" );
    print "before\n"
    check_events
    print "after\n"
    end
CODE
before
after
OUTPUT


pir_output_is( <<'CODE', <<'OUTPUT', "interpinfo & getinterp: current runcore" );
.include 'interpinfo.pasm'
.include 'interpcores.pasm'

.sub 'test' :main
    $I0 = interpinfo .INTERPINFO_CURRENT_RUNCORE
    if $I0 == .PARROT_FUNCTION_CORE   goto ok1
    if $I0 == .PARROT_FAST_CORE       goto ok1
    if $I0 == .PARROT_SWITCH_CORE     goto ok1
    if $I0 == .PARROT_CGOTO_CORE      goto ok1
    if $I0 == .PARROT_CGP_CORE        goto ok1
    if $I0 == .PARROT_JIT_CORE        goto ok1
    if $I0 == .PARROT_SWITCH_JIT_CORE goto ok1
    if $I0 == .PARROT_CGP_JIT_CORE    goto ok1
    if $I0 == .PARROT_EXEC_CORE       goto ok1
    if $I0 == .PARROT_GC_DEBUG_CORE   goto ok1
    print 'not '
  ok1:
    say 'ok 1'

    $P0 = getinterp
    $I1 = $P0[.INTERPINFO_CURRENT_RUNCORE]
    if $I0 == $I1 goto ok2
    print 'not '
  ok2:
    say 'ok 2'
.end
CODE
ok 1
ok 2
OUTPUT


# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
