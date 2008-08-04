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

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
