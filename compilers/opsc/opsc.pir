# Copyright (C) 2009, Parrot Foundation.
# $Id$

.namespace [ 'Ops';'Compiler' ]

.sub 'onload' :load :init
    load_bytecode 'PCT.pbc'
    load_bytecode 'compilers/nqp/nqp.pbc'

    .local pmc p6meta
    p6meta = new 'P6metaclass'

    p6meta.'new_class'('Ops::Compiler', 'parent'=>'PCT::HLLCompiler')

    # Register grammar and action
    $P0 = new [ 'Ops';'Compiler' ]
    $P0.'language'('Ops')
    $P0.'parsegrammar'('Ops::Grammar')
    $P0.'parseactions'('Ops::Grammar::Actions')

    #these stages aren't currently used, although generate_files exits before
    #they can be called anyway
    $P0.'removestage'('post')
    $P0.'removestage'('pir')
    $P0.'removestage'('evalpmc')
    $P0.'addstage'('exit', 'after'=>'past')

.end

.sub 'exit' :method
    .param pmc past
    .param pmc adverbs :slurpy :named

    exit 0
.end


.sub 'main' :main
    .param pmc args
    $P0 = compreg 'Ops'
    .tailcall $P0.'command_line'(args, 'encoding'=>'utf8', 'transcode'=>'ascii')
.end


.include 'builtins.pir'
.include 'compiler/gen_grammar.pir'
.include 'compiler/gen_actions.pir'
.include 'ops/op.pir'
.include 'ops/gen_op.pir'
.include 'ops/oplib.pir'
.include 'ops/gen_oplib.pir'

.include 'runcore/gen_base.pir'
.include 'runcore/gen_c.pir'
.include 'runcore/gen_cgoto.pir'
.include 'runcore/gen_cprederef.pir'
.include 'runcore/gen_cgp.pir'
.include 'runcore/gen_cswitch.pir'

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
