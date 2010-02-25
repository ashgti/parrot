# Copyright (C) 2009, Parrot Foundation.
# $Id$

.namespace [ 'Ops';'Compiler' ]

.loadlib 'PCT.pbc'

.sub 'main' :main
    .param pmc args
    $P0 = compreg 'Ops'
    $P1 = $P0.'command_line'(args, 'encoding'=>'utf8', 'transcode'=>'ascii')
.end

.include 'compilers/opsc/opsc_core.pir'

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
