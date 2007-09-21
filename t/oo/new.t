#!perl
# Copyright (C) 2007, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 14;

=head1 NAME

t/oo/new.t - Test OO instantiation

=head1 SYNOPSIS

    % prove t/oo/names.t

=head1 DESCRIPTION

Tests OO features related to instantiating new objects.

=cut

pir_output_is( <<'CODE', <<'OUT', 'instantiate from class object' );
.sub main :main
    $P1 = newpdd15class "Foo"
    $S1 = typeof $P1
    say $S1

    $I3 = isa $P1, "Class"
    print $I3
    print "\n"

    $P2 = new $P1

    $S1 = typeof $P2
    say $S1

    $I3 = isa $P2, "Foo"
    print $I3
    print "\n"

    $I3 = isa $P2, "Object"
    print $I3
    print "\n"
.end
CODE
Class
1
parrot;Foo
1
1
OUT

pir_output_is( <<'CODE', <<'OUT', 'manually create anonymous class object' );
.sub main :main
    $P1 = new "Class"
    $S1 = typeof $P1
    say $S1

    $I3 = isa $P1, "Class"
    print $I3
    print "\n"

    $P2 = new $P1

    $S1 = typeof $P2
    print "'"
    print $S1
    print "'\n"

    $I3 = isa $P2, "Foo"
    print $I3
    print "\n"

    $I3 = isa $P2, "Object"
    print $I3
    print "\n"
.end
CODE
Class
1
''
0
1
OUT

pir_output_is( <<'CODE', <<'OUT', 'manually create named class object' );
.sub main :main
    $P1 = new "Class"
    $P1.name("Foo")
    $S1 = typeof $P1
    say $S1

    $I3 = isa $P1, "Class"
    print $I3
    print "\n"

    $P2 = new $P1

    $S1 = typeof $P2
    say $S1

    $I3 = isa $P2, "Foo"
    print $I3
    print "\n"

    $I3 = isa $P2, "Object"
    print $I3
    print "\n"
.end
CODE
Class
1
parrot;Foo
1
1
OUT

pir_output_is( <<'CODE', <<'OUT', 'instantiate from class object method' );
.sub main :main
    $P1 = newpdd15class "Foo"
    $P2 = $P1.'new'()

    $S1 = typeof $P2
    say $S1

    $I3 = isa $P2, "Foo"
    print $I3
    print "\n"

    $I3 = isa $P2, "Object"
    print $I3
    print "\n"
.end
CODE
parrot;Foo
1
1
OUT

pir_output_is( <<'CODE', <<'OUT', 'instantiate from string name' );
.sub main :main
    $P1 = newpdd15class "Foo"
    $P2 = new 'Foo'

    $S1 = typeof $P2
    say $S1

    $I3 = isa $P2, "Foo"
    print $I3
    print "\n"

    $I3 = isa $P2, "Object"
    print $I3
    print "\n"
.end
CODE
parrot;Foo
1
1
OUT

pir_output_is( <<'CODE', <<'OUT', 'instantiate from string register name' );
.sub main :main
    $P1 = newpdd15class "Foo"
    $S1 = 'Foo'
    $P2 = new $S1

    $S1 = typeof $P2
    say $S1

    $I3 = isa $P2, "Foo"
    print $I3
    print "\n"

    $I3 = isa $P2, "Object"
    print $I3
    print "\n"
.end
CODE
parrot;Foo
1
1
OUT

pir_output_is( <<'CODE', <<'OUT', 'instantiate from string PMC name' );
.sub main :main
    $P1 = newpdd15class "Foo"
    $P3 = new 'String'
    $P3 = 'Foo'
    $P2 = new $P3

    $S1 = typeof $P2
    say $S1

    $I3 = isa $P2, "Foo"
    print $I3
    print "\n"

    $I3 = isa $P2, "Object"
    print $I3
    print "\n"
.end
CODE
parrot;Foo
1
1
OUT

pir_output_is( <<'CODE', <<'OUT', 'instantiate from key name' );
.sub main :main
    $P1 = newpdd15class ['Foo';'Bar']
    $S1 = typeof $P1
    say $S1

    $I3 = isa $P1, "Class"
    print $I3
    print "\n"

    $P2 = new ['Foo';'Bar']

    $S1 = typeof $P2
    say $S1

    $I3 = isa $P2, 'Bar'
    print $I3
    print "\n"

    $I3 = isa $P2, "Object"
    print $I3
    print "\n"
