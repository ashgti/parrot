#!/usr/bin/env parrot-nqp

INIT {
    pir::load_bytecode('ProfTest.pbc');
}


#basic tests


my $pir_code := 
'.sub main
  say "what"
.end';

my $prof := ProfTest::PIRProfile.new($pir_code);


ok(1, "profile creation didn't explode");

#Does the profile have a version string?
my $matcher := ProfTest::Matcher.new(
    version()
);

ok( $matcher.matches($prof), "profile has a version number");

#Does the profile have a CLI invocation?
$matcher := ProfTest::Matcher.new(
    cli()
); 

ok( $matcher.matches($prof), "profile contains a CLI string");

#Does the profile have a 'say' op somewhere?
$matcher := ProfTest::Matcher.new(
    op('say')
);

ok( $matcher.matches($prof), "profile has a say op");

#Does the profile have expected timing values?
$matcher := ProfTest::Matcher.new(
    op('say', :time(1))
);

ok( $matcher.matches($prof), "profile has canonical timing information");

#Does the matcher fail to find the non-existent 'lollercoaster' opcode?
$matcher := ProfTest::Matcher.new(
    op('lollercoaster')
);

ok( !$matcher.matches($prof), "matcher didn't find non-existent opcode");

#Does the profile show a 'say' op inside the 'main' sub?
$matcher := ProfTest::Matcher.new(
    cs(:ns('main')),
    any(:except('cs')),
    op('say'),
);
 
ok( $matcher.matches($prof), "profile shows 'say' inside main sub");


$pir_code :=
".sub first :main
  .local int i
  i = 0
  'second'()
  inc i
.end

.sub second
  .local pmc p
  p = new ['Interger']
  'third'()
  p = 1
.end

.sub third
  say 'in third'
.end";

$prof := ProfTest::PIRProfile.new($pir_code);

$matcher := ProfTest::Matcher.new(
    cs(:ns('first'),  :slurp_until('cs')),
    cs(:ns('second'), :slurp_until('cs')),
    cs(:ns('third'),  :slurp_until('cs')),
    cs(:ns('second'), :slurp_until('cs')),
    cs(:ns('first')),
);

ok( $matcher.matches($prof), "profile properly reflects normal control flow");


#test: main calls foo, foo tailcalls bar, bar returns to main
$pir_code :=
".sub first :main
  .local int i
  i = 'foo'(9)
.end

.sub foo
  .param int i
  i = i * i
  .tailcall bar(i)
.end

.sub bar
  .param int i
  i = i + 2
  .return (i)
.end";

$prof := ProfTest::PIRProfile.new($pir_code);

$matcher := ProfTest::Matcher.new(
    cs(:ns('first'), :slurp_until('cs')),
    cs(:ns('foo'),   :slurp_until('cs')),
    cs(:ns('bar'),   :slurp_until('cs')),
    cs(:ns('first')),
);

ok( $matcher.matches($prof), "profile properly reflects tailcall control flow");


#Does the profile show a 'say' op on line 2?
$matcher := ProfTest::Matcher.new(
    op('say', :line('2')),
);

ok( $matcher.matches($prof), "profile shows say on the correct line");


my $nqp_code := '
main();
sub main() {
    pir:say("nqp");
}';

$prof := ProfTest::NQPProfile.new($nqp_code, :annotations(1));

$matcher := ProfTest::Matcher.new(
    cs(:ns('parrot;main') ),
    any(:except('cs')), 
    op('say'),
);

ok( $matcher.matches($prof), "profile shows 'say' inside nqp sub");

#convenience subs to avoid repetitive typing and visual noise

sub version(*@p, *%n) { ProfTest::Want::Version.new(|@p, |%n) }
sub cli(*@p, *%n)     { ProfTest::Want::CLI.new(|@p, |%n) }
sub eor(*@p, *%n)     { ProfTest::Want::EndOfRunloop.new(|@p, |%n) }
sub op(*@p, *%n)      { ProfTest::Want::Op.new(|@p, |%n) }
sub cs(*@p, *%n)      { Proftest::Want::CS.new(|@p, |%n) }
sub any(*@p, *%n)     { ProfTest::Want::Any.new(|@p, |%n) }

