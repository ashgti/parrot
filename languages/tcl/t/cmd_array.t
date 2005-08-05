#!/usr/bin/perl

use strict;
use lib qw(tcl/t t . ../lib ../../lib ../../../lib);
use Parrot::Test tests => 18;
use Test::More;
use vars qw($TODO);

language_output_is("tcl",<<'TCL',<<OUT,"array, no args");
 array
TCL
wrong # args: should be "array option arrayName ?arg ...?"
OUT

language_output_is("tcl",<<'TCL',<<OUT,"array, good subcommand, no array");
 array exists
TCL
wrong # args: should be "array option arrayName ?arg ...?"
OUT

language_output_is("tcl",<<'TCL',<<OUT,"array, bad subcommand, bad arary");
 array bork foo
TCL
bad option "bork": must be anymore, donesearch, exists, get, names, nextelement, set, size, startsearch, statistics, or unset
OUT

language_output_is("tcl",<<'TCL',<<OUT,"array exists yes");
 set b(c) 2
 puts [array exists b]
TCL
1
OUT

language_output_is("tcl",<<'TCL',<<OUT,"array exists no");
 set a 2
 puts [array exists a]
TCL
0
OUT

language_output_is("tcl",<<'TCL',<<OUT,"array exists missing");
 puts [array exists a]
TCL
0
OUT

language_output_is("tcl",<<'TCL',<<OUT,"array exists too many args");
 puts [array exists a b]
TCL
wrong # args: should be "array exists arrayName"
OUT

language_output_is("tcl",<<'TCL',<<OUT,"array size too many args");
 array size a b
TCL
wrong # args: should be "array size arrayName"
OUT

language_output_is("tcl",<<'TCL',<<OUT,"array size 1");
 set a(1) 1
 puts [array size a]
TCL
1
OUT

language_output_is("tcl",<<'TCL',<<OUT,"array size 2");
 set a(1) 1; set a(2) 2
 puts [array size a]
TCL
2
OUT

language_output_is("tcl",<<'TCL',<<OUT,"array size not array");
 set a 1
 puts [array size a]
TCL
0
OUT

language_output_is("tcl",<<'TCL',<<OUT,"array set list");
 array set a [list a b]
 puts $a(a)
TCL
b
OUT

language_output_is("tcl",<<'TCL',<<OUT,"array set multi list");
 array set a [list a b c d e f]
 puts $a(a)
 puts $a(c)
 puts $a(e)
TCL
b
d
f
OUT

language_output_is("tcl",<<'TCL',<<OUT,"array set preserve old values");
 set a(a) b
 array set a [list c d e f]
 puts $a(a)
 puts $a(c)
 puts $a(e)
TCL
b
d
f
OUT

language_output_is("tcl",<<'TCL',<<OUT,"array set");
 array set a {a b}
 puts $a(a)
TCL
b
OUT

language_output_is("tcl",<<'TCL',<<OUT,"array set multi");
 array set a {a b c d e f}
 puts $a(a)
 puts $a(c)
 puts $a(e)
TCL
b
d
f
OUT

language_output_is("tcl",<<'TCL',<<OUT,"array set uneven");
 array set a a
TCL
list must have an even number of elements
OUT

language_output_is("tcl",<<'TCL',<<'OUT',"array set return value");
  puts [array set a [list a b]]
TCL

OUT
