# Copyright (C) 2009, Parrot Foundation.
# $Id$

.namespace [ 'Ops';'Compiler' ]

.loadlib 'PCT.pbc'

.include 'compilers/opsc/builtins.pir'
.include 'compilers/opsc/compiler/gen_grammar.pir'
.include 'compilers/opsc/compiler/gen_actions.pir'
.include 'compilers/opsc/compiler/gen_compiler.pir'
.include 'compilers/opsc/ops/gen_oplib.pir'

.include 'compilers/opsc/runcore/gen_base.pir'
.include 'compilers/opsc/runcore/gen_c.pir'
.include 'compilers/opsc/runcore/gen_cgoto.pir'
.include 'compilers/opsc/runcore/gen_cprederef.pir'
.include 'compilers/opsc/runcore/gen_cgp.pir'
.include 'compilers/opsc/runcore/gen_cswitch.pir'

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
