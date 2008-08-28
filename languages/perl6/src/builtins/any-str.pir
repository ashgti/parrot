## $Id$

=head1 NAME

src/builtins/any-str.pir -  C<Str>-like functions and methods for C<Any>

=head1 DESCRIPTION

This file implements the methods and functions of C<Any> that
are most closely associated with the C<Str> class or role.
We place them here instead of F<src/classes/Any.pir> to keep
the size of that file down and to emphasize their generic,
"built-in" nature.

=head2 Methods

=over 4

=cut

.namespace []
.sub 'onload' :anon :init :load
    $P0 = get_hll_namespace ['Any']
    '!EXPORT'('chars index substr', 'from'=>$P0)
.end


=item chars()

=cut

.namespace ['Any']
.sub 'chars' :method :multi(_)
    $S0 = self
    $I0 = length $S0
    .return ($I0)
.end

=item index()

=cut

.namespace ['Any']
.sub 'index' :method :multi(_)
    .param string substring
    .param int pos             :optional
    .param int has_pos         :opt_flag
    .local pmc retv

    if has_pos goto have_pos
    pos = 0
  have_pos:

    .local string s
    s = self

  check_substring:
    if substring goto substring_search
    $I0 = length s
    if pos < $I0 goto done
    pos = $I0
    goto done

  substring_search:
    pos = index s, substring, pos
    if pos < 0 goto fail

  done:
    $P0 = new 'Int'
    $P0 = pos
    .return ($P0)

  fail:
    $P0 = new 'Failure'
    .return ($P0)
.end

=item substr()

=cut

.namespace ['Any']
.sub 'substr' :method :multi(_, _)
    .param int start
    .param int len             :optional
    .param int has_len         :opt_flag

    if has_len goto have_len
    len = self.'chars'()
  have_len:
    if len >= 0 goto len_done
    $I0 = self.'chars'()
    len += $I0
    len -= start
  len_done:
    $S0 = self
    $S1 = substr $S0, start, len
    .return ($S1)
.end


=item trans()

=back

=cut

=item subst

 our Str method Str::subst ( Any $string: Any $substring, Any $replacement )
 our Str method Str::subst ( Any $string: Code $regexp,   Any $replacement )

Replaces the first occurrence of a given substring or a regular expression
match with some other substring.

Partial implementation. The :g modifier on regexps doesn't work, for example.

=cut

.sub subst :method :multi(_, _, _)
    .param string substring
    .param string replacement
    .local int pos
    .local int pos_after
    .local pmc retv

    retv = new 'Perl6Str'

    $S0 = self
    pos = index $S0, substring
    if pos < 0 goto no_match

    pos_after = pos
    $I0 = length substring
    add pos_after, $I0

    $S1 = substr $S0, 0, pos
    $S2 = substr $S0, pos_after
    concat retv, $S1
    concat retv, replacement
    concat retv, $S2

    goto done

  no_match:
    retv = self

  done:
    .return(retv)
.end

.sub subst :method :lex :multi(_, 'Sub', _)
    .param pmc regexp
    .param string replacement
    .local int pos
    .local int pos_after
    .local pmc retv
    .local pmc match

    retv = new 'Perl6Str'

    new $P14, "Perl6Scalar"
    .lex "$/", $P14

    match = regexp.'ACCEPTS'(self)
    unless match goto no_match
    pos = match.'from'()
    pos_after = match.'to'()

    $S0 = self
    $S1 = substr $S0, 0, pos
    $S2 = substr $S0, pos_after
    concat retv, $S1
    concat retv, replacement
    concat retv, $S2

    goto done

  no_match:
    retv = self

  done:
    .return(retv)
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
