#!/usr/bin/env parrot-nqp

INIT {
    pir::load_bytecode('ProfTest/ProfTest.pbc');
}

# XXX: Don't bother tryting to run these tests.  None of the supporting code
# exists.  This code is only here to help me figure out the final interface.

my $pir_code := 
'.sub main
  say "what"
.end';

my $prof := ProfTest::PirProfile.new($pir);

#Does the profile have a version string?
my $matcher := ProfTest::Matcher.new();
$matcher.push( ProfTest::Want::Version() ): #use count=1 by default


ok( $matcher.matches($prof), "profile has a version number");

#Does the profile have a CLI invocation?
$matcher := ProfTest::Matcher.new();
$matcher.push( ProfTest::Want::CLI() );

ok( $matcher.matches($prof), "profile contains a CLI string");


#Does the profile have a 'say' op somewhere?
$matcher := ProfTest::Matcher.new();
$matcher.push( ProfTest::Want::Op( 'say' ));

ok( $matcher.matches($prof), "profile has a say op");


#Does the profile show a 'say' op inside the 'main' sub?
$matcher := ProfTest::Matcher.new(
    ProfTest::Want::CS.new( :ns('main')),
    ProfTest::Want.new( :count('*'), :type_isnt('CS')),
    ProfTest::Want::Op.new( :op('say')),
);
 
ok( $matcher.matches($prof), "profile shows 'say' inside main sub");


#Does the profile show a 'say' op on line 2?
$match := ProfTest::Matcher.new();
$matcher.push (ProfTest::Want::Op.new( :count(1), :op('say'), :line('2')));

ok( $matcher.matches($prof), "profile shows say on the correct line");



#test: main calls foo, foo tailcalls bar, bar returns to main

my $nqp_code := '
main();
sub main() {
    pir:say("nqp");
}';

$prof := ProfTest::NQPProfile.new($nqp_code);

$matcher := ProfTest::Matcher.new();
$matcher.push( ProfTest::Want::CS.new( :ns('*main') ) ); #matches parrot::foo::main
$matcher.push( ProfTest::Want.new(    :count('*'), :type_isnt('CS') ) );
$matcher.push( ProfTest::Want::Op.new( :op('say') ) );

ok( $matcher.matches($prof), "profile shows 'say' inside nqp sub");

