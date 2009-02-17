#! parrot
# Copyright (C) 2006-2008, Parrot Foundation.
# $Id$

=head1 NAME

t/pmc/retcontinuation.t - test the RetContinuation PMC

=head1 SYNOPSIS

    % prove t/pmc/retcontinuation.t

=head1 DESCRIPTION

Tests the RetContinuation PMC.

=cut

.sub main :main
    .include 'test_more.pir'

    plan(1)

    new $P0, ['RetContinuation']
    ok(1, 'Instantiated a RetContinuation PMC')
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
