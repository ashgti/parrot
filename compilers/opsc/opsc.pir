# Copyright (C) 2009, Parrot Foundation.
# $Id$

.namespace [ 'Ops';'Compiler' ]

.sub 'onload' :load :init
    # opsc_core.pir don't load NQP to avoid clash in nqp-based tests.
    load_bytecode 'PCT.pbc'
    load_bytecode 'compilers/nqp/nqp.pbc'
.end

.sub 'main' :main
    .param pmc args
    $P0 = compreg 'Ops'
    .tailcall $P0.'command_line'(args, 'encoding'=>'utf8', 'transcode'=>'ascii')
.end

.include 'compilers/opsc/opsc_core.pir'

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
