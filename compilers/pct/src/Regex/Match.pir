# Copyright (C) 2009, Patrick R. Michaud
# $Id$

=head1 NAME

Regex::Match - Regex Match objects

=head1 DESCRIPTION

This file implements Match objects for the regex engine.

=cut

.namespace ['Regex';'Match']

.sub '' :anon :load :init
    load_bytecode 'P6object.pbc'
    .local pmc p6meta
    p6meta = new 'P6metaclass'
    $P0 = p6meta.'new_class'('Regex::Match', 'parent'=>'Capture', 'attr'=>'$!target $!from $!to $!ast')
    .return ()
.end

=head2 Methods

=over 4

=item from()

Returns the offset in the target string of the beginning of the match.

=cut

.sub 'from' :method
    $P0 = getattribute self, '$!from'
    .return ($P0)
.end


=item to()

Returns the offset in the target string of the end of the match.

=cut

.sub 'to' :method
    $P0 = getattribute self, '$!to'
    .return ($P0)
.end


=item chars()

Returns C<.to() - .from()>

=cut

.sub 'chars' :method
    $I0 = self.'to'()
    $I1 = self.'from'()
    $I2 = $I0 - $I1
    .return ($I2)
.end


=item orig()

Return the original item that was matched against.

=cut

.sub 'orig' :method
    $P0 = getattribute self, '$!target'
    .return ($P0)
.end


=item Str()

Returns the portion of the target corresponding to this match.

=cut

.sub 'Str' :method
    $S0 = self.'orig'()
    $I0 = self.'from'()
    $I1 = self.'to'()
    $I1 -= $I0
    $S1 = substr $S0, $I0, $I1
    .return ($S1)
.end


=item ast()

Returns the "abstract object" for the Match; if no abstract object
has been set then returns C<Str> above.

=cut

.sub 'ast' :method
    .local pmc ast
    ast = getattribute self, '$!ast'
    if null ast goto ret_null
    .return (ast)
  ret_null:
    .tailcall self.'Str'()
.end


=back

=head2 Vtable functions

=over 4

=item get_bool()

Returns 1 (true) if this is the result of a successful match,
otherwise returns 0 (false).

=cut

.sub '' :vtable('get_bool') :method
    $P1 = getattribute self, '$!to'
    $I0 = $P1
    $I1 = isge $I0, 0
    .return ($I1)
.end


=item get_integer()

Returns the integer value of the matched text.

=cut

.sub '' :vtable('get_integer') :method
    $I0 = self.'Str'()
    .return ($I0)
.end


=item get_number()

Returns the numeric value of this match

=cut

.sub '' :vtable('get_number') :method
    $N0 = self.'Str'()
    .return ($N0)
.end


=item get_string()

Returns the string value of the match

=cut

.sub '' :vtable('get_string') :method
    $S0 = self.'Str'()
    .return ($S0)
.end
    

=item !make(obj)

Set the "ast object" for the invocant.

=cut

.sub '!make' :method
    .param pmc obj
    setattribute self, '$!ast', obj
    .return (obj)
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
