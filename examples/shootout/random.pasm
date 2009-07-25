#!./parrot -R jit
# Copyright (C) 2005-2009, Parrot Foundation.
# $Id$
#
# random.pasm N         (N = 900000 for shootout)
# by Joshua Isom

# I0 = last
# I1 is the counter for the loop
# N2 is the argument for gen_random
# N3 is the return from gen_random
main:
    new P10, 'ResizableIntegerArray'
    get_params "0", P0
    elements I0, P0
    eq I0, 2, hasargs
    set I1, 900000
    branch argsdone
hasargs:
    set S0, P0[1]
    set I1, S0
argsdone:
    set I0, 42
    unless I1, ex
    set N2, 100.0
while_1:
    local_branch P10, gen_random
    dec I1
    if I1, while_1
    new P0, 'FixedFloatArray'
    set P0, 1
    set P0[0], N3
    sprintf S0, "%.9f\n", P0
    print S0
ex:
    end

.macro_const IM 139968
.macro_const IA 3877
.macro_const IC 29573

gen_random:
    mul I0, .IA
    add I0, .IC
    mod I0, .IM
    set N1, I0
    mul N3, N2, N1
    div N3, .IM
    local_return P10

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
