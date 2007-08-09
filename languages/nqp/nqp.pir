# Copyright (C) 2007, The Perl Foundation.
# $Id$

.namespace [ 'NQP::Compiler' ]

.sub '__onload' :load :init
    load_bytecode 'PGE.pbc'
    load_bytecode 'PCT.pbc'

    $P0 = new [ 'PCT::HLLCompiler' ]
    $P0.'language'('NQP')
    $P0.'parsegrammar'('NQP::Grammar')
    $P0.'parseactions'('NQP::Grammar::Actions')

    $P0 = new .Integer
    set_hll_global ['NQP::Grammar'], '$!endstmt', $P0
.end

.sub 'main' :main
    .param pmc args
    $P0 = compreg 'NQP'
    .return $P0.'command_line'(args, 'encoding'=>'utf8')
.end

.include 'src/Grammar_gen.pir'

.include 'src/Grammar/Actions.pir'

.include 'src/builtins.pir'

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
