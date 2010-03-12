#! ./parrot-nqp

pir::load_bytecode("compilers/opsc/opsc.pbc");

my @files := <
    src/ops/core.ops
    src/ops/bit.ops
    src/ops/cmp.ops
    src/ops/debug.ops
    src/ops/io.ops
    src/ops/math.ops
    src/ops/object.ops
    src/ops/pmc.ops
    src/ops/set.ops
    src/ops/string.ops
    src/ops/sys.ops
    src/ops/var.ops
    src/ops/experimental.ops
>;


my $trans := Ops::Trans::C.new();
my $lib := Ops::OpLib.new(
    :num_file('src/ops/ops.num'),
    :skip_file('src/ops/ops.skip'),
);

my $start_time := pir::time__N();
my $f := Ops::File.new(|@files, :oplib($lib));
my $end_time := pir::time__N();
say('# Ops parsed ' ~ ($end_time - $start_time));
my $emitter := Ops::Emitter.new(
    :ops_file($f), :trans($trans), :script('ops2c'),
    :flags(
        hash( core => 1 )
    ),
);

$emitter.print_c_header_file();
$emitter.print_c_source_file();


# vim: expandtab shiftwidth=4 ft=perl6:
