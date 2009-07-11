# $Id$

=head1 DESCRIPTION

Not everything possible to implement in NQP.

=cut

.namespace ['Ops';'OpLib']

# Cheat-cheat! 
.sub '' :anon :load :init
    ##   create the classes
    .local pmc p6meta
    p6meta = new 'P6metaclass'

    p6meta.'new_class'('Ops::OpLib', 'parent'=>'Hash')

    .return ()
.end

=head1 COPYRIGHT

Copyright (C) 2009, Parrot Foundation.

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
