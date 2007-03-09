#! perl
# Copyright (C) 2007, The Perl Foundation.
# $Id$

=head1 NAME

t/package.t - Lua Package Library

=head1 SYNOPSIS

    % perl -I../lib -Ilua/t lua/t/package.t

=head1 DESCRIPTION

Tests Lua Package Library
(implemented in F<languages/lua/lib/luapackage.pir>).

See "Lua 5.1 Reference Manual", section 5.3 "Modules",
L<http://www.lua.org/manual/5.1/manual.html#5.3>.

=cut

use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin";

use Parrot::Test tests => 15;
use Test::More;

language_output_is( 'lua', << 'CODE', << 'OUTPUT', 'function module' );
print(type(mod))
local _G = _G
module("mod")
_G.print(_G.type(_G.mod))
_G.assert(_G.mod == _G.package.loaded.mod)
CODE
nil
table
OUTPUT

language_output_is( 'lua', << 'CODE', << 'OUTPUT', 'function require' );
local m = require "io"
m.write("hello world\n")
CODE
hello world
OUTPUT

unlink('../complex.lua') if ( -f '../complex.lua' );
my $X;
open $X, '>', '../complex.lua';
print {$X} << 'CODE';

complex = {}

function complex.new (r, i) return {r=r, i=i} end

--defines a constant 'i'
complex.i = complex.new(0, 1)

function complex.add (c1, c2)
    return complex.new(c1.r + c2.r, c1.i + c2.i)
end

function complex.sub (c1, c2)
    return complex.new(c1.r - c2.r, c1.i - c2.i)
end

function complex.mul (c1, c2)
    return complex.new(c1.r*c2.r - c1.i*c2.i,
                       c1.r*c2.i + c1.i*c2.r)
end

local function inv (c)
    local n = c.r^2 + c.i^2
    return complex.new(c.r/n, -c.i/n)
end

function complex.div (c1, c2)
    return complex.mul(c1, inv(c2))
end

return complex
CODE
close $X;

language_output_is( 'lua', << 'CODE', << 'OUTPUT', 'function require' );
m = require "complex"
assert(m == complex)
print(complex.i.r, complex.i.i)
CODE
0	1
OUTPUT

language_output_like( 'lua', << 'CODE', << 'OUTPUT', 'function require (no module)' );
require "no_module"
CODE
/module 'no_module' not found:\n/
OUTPUT

unlink('../foo.lua') if ( -f '../foo.lua' );
open $X, '>', '../foo.lua';
print {$X} '?syntax error?';
close $X;

language_output_like( 'lua', << 'CODE', << 'OUTPUT', 'function require (syntax error)');
require "foo"
CODE
/error loading module 'foo' from file '.*foo.lua':\n/
OUTPUT

language_output_is( 'lua', << 'CODE', << 'OUTPUT', 'function require & package.preload' );
foo = {}
foo.bar = 1234
function foo_loader ()
    return foo
end
package.preload.foo = foo_loader

m = require 'foo'
assert(m == foo)
print(m.bar)
CODE
1234
OUTPUT

TODO:
{
    local $TODO = 'need setfenv';

unlink('../complex.lua') if ( -f '../complex.lua' );
open $X, '>', '../complex.lua';
print {$X} << 'CODE';
module(...)

function new (r, i) return {r=r, i=i} end

--defines a constant 'i'
i = new(0, 1)

function add (c1, c2)
    return new(c1.r + c2.r, c1.i + c2.i)
end

function sub (c1, c2)
    return new(c1.r - c2.r, c1.i - c2.i)
end

function mul (c1, c2)
    return new(c1.r*c2.r - c1.i*c2.i,
               c1.r*c2.i + c1.i*c2.r)
end

local function inv (c)
    local n = c.r^2 + c.i^2
    return new(c.r/n, -c.i/n)
end

function div (c1, c2)
    return mul(c1, inv(c2))
end
CODE
close $X;

language_output_is( 'lua', << 'CODE', << 'OUTPUT', 'function require & module' );
require "complex"
print(complex.i.r, complex.i.i)
CODE
0	1
OUTPUT
}

SKIP:
{
skip('only with Parrot', 1) if (exists $ENV{PARROT_LUA_TEST_PROG});

unlink('../mod_foo.pbc') if ( -f '../mod_foo.pbc' );
unlink('../mod_foo.pir') if ( -f '../mod_foo.pir' );
open $X, '>', '../mod_foo.pir';
print {$X} <<'PIR';
.HLL 'Lua', 'lua_group'

.sub '__onload' :anon :load
#    print "__onload mod_foo\n"
    .const .Sub entry = 'luaopen_mod_foo'
    set_root_global 'luaopen_mod_foo', entry
.end

.sub 'luaopen_mod_foo'
#    print "luaopen_mod_foo\n"
    .local pmc _lua__GLOBAL
    _lua__GLOBAL = global '_G'
    new $P1, .LuaString
    .local pmc _mod_foo
    new _mod_foo, .LuaTable
    set $P1, 'mod_foo'
    _lua__GLOBAL[$P1] = _mod_foo
    .const .Sub _mod_foo_bar = '_mod_foo_bar'
    set $P1, 'bar'
    _mod_foo[$P1] = _mod_foo_bar
    .return (_mod_foo)
.end

.sub '_mod_foo_bar' :anon
    .param pmc extra :slurpy
    new $P0, .LuaNumber
    set $P0, 3.14
    .return ($P0)
.end
PIR
close $X;

language_output_is( 'lua', << 'CODE', << 'OUTPUT', 'function require (PIR)' );
assert(nil == mod_foo)
m = require "mod_foo"
assert(m == mod_foo)
assert(m == package.loaded.mod_foo)
print(m.bar())
CODE
3.14
OUTPUT
}

language_output_is( 'lua', << 'CODE', << 'OUTPUT', 'table package.loaded' );
t = {}
for k in pairs(package.loaded) do
    table.insert(t, k)
end
table.sort(t)
for k, v in ipairs(t) do
    print(v)
end
CODE
_G
coroutine
debug
io
math
os
package
string
table
OUTPUT

language_output_is( 'lua', << 'CODE', << 'OUTPUT', 'table package.path' );
print(type(package.path))
CODE
string
OUTPUT

SKIP:
{
skip('only with Parrot', 3) if (exists $ENV{PARROT_LUA_TEST_PROG});

delete $ENV{LUA_PBCPATH};
language_output_is( 'lua', << 'CODE', << 'OUTPUT', 'table package.pbcpath' );
print(type(package.pbcpath))
print(package.pbcpath)
CODE
string
./?.pbc;./?.pir;languages/lua/lib/?.pbc
OUTPUT

$ENV{LUA_PBCPATH} = "?.pbc";
language_output_is( 'lua', << 'CODE', << 'OUTPUT', 'table package.pbcpath' );
print(package.pbcpath)
CODE
?.pbc
OUTPUT

$ENV{LUA_PBCPATH} = ";;languages/lua/?.pbc";
language_output_is( 'lua', << 'CODE', << 'OUTPUT', 'table package.pbcpath' );
print(package.pbcpath)
CODE
;./?.pbc;./?.pir;languages/lua/lib/?.pbc;languages/lua/?.pbc
OUTPUT

delete $ENV{LUA_PBCPATH};
}

language_output_is( 'lua', << 'CODE', << 'OUTPUT', 'table package.preload' );
print(type(package.preload))
print(# package.preload)
CODE
table
0
OUTPUT

language_output_is( 'lua', << 'CODE', << 'OUTPUT', 'function package.seeall' );
m = {}
package.seeall(m)
m.print("hello")
CODE
hello
OUTPUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:

