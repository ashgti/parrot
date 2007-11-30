#! parrot
# Copyright (C) 2006-2007, The Perl Foundation.
# $Id$

=head1 NAME

t/pmc/addrregistry.t - test AddrRegistry PMC

=head1 SYNOPSIS

    % prove t/pmc/addrregistry.t

=head1 DESCRIPTION

Tests the AddrRegistry PMC.

=cut

.sub main :main
    .include 'include/test_more.pir'

    plan(1)

    new P0, 'AddrRegistry'
    ok(1, 'Instantiated .AddrRegistry')
.end

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
