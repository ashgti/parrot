#!../../parrot ../nqp/nqp.pbc

PIR q<
    load_bytecode "opsc_core.pbc"
>;


my @files := qw{
    ../../src/ops/core.ops
    ../../src/ops/math.ops
};

my $lib := Ops::OpLib.new.BUILD(:files(@files));

$lib.parse_ops();

# 86 core
# 116 math
ok(+($lib.ops) == 86 + 116, "ops file parsed");

# vim: expandtab shiftwidth=4 ft=perl6:
