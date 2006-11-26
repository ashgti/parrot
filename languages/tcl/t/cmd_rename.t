#!perl

use strict;
use warnings;
use lib qw(tcl/lib ./lib ../lib ../../lib ../../../lib);

use Parrot::Test tests => 8;
use Test::More;

language_output_is( "tcl", <<'TCL', <<OUT, "rename" );
 set a 2
 rename puts fnord
 fnord $a
TCL
2
OUT

language_output_is( "tcl", <<'TCL', <<OUT, "remove" );
 rename puts ""
 puts "Whee"
TCL
invalid command name "puts"
OUT

language_output_is( "tcl", <<'TCL', <<'OUT', "rename non-existant command" );
 rename foo blah
TCL
can't rename "foo": command doesn't exist
OUT

language_output_is( "tcl", <<'TCL', <<'OUT', "delete non-existant command" );
 rename foo ""
TCL
can't delete "foo": command doesn't exist
OUT

language_output_is( 'tcl', <<'TCL', <<'OUT', 'new command already exists' );
  rename if incr
TCL
can't rename to "incr": command already exists
OUT

language_output_is( "tcl",
    <<'TCL', <<'OUT', "test fallback to interpreted versions of normally inlined commands." );
 set a 1
 incr a
 rename if {}
 incr a
 puts $a
TCL
3
OUT

language_output_is( "tcl", <<'TCL', <<'OUT', "delete inlined sub" );
 set a 1
 incr a
 puts $a
 rename incr {}
 incr a
TCL
2
invalid command name "incr"
OUT

language_output_is( "tcl", <<'TCL', <<'OUT', "rename inlined sub" );
 set a 1
 rename incr foo
 foo a
 puts $a
TCL
2
OUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
