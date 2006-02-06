#! perl -w
# Copyright: 2006 The Perl Foundation.  All Rights Reserved.
# $Id: closure.t  $

=head1 NAME

t/closure.t - Lua closures 

=head1 SYNOPSIS

    % perl -I../lib -Ilua/t lua/t/closure.t

=head1 DESCRIPTION

See "Lua 5.0 Reference Manual", section 2.6 "Visibility Rules".

See "Programming in Lua", section 6.1 "Closures". 

=cut

use strict;
use FindBin;
use lib "$FindBin::Bin";

use Parrot::Test tests => 3;
use Test::More;

language_output_is( 'lua', <<'CODE', <<'OUT', 'inc' );
local counter = 0

function inc (x)
    counter = counter + x
    return counter
end

print(inc(1))
print(inc(2))
CODE
1
3
OUT


language_output_is( 'lua', <<'CODE', <<'OUT', 'newCounter' );
function newCounter ()
    local i = 0
    return function ()  -- anonymous function
               i = i + 1
               return i
           end
end

c1 = newCounter()
print(c1())
print(c1())

c2 = newCounter()
print(c2())
print(c1())
print(c2())
CODE
1
2
1
3
2
OUT

TODO: {
local $TODO = "each execution of a local statement defines new local variables";

language_output_is( 'lua', <<'CODE', <<'OUT', '' );
a = {}
local x = 20
for i=1,10 do
    local y = 0
    a[i] = function () y=y+1; return x+y end
end

print(a[1]())
print(a[1]())
print(a[2]())

--[[
The loop creates ten closures (that is, ten instances of the anonymous
function). Each of these closures uses a different y variable, while all
of them share the same x.
]]
CODE
21
22
21
OUT
}
