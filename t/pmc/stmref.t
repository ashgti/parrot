#! parrot
# Copyright (C) 2007, The Perl Foundation.
# $Id$

=head1 NAME

t/pmc/stmref.t - test the STMRef PMC

=head1 SYNOPSIS

    % prove t/pmc/stmref.t

=head1 DESCRIPTION

Tests the STMRef PMC.

=cut

.sub main :main
    .include 'include/test_more.pir'

    plan(1)

    new P0, 'STMRef'
    ok(1, 'Instantiated .STMRef')
.end

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
