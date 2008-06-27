## $Id:$

=head1 TITLE

Code - Perl 6 Routine class

=head1 DESCRIPTION

This file sets up the Perl 6 C<Routine> class, the base class for all
wrappable executable objects.

=cut

.namespace ['Routine']

.sub 'onload' :anon :load :init
    .local pmc p6meta, routineproto
    p6meta = get_hll_global ['Perl6Object'], '$!P6META'
    routineproto = p6meta.'new_class'('Routine', 'parent'=>'Block')
    p6meta.'register'('Perl6Routine', 'parent'=>routineproto, 'protoobject'=>routineproto)
.end

=over 4

=back

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