.end
CODE
Class
1
parrot;Foo;Bar
1
1
OUT

pir_output_is(
    <<'CODE', <<'OUT', 'instantiate from key PMC name', todo => 'create non-constant key' );
.sub main :main
    $P1 = newpdd15class ['Foo';'Bar']
    $S1 = typeof $P1
    say $S1

    $I3 = isa $P1, "Class"
    print $I3
    print "\n"

    # How do you set the value of a non-constant key PMC?
    $P3 = new 'Key'

    $P2 = new $P3

    $S1 = typeof $P2
    say $S1

    $I3 = isa $P2, 'Bar'
    print $I3
    print "\n"

    $I3 = isa $P2, "Object"
    print $I3
    print "\n"
.end
CODE
Class
1
parrot;Foo;Bar
1
1
OUT

pir_output_is( <<'CODE', <<'OUT', 'instantiate from class object with init' );
.sub main :main
    $P1 = newpdd15class "Foo"
    addattribute $P1, 'data'
    $P3 = new 'Hash'
    $P4 = new 'String'
    $P4 = "data for Foo\n"
    $P3['data'] = $P4

    $P2 = new $P1, $P3

    $S1 = typeof $P2
    say $S1

    $I3 = isa $P2, "Foo"
    print $I3
    print "\n"

    $I3 = isa $P2, "Object"
    print $I3
    print "\n"

    $P5 = getattribute $P2, 'data'
    print $P5
.end
CODE
parrot;Foo
1
1
data for Foo
OUT

pir_output_is( <<'CODE', <<'OUT', 'instantiate from string name with init' );
.sub main :main
    $P1 = newpdd15class "Foo"
    addattribute $P1, 'data'
    $P3 = new 'Hash'
    $P4 = new 'String'
    $P4 = "data for Foo\n"
    $P3['data'] = $P4

    $P2 = new 'Foo', $P3

    $S1 = typeof $P2
    say $S1

    $I3 = isa $P2, "Foo"
    print $I3
    print "\n"

    $I3 = isa $P2, "Object"
    print $I3
    print "\n"

    $P5 = getattribute $P2, 'data'
    print $P5
.end
CODE
parrot;Foo
1
1
data for Foo
OUT

pir_output_is( <<'CODE', <<'OUT', 'instantiate from string register name with init' );
.sub main :main
    $P1 = newpdd15class "Foo"
    addattribute $P1, 'data'
    $P3 = new 'Hash'
    $P4 = new 'String'
    $P4 = "data for Foo\n"
    $P3['data'] = $P4

    $S1 = 'Foo'
    $P2 = new $S1, $P3

    $S1 = typeof $P2
    say $S1

    $I3 = isa $P2, "Foo"
    print $I3
    print "\n"

    $I3 = isa $P2, "Object"
    print $I3
    print "\n"

    $P5 = getattribute $P2, 'data'
    print $P5
.end
CODE
parrot;Foo
1
1
data for Foo
OUT

pir_output_is( <<'CODE', <<'OUT', 'instantiate from string PMC name with init' );
.sub main :main
    $P1 = newpdd15class "Foo"
    addattribute $P1, 'data'
    $P3 = new 'Hash'
    $P4 = new 'String'
    $P4 = "data for Foo\n"
    $P3['data'] = $P4

    $P6 = new 'String'
    $P6 = 'Foo'
    $P2 = new $P6, $P3

    $S1 = typeof $P2
    say $S1

    $I3 = isa $P2, "Foo"
    print $I3
    print "\n"

    $I3 = isa $P2, "Object"
    print $I3
    print "\n"

    $P5 = getattribute $P2, 'data'
    print $P5
.end
CODE
parrot;Foo
1
1
data for Foo
OUT

pir_output_is( <<'CODE', <<'OUT', 'instantiate from key name with init', todo => 'init keyed' );
.sub main :main
    $P1 = newpdd15class ['Foo';'Bar']
    addattribute $P1, 'data'

    $P3 = new 'Hash'
    $P4 = new 'String'
    $P4 = "data for Foo;Bar\n"
    $P3['data'] = $P4

    $P2 = new ['Foo';'Bar'], $P3

    $S1 = typeof $P2
    say $S1

    $I3 = isa $P2, 'Bar'
    print $I3
    print "\n"

    $I3 = isa $P2, "Object"
    print $I3
    print "\n"

    $P5 = getattribute $P2, 'data'
    print $P5
.end
CODE
parrot;Foo;Bar
1
1
data for Foo;Bar
OUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
