#!/usr/bin/perl

use strict;
use lib qw(tcl/lib ./lib ../lib ../../lib ../../../lib);
use Parrot::Test tests => 7;
use Test::More;

language_output_is("tcl",<<'TCL',<<OUT,"bad args, 0");
  lassign
TCL
wrong # args: should be "lassign list varName ?varName ...?"
OUT

language_output_is("tcl",<<'TCL',<<OUT,"bad args, 1");
  lassign {a b}
TCL
wrong # args: should be "lassign list varName ?varName ...?"
OUT

language_output_is("tcl",<<'TCL',<<OUT,"assign list of two to one var");
  puts [lassign {x y} a]
  puts $a
TCL
y
x
OUT

language_output_is("tcl",<<'TCL',<<OUT,"assign list of three to one var");
  puts [lassign {x y z} a]
  puts $a
TCL
y z
x
OUT

language_output_is("tcl",<<'TCL',<<OUT,"assign list of three to two vars");
  puts [lassign {x y z} a b]
  puts $a
  puts $b
TCL
z
x
y
OUT

language_output_is("tcl",<<'TCL',<<OUT,"assign list of three to three vars");
  puts [lassign {x y z} a b c]
  puts $a
  puts $b
  puts $c
TCL

x
y
z
OUT

language_output_is("tcl",<<'TCL',<<OUT,"assign list of three to four vars");
  puts [lassign {x y z} a b c d]
  puts $a
  puts $b
  puts $c
  puts $d
TCL

x
y
z

OUT


