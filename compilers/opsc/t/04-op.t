#!./parrot-nqp

# Checking Ops::Op

pir::load_bytecode("compilers/opsc/opsc.pbc");
pir::load_bytecode("nqp-settings.pbc");

plan(7);

my $ok := 1;
try {
    Ops::Op.new(code => "BANG");
    $ok := 0;
}

ok( $ok, "Can't create Op without mandatory arguments");

my $op := Ops::Op.new(
    code => 42,
    name => 'set',
    type => 'inline',
    args => <foo bar baz>,
    flags => hash()
);

ok( 1, "Op created");

ok( $op.code == 42,         "... with proper code");
ok( $op.name == 'set',      "... with proper name");
ok( $op.type == 'inline',   "... with proper type");
ok( $op.body == '',         "... with empty body");
$op.body("SOME BODY");
ok( $op.body == 'SOME BODY', "Op.body set");

# vim: expandtab shiftwidth=4 ft=perl6:
