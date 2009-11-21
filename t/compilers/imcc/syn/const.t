#!perl
# Copyright (C) 2001-2008, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use vars qw($TODO);

use Test::More;
use Parrot::Config;
use Parrot::Test tests => 34;

pir_output_is( <<'CODE', <<'OUT', "globalconst 1" );

.sub 'main' :main
    .globalconst int N = 5
    _main()
.end

.sub '_sub1'
    print N
    print "\n"
.end

.sub '_main'
    _sub1()
.end
CODE
5
OUT

pir_output_is( <<'CODE', <<'OUT', "globalconst 2" );
.sub 'test' :main
    .globalconst int N = 5
    _main()
.end

.sub '_sub1'
    .local int x
    x = 10 + N
    print x
    print "\n"
.end

.sub '_main'
     _sub1()
.end
CODE
15
OUT

pir_output_is( <<'CODE', <<'OUT', "globalconst 3" );

.sub 'call_sub1'
    'sub1'()
.end

.sub 'test' :main
    .globalconst int N = 5
    'call_sub1'()
.end

.sub 'sub1'
    print N
    print "\n"
.end

CODE
5
OUT

pir_output_is( <<'CODE', <<'OUT', "array/hash consts" );
.sub 'main' :main
   .local pmc ar
   .local pmc ha
   .local string key1
   .const string key2 = "key2"
   .local int idx1
   .const int idx2 = 2
   ar = new 'Array'
   ar = 3
   ha = new 'Hash'
   key1 = "key1"
   idx1 = 1
   ha[key1] = idx1
   ha[key2] = idx2
   $I0 = ha[key1]
   $I1 = ha[key2]
   ar[idx1] = $I0
   ar[idx2] = $I1
   $I2 = ar[idx1]
   $I3 = ar[idx2]
   print $I2
   print $I3
   print "\n"
.end
CODE
12
OUT

pir_output_is( <<'CODE', <<'OUT', "escaped" );
.sub 'main' :main
   $S0 = "\""
   print $S0
   print "\\"
   $S0 = "\"\\\"\n"
   print $S0
.end
CODE
"\"\"
OUT

# fix editor highlighting "

pir_output_is( <<'CODE', <<'OUT', "PMC const 1 - Sub" );
.sub 'main' :main
    .const 'Sub' $P0 = "foo"
    print "ok 1\n"
    $P0()
    print "ok 3\n"
.end
.sub foo
    print "ok 2\n"
.end
CODE
ok 1
ok 2
ok 3
OUT

pir_output_is( <<'CODE', <<'OUT', "PMC const 2 - Sub ident" );
.sub 'main' :main
    .const 'Sub' func = "foo"
    print "ok 1\n"
    func()
    print "ok 3\n"
.end
.sub foo
    print "ok 2\n"
.end
CODE
ok 1
ok 2
ok 3
OUT

pasm_output_is( <<'CODE', <<'OUT', "const I/N mismatch" );
    set I0, 2.0
    print I0
    print "\n"
    set N0, 2
    print N0
    print "\nok\n"
    end
CODE
2
2
ok
OUT

pir_output_is( <<'CODE', <<'OUT', "const I/N mismatch 2" );
.sub 'main' :main
    .const int i = 2.0
    print i
    print "\n"
    .const num n = 2
    print n
    print "\nok\n"
    .const string s = ascii:"ok 2\n"
    print s
.end
CODE
2
2
ok
ok 2
OUT

pir_output_is( <<'CODE', <<'OUT', 'PIR heredocs: accepts double quoted terminator' );
.sub 'main' :main
    $S0 = <<"quotage"
I want an elephant
Oh, I want an elephat!
Oh, woo, elephants, yeah :-O
quotage

    print $S0
.end
CODE
I want an elephant
Oh, I want an elephat!
Oh, woo, elephants, yeah :-O
OUT

pir_output_is( <<'CODE', <<'OUT', 'PIR heredocs: accepts inline with concat' );
.sub 'main' :main
    $S0 = ""
    $I0 = 0
LOOP:
    $S0 = concat <<"end"
ending
end
    inc $I0
    if $I0 < 5 goto LOOP
    print $S0
.end
CODE
ending
ending
ending
ending
ending
OUT

