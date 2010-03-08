#!./parrot-nqp

# Checking for OpLib num and skip files parsing.

pir::load_bytecode("compilers/opsc/opsc.pbc");

plan(6);

my @files := ('src/ops/core.ops', 'src/ops/math.ops');

my $lib := Ops::OpLib.new(
    :files(@files),
    :num_file('src/ops/ops.num'),
    :skip_file('src/ops/ops.skip'),
);

$lib.parse_ops();

# 84 core
# 116 math
# We generate all variants during compilation. So check for ">"
ok(+($lib.ops_past) > 84 + 116, "ops file parsed");
say("# count " ~+$lib.ops_past);
#_dumper($lib.ops_past);

# It's 1258 currently. But testing for exact match isn't nessary.
ok( $lib.max_op_num > 1200, "ops.num file parsed");

# Check couple random ops.
ok( $lib.optable<end> == 0, "'end' have code 0");
ok( $lib.optable<set_addr_p_i> != 42, "'set_addr_p_i' have non 0 code");

ok( $lib.skiptable<abs_i_ic>,       "'abs_i_ic' in skiptable");
ok( $lib.skiptable<ne_nc_nc_ic>,    "'ne_nc_nc_ic' in skiptable");
#_dumper($lib.skiptable);

# vim: expandtab shiftwidth=4 ft=perl6:
