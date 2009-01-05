## $Id$

=head1 TITLE

Bool - Cardinal boolean class
Swipped from RAKUDO

=head1 DESCRIPTION

This file sets up the Cardinal C<Bool> class, and initializes
symbols for C<Bool::True> and C<Bool::False>.

=cut

.namespace ['Bool']

.sub 'onload' :anon :init :load
    .local pmc cardinalmeta, boolproto
    cardinalmeta = get_hll_global ['CardinalObject'], '!CARDINALMETA'
    boolproto = cardinalmeta.'new_class'('Bool', 'parent'=>'parrot;Boolean')
    cardinalmeta.'register'('Boolean', 'parent'=>boolproto, 'protoobject'=>boolproto)

    $P0 = boolproto.'new'()
    $P0 = 0
    set_hll_global ['Bool'], 'False', $P0

    $P0 = boolproto.'new'()
    $P0 = 1
    set_hll_global ['Bool'], 'True', $P0
.end


.sub 'ACCEPTS' :method
    .param pmc topic
    .return (self)
.end


.sub 'perl' :method
    if self goto false
    .return ('Bool::False')
  false:
    .return ('Bool::True')
.end


.sub 'succ' :method :vtable('increment')
    self = 1
.end


.sub 'pred' :method :vtable('decrement')
    self = 0
.end


# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
