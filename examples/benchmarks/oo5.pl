#! perl

# Copyright (C) 2004-2007, Parrot Foundation.
# $Id$

use strict;
use warnings;

my $o = new Foo();
for my $i ( 1 .. 500000 ) {
    my $x = $o->i;
    my $y = $o->j;
}
print $o->i, "\n";

package Foo;

sub new {
    my $self = ref $_[0] ? ref shift : shift;
    return bless [ 10, 20 ], $self;
}

sub i {
    my $self = shift;
    if (@_) { return $self->[0] = shift }
    else    { return $self->[0] }
}

sub j {
    my $self = shift;
    if (@_) { return $self->[1] = shift }
    else    { return $self->[1] }
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
