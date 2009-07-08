# $Id$

=head1 NAME

PAST - Parrot abstract syntax tree for PMC.

=head1 DESCRIPTION

PAST node for a single Op

=cut

.sub '' :anon :load :init
    ##   create the classes
    .local pmc p6meta
    p6meta = new 'P6metaclass'

    p6meta.'new_class'('Ops::Op', 'parent'=>'PAST::Op')

    .return ()
.end

=head1 NODES

=head2 C<PMC::Class>

PMC class by it self.

=cut

.namespace [ 'Ops';'Op' ]

.sub 'new' :method
    .param pmc children        :slurpy
    .param pmc adverbs         :slurpy :named

    .local pmc res
    $P0 = self.'HOW'()
    $P0 = getattribute $P0, 'parrotclass'
    res = new $P0
    res.'init'(children :flat, adverbs :flat :named)

    # Initialize various attributes
    #$P1 = new ['Hash']
    #res.'attr'('op_flags', $P1, 1)

    .return (res)
.end

=item C<does>

How does this PMC act?.

=cut

#.sub 'op_flags' :method
#    .tailcall self.'attr'('op_flags',0,0)
#.end


=head1 COPYRIGHT

Copyright (C) 2009, Parrot Foundation.

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
