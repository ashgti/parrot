#!./parrot-nqp

pir::load_bytecode("compilers/opsc/opsc.pbc");
pir::load_bytecode("nqp-settings.pbc");

plan(2);

my $f := Ops::File.new();
ok($f, "Empty Ops::File created");

my @files := <
    src/ops/core.ops
    src/ops/math.ops
>;

$f := Ops::File.new(|@files);

# 84 core
# 116 math
# We can generate more than 1 Ops::Op per op due args expansion.
say( "# Parsed " ~ +$f<parsed_ops>);
ok( $f<parsed_ops> > 84 + 116, "Ops parsed correctly");


# vim: expandtab shiftwidth=4 ft=perl6:
