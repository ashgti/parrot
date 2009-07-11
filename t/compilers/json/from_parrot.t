#!perl
# Copyright (C) 2001-2006, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( t . lib ../lib ../../lib );

use Test::More;
use Parrot::Test tests => 18;

=head1 NAME

t/compilers/json/from_parrot.t - test parrot to JSON conversion.

=head1 SYNOPSIS

    % prove t/compilers/json/from_parrot.t

=head1 DESCRIPTION

Tests JSON->Parrot conversions.

=cut

# no. 1
pir_output_is( <<'CODE', <<'OUT', 'Create JSON of an empty string' );

.sub test :main
    .local string s
    s = ''

    load_bytecode 'JSON.pbc'
    $S0 = _json( s, 0 )
    say $S0
.end
CODE
""
OUT

# no. 2
pir_output_is( <<'CODE', <<'OUT', 'Create JSON of a non-empty string' );

.sub test :main
    .local string s
    s = 'abcdeABCDE01234$%^&*'

    load_bytecode 'JSON.pbc'
    $S0 = _json( s, 0 )
    say $S0
.end
CODE
"abcdeABCDE01234$%^&*"
OUT

# no. 3
pir_output_is( <<'CODE', <<'OUT', 'Create JSON of a string with simple escapes' );

.sub test :main
    .local string s
    s = "abcde\\ABCDE\"01234\n$%^&*"
    # XXX more escapes need to be tested; see http://www.json.org/
    load_bytecode 'JSON.pbc'
    $S0 = _json( s, 0 )
    say $S0
.end
CODE
"abcde\\ABCDE\"01234\n$%^&*"
OUT

# no. 4
pir_output_is( <<'CODE', <<'OUT', 'Create JSON of some integers' );

.sub test :main
    .local int i
    i = 0
    load_bytecode 'JSON.pbc'
    $S0 = _json( i, 0 )
    say $S0
    i = 35
    $S0 = _json( i, 0 )
    say $S0
    i = -42
    $S0 = _json( i, 0 )
    say $S0
    i = 2147483647
    $S0 = _json( i, 0 )
    say $S0
    i = -2147483648
    $S0 = _json( i, 0 )
    say $S0
.end
CODE
0
35
-42
2147483647
-2147483648
OUT

# no. 5
pir_output_is( <<'CODE', <<'OUT', 'Create JSON of some numbers' );

.sub test :main
    .local num n
    n = 0.0
    load_bytecode 'JSON.pbc'
    $S0 = _json( n )
    say $S0
    n = 2.50
    $S0 = _json( n )
    say $S0
    n = -42.0
    $S0 = _json( n )
    say $S0
    n = 4.5e1
    $S0 = _json( n )
    say $S0
.end
CODE
0
2.5
-42
45
OUT

# no. 6
pir_output_is( <<'CODE', <<'OUT', 'Create JSON of various scalars with pretty option' );

.sub test :main
    .local string s
    s = "abcde\\ABCDE\"01234\n$%^&*"
    load_bytecode 'JSON.pbc'
    $S0 = _json( s, 1 )
    print $S0

    .local int i
    i = -42
    $S0 = _json( i, 1 )
    print $S0

    .local num n
    n = 2.50
    $S0 = _json( n, 1 )
    print $S0
.end
CODE
"abcde\\ABCDE\"01234\n$%^&*"
-42
2.5
OUT

# no. 7
pir_output_is( <<'CODE', <<'OUT', 'Create JSON of an array' );

.sub test :main
    .local pmc array

    new array, 'Array'
    array = 10
    array[0] = 0
    array[1] = 1
    array[2] = 2
    array[3] = 3
    array[4] = 4
    array[5] = 5
    array[6] = 6
    array[7] = 7
    array[8] = 8
    array[9] = 9

    load_bytecode 'JSON.pbc'
    $S0 = _json( array, 0 )
    say $S0
.end
CODE
[0,1,2,3,4,5,6,7,8,9]
OUT

# no. 8
pir_output_is( <<'CODE', <<'OUT', 'Create pretty JSON of an array' );

.sub test :main
    .local pmc array

    new array, 'Array'
    array = 10
    array[0] = 0
    array[1] = 1
    array[2] = 2
    array[3] = 3
    array[4] = 4
    array[5] = 5
    array[6] = 6
    array[7] = 7
    array[8] = 8
    array[9] = 9

    load_bytecode 'JSON.pbc'
    $S0 = _json( array, 1 )
    print $S0
.end
CODE
[
  0,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9
]
OUT

# no. 9
pir_output_is( <<'CODE', <<'OUT', 'Create JSON of array, keep element ordering' );

.sub test :main
    .local pmc array

    new array, 'Array'
    array = 6
    array[0] = 35
    array[1] = 1
    array[2] = -5
    array[3] = 0
    array[4] = -2147483648
    array[5] = 2147483647

    load_bytecode 'JSON.pbc'
    $S0 = _json( array, 1 )
    print $S0
.end
CODE
[
  35,
  1,
  -5,
  0,
  -2147483648,
  2147483647
]
OUT

# no. 10
pir_output_is( <<'CODE', <<'OUT', 'Create JSON of a mixed array' );

.sub test :main
    .local pmc array

    new array, 'Array'
    array = 6
    array[0] = 0
    array[1] = 1500e-2
    array[2] = "JSON"
    array[3] = -7
    array[4] = "json"
    array[5] = 0.0

    load_bytecode 'JSON.pbc'
    $S0 = _json( array, 1 )
    print $S0
