#! perl
# Copyright (C) 2006-2007, The Perl Foundation.
# $Id$

=head1 NAME

t/userdata.t - Lua userdata & coercion

=head1 SYNOPSIS

    % perl -I../lib -Ilua/t lua/t/userdata.t

=head1 DESCRIPTION

=cut

use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin";

use Parrot::Test tests => 24;
use Test::More;

language_output_like( 'lua', <<'CODE', <<'OUT', '- u' );
local u = io.stdin
print(- u)
CODE
/^[^:]+: [^:]+:\d+: attempt to perform arithmetic on/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', '# u' );
local u = io.stdin
print(# u)
CODE
/^[^:]+: [^:]+:-?\d+: attempt to get length of/
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', 'not u' );
local u = io.stdin
print(not u)
CODE
false
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', 'u + 10' );
local u = io.stdin
print(u + 10)
CODE
/^[^:]+: [^:]+:\d+: attempt to perform arithmetic on/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', 'u - 2' );
local u = io.stdin
print(u - 2)
CODE
/^[^:]+: [^:]+:\d+: attempt to perform arithmetic on/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', 'u * 3.14' );
local u = io.stdin
print(u * 3.14)
CODE
/^[^:]+: [^:]+:\d+: attempt to perform arithmetic on/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', 'u / -7' );
local u = io.stdin
print(u / -7)
CODE
/^[^:]+: [^:]+:\d+: attempt to perform arithmetic on/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', 'u % 4' );
local u = io.stdin
print(u % 4)
CODE
/^[^:]+: [^:]+:\d+: attempt to perform arithmetic on/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', 'u ^ 3' );
local u = io.stdin
print(u ^ 3)
CODE
/^[^:]+: [^:]+:\d+: attempt to perform arithmetic on/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', 'u .. "end"' );
local u = io.stdin
print(u .. "end")
CODE
/^[^:]+: [^:]+:\d+: attempt to concatenate/
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', 'u == u' );
local u = io.stdin
print(u == u)
CODE
true
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', 'u ~= v' );
local u = io.stdin
local v = io.stdout
print(u ~= v)
CODE
true
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', 'u == 1' );
local u = io.stdin
print(u == 1)
CODE
false
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', 'u ~= 1' );
local u = io.stdin
print(u ~= 1)
CODE
true
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', 'u < v' );
local u = io.stdin
local v = io.stdout
print(u < v)
CODE
/^[^:]+: [^:]+:\d+: attempt to compare two userdata values\nstack traceback:\n/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', 'u <= v' );
local u = io.stdin
local v = io.stdout
print(u <= v)
CODE
/^[^:]+: [^:]+:\d+: attempt to compare two userdata values\nstack traceback:\n/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', 'u > v' );
local u = io.stdin
local v = io.stdout
print(u > v)
CODE
/^[^:]+: [^:]+:\d+: attempt to compare two userdata values\nstack traceback:\n/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', 'u >= v' );
local u = io.stdin
local v = io.stdout
print(u >= v)
CODE
/^[^:]+: [^:]+:\d+: attempt to compare two userdata values\nstack traceback:\n/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', 'u < 0' );
local u = io.stdin
print(u < 0)
CODE
/^[^:]+: [^:]+:\d+: attempt to compare \w+ with \w+\nstack traceback:\n/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', 'u <= 0' );
local u = io.stdin
print(u <= 0)
CODE
/^[^:]+: [^:]+:\d+: attempt to compare \w+ with \w+\nstack traceback:\n/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', 'u > 0' );
local u = io.stdin
print(u > 0)
CODE
/^[^:]+: [^:]+:\d+: attempt to compare \w+ with \w+\nstack traceback:\n/
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', 'u >= 0' );
local u = io.stdin
print(u >= 0)
CODE
/^[^:]+: [^:]+:\d+: attempt to compare \w+ with \w+\nstack traceback:\n/
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', 'get_pmc_keyed' );
local u = io.stdin
print(u[1])
CODE
nil
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', 'set_pmc_keyed' );
local u = io.stdin
u[1] = 1
CODE
/^[^:]+: [^:]+:\d+: attempt to index/
OUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:

