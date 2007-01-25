#!perl
# Copyright (C) 2006, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 8;

=head1 NAME

t/pmc/parrotobject.t - test the ParrotObject PMC


=head1 SYNOPSIS

    % prove t/pmc/parrotobject.t

=head1 DESCRIPTION

Tests the ParrotObject PMC.

=cut

pir_output_like( <<'CODE', <<'OUT', 'new' );
.sub 'test' :main
    new P0, .ParrotObject
    print "ok 1\n"
.end
CODE
/Can't create new ParrotObjects - use the registered class instead
current instr\.:/
OUT

# '

pir_output_is( <<'CODE', <<'OUT', ':vtable override' );
.sub main :main
   $P0 = newclass [ "Test" ]
   $P1 = new [ "Test" ]
   $I1 = $P1[11]
   print $I1
   print "\n"
.end

.namespace [ "Test" ]

.sub get_integer_keyed_int :method :vtable
   .param int key
   .return(42)
.end
CODE
42
OUT

# '

pir_output_is( <<'CODE', <<'OUT', ':vtable("...") override' );
.sub main :main
    $P0 = newclass [ "Test" ]
    $P1 = new [ "Test" ]
    $S1 = $P1[11]
    print $S1
    print "\n"
.end

.namespace [ "Test" ]

.sub monkey :method :vtable("get_string_keyed_int")
    .param int key
    .return("monkey")
.end
CODE
monkey
OUT

# '

pir_output_like( <<'CODE', <<'OUT', ':vtable with bad name' );
.namespace [ "Test" ]

.sub monkey :method :vtable("not_in_the_vtable")
    .param int key
    .return("monkey")
.end
CODE
/'not_in_the_vtable' is not a v-table method, but was used with :vtable/
OUT

# '

pir_output_is( <<'CODE', <<'OUT', ':vtable and init' );
.sub main :main
   $P0 = newclass [ "Test" ]
   $P1 = new [ "Test" ]
   print "ok\n"
.end

.namespace [ "Test" ]

.sub init :method :vtable
   print "init\n"
.end
CODE
init
ok
OUT

# '

pir_output_is( <<'CODE', <<'OUT', ':vtable inheritance' );
.sub main :main
   $P0 = newclass [ "Test" ]
   $P1 = newclass [ "Test2" ]
   addparent $P1, $P0
   $P2 = new [ "Test2" ]
   $P3 = clone $P2
   print "ok\n"
.end

.namespace [ "Test" ]

.sub clone :method :vtable
   print "cloned\n"
.end

.namespace [ "Test2" ]
CODE
cloned
ok
OUT

# '

# :vtable inheritance; RT #40626
pir_output_is( <<'CODE', <<'OUT', ':vtable inheritance from core classes' );
.sub main :main
    $P0 = subclass 'Hash', 'Foo'
    $P0 = subclass 'Hash', 'Bar'

    $P1 = new 'Foo'
    $S1 = $P1
    say $S1

    $P1 = new 'Bar'
    $S1 = $P1
    say $S1
.end


.namespace [ 'Foo' ]

.sub '__get_string' :method
    .return('Foo::__get_string')
.end


.namespace [ 'Bar' ]

.sub 'get_string' :method :vtable
    .return('Bar::get_string')
.end
CODE
Foo::__get_string
Bar::get_string
OUT


# assign opcode in inherited classes
pir_output_is( <<'CODE', <<'OUT', 'assign opcode in inherited classes', 'todo' => 'assign opcode inheritance' );
.sub main :main
    $P1 = new .ResizablePMCArray
    push $P1, 3
    $P2 = new .ResizablePMCArray
    assign $P2, $P1
    $I0 = elements $P2
    print $I0
    print "\n"

    $P99 = subclass 'ResizablePMCArray', 'Perl6List'
    $P1 = new 'Perl6List'
    push $P1, 3
    $P2 = new 'Perl6List'
    assign $P2, $P1
    $I0 = elements $P2
    print $I0
    print "\n"
.end
CODE
1
1
OUT

# '

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
