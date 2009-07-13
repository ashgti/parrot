#!../../parrot ../nqp/nqp.pbc

PIR q<
    load_bytecode "opsc_core.pbc"
>;


my @files := qw{
    src/ops/core.ops
    src/ops/math.ops
};

my $lib := Ops::OpLib.new.BUILD(:files(@files));

$lib.parse_ops();

# Not a real number. But I'll adjust it soon.
ok(+($lib.ops) == 42, "ops file parsed");

# vim: expandtab shiftwidth=4 ft=perl6:
