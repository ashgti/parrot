## $Id$

=head1 TITLE

Num - JS numbers

=head1 SUBROUTINES

=over 4

=item onload()

=cut

.namespace [ 'Num' ]

.sub 'onload' :anon :init :load
    .local pmc jsmeta, numproto
    jsmeta = get_hll_global ['JSObject'], '$!JSMETA'
    numproto = jsmeta.'new_class'('Num', 'parent'=>'Float Any')
    numproto.'!IMMUTABLE'()
    jsmeta.'register'('Float', 'parent'=>numproto, 'protoobject'=>numproto)

    # Override the proto's ACCEPT method so we also accept Ints.
    .const 'Sub' $P0 = "Num::ACCEPTS"
    $P1 = typeof numproto
    $P1.'add_method'('ACCEPTS', $P0)
.end


.sub 'Num::ACCEPTS' :anon :method
    .param pmc topic

    ##  first, try our superclass .ACCEPTS
    $P0 = get_hll_global 'Any'
    $P1 = find_method $P0, 'ACCEPTS'
    $I0 = self.$P1(topic)
    unless $I0 goto try_int
    .return ($I0)

  try_int:
    $P0 = get_hll_global 'Int'
    $I0 = $P0.'ACCEPTS'(topic)
    .return ($I0)
.end


=item ACCEPTS()

=cut

.sub 'ACCEPTS' :method
    .param num topic
    .tailcall 'infix:=='(topic, self)
.end


=item js()

Returns a JS representation of the Num.

=cut

.sub 'js' :method
    $S0 = self
    .return($S0)
.end


=item WHICH()

Returns the identify value.

=cut

.sub 'WHICH' :method
    $N0 = self
    .return ($N0)
.end


=item infix:===

Overridden for Num.

=cut

.namespace []
.sub 'infix:===' :multi(Float,Float)
    .param num a
    .param num b
    .tailcall 'infix:=='(a, b)
.end


=back

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
