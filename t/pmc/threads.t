#! perl -w
# Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

t/pmc/threads.t - Threads

=head1 SYNOPSIS

	% perl -Ilib t/pmc/threads.t

=head1 DESCRIPTION

Tests running threads. All tests skipped unless running on known-good
platform.

=cut

use Parrot::Test;
use Test::More;

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
/;

if ($^O eq "cygwin" ) {
    my @uname = split / /, qx'uname -v';
    
    if ($uname[0] eq "2004-09-04" ) {
	plan skip_all => "This cygwin version is known to fail the thread tests";
	exit;
    }
}
if ($platforms{$^O}) {
   plan tests => 11;
}
else {
   plan skip_all => "No threading yet or test not enabled for '$^O'";
}


output_is(<<'CODE', <<'OUTPUT', "interp identity");
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

output_is(<<'CODE', <<'OUTPUT', "thread type 1");
    set I5, 10
    #
    # set regs P2 = thread-interp, P5 = sub
    find_global P5, "_foo"
    new P2, .ParrotThread
    find_method P0, P2, "thread1"
    set I3, 1
    invoke	# start the thread

    sleep 1
    print "main "
    print I5
    print "\n"
    # get tid of thread
    set I5, P2
    # wait for it
    find_method P0, P2, "join"
    invoke
    end

.pcc_sub _foo:
    # check if vars are fresh
    inc I5
    print "thread "
    print I5
    print "\n"
    set I3, 0   # no retval
    invoke P1	# ret and be done with thread

# output from threads could be reversed
CODE
thread 1
main 10
OUTPUT


output_is(<<'CODE', <<'OUTPUT', "thread type 2");
    set I5, 10
    set S5, " interp\n"
    new P6, .PerlString
    set P6, "from "

    #
    # set regs P2 = thread-interp, P5 = sub
    print "ok 1\n"
    find_global P5, "_foo"
    print "ok 2\n"
    new P2, .ParrotThread
    set I3, 2   # 2 args
    find_method P0, P2, "thread2"
    invoke	# start the thread

    sleep 1	# now the thread should run
    print P6
    print I5
    print S5
    # get tid of thread
    set I5, P2
    # wait for it
    find_method P0, P2, "join"
    invoke
    end

.pcc_sub _foo:
    # check if vars a really separate
    inc I5
    set S5, " thread\n"
    set P6, "hello from "
    print P6
    print I5
    print S5
    typeof S0, P2
    print S0
    print " tid "
    set I0, P2
    print I0
    print "\n"
    typeof S0, P0
    print S0
    print "\n"
    set I3, 0
    invoke P1	# ret and be done with thread

CODE
ok 1
ok 2
hello from 1 thread
ParrotThread tid 1
Sub
from 10 interp
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "thread - kill");
    bounds 1	# assert slow core -S and -g are fine too
    find_global P5, "_foo"
    new P2, .ParrotThread
    set I10, P2
    print "start "
    print I10
    print "\n"
    find_method P0, P2, "thread3"
    set I3, 1
    invoke	# start the thread
    sleep 1

    set I5, I10
    getinterp P2
    find_method P0, P2, "kill"
    invoke

    print "done\n"
    end

.pcc_sub _foo:
    print "in thread\n"
    # run a endless loop
lp:
    noop
    branch lp
    print "never\n"
    invoke P1
CODE
start 1
in thread
done
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "join, get retval");
##PIR##
.sub _main
    .const int MAX = 1000
    .sym pmc kid
    .sym pmc Adder
    Adder = global "_add"
    kid = new ParrotThread
    .sym pmc from
    from = new PerlInt
    from = 0
    .sym pmc to
    to = new PerlInt
    to = MAX
    .sym pmc Thread_new
    find_method Thread_new, kid, "thread3"
    .pcc_begin prototyped
    .arg Adder
    .arg from
    .arg to
    .invocant kid
    .nci_call Thread_new
    .pcc_end

    .sym int tid
    tid = kid
    .sym pmc Thread_join
    find_method Thread_join, kid, "join"
    .pcc_begin prototyped
    .arg tid
    .nci_call Thread_join
    .sym pmc result
    .result result
    .pcc_end
    print result
    print "\n"
    # sum = n * (n + 1)/2
    .sym pmc Mul
    Mul = new PerlInt
    assign Mul, to
    inc Mul
    Mul = to * Mul
    Mul = Mul / 2
    print Mul
    print "\n"
    end
.end

.sub _add prototyped
   .param pmc sub
   .param pmc from
   .param pmc to
   .sym   pmc sum
   sum = new PerlInt
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

output_like(<<'CODE', <<'OUTPUT', "detach");
    find_global P5, "_foo"
    new P2, .ParrotThread
    find_method P0, P2, "thread3"
    new P6, .TQueue	# need a flag that thread is done
    set I3, 2
    invoke	# start the thread

    set I5, P2
    getinterp P2
    find_method P0, P2, "detach"
    invoke
wait:
    defined I0, P6
    unless I0, wait
    print "done\n"
    end

.pcc_sub _foo:
    print "thread\n"
    sleep 0.1
    new P2, .PerlInt
    push P6, P2		# push item on queue
    invoke P1
CODE
/(done\nthread\n)|(thread\ndone\n)/
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "share a PMC");
    find_global P5, "_foo"
    new P2, .ParrotThread
    find_method P0, P2, "thread3"
    new P20, .PerlInt
    new P6, .SharedRef, P20
    set P6, 20
    set I3, 2   # P5..P6
    invoke	# start the thread
    # now sleep a bit, so that the thread runs
    sleep 0.1

    set I5, P2
    getinterp P2
    find_method P0, P2, "join"
    invoke
    print "done\n"
    print P6
    print "\n"
    end

.pcc_sub _foo:
    print "thread\n"
    print P6
    print "\n"
    inc P6
    invoke P1
CODE
thread
20
done
21
OUTPUT


output_is(<<'CODE', <<'OUT', "multi-threaded");
    new P10, .TQueue
    new P6, .PerlInt
    set P6, 1
    push P10, P6
    new P6, .PerlInt
    set P6, 2
    push P10, P6
    new P6, .PerlInt
    set P6, 3
    push P10, P6
    set P6, P10

    new P2, .ParrotThread
    find_method P0, P2, "thread3"
    find_global P5, "_foo"
    set I3, 2
    invoke	# start the thread
    set I5, P2
    getinterp P2
    find_method P0, P2, "join"
    invoke	# join the thread
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
    invoke P1
CODE
3
1
2
3
done thread
done main
OUT

output_is(<<'CODE', <<'OUT', "multi-threaded strings via SharedRef");
    new P10, .TQueue
    new P7, .PerlString
    set P7, "ok 1\n"
    new P8, .SharedRef, P7
    push P10, P8
    new P7, .PerlString
    new P8, .SharedRef, P7
    set P7, "ok 2\n"
    push P10, P8
    new P7, .PerlString
    set P7, "ok 3\n"
    new P8, .SharedRef, P7
    push P10, P8
    set P6, P10

    new P2, .ParrotThread
    find_method P0, P2, "thread3"
    find_global P5, "_foo"
    set I3, 2
    invoke	# start the thread
    set I5, P2
    getinterp P2
    find_method P0, P2, "join"
    invoke	# join the thread
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
    invoke P1
CODE
3
ok 1
ok 2
ok 3
done thread
done main
OUT

SKIP: {
skip("no shared PerlStrings yet", 2);
output_is(<<'CODE', <<'OUT', "thread safe queue strings 1");
    new P10, .TQueue
    print "ok 1\n"
    set I0, P10
    print I0
    print "\n"
    new P7, .PerlString
    set P7, "ok 2\n"
    push P10, P7
    new P7, .PerlString
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

output_is(<<'CODE', <<'OUT', "multi-threaded strings");
    new P10, .TQueue
    new P7, .PerlString
    set P7, "ok 1\n"
    push P10, P7
    new P7, .PerlString
    set P7, "ok 2\n"
    push P10, P7
    new P7, .PerlString
    set P7, "ok 3\n"
    push P10, P7
    set P6, P10

    find_global P5, "_foo"
    new P2, .ParrotThread
    find_method P0, P2, "thread3"
    invoke	# start the thread
    set I5, P2
    getinterp P2
    find_method P0, P2, "join"
    invoke	# join the thread
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
    invoke P1
CODE
3
ok 1
ok 2
ok 3
done thread
done main
OUT
}
