## $Id$

=head1 NAME

src/builtins/named-unary.pir - Perl6 named unary builtins

=head1 Functions

=over 4

=cut

.namespace []

=item HOW($x)

=item WHAT($x)

Return the metaclass or protoobject for C<$x>.

=cut

.sub 'HOW'
    .param pmc x
    .return x.'HOW'()
.end


.sub 'WHAT'
    .param pmc x
    .return x.'WHAT'()
.end


=item defined($x)

Returns a true value if $x is defined, and a false value otherwise.

=cut

.sub 'defined'
    .param pmc x
    $I0 = defined x
    .return ($I0)
.end


=item undefine $x

Sets $x to an undefined value

=cut

.sub 'undefine'
    .param pmc x
    $P0 = new 'Undef'
    $I0 = isa x, 'Mutable'
    unless $I0 goto copy
    assign x, $P0
    .return ()
  copy:
    copy x, $P0
.end


=back

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
