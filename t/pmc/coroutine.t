#! perl -w
# Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

t/pmc/coroutines.t - Coroutines

=head1 SYNOPSIS

	% perl t/pmc/coroutines.t

=head1 DECSRIPTION

Tests the C<Coroutine> PMC.

=cut

use Parrot::Test tests => 4;
use Test::More;

output_is(<<'CODE', <<'OUTPUT', "Coroutines");
    new P0, .Coroutine
    set_addr P0, co2
co1:
    set I1, 4
    print "start 1\n"
co1_loop:
    invoke
    print "back  1\n"
    dec I1
    ne I1, 0, co1_loop
    print "done\n"
    end

co2:
    print "start 2\n"
    new P5, .Coroutine
    set_addr P5, co3
    set P6, P0
co2_loop:
    invoke
    print "back  2\n"
    set P0, P5
    invoke
    print "back  2b\n"
    set P0, P6
    branch co2_loop

co3:
    print "start 3\n"
    new P7, .Coroutine
    set_addr P7, co2_loop
    set P8, P0
co3_loop:
    invoke
    print "back  3\n"
    set P0, P7
    invoke
    print "back  3b\n"
    set P0, P8
    branch co3_loop

CODE
start 1
start 2
back  1
back  2
start 3
back  2b
back  1
back  2
back  3
back  3b
back  2b
back  1
back  2
back  3
back  2
back  2b
back  1
done
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "Coroutines and lexicals");
    new_pad 0
    new_pad 1

    new P20, .PerlInt
    set P20, 20
    new P21, .PerlInt
    set P21, 21

    store_lex -1, "a", P20
    store_lex -2, "b", P21

    new P1, .Coroutine
    set_addr P1, co1

    new P2, .Coroutine
    set_addr P2, co2

    set P0, P1
    invoke

    find_lex P10, "b"
    print P10
    print "\n"

    find_lex P10, "a"
    print P10
    print "\n"

    invoke # reenter co1

    find_lex P10, "b"
    print P10
    print "\n"

    find_lex P10, "a"
    print P10
    print "\n"

    invoke # reenter co1

    print "done\n"
    end

co1:
    new_pad 2

    new P22, .PerlInt
    set P22, 22

    store_lex -1, "b", P22  # hides
    store_lex "a", P22      # replaces

    # invoke c02
    set P0, P2
    invoke

    # return
    set P0, P1
    invoke

    find_lex P10, "b"
    print P10
    print "\n"

    find_lex P10, "a"
    print P10
    print "\n"

    # return again
    set P0, P1
    invoke

    find_lex P10, "b"
    print P10
    print "\n"

    find_lex P10, "a"
    print P10
    print "\n"

    # return again
    set P0, P1
    invoke

co2:
    new_pad 1

    # return
    invoke

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

output_is(<<'CODE', <<'OUTPUT', "Coroutines and registers");
    new P1, .Coroutine
    set_addr P1, co1

    new P2, .Coroutine
    set_addr P2, co2

    set P0, P1

    set S0, "main registers"

    saveall
    invoke # call co1
    restoreall
    saveall
    invoke # reenter co1
    restoreall
    saveall
    invoke # reenter co1
    restoreall

    print S0
    print "\ndone\n"
    end

co1:
    set S0, "co1 registers"

    # invoke c02
    save S0
    set P0, P2
    invoke
    restore S0

    print S0
    print "\n"

    # return
    save S0
    set P0, P1
    invoke
    restore S0

    print S0
    print "\n"

    # invoke c02
    save S0
    set P0, P2
    invoke
    restore S0

    print S0
    print "\n"

    # return again
    save S0
    set P0, P1
    invoke
    restore S0

    print S0
    print "\n"

    # invoke c02
    save S0
    set P0, P2
    invoke
    restore S0

    print S0
    print "\n"

    # last return
    set P0, P1
    invoke

co2:
    set S0, "co2 registers"
    save S0
    invoke # return
    restore S0

    print S0
    print "\n"

    save S0
    invoke # return
    restore S0

    print S0
    print "\n"

    invoke # return

CODE
co1 registers
co1 registers
co2 registers
co1 registers
co1 registers
co2 registers
co1 registers
main registers
done
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "Coroutines - M. Wallace yield example");
##PIR##
.sub __main__
    .local Coroutine itr
    .local object return
    .local object counter
    newsub itr, .Coroutine, _iterator

    .local object zero
    zero = new PerlInt
    zero = 0

    newsub return, .Continuation, return_here
    loop:
        .pcc_begin non_prototyped
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

.pcc_sub _iterator prototyped
    .local object x
    x = new PerlInt
    x = 0
    iloop:
        .pcc_begin_yield
        .return x
        .pcc_end_yield
        x = x + 1
    if x <= 10 goto iloop
    invoke P1
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

1;
