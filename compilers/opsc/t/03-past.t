#! ./parrot-nqp

# "Comprehensive" test for creating PAST for op.
# Parse single op and check various aspects of created PAST.

pir::load_bytecode('compilers/opsc/opsc.pbc');
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

#    $P1 = $P0[0]
#    $S0 = $P1[0]
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
ok($arg<direction> == 'out', 'First direction is correct');
ok($arg<type> == 'INT', 'First type is correct');

$arg := @args[1];
ok($arg<direction> == 'in', 'Second direction is correct');
ok($arg<type> == 'PMC', 'Second type is correct');

ok($arg<direction> == 'inconst', 'Third direction is correct');
ok($arg<type> == 'NUM', 'Third type is correct');

# Don't forget to update plan!

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=perl6:
