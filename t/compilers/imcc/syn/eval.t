#!perl
# Copyright (C) 2001-2005, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Config;
use Parrot::Test tests => 7;

SKIP: {
    skip( "changed eval semantics - see t/pmc/eval.t", 7 );

##############################
    pir_output_is( <<'CODE', <<'OUT', "eval pasm" );
.sub test :main
        $S0 = 'set S1, "in eval\n"'
        concat $S0, "\n"
        concat $S0, "print S1\nend\n"
        compreg $P0, "PASM"
        compile P0, $P0, $S0
        invoke
        print "back\n"
        end
.end
CODE
in eval
back
OUT

    pir_output_is( <<'CODE', <<'OUT', "eval pir" );
.sub test :main
        $S1 = ".sub _foo\n"
        concat $S1, '$S1 = "42\n"'
        concat $S1, "\nprint $S1\nend\n"
        concat $S1, "\n.end\n"
        compreg $P0, "PIR"
        compile P0, $P0, $S1
        invoke
        print "back\n"
        end
.end
CODE
42
back
OUT

    pir_output_is( <<'CODE', <<'OUT', "intersegment branch" );
# #! perl -w
# my $i= 5;
# LAB:
#    $i++;
#    eval("goto LAB if ($i==6)");
#    print "$i\n";
#
# 7
#####

.sub test :main
    I1 = 5
    $S0 = ".sub _e\nif I1 == 6 goto LAB\nend\n.end\n"
    compreg P2, "PIR"
    compile P0, P2, $S0
LAB:
    inc I1
    invoke
    print I1
    print "\n"
    end
.end
CODE
7
OUT

    pir_output_is( <<'CODE', <<'OUT', "intersegment branch 2" );
.sub test :main
    I1 = 4
    $S0 = ".sub _e\nif I1 <= 6 goto LAB\nend\n.end\n"
    compreg P2, "PIR"
    compile P0, P2, $S0
LAB:
    inc I1
    invoke
    print I1
    print "\n"
    end
.end
CODE
7
OUT

    pir_output_is( <<'CODE', <<'OUT', "intersegment branch 3" );
.sub test :main
    I1 = 4
    compreg P2, "PIR"
    $S0 = ".sub _e\nif I1 <= 5 goto LAB\nend\n.end\n"
    compile P0, P2, $S0
    $S0 = ".sub _e\nif I1 <= 6 goto LAB\nend\n.end\n"
    compile P1, P2, $S0
LAB:
    inc I1
    invoke
    set P0, P1
    invoke
    print I1
    print "\n"
    end
.end
CODE
7
OUT

    pir_output_is( <<'CODE', <<'OUT', "intersegment branch 4" );
.sub test :main
    I1 = 4
    compreg P2, "PIR"
    $S0 = ".sub _e\nif I1 <= 5 goto LAB\nend\n.end\n"
    compile P0, P2, $S0
    $S0 = ".sub _e\nif I1 <= 6 goto LAB\nend\n.end\n"
    compile P1, P2, $S0
LAB:
    inc I1
    invoke
    set P0, P1
    invoke
    if I1 <= 7 goto LAB
    print I1
    print "\n"
    end
.end
CODE
8
OUT

    pir_output_is( <<'CODE', <<'OUT', "eval - same constants" );
.sub test :main
        print "hello"
        print "\n"
        $S0 = 'print "hello"'
        concat $S0, "\n"
        concat $S0, 'print "\n"'
        concat $S0, "\nend\n"
        compreg $P0, "PASM"
        compile P0, $P0, $S0
        invoke
        print "back\n"
        end
.end
CODE
hello
hello
back
OUT
}

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
