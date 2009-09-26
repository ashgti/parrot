# $Id$

=head1 NAME

PAST::Compiler-Regex - Compiler for PAST::Regex nodes

=head1 DESCRIPTION

PAST::Compiler-Regex implements the transformations to convert 
PAST::Regex nodes into POST.  It's still a part of PAST::Compiler;
we've separated out the regex-specific transformations here for
better code management and debugging.

=head2 Compiler methods

=head3 C<PAST::Regex>

=over 4

=item as_post(PAST::Regex node)

Return the POST representation of the regex AST rooted by C<node>.

=cut

.sub 'as_post' :method :multi(_, ['PAST';'Regex'])
    .param pmc node
    .param pmc options         :slurpy :named

    .local pmc ops
    ops = self.'post_new'('Ops', 'node'=>node)

    .local pmc reghash
    reghash = new ['Hash']
    .lex '$*REG', reghash

    .local string prefix, rname, rtype
    prefix = self.'unique'('rx')
    concat prefix, '_'
    $P0 = split ' ', 'tgt string pos int off int len int cur pmc'
    $P1 = iter $P0
  iter_loop:
    unless $P1 goto iter_done
    rname = shift $P1
    rtype = shift $P1
    $S1 = concat prefix, rname
    reghash[rname] = $S1
    $S2 = concat '.local ', rtype
    ops.'push_pirop'($S2, $S1)
    goto iter_loop
  iter_done:

    .local pmc faillabel
    $S1 = concat prefix, 'fail'
    faillabel = self.'post_new'('Label', 'result'=>$S1)
    reghash['fail'] = faillabel

    .local string cur, pos
    (cur, pos) = self.'!rxregs'('cur pos')

    $P0 = self.'post_regex'(node)
    ops.'push'($P0)
    ops.'push'(faillabel)
    $S0 = concat '(', cur
    concat $S0, ','
    concat $S0, pos
    concat $S0, ',$I10)'
    ops.'push_pirop'('callmethod', "'!popmark'", cur, 'result'=>$S0)
    ops.'push_pirop'('jump', '$I10')
    .return (ops)
.end

=item !rxregs(keystr)

Helper function -- looks up the current regex register table
in the dynamic scope and returns a slice based on the keys
given in C<keystr>.

=cut

.sub '!rxregs' :method
    .param string keystr

    .local pmc keys, reghash, vals
    keys = split ' ', keystr
    reghash = find_dynamic_lex '$*REG'
    vals = new ['ResizablePMCArray']
  keys_loop:
    unless keys goto keys_done
    $S0 = shift keys
    $P0 = reghash[$S0]
    push vals, $P0
    goto keys_loop
  keys_done:
    .return (vals :flat)
.end


=item post_regex(PAST::Regex node)

Return the POST representation of the regex component given by C<node>.
Normally this is handled by redispatching to a method corresponding to
the node's "pasttype" and "backtrack" attributes.

=cut

.sub 'post_regex' :method :multi(_,['PAST';'Regex'])
    .param pmc node
    .param string cur          :optional
    .param int have_cur        :opt_flag

    .local string pasttype
    pasttype = node.'pasttype'()
    $P0 = find_method self, pasttype
    $P1 = self.$P0(node)
    unless have_cur goto done
    $S0 = $P1.'result'()
    if $S0 == cur goto done
    $P1 = self.'coerce'($P1, cur)
  done:
    .return ($P1)
.end


.sub 'post_regex' :method :multi(_, _)
    .param pmc node
    .param string cur          :optional
    .param int have_cur        :opt_flag

    $P0 = self.'as_post'(node)
    unless have_cur goto done
    $P0 = self.'coerce'($P0, cur)
  done:
    .return ($P0)
.end


=item regex_mark(prefix)

Create a label starting with C<prefix> and POST instructions
to set a backtrack to the label in the current cursor.

=cut

.sub 'regex_mark' :method
    .param string prefix

    .local pmc cur, pos, ops, backlabel
    (cur, pos) = self.'!rxregs'('cur pos')
    ops = self.'post_new'('Ops')
    backlabel = self.'post_new'('Label', 'name'=>prefix)
    ops.'push_pirop'('set_addr', '$I10', backlabel)
    ops.'push_pirop'('callmethod', "'!pushmark'", cur, pos, '$I10')
    .return (ops, backlabel)
.end


=item alt(PAST::Regex node)

Create POST to alternate among child regexes of C<node>, including
backtracking.

=cut

.sub 'alt' :method :multi(_, ['PAST';'Regex'])
    .param pmc node

    .local pmc cur
    cur = self.'!rxregs'('cur')

    .local pmc ops, iter, cpast, cpost
    ops = self.'post_new'('Ops', 'node'=>node, 'result'=>cur)

    iter = node.'iterator'()
    unless iter goto done

    # get post for first alternative
    .local pmc apast, apost, amark, alabel, endlabel
    apast = shift iter
    apost = self.'post_regex'(apast, cur)
    ops.'push'(apost)
    unless iter goto done

    endlabel = self.'post_new'('Label', 'name'=>'rx_alt_end_')

    # for all remaining alternatives, we put a label at the end
    # of the previous alternative, generate a label and backtracking
    # mark for the new alternative, and add those to our ops list
  iter_loop:
    ops.'push_pirop'('goto', endlabel)
    apast = shift iter
    apost = self.'post_regex'(apast, cur)
    (amark, alabel) = self.'regex_mark'('rx_alt_')
    ops.'unshift'(amark)
    ops.'push'(alabel)
    ops.'push'(apost)
    if iter goto iter_loop

  d1:
    ops.'push'(endlabel)

  done:
    .return (ops)
.end


=item concat(PAST::Regex node)

Handle a concatenation of regexes.

=cut

.sub 'concat' :method :multi(_, ['PAST';'Regex'])
    .param pmc node

    .local pmc cur, ops, iter
    (cur) = self.'!rxregs'('cur')
    ops = self.'post_new'('Ops', 'node'=>node, 'result'=>cur)
    iter = node.'iterator'()

  iter_loop:
    unless iter goto iter_done
    .local pmc cpast, cpost
    cpast = shift iter
    cpost = self.'post_regex'(cpast, cur)
    ops.'push'(cpost)
    goto iter_loop
  iter_done:

    .return (ops)
.end


=item literal(PAST::Regex node)

Generate POST for matching a literal string provided as the
second child of this node.

=cut

.sub 'literal' :method :multi(_,['PAST';'Regex'])
    .param pmc node

    .local pmc cur, pos, len, tgt, fail, off
    (cur, pos, len, tgt, fail, off) = self.'!rxregs'('cur pos len tgt fail off')
    .local pmc ops, cpast, cpost, lpast, lpost
    ops = self.'post_new'('Ops', 'node'=>node, 'result'=>cur)

    # literal to be matched is our first child
    lpast = node[0]
    lpost = self.'as_post'(lpast, 'rtype'=>'~')

    $S0 = lpost.'result'()
    ops.'push_pirop'('inline', $S0, 'inline'=>'  # rx literal %0')
    ops.'push'(lpost)

    # compute constant literal length at compile time
    .local string litlen
    $I0 = isa lpast, ['String']
    if $I0 goto literal_string
    litlen = '$I10'
    ops.'push_pirop'('length', '$I10', lpost)
    goto have_litlen
  literal_string:
    $S0 = lpast
    $I0 = length $S0
    litlen = $I0
    if $I0 > 0 goto have_litlen
    .return (cpost)
  have_litlen:

    # fail if there aren't enough characters left in string
    ops.'push_pirop'('add', '$I11', pos, litlen)
    ops.'push_pirop'('gt', '$I11', len, fail)

    # compute string to be matched and fail if mismatch
    ops.'push_pirop'('sub', '$I11', pos, off)
    ops.'push_pirop'('substr', '$S10', tgt, '$I11', litlen)
    ops.'push_pirop'('ne', '$S10', lpost, fail)

    # increase position by literal length and move on
    ops.'push_pirop'('add', pos, litlen)
    .return (ops)
.end


.sub 'pass' :method :multi(_,['PAST';'Regex'])
    .param pmc node

    .local pmc cur, ops
    cur = self.'!rxregs'('cur')
    ops = self.'post_new'('Ops', 'result'=>cur)
    ops.'push_pirop'('yield', cur)
    .return (ops)
.end
    

=back

=head1 AUTHOR

Patrick Michaud <pmichaud@pobox.com> is the author and maintainer.

=head1 COPYRIGHT

Copyright (C) 2009, Patrick R. Michaud.

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:

