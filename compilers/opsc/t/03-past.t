#! ../../parrot

# "Comprehensive" test for creating PAST for op.
# Parse single op and check various aspects of created PAST.

.sub 'main'
    .include 'test_more.pir'
    load_bytecode 'opsc.pbc'

    plan(1)

    .local pmc compiler, past
    .local string buf
    buf = <<"END"
VERSION = PARROT_VERSION;
inline op foo(out INT, in PMC, inconst NUM) :flow :deprecated {
    foo # We don't handle anything in C<body> during parse/past.
}
END

    compiler = compreg 'Ops'
    past = compiler.'parse'(buf, 'target'=>'past')
    ok(1, "PAST::Node created")

.end

# Don't forget to update plan!

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