pir_output_is( <<'CODE', <<'OUT', "PIR heredoc: accepts terminator with any word chars" );
.sub 'main' :main
    $S0 = <<"AnY_w0Rd_ch4rS"
so much depends
upon

a red wheel
barrow

glazed with rain
water

beside the white
chickens
AnY_w0Rd_ch4rS

    print $S0
.end
CODE
so much depends
upon

a red wheel
barrow

glazed with rain
water

beside the white
chickens
OUT

pir_output_is( <<'CODE', <<'OUT', 'PIR heredoc: single quoted terminator' );
.sub 'main' :main
    $S0 = <<'Jabberwocky'
`Twas brillig, and the slithy toves
  Did gyre and gimble in the wabe;
All mimsy were the borogoves,
  And the mome raths outgrabe.
Jabberwocky
    print $S0
.end
CODE
`Twas brillig, and the slithy toves
  Did gyre and gimble in the wabe;
All mimsy were the borogoves,
  And the mome raths outgrabe.
OUT

pir_output_is( <<'CODE', <<'OUT', 'PIR heredoc: single quoted - backslash' );
.sub 'main' :main
    $S0 = <<'SQ'
abc\tdef
SQ
    print $S0
.end
CODE
abc\tdef
OUT

pir_error_output_like( <<'CODE', <<'OUT', 'PIR heredoc: rejects unquoted terminator' );
.sub 'main' :main
    $S0 = <<Jabberwocky
"Beware the Jabberwock, my son!
  The jaws that bite, the claws that catch!
Beware the Jubjub bird, and shun
  The frumious Bandersnatch!"
Jabberwocky
    print $S0
.end
CODE
/^error:imcc:syntax error, unexpected SHIFT_LEFT.*/
OUT

pir_error_output_like( <<'CODE', <<'OUT', "PIR heredoc: rejects inline heredoc" );
.sub 'main' :main
    $S0 .= <<Jabberwocky
He took his vorpal sword in hand:
  Long time the manxome foe he sought --
So rested he by the Tumtum tree,
  And stood awhile in thought.
Jabberwocky

    print $S0
.end
CODE
/^error:imcc:syntax error, unexpected SHIFT_LEFT.*/
OUT

pir_error_output_like( <<'CODE', <<'OUT', "PIR heredoc: rejects null terminator" );
.sub 'main' :main
    $S0 = <<
And, as in uffish thought he stood,
  The Jabberwock, with eyes of flame,
Came whiffling through the tulgey wood,
  And burbled as it came!

    print $S0
.end
CODE
/^error:imcc:syntax error, unexpected SHIFT_LEFT.*/
OUT

pir_error_output_like( <<'CODE', <<'OUT', "PIR heredoc: rejects terminator with spaces" );
.sub 'main' :main
    $S0 = << "terminator with spaces"
One, two! One, two! And through and through
  The vorpal blade went snicker-snack!
He left it dead, and with its head
  He went galumphing back.
terminator with spaces

    print $S0
.end
CODE
/^error:imcc:syntax error, unexpected SHIFT_LEFT.*/
OUT

pir_output_is( <<'CODE', <<'OUT', "PIR heredoc: accepts terminator with non-word chars" );
.sub 'main' :main
    $S0 = <<"#non$word-chars."
'And, has thou slain the Jabberwock?
  Come to my arms, my beamish boy!
O frabjous day! Callooh! Callay!'
  He chortled in his joy.
#non$word-chars.

    print $S0
.end
CODE
'And, has thou slain the Jabberwock?
  Come to my arms, my beamish boy!
O frabjous day! Callooh! Callay!'
  He chortled in his joy.
OUT

pir_output_is( <<'CODE', <<'OUT', "PIR heredoc: accepts terminator with unprintable chars" );
.sub 'main' :main
    $S0 = <<"\0\1\2\3"
`Twas brillig, and the slithy toves
  Did gyre and gimble in the wabe;
All mimsy were the borogoves,
  And the mome raths outgrabe.
\0\1\2\3
    print $S0
.end
CODE
`Twas brillig, and the slithy toves
  Did gyre and gimble in the wabe;
All mimsy were the borogoves,
  And the mome raths outgrabe.
OUT

pir_error_output_like( <<'CODE', <<'OUT', "PIR heredoc: rejects interpolated terminator" );
.sub 'main' :main
    $S1 = 'e_e_cummings'
    $S0 = <<$S1
