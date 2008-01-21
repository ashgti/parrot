#! parrot
# Copyright (C) 2006-2007, The Perl Foundation.
# $Id$

=head1 NAME

t/pmc/pointer.t - test the Pointer PMC

=head1 SYNOPSIS

    % prove t/pmc/pointer.t

=head1 DESCRIPTION

Tests the Pointer PMC.

=cut

.sub main :main
    .include 'include/test_more.pir'

    plan(1)

    new P0, 'Pointer'
    ok(1, 'Instantiated .Pointer')
.end

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
