#! perl -w
# Copyright: 2005-2006 The Perl Foundation.  All Rights Reserved.
# $Id: strings.t 11446 2006-02-06 14:07:49Z fperrad $

=head1 NAME

t/strings.t - Lua string & coercion

=head1 SYNOPSIS

    % perl -I../lib -Ilua/t lua/t/strings.t

=head1 DESCRIPTION

=cut

use strict;
use FindBin;
use lib "$FindBin::Bin";

use Parrot::Test tests => 51;
use Test::More;

language_output_is( 'lua', <<'CODE', <<'OUT', '- "1"' );
print(- "1")
CODE
-1
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '- "text"' );
print(- "text")
CODE
/attempt to perform arithmetic on a string value/
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '# "text"' );
print(# "text")
CODE
4
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', 'not "text"' );
print(not "text")
CODE
false
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"10" + 2' );
print("10" + 2)
CODE
12
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"2" - 10' );
print("2" - 10)
CODE
-8
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"3.14" * 1' );
print("3.14" * 1)
CODE
3.14
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"-7" / 0.5' );
print("-7" / 0.5)
CODE
-14
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"-25" % 3' );
print("-25" % 3)
CODE
2
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"3" ^ 3' );
print("3" ^ 3)
CODE
27
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"10" + true' );
print("10" + true)
CODE
/attempt to perform arithmetic on a boolean value/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"2" - nil' );
print("2" - nil)
CODE
/attempt to perform arithmetic on a nil value/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"3.14" * false' );
print("3.14" * false)
CODE
/attempt to perform arithmetic on a boolean value/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"-7" / {}' );
print("-7" / {})
CODE
/attempt to perform arithmetic on a table value/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"-25" % false' );
print("-25" % false)
CODE
/attempt to perform arithmetic on a boolean value/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"3" ^ true' );
print("3" ^ true)
CODE
/attempt to perform arithmetic on a boolean value/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"10" + "text"' );
print("10" + "text")
CODE
/attempt to perform arithmetic on a string value/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"2" - "text"' );
print("2" - "text")
CODE
/attempt to perform arithmetic on a string value/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"3.14" * "text"' );
print("3.14" * "text")
CODE
/attempt to perform arithmetic on a string value/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"-7" / "text"' );
print("-7" / "text")
CODE
/attempt to perform arithmetic on a string value/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"-25" % "text"' );
print("-25" % "text")
CODE
/attempt to perform arithmetic on a string value/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"3" ^ "text"' );
print("3" ^ "text")
CODE
/attempt to perform arithmetic on a string value/
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"10" + "2"' );
print("10" + "2")
CODE
12
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"2" - "10"' );
print("2" - "10")
CODE
-8
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"3.14" * "1"' );
print("3.14" * "1")
CODE
3.14
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"-7" / "0.5"' );
print("-7" / "0.5")
CODE
-14
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"-25" % "3"' );
print("-25" % "3")
CODE
2
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"3" ^ "3"' );
print("3" ^ "3")
CODE
27
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"1" .. "end"' );
print("1" .. "end")
CODE
1end
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"1" .. 2' );
print("1" .. 2)
CODE
12
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"1" .. true' );
print("1" .. true)
CODE
/attempt to concatenate a boolean value/
OUT


language_output_is( 'lua', <<'CODE', <<'OUT', '"1.0" == "1"' );
print("1.0" == "1")
CODE
false
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"1" ~= "2"' );
print("1" ~= "2")
CODE
true
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"1" == true' );
print("1" == true)
CODE
false
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"1" ~= nil' );
print("1" ~= nil)
CODE
true
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"1" == 1' );
print("1" == 1)
CODE
false
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"1" ~= 1' );
print("1" ~= 1)
CODE
true
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"1" < "0"' );
print("1" < "0")                                       
CODE
false
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"1" <= "0"' );
print("1" <= "0")
CODE
false
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"1" > "0"' );
print("1" > "0")
CODE
true
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', '"1" >= "0"' );
print("1" >= "0")
CODE
true
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"1" < false' );
print("1" < false)
CODE
/attempt to compare \w+ with \w+/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"1" <= nil' );
print("1" <= nil)
CODE
/attempt to compare \w+ with \w+/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"1" > true' );
print("1" > true)
CODE
/attempt to compare \w+ with \w+/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"1" >= {}' );
print("1" >= {})
CODE
/attempt to compare \w+ with \w+/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"1" < 0' );
print("1" < 0)                                       
CODE
/attempt to compare \w+ with \w+/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"1" <= 0' );
print("1" <= 0)
CODE
/attempt to compare \w+ with \w+/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"1" > 0' );
print("1" > 0)
CODE
/attempt to compare \w+ with \w+/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '"1" >= 0' );
print("1" >= 0)
CODE
/attempt to compare \w+ with \w+/
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', 'get_pmc_keyed' );
a = "text"
print(a[1])
CODE
nil
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', 'set_pmc_keyed' );
a = "text"
a[1] = 1
CODE
/attempt to index/
OUT

