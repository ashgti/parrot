#!/usr/bin/perl

use strict;
use lib qw(tcl/t t . ../lib ../../lib ../../../lib);
use Parrot::Test tests => 42;
use vars qw($TODO);

language_output_is("tcl",<<TCL,<<OUT,"mul");
 puts [expr 2 * 3]
TCL
6
OUT

language_output_is("tcl",<<TCL,<<OUT,"div");
 puts [expr 6 / 2]
TCL
3
OUT

language_output_is("tcl",<<TCL,<<OUT,"remainder");
 puts [expr 3 % 2]
TCL
1
OUT

language_output_is("tcl",<<TCL,<<OUT,"plus");
 puts [expr 2 + 3]
TCL
5
OUT

language_output_is("tcl",<<TCL,<<OUT,"minus");
 puts [expr 2 - 3]
TCL
-1
OUT


language_output_is("tcl",<<TCL,<<OUT,"left shift");
 puts [expr 16 << 2]
TCL
64
OUT

language_output_is("tcl",<<TCL,<<OUT,"right shift");
 puts [expr 16 >> 2]
TCL
4
OUT

language_output_is("tcl",<<TCL,<<OUT,"lt, true");
 puts [expr 2 < 3]
TCL
1
OUT

language_output_is("tcl",<<TCL,<<OUT,"lt, false");
 puts [expr 3 < 2]
TCL
0
OUT

language_output_is("tcl",<<TCL,<<OUT,"gt, true");
 puts [expr 3 > 2]
TCL
1
OUT

language_output_is("tcl",<<TCL,<<OUT,"gt, false");
 puts [expr 2 > 3]
TCL
0
OUT

language_output_is("tcl",<<TCL,<<OUT,"lte, lt");
 puts [expr 2 <= 3]
TCL
1
OUT

language_output_is("tcl",<<TCL,<<OUT,"lte, gt");
 puts [expr 3 <= 2]
TCL
0
OUT

language_output_is("tcl",<<TCL,<<OUT,"lte, eq");
 puts [expr 3 <= 3]
TCL
1
OUT

language_output_is("tcl",<<TCL,<<OUT,"gte, gt");
 puts [expr 3 >= 2]
TCL
1
OUT

language_output_is("tcl",<<TCL,<<OUT,"gte, lt");
 puts [expr 2 >= 3]
TCL
0
OUT

language_output_is("tcl",<<TCL,<<OUT,"gte, eq");
 puts [expr 3 >= 3]
TCL
1
OUT

language_output_is("tcl",<<TCL,<<OUT,"==, eq");
 puts [expr 1 == 1]
TCL
1
OUT

language_output_is("tcl",<<TCL,<<OUT,"==, ne");
 puts [expr 2 == 1]
TCL
0
OUT

language_output_is("tcl",<<TCL,<<OUT,"==, ne");
 puts [expr 1 != 1]
TCL
0
OUT

language_output_is("tcl",<<TCL,<<OUT,"==, eq");
 puts [expr 2 != 1]
TCL
1
OUT

language_output_is("tcl",<<TCL,<<OUT,"&");
 puts [expr 5 & 6 ]
TCL
4
OUT

language_output_is("tcl",<<TCL,<<OUT,"|");
 puts [expr 5 | 6 ]
TCL
7
OUT

language_output_is("tcl",<<TCL,<<OUT,"^");
 puts [expr 5 ^ 6 ]
TCL
3
OUT

#
# now, functions - the accuracy and int vs. float nature here is
# still an issue - we're testing to make sure that the functions
# exist, basically. better tests will need to be written (or the tcl
# test suite used.) (XXX)
#

language_output_is("tcl",<<TCL,<<OUT,"abs");
 puts [expr abs(1-2)]
TCL
1
OUT

language_output_is("tcl",<<TCL,<<OUT,"acos");
 puts [expr acos(0)]
TCL
1.570796
OUT

language_output_is("tcl",<<TCL,<<OUT,"asin");
 puts [expr asin(1)]
TCL
1.570796
OUT

language_output_is("tcl",<<TCL,<<OUT,"atan");
 puts [expr atan(1)]
TCL
0.785398
OUT

language_output_is("tcl",<<TCL,<<OUT,"cos");
 puts [expr cos(1)]
TCL
0.540302
OUT

language_output_is("tcl",<<TCL,<<OUT,"cosh");
 puts [expr cosh(1)]
TCL
1.543081
OUT

language_output_is("tcl",<<TCL,<<OUT,"exp");
 puts [expr exp(1)]
TCL
2.718282
OUT

language_output_is("tcl",<<TCL,<<OUT,"log");
 puts [expr log(32)]
TCL
3.465736
OUT

language_output_is("tcl",<<TCL,<<OUT,"log10");
 puts [expr log10(32)]
TCL
1.50515
OUT

language_output_is("tcl",<<TCL,<<OUT,"sin");
 puts [expr sin(1)]
TCL
0.841471
OUT

language_output_is("tcl",<<TCL,<<OUT,"sinh");
 puts [expr sinh(1)]
TCL
1.175201
OUT

language_output_is("tcl",<<TCL,<<OUT,"sqrt");
 puts [expr sqrt(64)]
TCL
8.0
OUT

language_output_is("tcl",<<TCL,<<OUT,"tan");
 puts [expr tan(1)]
TCL
1.557408
OUT

language_output_is("tcl",<<TCL,<<OUT,"tanh");
 puts [expr tanh(1)]
TCL
0.761594
OUT

# misc.

language_output_is("tcl",<<TCL,<<OUT,"simple precedence");
 puts [expr 2*3+4*2]
TCL
14
OUT

language_output_is("tcl",<<TCL,<<OUT,"parens");
 puts [expr 2*(3+4)*2]
TCL
28
OUT

TODO: {
local $TODO = "bugs";

language_output_is("tcl",<<'TCL',<<'OUT',"int vs. float");
 set n 1
 puts [expr {$n * 1}]
TCL
1
OUT

language_output_is("tcl",<<'TCL',<<'OUT',"nested expr");
 puts [expr {2 * [expr {2 - 1}]}];
TCL
2
OUT
}
