#!perl
# Copyright (C) 2006-2007, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 9;

=head1 NAME

t/pmc/codestring.t - test the CodeString class


=head1 SYNOPSIS

        % prove t/pmc/codestring.t

=head1 DESCRIPTION

Tests the CodeString class directly.

=cut

pir_output_is( <<'CODE', <<'OUTPUT', 'create a CodeString object' );
.sub main :main
    .local pmc code
    code = new 'CodeString'
    code = 'ok 1'
    say code
.end
CODE
ok 1
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'calls to unique' );
.sub main :main
    .local pmc code
    code = new 'CodeString'
    $P1 = code.'unique'('ok ')
    say $P1
    $P1 = code.'unique'()
    say $P1
    $P1 = code.'unique'('$P')
    say $P1
.end
CODE
ok 10
11
$P12
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'basic emit' );
.sub main :main
    .local pmc code
    code = new 'CodeString'
    code.emit('  label:')
    code.emit('    say "Hello, World"')
    code.emit('    $I0 = 1')
    code.emit('    $N0 = 0.1')
    print code
.end
CODE
  label:
    say "Hello, World"
    $I0 = 1
    $N0 = 0.1
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'emit with pos args' );
.sub main :main
    .local pmc code
    code = new 'CodeString'
    code.emit('  label_%0:', 1234)
    code.emit('    say "%0, %1"', 'Hello', 'World')
    code.emit('    %0 = %2', '$I0', 24, 48)
    print code
.end
CODE
  label_1234:
    say "Hello, World"
    $I0 = 48
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'emit with %, args' );
.sub main :main
    .local pmc code
    code = new 'CodeString'
    code.emit('  label_%0:', 1234)
    code.emit('    say "%,"', 'Hello')
    code.emit('    say "%,"', 'Hello', 'World', 'of', 'Parrot')
    print code
.end
CODE
  label_1234:
    say "Hello"
    say "Hello, World, of, Parrot"
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'emit with named args' );
.sub main :main
    .local pmc code
    code = new 'CodeString'
    code.emit('  label_%a:', 'a'=>1234)
    code.emit('    say "%b, %c"', 'b'=>'Hello', 'c'=>'World')
    code.emit('    say "%d"', 'b'=>'Hello', 'c'=>'World')
    print code
.end
CODE
  label_1234:
    say "Hello, World"
    say "%d"
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'emit with pos + named args' );
.sub main :main
    .local pmc code
    code = new 'CodeString'
    code.emit('  label_%a:', 'a'=>1234)
    code.emit('    %0 "%b, %c"', 'say', 'print', 'b'=>'H', 'c'=>'W')
    code.emit('    say "%,, %c"', 'alpha', 'beta', 'b'=>'H', 'c'=>'W')
    print code
.end
CODE
  label_1234:
    say "H, W"
    say "alpha, beta, W"
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'global unique #' );
.sub main :main
    .local pmc code1, code2
    code1 = new 'CodeString'
    code2 = new 'CodeString'
    .local string unique1, unique2
    unique1 = code1.'unique'()
    unique2 = code2.'unique'('$P')
    say unique1
    say unique2
.end
CODE
10
$P11
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'namespace keys' );
.sub main :main
    .local pmc code
    code = new 'CodeString'
    $S0 = code.'key'('abc')
    say $S0
    $S0 = code.'key'('abc', 'def')
    say $S0
    $P0 = split ' ', unicode:"abc def T\xe9st"
    $S0 = code.'key'($P0 :flat)
    say $S0
    $S0 = code.'key'($P0)
    say $S0
    $S0 = code.'key'('_perl6', $P0)
    say $S0
.end
CODE
["abc"]
["abc";"def"]
["abc";"def";unicode:"T\x{e9}st"]
["abc";"def";unicode:"T\x{e9}st"]
["_perl6";"abc";"def";unicode:"T\x{e9}st"]
OUTPUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
