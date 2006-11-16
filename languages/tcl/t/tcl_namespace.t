#!/usr/bin/perl

use strict;
use warnings;
use lib qw(tcl/lib ./lib ../lib ../../lib ../../../lib);

use Parrot::Test tests => 6;
use Test::More;
use vars qw($TODO);

language_output_is( "tcl", <<'TCL', <<'OUT', "command: global explicit" );
  ::puts ok
TCL
ok
OUT

language_output_is( "tcl", <<'TCL', <<'OUT', "command: global explicit (extra colons)" );
  :::::::puts ok
TCL
ok
OUT

language_output_is( "tcl", <<'TCL', <<'OUT', "command: all colons" );
  proc ::: {} {puts ok}
  {}
TCL
ok
OUT

language_output_is( "tcl", <<'TCL', <<'OUT', "command: global explicit (not enough colons)" );
  :puts ok
TCL
invalid command name ":puts"
OUT

language_output_is('tcl', <<'TCL', <<'OUT', 'command: invalid command in ns');
  foo::bar
TCL
invalid command name "foo::bar"
OUT

language_output_is('tcl', <<'TCL', <<'OUT', "command: relative namespace");
  proc test {} {puts ok1}
  namespace eval lib {test}
  proc ::lib::test {} {puts ok2}
  namespace eval lib {test}
TCL
ok1
ok2
OUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