l(a

le
af

fa
ll
s)

one
l
iness
e_e_cummings
    print $S0
.end
CODE
/^error:imcc:syntax error, unexpected SHIFT_LEFT.*/
OUT

pir_output_is( <<'CODE', <<'OUT', "PIR heredoc: rejects variable interpolation" );
.sub 'main' :main
    $S0 = 'parrot'
    print <<"*<:-O"
Happy Birthday to you,
Happy Birthday to you.
Happy Birthday dear $S0,
Happy Birthday to you!
*<:-O
.end
CODE
Happy Birthday to you,
Happy Birthday to you.
Happy Birthday dear $S0,
Happy Birthday to you!
OUT

pir_output_is( <<'CODE', <<'OUT', "PIR heredoc: allow empty lines" );
.sub 'main' :main
    $S0 = 'parrot'
    print <<"END_HERE"

The line above is empty.
END_HERE
.end
CODE

The line above is empty.
OUT

pir_error_output_like( <<'CODE', <<'OUT', "PIR heredoc: line numbers" );
.sub main :main
    .local string s
    .local pmc nil
    bounds 1   # force line nums
    s = <<"EOT"
line 1
line 2
line 3
line 4
EOT
    print nil  # force err
.end
CODE
/^Null PMC.*:11\)$/s
OUT

pir_output_is( <<'CODE', <<'OUT', "PIR heredoc: double quoted strings" );
.sub main :main
  $S0 = <<"HEREDOC"
print "hello"
HEREDOC
  print $S0
  end
.end
CODE
print "hello"
OUT

pir_output_is( <<'CODE', <<'OUT', "PIR heredoc: double quotes - two in a row" );
.sub main :main
    print <<"QUOTES"
""
QUOTES
.end
CODE
""
OUT

pir_output_is( <<'CODE', <<'OUT', "PIR heredoc: double quotes - with anything between" );
.sub main :main
    print <<"QUOTES"
"anything"
QUOTES
.end
CODE
"anything"
OUT

pir_output_is(
    <<'CODE', <<'OUT', "PIR heredoc: double quotes - two in a row prefaced by anything" );
.sub main :main
    print <<"QUOTES"
anything""
QUOTES
.end
CODE
anything""
OUT

pir_output_is( <<'CODE', <<'OUT', "PIR heredoc: double quotes - escaped with anything between" );
.sub main :main
    print <<"QUOTES"
\"anything\"
QUOTES
.end
CODE
"anything"
OUT

pir_output_is(
    <<'CODE', <<'OUT', "PIR heredoc: escaped characters, escaped quotes, starting quotes" );
.sub test :main
	.local string test

	test = <<"TEST"
{ \{ \\{
w \w \\w
" \" \\"
{ \{ \\{
w \w \\w
" \" \\"
{ \{ \\{
w \w \\w
TEST
	print test
.end
CODE
{ { \{
w w \w
" " \"
{ { \{
w w \w
" " \"
{ { \{
w w \w
OUT

pir_output_is( <<'CODE', <<'OUT', "heredoc not eol 1" );
.sub main :main
    .local string code
    code = ''
    emit(code, <<"HERE", 10)
line 1
line %d
line 2
HERE
.end
.sub emit
    .param string code
    .param string more
    .param pmc args  :slurpy
    $S0 = sprintf more, args
    code .= $S0
    print code
.end
CODE
line 1
line 10
line 2
OUT

pir_error_output_like( <<'CODE', <<'OUT', "heredoc not eol 2 - nested" );
.sub main :main
    cat(<<"H1", <<"H2")
line 1
line 2
H1
line 3
line 4
H2
.end
.sub cat
    .param string p1
    .param string p2
    p1 .= p2
    print p1
.end
CODE
/nested heredoc not supported/
OUT

pir_output_is( <<'CODE', <<'OUT', ".const in mixed opcodes" );
.sub main :main
    .const int I = 5
    .local num f
    f = 2.0
    f *= I
    print f
    print "\n"
.end
CODE
10
OUT

pir_output_is( <<'CODE', <<'OUT', "const int" );
.const int c = 12
.sub test
    .local num a
    a = 96
    # Uncomment this line, and the c symbol is 'forgotten'
    a += c
    print a
    print "\n"
    print c
    print "\n"
    end
.end
CODE
108
12
OUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
