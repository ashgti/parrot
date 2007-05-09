#! perl
# Copyright (C) 2005-2007, The Perl Foundation.
# $Id$

=head1 NAME

t/examples.t - some Lua code examples

=head1 SYNOPSIS

    % perl -I../lib -Ilua/t lua/t/examples.t

=head1 DESCRIPTION

First tests in order to check infrastructure.

=cut

use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin";

use Parrot::Test tests => 7;
use Test::More;

language_output_is( 'lua', <<'CODE', <<'OUT', 'hello world' );
print("Hello World")
CODE
Hello World
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', 'hello world' );
print "Hello World"
CODE
Hello World
OUT

language_output_like( 'lua', <<'CODE', <<'OUT', 'version' );
print(_VERSION)
CODE
/^Lua 5\.1/
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', 'factorial (recursive)' );
function factorial (n)
    if n == 0 then
        return 1
    else
        return n * factorial(n-1)
    end
end

print(factorial(7))
CODE
5040
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', 'factorial (recursive)' );
local function factorial (n)
    if n == 0 then
        return 1
    else
        return n * factorial(n-1)
    end
end

print(factorial(7))
CODE
5040
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', 'factorial (loop)' );
function factorial (n)
    local a = 1
    for i = 1,n,1 do
        a = a*i
    end
    return a
end

print(factorial(7))
CODE
5040
OUT

language_output_is( 'lua', <<'CODE', <<'OUT', 'with args', params => "abc def"  );
print(#arg)
print(...)
CODE
2
abc	def
OUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:

