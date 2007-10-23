#!perl
# Copyright (C) 2001-2005, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw( t . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 7;

=head1 NAME

t/library/protoobject.t - testing Protoobject.pir

=head1 SYNOPSIS

        % prove t/library/protoobject.t

=head1 DESCRIPTION

This test exercises the protoobject/Protomaker implementations.

=cut

pir_output_is( << 'END_CODE', << 'END_OUT', 'basic load' );
.sub main :main
    load_bytecode 'Protoobject.pbc'

    $P0 = new 'Protomaker'
    $S0 = typeof $P0
    say $S0
.end
END_CODE
Protomaker
END_OUT

pir_output_is( << 'END_CODE', << 'END_OUT', 'type of protoobject' );
.sub main :main
    load_bytecode 'Protoobject.pbc'

    $P0 = new 'Protomaker'
    $P1 = newclass 'XYZ'
    $P2 = $P0.'new_proto'($P1)

    $S0 = typeof $P2
    say $S0
.end
END_CODE
XYZ
END_OUT

pir_output_is( << 'END_CODE', << 'END_OUT', 'type of ns-based protoobject' );
.sub main :main
    load_bytecode 'Protoobject.pbc'

    $P0 = new 'Protomaker'
    $P1 = newclass ['Foo';'Bar']
    $P2 = $P0.'new_proto'($P1)

    $S0 = typeof $P2
    say $S0
.end
END_CODE
Foo;Bar
END_OUT

pir_output_is( << 'END_CODE', << 'END_OUT', 'Protoobject symbol' );
.sub main :main
    load_bytecode 'Protoobject.pbc'

    $P0 = new 'Protomaker'
    $P1 = newclass ['Foo';'Bar']
    $P2 = $P0.'new_proto'($P1)

    $P2 = get_hll_global ['Foo'], 'Bar'
    $S0 = typeof $P2
    say $S0
.end
END_CODE
Foo;Bar
END_OUT

pir_output_is( << 'END_CODE', << 'END_OUT', 'Protoobject symbol' );
.sub main :main
    load_bytecode 'Protoobject.pbc'

    $P0 = new 'Protomaker'
    $P1 = newclass 'Foo'
    $P2 = $P0.'new_proto'($P1)

    $P2 = get_hll_global 'Foo'
    $S0 = typeof $P2
    say $S0
.end
END_CODE
Foo
END_OUT


pir_output_is(<<'END_CODE', <<'END_OUT', 'Protoobject symbol for :: classes');
.sub main :main
    load_bytecode 'Protoobject.pbc'

    $P0 = new 'Protomaker'
    $P1 = newclass 'Foo::Bar'
    $P2 = $P0.'new_proto'($P1)

    $P2 = get_hll_global ['Foo'], 'Bar'
    $S0 = typeof $P2
    say $S0
.end
END_CODE
Foo::Bar
END_OUT

pir_output_is(<<'END_CODE', <<'END_OUT', 'new_subclass for :: classes');
.sub main :main
    load_bytecode 'Protoobject.pbc'

    $P0 = new 'Protomaker'
    $P1 = get_class 'Hash'
    $P0.'new_subclass'($P1, 'Foo::Bar')

    $P2 = new 'Foo::Bar'
    $S0 = typeof $P2
    say $S0

    $P2 = get_hll_global ['Foo'], 'Bar'
    $S0 = typeof $P2
    say $S0
.end
END_CODE
Foo::Bar
Foo::Bar
END_OUT


# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:

