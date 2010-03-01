#! ./parrot-nqp

# "Comprehensive" test for creating PAST for op.
# Parse single op and check various aspects of created PAST.

pir::load_bytecode('compilers/opsc/opsc.pbc');
pir::load_bytecode('nqp-settings.pbc');
pir::load_bytecode('dumper.pbc');

plan(14);

my $buf := q|
BEGIN_OPS_PREAMBLE    
/*
THE HEADER
*/
END_OPS_PREAMBLE

op bar() {
    # Nothing here
}

inline op foo(out INT, in PMC, inconst NUM) :flow :deprecated {
    foo # We don't handle anything in C<body> during parse/past.
}

|;

my $compiler := pir::compreg__Ps('Ops');

my $past := $compiler.compile($buf, target => 'past');

ok(1, "PAST::Node created");

my $preambles := $past<preamble>;

ok($preambles[0][0] ~~ /HEADER/, 'Header parsed');

my @ops := @($past<ops>);
ok(+@ops == 2, 'We have 2 ops');

my $op := @ops[0];
ok($op.name == 'foo', "Name parsed");

my %flags := $op.flags;
ok(%flags<flow>, ':flow flag parsed');
ok(%flags<deprecated>, ':deprecated flag parsed');
ok(%flags == 2, "And there are only 2 flags");

# Check op params
my @args := $op.arguments;
ok(+@args == 3, "Got 3 parameters");

my $arg;

$arg := @args[0];
ok($arg<direction> eq 'out', 'First direction is correct');
ok($arg<type> eq 'INT', 'First type is correct');

$arg := @args[1];
ok($arg<direction> eq 'in', 'Second direction is correct');
ok($arg<type> eq 'PMC', 'Second type is correct');

$arg := @args[2];
ok($arg<direction> eq 'inconst', 'Third direction is correct');
ok($arg<type> eq 'NUM', 'Third type is correct');

# Check normalization
@args := $op<NORMARGS>;
$arg := @args[0];
ok($arg<direction> eq 'o', 'First direction is correct');
ok($arg<type> eq 'i', 'First type is correct');
ok(!($arg<variant>), 'First arg without variant');

$arg := @args[1];
ok($arg<direction> eq 'i', 'Second direction is correct');
ok($arg<type> eq 'p', 'Second type is correct');
ok($arg<variant> eq 'pc', 'Second variant is correct');

$arg := @args[2];
ok($arg<direction> eq 'i', 'Third direction is correct');
ok($arg<type> eq 'nc', 'Third type is correct');
ok(!($arg<variant>), 'Third arg without variant');

my @expanded := Ops::Compiler::Actions::expand_args(@args);

#_dumper(@expanded);
ok( @expanded[0].join('_') eq 'i_p_nc', "First variant correct");
ok( @expanded[1].join('_') eq 'i_pc_nc', "Second variant correct");

# Don't forget to update plan!

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=perl6:
