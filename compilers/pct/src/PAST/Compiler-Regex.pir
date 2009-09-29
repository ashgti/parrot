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
    $P0 = split ' ', 'tgt string pos int off int len int rep int cur pmc'
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

    .local string cur, rep, pos
    (cur, rep, pos) = self.'!rxregs'('cur rep pos')

    $P0 = self.'post_regex'(node)
    ops.'push'($P0)
    ops.'push'(faillabel)
    $S0 = concat '(', rep
    concat $S0, ','
    concat $S0, pos
    concat $S0, ',$I10)'
    ops.'push_pirop'('callmethod', "'!mark_cut'", cur, 'result'=>$S0)
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
the node's "pasttype" and "backtrack" attributes.  If no "pasttype" is
given, then "concat" is assumed.

=cut

.sub 'post_regex' :method :multi(_,['PAST';'Regex'])
    .param pmc node
    .param string cur          :optional
    .param int have_cur        :opt_flag

    .local string pasttype
    pasttype = node.'pasttype'()
    if pasttype goto have_pasttype
    pasttype = 'concat'
  have_pasttype:
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


=item alt(PAST::Regex node)

=cut

.sub 'alt' :method :multi(_, ['PAST';'Regex'])
    .param pmc node

    .local pmc cur, pos
    (cur, pos) = self.'!rxregs'('cur pos')

    .local string name
    name = self.'unique'('alt')
    concat name, '_'

    .local pmc ops, iter
    ops = self.'post_new'('Ops', 'node'=>node, 'result'=>cur)
    iter = node.'iterator'()
    unless iter goto done

    .local int acount
    .local pmc alabel, endlabel
    acount = 0
    $S0 = acount
    $S0 = concat name, $S0
    alabel = self.'post_new'('Label', 'result'=>$S0)
    $S0 = concat name, 'end'
    endlabel = self.'post_new'('Label', 'result'=>$S0)

  iter_loop:
    ops.'push'(alabel)
    .local pmc apast, apost
    apast = shift iter
    apost = self.'post_regex'(apast, cur)
    unless iter goto iter_done
    inc acount
    $S0 = acount
    $S0 = concat name, $S0
    alabel = self.'post_new'('Label', 'result'=>$S0)
    ops.'push_pirop'('set_addr', '$I10', alabel)
    ops.'push_pirop'('callmethod', '"!mark_push"', cur, 0, pos, '$I10')
    ops.'push'(apost)
    ops.'push_pirop'('goto', endlabel)
    goto iter_loop
  iter_done:
    ops.'push'(apost)
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


=item pass(PAST::Regex node)

=cut

.sub 'pass' :method :multi(_,['PAST';'Regex'])
    .param pmc node

    .local pmc cur, ops
    cur = self.'!rxregs'('cur')
    ops = self.'post_new'('Ops', 'result'=>cur)
    ops.'push_pirop'('yield', cur)
    .return (ops)
.end


=item quant(PAST::Regex node)

=cut

.sub 'quant' :method :multi(_,['PAST';'Regex'])
    .param pmc node

    .local string backtrack
    backtrack = node.'backtrack'()
    if backtrack goto have_backtrack
    backtrack = 'g'
  have_backtrack:

     .local int min, max
     min = node.'min'()
     $P0 = node.'max'()
     max = $P0
     $I0 = defined $P0
     if $I0 goto have_max
     max = -1                          # -1 represents Inf
   have_max:

    .local pmc cur, pos, rep, fail
    (cur, pos, rep, fail) = self.'!rxregs'('cur pos rep fail')

    .local string qname
    .local pmc ops, q1label, q2label, q2reg, cpost
    $S0 = concat 'quant', backtrack
    qname = self.'unique'($S0)
    ops = self.'post_new'('Ops', 'node'=>node)
    $S0 = concat qname, '_loop'
    q1label = self.'post_new'('Label', 'result'=>$S0)
    $S0 = concat qname, '_done'
    q2label = self.'post_new'('Label', 'result'=>$S0)
    q2reg = self.'uniquereg'('I')
    cpost = self.'concat'(node)

    $S0 = max
    .local int needrep
    $I0 = isgt min, 1
    $I1 = isgt max, 1
    needrep = or $I0, $I1

    unless max < 0 goto have_s0
    $S0 = '*'
  have_s0:
    ops.'push_pirop'('inline', qname, min, $S0, 'inline'=>'  # rx %0 ** %1..%2')
    ops.'push_pirop'('set_addr', q2reg, q2label)


  greedy:
    .local int needmark
    .local string peekcut
    needmark = needrep
    peekcut = '"!mark_peek"'
    if backtrack != 'r' goto greedy_1
    needmark = 1
    peekcut = '"!mark_cut"'
  greedy_1:
    if min == 0 goto greedy_2
    unless needmark goto greedy_loop
    ops.'push_pirop'('callmethod', '"!mark_push"', cur, 0, -1, q2reg)
    goto greedy_loop
  greedy_2:
    ops.'push_pirop'('callmethod', '"!mark_push"', cur, 0, pos, q2reg)
  greedy_loop:
    ops.'push'(q1label)
    ops.'push'(cpost)
    unless needmark goto greedy_3
    ops.'push_pirop'('callmethod', peekcut, cur, q2reg, 'result'=>rep)
    unless needrep goto greedy_3
    ops.'push_pirop'('inc', rep)
  greedy_3:
    unless max > 1 goto greedy_4
    ops.'push_pirop'('ge', rep, max, q2label)
  greedy_4:
    unless max != 1 goto greedy_5
    ops.'push_pirop'('callmethod', '"!mark_push"', cur, rep, pos, q2reg)
    ops.'push_pirop'('goto', q1label)
  greedy_5:
    ops.'push'(q2label)
    unless min > 1 goto greedy_6
    ops.'push_pirop'('lt', rep, min, fail)
  greedy_6:
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

