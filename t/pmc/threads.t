#! perl
# Copyright: 2001-2005 The Perl Foundation.  All Rights Reserved.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test;

=head1 NAME

t/pmc/threads.t - Threads

=head1 SYNOPSIS

	% prove t/pmc/threads.t

=head1 DESCRIPTION

Tests running threads. All tests skipped unless running on known-good
platform.

=cut

my %platforms = map {$_=>1} qw/
    aix
    cygwin
    darwin
    dec_osf
    freebsd
    hpux
    irix
    linux
    openbsd
    MSWin32
/;

if ($^O eq "cygwin" ) {
    my @uname = split / /, qx'uname -v';

    if ($uname[0] eq "2004-09-04" ) {
	plan skip_all => "This cygwin version is known to fail the thread tests";
	exit;
    }
}
if (0 && $platforms{$^O}) {
   plan tests => 11;
}
else {
   # plan skip_all => "No threading yet or test not enabled for '$^O'";
   plan skip_all => "Needs COPY for argument passing";
}


pasm_output_is(<<'CODE', <<'OUTPUT', "interp identity");
    getinterp P2
    clone P3, P2
    eq P3, P2, ok1
    print "not"
ok1:
    print "ok 1\n"
    new P4, .ParrotThread
    ne P4, P2, ok2
    print "not"
ok2:
    print "ok 2\n"
    end
CODE
ok 1
ok 2
OUTPUT

pasm_output_is(<<'CODE', <<'OUTPUT', "thread type 1");
    set I5, 10
    #
    # set regs P2 = thread-interp, P5 = sub
    find_global P5, "_foo"
    new P2, .ParrotThread
    set I3, 1
    callmethod "thread1"

    sleep 1
    print "main "
    print I5
    print "\n"
    # get tid of thread
    set I5, P2
    # wait for it
    callmethod "join"
    end

.pcc_sub _foo:
    # check if vars are fresh
    inc I5
    print "thread "
    print I5
    print "\n"
    set I3, 0   # no retval
    returncc	# ret and be done with thread

# output from threads could be reversed
CODE
thread 1
main 10
OUTPUT


pasm_output_is(<<'CODE', <<'OUTPUT', "thread type 2");
    set I5, 10
    set S5, " interp\n"
    new P6, .String
    set P6, "from "

    #
    # set regs P2 = thread-interp, P5 = sub
    print "ok 1\n"
    find_global P5, "_foo"
    print "ok 2\n"
    new P2, .ParrotThread
    set I3, 2   # 2 args
    callmethod "thread2"

    sleep 1	# now the thread should run
    print P6
    print I5
    print S5
    # get tid of thread
    set I5, P2
    # wait for it
    callmethod "join"
    end

.pcc_sub _foo:
    # check if vars a really separate
    inc I5
    set S5, " thread\n"
    set P6, "hello from "
    print P6
    print I5
    print S5
    getinterp P2
    typeof S0, P2
    print S0
    print " tid "
    set I0, P2
    print I0
    print "\n"
    set I3, 0
    returncc	# ret and be done with thread

CODE
ok 1
ok 2
hello from 1 thread
ParrotThread tid 1
from 10 interp
OUTPUT

pasm_output_is(<<'CODE', <<'OUTPUT', "thread - kill");
    bounds 1	# assert slow core -S and -g are fine too
    find_global P5, "_foo"
    new P2, .ParrotThread
    set I10, P2
    print "start "
    print I10
    print "\n"
    set I3, 1
    callmethod "thread3"
    sleep 1

    set I5, I10
    getinterp P2
    callmethod "kill"

    print "done\n"
    end

.pcc_sub _foo:
    print "in thread\n"
    # run a endless loop
lp:
    noop
    branch lp
    print "never\n"
    returncc
CODE
start 1
in thread
done
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "join, get retval");

.sub _main
    .const int MAX = 1000
    .sym pmc kid
    .sym pmc Adder
    Adder = global "_add"
    kid = new ParrotThread
    .sym pmc from
    from = new Integer
    from = 0
    .sym pmc to
    to = new Integer
    to = MAX
    kid."thread3"(Adder, from, to)

    .local int tid
    tid = kid
    .local pmc me, result
    me = getinterp
    result = me."join"(tid)
    print result
    print "\n"
    # sum = n * (n + 1)/2
    .sym pmc Mul
    Mul = new Integer
    assign Mul, to
    inc Mul
    Mul = to * Mul
    Mul = Mul / 2
    print Mul
    print "\n"
    end
.end

.sub _add
   .param pmc sub
   .param pmc from
   .param pmc to
   .sym   pmc sum
   sum = new Integer
loop:
    add sum, from
    inc from
    le from, to, loop

    .pcc_begin_return
    .return sum
    .pcc_end_return
