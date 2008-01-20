#! perl
# Copyright (C) 2001-2005, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 3;

=head1 NAME

t/pmc/pair.t - pair tests

=head1 SYNOPSIS

    % prove t/pmc/pair.t

=head1 DESCRIPTION

Tests the C<Pair> PMC.

=cut

pasm_output_is( <<'CODE', <<'OUT', 'create' );
    new P0, 'Pair'
    print "ok 1\n"
    new P1, 'Integer'
    set P1, 42
    set P0["key"], P1
    print "ok 2\n"
    set P2, P0["key"]
    print P2
    print "\n"
    end
CODE
ok 1
ok 2
42
OUT

pir_output_is( <<'CODE', <<'OUT', 'methods' );
.sub main :main
    .local pmc p, kv
    new p, 'Pair'
    new $P1, 'Integer'
    set $P1, 42
    set p["key"], $P1

    $P0 = p."key"()
    print $P0
    print ' '
    $P0 = p."value"()
    print $P0
    print ' '
    kv = p."kv"()
    $I0 = elements kv
    print $I0
    print ' '
    $P0 = kv[0]
    print $P0
    print ' '
    $P0 = kv[1]
    say $P0
.end
CODE
key 42 2 key 42
OUT

SKIP: {
    skip( "instantiate disabled", 1 );
    pir_output_is( <<'CODE', <<'OUT', 'instantiate, assign' );
.sub main :main
    .local pmc cl, p, kv, k, v
    k = new 'String'
    k = "key"
    v = new 'String'
    v = "value"
    cl = get_class "Pair"
    p = cl."instantiate"(k, v)

    $P0 = p."key"()
    print $P0
    print ' '
    $P0 = p."value"()
    print $P0
    print ' '

    v = new 'Integer'
    v = 77
    assign p, v
    $P0 = p."value"()
    say $P0
.end
CODE
key value 77
OUT
}

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
