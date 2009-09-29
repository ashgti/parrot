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

=head2 Private methods

=over 4

=item !mark_push(rep, pos, mark)

Push a new backtracking point onto the cursor with the given
C<rep>, C<pos>, and backtracking C<mark>.  (The C<mark> is typically
the address of a label to branch to when backtracking occurs.)

=cut

.sub '!mark_push' :method
    .param int rep
    .param int pos
    .param int mark

    .local pmc bstack, mstack
    bstack = getattribute self, '@!bstack'
    mstack = getattribute self, '@!mstack'

    push bstack, mark
    push bstack, pos
    push bstack, rep
    $I0 = mstack
    push bstack, $I0
.end

=item !mark_fail([mark])

Remove the most recent C<mark> and backtrack the cursor to the
point given by that mark.  If no C<mark> is provided, then
backtracks the most recent mark.  Returns the backtracked
values of repetition count, cursor position, and mark (address).

=cut

.sub '!mark_fail' :method
    .param int mark            :optional
    .param int has_mark        :opt_flag

    .local pmc bstack, mstack
    bstack = getattribute self, '@!bstack'
    bstack = getattribute self, '@!mstack'

    # get the frame associated with the mark
    .local int bptr, rep, pos, mark, mptr
    bptr = self.'!mark_bptr'(mark, has_mark)

    # retrieve the mark, pos, rep, and match index from the frame
    mark = bstack[bptr]
    $I0  = bptr + 1
    pos  = bstack[$I0]
    inc $I0
    rep  = bstack[$I0]
    inc $I0
    mptr = bstack[$I0]

    # release the backtracked stack elements
    assign bstack, bptr
    assign mstack, mptr

    .local pmc match
    match = mstack[-1]

    # return mark values
    .return (rep, pos, mark, match)
.end


=item '!mark_commit'(mark)

Like C<!mark_fail> above this backtracks the cursor to C<mark>
(releasing any intermediate marks), but preserves the current 
match state.

=cut

.sub '!mark_commit' :method
    .param int mark

    # preserve the current match object
    .local pmc mstack, match
    mstack = getattribute self, '@!mstack'

    # backtrack
    .local int rep, pos, mark
    (rep, pos, mark, $P0) = self.'!mark_fail'(mark)

    # if match we backtracked to is the same as this one,
    # we don't need to preserve it specially
    $I0 = issame $P0, match
    if $I0 goto done
    # save current match state
    push mstack, match

  done:
    .return (rep, pos, mark, match)
.end

=back

=head1 AUTHORS

Patrick Michaud <pmichaud@pobox.com> is the author and maintainer.

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
