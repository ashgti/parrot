#!./parrot-nqp

# Checking for OpLib num and skip files parsing.

# opsc_core contains everything except nqp.pbc
Q:PIR<
    load_bytecode "compilers/opsc/opsc_core.pbc"
>;

plan(6);

my @files := <
    src/ops/core.ops
    src/ops/math.ops
>;

my $lib := Ops::OpLib.new.BUILD(
    :files(@files),
    :num_file('src/ops/ops.num'),
    :skip_file('src/ops/ops.skip'),
);

$lib.parse_ops();

# 86 core
# 116 math
ok(+($lib.ops_past) == 86 + 116, "ops file parsed");

# It's 1258 currently. But testing for exact match isn't nessary.
ok( $lib.max_op_num > 1200, "ops.num file parsed");

# Check couple random ops.
ok( $lib.optable<end> == 0, "'end' have code 0");
ok( $lib.optable<set_args_pc> == 42, "'set_args_pc' have code 42");

ok( $lib.skiptable<abs_i_ic>,       "'abs_i_ic' in skiptable");
ok( $lib.skiptable<ne_nc_nc_ic>,    "'ne_nc_nc_ic' in skiptable");

# vim: expandtab shiftwidth=4 ft=perl6:
