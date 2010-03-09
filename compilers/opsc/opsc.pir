#! parrot
# Copyright (C) 2009-2010, Parrot Foundation.
# $Id$

.namespace [ 'Ops';'Compiler' ]

.loadlib 'P6object.pbc'
.loadlib 'HLL.pbc'
.loadlib 'PCT.pbc'
.loadlib 'nqp-settings.pbc'

.include 'compilers/opsc/src/builtins.pir'
.include 'compilers/opsc/gen/Ops/Compiler/Grammar.pir'
.include 'compilers/opsc/gen/Ops/Compiler/Actions.pir'
.include 'compilers/opsc/gen/Ops/Compiler.pir'

.include 'compilers/opsc/gen/Ops/Emitter.pir'
.include 'compilers/opsc/gen/Ops/Trans.pir'
.include 'compilers/opsc/gen/Ops/Trans/C.pir'

.include 'compilers/opsc/gen/Ops/Op.pir'
.include 'compilers/opsc/gen/Ops/OpLib.pir'
.include 'compilers/opsc/gen/Ops/File.pir'

.sub 'main' :main
    .param pmc args
    $P0 = compreg 'Ops'
    $P1 = $P0.'command_line'(args, 'encoding'=>'utf8', 'transcode'=>'ascii')
.end

.sub 'onload' :load :init
    # Register grammar and action
    $P0 = new [ 'Ops';'Compiler' ]

    #these stages aren't currently used, although generate_files exits before
    #they can be called anyway
    $P0.'removestage'('post')
    $P0.'removestage'('pir')
    $P0.'removestage'('evalpmc')
    $P0.'addstage'('generate_runcores', 'after'=>'past')
.end

.sub 'generate_runcores' :method
    .param pmc past
    .param pmc adverbs :slurpy :named

    .local pmc oplib, files

    $P0 = find_caller_lex "$?FILES"
    $S0 = $P0
    files = split " ", $S0
    #do oplib stuff
    # - make an oplib
    $P0 = get_hll_global ['Ops'], 'OpLib'
    oplib = $P0.'new'()
    #use default location for ops.num and ops.skip for now
    oplib.'BUILD'(files :named("files"))
    oplib.'set_ops_past'(past)
    oplib.'build_ops'()

    #for each runcore
    # * make a clone of the op tree
    # * call the runcore with it

    exit 0
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