.end
CODE
500500
500500
OUTPUT

SKIP: {
	skip("detatch broken on $^O", 1) if ($^O =~ /MSWin32/);
pasm_output_like(<<'CODE', <<'OUTPUT', "detach");
    find_global P5, "_foo"
    new P2, .ParrotThread
    new P6, .TQueue	# need a flag that thread is done
    set I3, 2
    callmethod "thread3"

    set I5, P2
    getinterp P2
    callmethod "detach"
wait:
    defined I0, P6
    unless I0, wait
    print "done\n"
    end

.pcc_sub _foo:
    print "thread\n"
    sleep 0.1
    new P2, .Integer
    push P6, P2		# push item on queue
    returncc
CODE
/(done\nthread\n)|(thread\ndone\n)/
OUTPUT
}

pasm_output_is(<<'CODE', <<'OUTPUT', "share a PMC");
    find_global P5, "_foo"
    new P2, .ParrotThread
    new P20, .Integer
    new P6, .SharedRef, P20
    set P6, 20
    set I3, 2   # P5..P6
    callmethod "thread3"
    # now sleep a bit, so that the thread runs
    sleep 0.1

    set I5, P2
    getinterp P2
    callmethod "join"
    print "done\n"
    print P6
    print "\n"
    end

.pcc_sub _foo:
    print "thread\n"
    print P6
    print "\n"
    inc P6
    returncc
CODE
thread
20
done
21
OUTPUT


pasm_output_is(<<'CODE', <<'OUT', "multi-threaded");
    new P10, .TQueue
    new P6, .Integer
    set P6, 1
    push P10, P6
    new P6, .Integer
    set P6, 2
    push P10, P6
    new P6, .Integer
    set P6, 3
    push P10, P6
    set P6, P10

    new P2, .ParrotThread
    find_global P5, "_foo"
    set I3, 2
    callmethod "thread3"
    set I5, P2
    getinterp P2
    callmethod "join"
    print "done main\n"
    end

.pcc_sub _foo:
    set I0, P6
    print I0
    print "\n"
loop:
    set I0, P6
    unless I0, ex
    shift P8, P6
    print P8
    print "\n"
    branch loop
ex:
    print "done thread\n"
    returncc
CODE
3
1
2
3
done thread
done main
OUT

pasm_output_is(<<'CODE', <<'OUT', "multi-threaded strings via SharedRef");
    new P10, .TQueue
    new P7, .String
    set P7, "ok 1\n"
    new P8, .SharedRef, P7
    push P10, P8
    new P7, .String
    new P8, .SharedRef, P7
    set P7, "ok 2\n"
    push P10, P8
    new P7, .String
    set P7, "ok 3\n"
    new P8, .SharedRef, P7
    push P10, P8
    set P6, P10

    new P2, .ParrotThread
    find_global P5, "_foo"
    set I3, 2
    callmethod "thread3"
    set I5, P2
    getinterp P2
    callmethod "join"
    print "done main\n"
    end

.pcc_sub _foo:
    set I0, P6
    print I0
    print "\n"
loop:
    set I0, P6
    unless I0, ex
    shift P8, P6
    print P8
    branch loop
ex:
    print "done thread\n"
    returncc
CODE
3
ok 1
ok 2
ok 3
done thread
done main
OUT

SKIP: {
skip("no shared Strings yet", 2);
pasm_output_is(<<'CODE', <<'OUT', "thread safe queue strings 1");
    new P10, .TQueue
    print "ok 1\n"
    set I0, P10
    print I0
    print "\n"
    new P7, .String
    set P7, "ok 2\n"
    push P10, P7
    new P7, .String
    set P7, "ok 3\n"
    push P10, P7
    set I0, P10
    print I0
    print "\n"

    shift P8, P10
    print P8
    shift P8, P10
    print P8
    end
CODE
ok 1
0
2
ok 2
ok 3
OUT

pasm_output_is(<<'CODE', <<'OUT', "multi-threaded strings");
    new P10, .TQueue
    new P7, .String
    set P7, "ok 1\n"
    push P10, P7
    new P7, .String
    set P7, "ok 2\n"
    push P10, P7
    new P7, .String
    set P7, "ok 3\n"
    push P10, P7
    set P6, P10

    find_global P5, "_foo"
    new P2, .ParrotThread
    callmethod "thread3"
    set I5, P2
    getinterp P2
    callmethod "join"
    print "done main\n"
    end

.pcc_sub _foo:
    set I0, P6
    print I0
    print "\n"
loop:
    set I0, P6
    unless I0, ex
    shift P8, P6
    print P8
    branch loop
ex:
    print "done thread\n"
    returncc
CODE
3
ok 1
ok 2
ok 3
done thread
done main
OUT
}
