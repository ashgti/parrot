# Copyright (C) 2009, Patrick R. Michaud
# $Id$

=head1 NAME

Regex::P6Regex - Parser/compiler for Perl 6 regexes

=head1 DESCRIPTION

=cut

.namespace ['Regex';'P6Regex']

.include 'cclass.pasm'

.sub '' :anon :load :init
    load_bytecode 'P6object.pbc'
    .local pmc p6meta
    p6meta = new 'P6metaclass'
    $P0 = p6meta.'new_class'('Regex::P6Regex', 'parent'=>'Regex::Cursor')
    .return ()
.end

=head2 Methods

=over 4

=item atom

    token atom {
        :dba('regex atom')
        [
        | \w [\w+! <?before \w>]
        | <metachar> ::
        ]
    }

=cut

.sub 'atom' :method
    .param pmc peek            :named('peek') :optional

    if null peek goto peek_done
    .return ()
  peek_done:

    .local pmc cur
    .local string target
    .local int pos, eos
    (cur, pos, target) = self.'!cursor_start'()
    eos = length target

    # \w [\w+! <?before \w>]
    .local int eow, len
    eow = find_not_cclass .CCLASS_WORD, target, pos, eos
    if eow == pos goto metachar
    .local int len
    len = eow - pos
    if len == 1 goto word_done
    dec eow
  word_done:
    cur.'!matchify'(eow, 'atom')
    .return (cur)

  metachar:
    .return (cur)
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
