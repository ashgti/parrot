# Copyright (C) 2009, Patrick R. Michaud
# $Id$

=head1 NAME

RX::Cursor - RX Cursor nodes

=head1 DESCRIPTION

This file implements the RX::Cursor class, used for managing regular
expression control flow.  RX::Cursor is also a base class for
grammars.

=cut

.namespace ['RX';'Cursor']

.sub '' :anon :load :init
    load_bytecode 'P6object.pbc'
    .local pmc p6meta
    p6meta = new 'P6metaclass'
    $P0 = p6meta.'new_class'('RX::Cursor', 'attr'=>'$!target $!from $!pos @!pstack')
    .return ()
.end

=head2 Methods

=over 4

=back

=head1 AUTHORS

Patrick Michaud <pmichaud@pobox.com> is the author and maintainer.

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
