#! ./parrot

.include 'compilers/opsc/t/common.pir'

.sub 'main' :main

    .include 'test_more.pir'
    load_bytecode 'compilers/opsc/opsc.pbc'

    plan(2)

    test_build()
.end

# Check that Ops::OpLib::BUILD dtrt
.sub 'test_build'

    .local pmc ctor

    # It should fail without files.
    push_eh fail
    $I0 = 1
    $P0 = new ['Ops';'OpLib']
    $P1 = $P0.'BUILD'()
    $I0 = 0
  fail:
    pop_eh
    ok($I0, "BUILD without args failed")

    $P1 = split ' ', 'src/ops/core.ops src/ops/math.ops'
    $P1 = $P0.'BUILD'('files'=>$P1)
    ok(1, "BUILD with @files passed")

.end


# Don't forget to update plan!

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
