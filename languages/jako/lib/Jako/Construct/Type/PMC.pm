#
# PMC.pm
#
# Copyright (C) 2002-2005, Parrot Foundation.
# This program is free software. It is subject to the same license
# as the Parrot interpreter.
#
# $Id$
#

use strict;
use warnings;

package Jako::Construct::Type::PMC;

use base qw(Jako::Construct::Type);

sub new {
    my $class = shift;
    my ($token) = @_;

    return bless {
        TOKEN    => $token,
        CODE     => 'P',
        NAME     => 'pmc',
        IMCC     => 'pmc',
        IMCC_PMC => 'PMC'
    }, $class;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
