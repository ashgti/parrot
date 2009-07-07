#! ../../parrot

# "Comprehensive" test for creating PAST for op.
# Parse single op and check various aspects of created PAST.

.sub 'main'
    .include 'test_more.pir'
    load_bytecode 'opsc.pbc'

    plan(4)

    .local pmc compiler, past
    .local string buf
    buf = <<"END"
/*
THE HEADER
*/

VERSION = PARROT_VERSION;

inline op foo(out INT, in PMC, inconst NUM) :flow :deprecated {
    foo # We don't handle anything in C<body> during parse/past.
}
END

    compiler = compreg 'Ops'
    past = compiler.'compile'(buf, 'target'=>'past')
    ok(1, "PAST::Node created")

    $P0 = past['header']
    $S0 = $P0.'inline'()
    like($S0, 'HEADER', 'Header parsed')

    $P0 = past['ops']
    $P0 = $P0.'list'()
    $I0 = $P0
    is($I0, 1, 'We have 1 op')

    # Check op
    .local pmc op
    op = $P0[0]
    $S0 = op['name']
    is($S0, 'foo', "Name parsed")

.end

# Don't forget to update plan!

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