.end
CODE
[
  0,
  15,
  "JSON",
  -7,
  "json",
  0
]
OUT

# no. 11
pir_output_is( <<'CODE', <<'OUT', 'Create JSON of hash' );

.sub test :main
    .local pmc hash

    new hash, 'Hash'
    hash["alpha"] = 29
    hash["beta"] = "B"
    hash["gamma"] = 3.1
    hash["delta"] = "DELTA"

    load_bytecode 'JSON.pbc'
    $S0 = _json( hash, 1 )
    print $S0
.end
CODE
{
  "alpha" : 29,
  "beta" : "B",
  "delta" : "DELTA",
  "gamma" : 3.1
}
OUT

# no. 12
pir_output_is( <<'CODE', <<'OUT', 'Create non-pretty JSON of hash' );

.sub test :main
    .local pmc hash

    new hash, 'Hash'
    hash["alpha"] = 29
    hash["beta"] = "B"
    hash["gamma"] = 3.1
    hash["delta"] = "DELTA"

    load_bytecode 'JSON.pbc'
    $S0 = _json( hash, 0 )
    say $S0
.end
CODE
{"alpha":29,"beta":"B","delta":"DELTA","gamma":3.1}
OUT

# no. 13
pir_output_is(
    <<'CODE', <<'OUT', 'Create JSON of nested structure including ResizablePMCArray and empties' );

.sub test :main
    .local pmc street1, street2, city1, city2, country, world

    street1 = new 'Hash'
    street1["Perl"] = "Highway"
    street1["Python"] = "Grove"
    street1["Ruby"] = "Lane"

    street2 = new 'Hash'  # empty

    city1 = new 'ResizablePMCArray'
    push city1, street1
    push city1, street2

    city2 = new 'Array'  # empty

    country = new 'Array'
    country = 2
    country[0] = city1
    country[1] = city2

    world = new 'Hash'
    world["population"] = 1234567890
    world["some_country"] = country

    load_bytecode 'JSON.pbc'
    $S0 = _json( world, 1 )
    print $S0
.end
CODE
{
  "population" : 1234567890,
  "some_country" : [
    [
      {
        "Perl" : "Highway",
        "Python" : "Grove",
        "Ruby" : "Lane"
      },
      {
      }
    ],
    [
    ]
  ]
}
OUT

# no. 14
pir_output_is( <<'CODE', <<'OUT', 'Create non-pretty JSON of nested structure' );

.sub test :main
    .local pmc street1, street2, city1, city2, country, world

    street1 = new 'Hash'
    street1["Perl"] = "Highway"
    street1["Python"] = "Grove"
    street1["Ruby"] = "Lane"

    street2 = new 'Hash'  # empty

    city1 = new 'ResizablePMCArray'
    push city1, street1
    push city1, street2

    city2 = new 'Array'  # empty

    country = new 'Array'
    country = 2
    country[0] = city1
    country[1] = city2

    world = new 'Hash'
    world["population"] = 1234567890
    world["some_country"] = country

    load_bytecode 'JSON.pbc'
    $S0 = _json( world, 0 )
    say $S0
.end
CODE
{"population":1234567890,"some_country":[[{"Perl":"Highway","Python":"Grove","Ruby":"Lane"},{}],[]]}
OUT

# no. 15
pir_output_is( <<'CODE', <<'OUT', 'Create JSON of String PMCs' );

.sub test :main
    .local pmc s

    s = new 'String'
    s = ''
    load_bytecode 'JSON.pbc'
    $S0 = _json( s, 0 )
    say $S0
    $S0 = _json( s, 1 )
    print $S0

    s = new 'String'
    s = "12345\"67890"
    $S0 = _json( s, 0 )
    say $S0
    $S0 = _json( s, 1 )
    print $S0
.end
CODE
""
""
"12345\"67890"
"12345\"67890"
OUT

# no. 16
pir_output_is( <<'CODE', <<'OUT', 'Create JSON of Integer PMCs' );

.sub test :main
    .local pmc i

    i = new 'Integer'
    i = 0
    load_bytecode 'JSON.pbc'
    $S0 = _json( i, 0 )
    say $S0
    $S0 = _json( i, 1 )
    print $S0

    i = new 'Integer'
    i = -42
    $S0 = _json( i, 0 )
    say $S0
    $S0 = _json( i, 1 )
    print $S0
.end
CODE
0
0
-42
-42
OUT

# no. 17
pir_output_is( <<'CODE', <<'OUT', 'Create JSON of Boolean PMCs' );

.sub test :main
    .local pmc b

    b = new 'Boolean'
    b = 0
    load_bytecode 'JSON.pbc'
    $S0 = _json( b, 0 )
    say $S0
    $S0 = _json( b, 1 )
    print $S0

    b = new 'Boolean'
    b = 1
    $S0 = _json( b, 0 )
    say $S0
    $S0 = _json( b, 1 )
    print $S0
.end
CODE
false
false
true
true
OUT

# no. 18
pir_output_is( <<'CODE', <<'OUT', 'Create JSON of null and .Undef' );

.sub test :main
    .local pmc n
    null n

    load_bytecode 'JSON.pbc'
    $S0 = _json( n, 0 )
    say $S0
    $S0 = _json( n, 1 )
    print $S0

    n = new 'Undef'
    $S0 = _json( n, 0 )
    say $S0
    $S0 = _json( n, 1 )
    print $S0
.end
CODE
null
null
null
null
OUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
