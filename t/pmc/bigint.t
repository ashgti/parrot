#! perl
# Copyright (C) 2001-2007, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );

use Test::More;
use Parrot::Test;
use Parrot::Config;

=head1 NAME

t/pmc/bigint.t - BigInt PMC

=head1 SYNOPSIS

    % prove t/pmc/bigint.t

=head1 DESCRIPTION

Tests the BigInt PMC.

=cut

if ( $PConfig{gmp} ) {
    plan tests => 44;
}
else {
    plan skip_all => "No BigInt Lib configured";
}

my $vers_check = <<'EOP';
.sub main :main
    .local pmc b, ar
    .local string v
    .local int ma, mi, pa
    b = new ['BigInt']
    v = b.'version'()
    ar = split '.', v
    ma = ar[0]
    mi = ar[1]
    pa = ar[2]
    if ma >= 4 goto ge_4
warn:
    print 'GMP version '
    print v
    print " is buggy with huge digit multiply - please upgrade\n"
    end
ge_4:
   if mi >= 2 goto ok
   if mi == 0 goto warn
   # test 4.1.x
   if pa >= 4 goto ok
   goto warn
   end
ok:
.end
EOP

if ( $PConfig{gmp} ) {

    # argh
    my $parrot = '.' . $PConfig{slash} . 'parrot' . $PConfig{exe};
    my $test   = 'temp_gmp_vers.pir';
    open my $O, '>', "$test" or die "can't open $test: $!";
    print $O $vers_check;
    close $O;
    my $warn = `$parrot $test`;
    diag $warn if $warn;
    unlink $test;
}

pasm_output_is( <<'CODE', <<'OUT', "create" );
   new P0, ['BigInt']
   print "ok\n"
   end
CODE
ok
OUT

pasm_output_is( <<'CODE', <<'OUT', "set/get int" );
   new P0, ['BigInt']
   set P0, 999999
   set I1, P0
   print I1
   print "\n"
   get_repr S0, P0
   print S0
   print "\n"
   end
CODE
999999
999999L
OUT

pasm_output_is( <<"CODE", <<'OUT', "set int, get double" );
     .include 'fp_equality.pasm'
     new P0, ['BigInt']
     set P0, 999999
     set N1, P0
     .fp_eq_pasm(N1, 999999.0, OK1)
     print "not "
OK1: print "ok 1\\n"

     set P0, -999999
     set N1, P0
     .fp_eq_pasm(N1, -999999.0, OK2)
     print "not "
OK2: print "ok 2\\n"

     set P0, 2147483646
     set N1, P0
     .fp_eq_pasm(N1, 2.147483646e9, OK3)
     print "not "
OK3: print "ok 3\\n"

     set P0, -2147483646
     set N1, P0
     .fp_eq_pasm(N1, -2.147483646e9, OK4)
     print "not "
OK4: print "ok 4\\n"
     end
CODE
ok 1
ok 2
ok 3
ok 4
OUT

pasm_output_is( <<'CODE', <<'OUT', "set double, get str" );
   new P0, ['BigInt']
   set P0, 1.23e12
   print P0
   print "\n"
   end
CODE
1230000000000
OUT

pasm_output_is( <<'CODE', <<'OUT', "set str, get str" );
   new P0, ['BigInt']
   set P0, "1230000000000"
   print P0
   print "\n"
   end
CODE
1230000000000
OUT

pasm_output_is( <<'CODE', <<'OUT', "add" );
   new P0, ['BigInt']
   set P0, 999999
   new P1, ['BigInt']
   set P1, 1000000
   new P2, ['BigInt']
   add P2, P0, P1
   set S0, P2
   print S0
   print "\n"
   set P0, "12345678987654321"
   set P1, "10000000000000000"
   add P2, P1, P0
   set S0, P2
   print S0
   print "\n"
   end
CODE
1999999
22345678987654321
OUT

pasm_output_is( <<'CODE', <<'OUT', "add_int" );
   new P0, ['BigInt']
   set P0, 999999
   new P2, ['BigInt']
   add P2, P0, 1000000
   set S0, P2
   print S0
   print "\n"
   set P0, "100000000000000000000"
   add P2, P0, 1000000
   set S0, P2
   print S0
   print "\n"
   end
CODE
1999999
100000000000001000000
OUT

pasm_output_is( <<'CODE', <<'OUTPUT', "sub bigint" );
     new P0, ['BigInt']
     set P0, 12345678
     new P1, ['BigInt']
     set P1, 5678
     new P2, ['BigInt']
     sub P2, P0, P1
     set I0, P2
     eq I0, 12340000, OK1
     print "not "
OK1: print "ok 1\n"
     set P0, "123456789012345678"
     sub P2, P0, P1
     new P3, ['BigInt']
     set P3, "123456789012340000"
     eq P2, P3, OK2
     print "not "
OK2: print "ok 2\n"
     set P1, "223456789012345678"
     sub P2, P0, P1
     set P3, "-100000000000000000"
     eq P2, P3, OK3
     print "not "
OK3: print "ok 3\n"
     end
CODE
ok 1
ok 2
ok 3
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "sub native int" );
     new P0, ['BigInt']
     set P0, 12345678
     new P2, ['BigInt']
     sub P2, P0, 5678
     set I0, P2
     eq I0, 12340000, OK1
     print "not "
OK1: print "ok 1\n"
     set P0, "123456789012345678"
     sub P2, P0, 5678
     new P3, ['BigInt']
     set P3, "123456789012340000"
     eq P2, P3, OK2
     print "not "
OK2: print "ok 2\n"
     end
CODE
ok 1
ok 2
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "sub other int" );
     new P0, ['BigInt']
     set P0, 12345678
     new P1, ['Integer']
     set P1, 5678
     new P2, ['BigInt']
     sub P2, P0, P1
     set I0, P2
     eq I0, 12340000, OK1
     print "not "
OK1: print "ok 1\n"
     set P0, "123456789012345678"
     sub P2, P0, P1
     new P3, ['BigInt']
     set P3, "123456789012340000"
     eq P2, P3, OK2
     print "not "
OK2: print "ok 2\n"
     set P0, 9876543
     new P4, ['Integer']
     set P4, 44
     sub P2, P0, P4
     set I0, P2
     eq I0, 9876499, OK3
     print "not "
OK3: print "ok 3\n"
     set P0, "9876543219876543"
     sub P2, P0, P4
     set P3, "9876543219876499"
     eq P3, P2, OK4
     print "not "
OK4: print "ok 4\n"
     end
CODE
ok 1
ok 2
ok 3
ok 4
OUTPUT

pasm_output_is( <<'CODE', <<'OUT', "mul" );
   new P0, ['BigInt']
   set P0, 999999
   new P1, ['BigInt']
   set P1, 1000000
   new P2, ['BigInt']
   mul P2, P0, P1
   set S0, P2
   print S0
   print "\n"
   end
CODE
999999000000
OUT

pasm_output_is( <<'CODE', <<'OUT', "mul_int" );
   new P0, ['BigInt']
   set P0, 999999
   new P2, ['BigInt']
   mul P2, P0, 1000000
   print P2
   print "\n"
   end
CODE
999999000000
OUT

pasm_output_is( <<'CODE', <<'OUT', "div bigint" );
     new P0, ['BigInt']
     set P0, "100000000000000000000"
     new P1, ['BigInt']
     set P1, "100000000000000000000"
     new P2, ['BigInt']
     div P2, P0, P1
     set I0, P2
     eq I0, 1, OK1
     print "not "
OK1: print "ok 1\n"

     new P3, ['BigInt']
     set P3, "10000000000000"
     set P1, 10000000
     div P2, P0, P1
     eq  P2, P3, OK2
     print "not "
OK2: print "ok 2\n"

     set P1, 10
     set P3, "10000000000000000000"
     div P2, P0, P1
     eq  P2, P3, OK3
     print "not "
OK3: print "ok 3\n"

     set P1, -1
     set P3, "-100000000000000000000"
     div P2, P0, P1
     eq  P2, P3, OK4
     print "not "
OK4: print "ok 4\n"
     end
CODE
ok 1
ok 2
ok 3
ok 4
OUT

pasm_output_is( <<'CODE', <<'OUT', "div native int" );
     new P0, ['BigInt']
     set P0, "100000000000000000000"
     new P1, ['BigInt']
     div P1, P0, 10
     new P2, ['BigInt']
     set P2, "10000000000000000000"
     eq P1, P2, OK1
     print "not "
OK1: print "ok 1\n"

     set P0, "100000000000000"
     div P1, P0, 10000000
     set P2, 10000000
     eq  P1, P2, OK2
     print "not "
OK2: print "ok 2\n"
     end
CODE
ok 1
ok 2
OUT

pasm_output_is( <<'CODE', <<'OUT', "div other int" );
     new P0, ['BigInt']
     set P0, "100000000000000000000"
     new P1, ['BigInt']
     new P3, ['Integer']
     set P3, 10
     div P1, P0, P3
     new P2, ['BigInt']
     set P2, "10000000000000000000"
     eq P1, P2, OK1
     print "not "
OK1: print "ok 1\n"

     set P0, "100000000000000"
     new P4, ['Integer']
     set P4, 10000000
     div P1, P0, P4
     set P2, 10000000
     eq  P1, P2, OK2
     print "not "
OK2: print "ok 2\n"
     end
CODE
ok 1
ok 2
OUT

for my $op ( "/", "%" ) {
    for my $type ( "BigInt", "Integer" ) {
        pir_output_is( <<"CODE", <<OUTPUT, "bigint $op by zero $type" );
.sub _main :main
    \$P0 = new ['BigInt']
    set \$P0, "1000000000000000000000"
    \$P1 = new ['BigInt']
    ## divide by a zero $type
    \$P2 = new ['$type']
    set \$P2, 0
    push_eh OK
    \$P1 = \$P0 $op \$P2
    print "fail\\n"
    pop_eh
OK:
    get_results '0', \$P0
    \$S0 = \$P0
    print "ok\\n"
    print \$S0
    print "\\n"
.end
CODE
ok
Divide by zero
OUTPUT
    }
}

{
    my ( $a, $b, $c, $d, $e );
    if ( $PConfig{intvalsize} == 8 ) {
        $a = '9223372036854775806';    # 2**63-2
        $b = '1';
        $c = '9223372036854775807';    # still Integer
        $d = '9223372036854775808';    # no more Integer
        $e = '9223372036854775809';    # still no more Integer
    }
    elsif ( $PConfig{intvalsize} == 4 ) {
        $a = '2147483646';             # 2**31-2
        $b = '1';
        $c = '2147483647';             # still Integer
        $d = '2147483648';             # no more PerlInt
        $e = '2147483649';             # still no more PerlInt
    }
    else {
        die "\$PConfig{intvalsize} == $PConfig{intvalsize}?\n";
    }

    pasm_output_is( <<CODE, <<OUT, "add overflow Integer" );
   new P0, ['Integer']
   set P0, $a
   new P1, ['Integer']
   set P1, $b
   new P2, ['Integer']
   new P3, ['BigInt']
   set I3, 3
lp:
   add P2, P0, P1
   set S0, P2
   print S0
   print " "
   typeof S1, P2
   print S1
   print "\\n"
   add P1, $b
   dec I3
   if I3, lp
   print "ok\\n"
ex:
   end
CODE
$c Integer
$d BigInt
$e BigInt
ok
OUT

    pasm_output_is( <<CODE, <<OUT, "add overflow Integer" );
   new P0, ['Integer']
   set P0, $a
   new P1, ['Integer']
   set P1, $b
   new P2, ['Integer']
   new P3, ['BigInt']
   set I3, 3
lp:
   add P2, P0, P1
   set S0, P2
   print S0
   print " "
   typeof S1, P2
   print S1
   print "\\n"
   add P1, $b
   dec I3
   if I3, lp
   print "ok\\n"
ex:
   end
CODE
$c Integer
$d BigInt
$e BigInt
ok
OUT
}

pasm_output_is( <<'CODE', <<'OUT', "abs" );
   new P0, ['BigInt']
   set P0, "-1230000000000"
   new P1, ['Undef']
   abs P1, P0
   print P1
   print "\n"
   print P0
   print "\n"
   abs P0
   print P0
   print "\n"
   end
CODE
1230000000000
-1230000000000
1230000000000
OUT

pir_output_is( << 'CODE', << 'OUTPUT', "check whether interface is done" );

.sub _main
    .local pmc pmc1
    pmc1 = new ['BigInt']
    .local int bool1
    does bool1, pmc1, "scalar"
    print bool1
    print "\n"
    does bool1, pmc1, "no_interface"
    print bool1
    print "\n"
    end
.end
CODE
1
0
OUTPUT

pasm_output_is( <<"CODE", <<'OUTPUT', "Truth" );
     new P0, ['BigInt']
     set P0, "123456789123456789"
     if P0, OK1
     print "not "
OK1: print "ok 1\\n"
     set P0, 0
     unless P0, OK2
     print "not "
OK2: print "ok 2\\n"
     end
CODE
ok 1
ok 2
OUTPUT

pasm_output_is( <<"CODE", <<'OUTPUT', "neg" );
     new P0, ['BigInt']
     new P1, ['BigInt']
     set P0, "123456789123456789"
     neg P0
     set P1, "-123456789123456789"
     eq P0, P1, OK1
     print "not "
OK1: print "ok 1\\n"
     end
CODE
ok 1
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "pi() generator" );
.sub PI
    .local pmc k, a, b, a1, b1
    k = new ['Integer']
    k = 2
    a = new ['Integer']
    a = 4
    b = new ['Integer']
    b = 1
    a1 = new ['Integer']
    a1 = 12
    b1 = new ['Integer']
    b1 = 4
forever:
    .local pmc p, q
    p = mul k, k
    q = mul k, 2
    inc q
    inc k
    .local pmc ta, tb, ta1, tb1
    ta = clone a1
    tb = clone b1
    $P0 = mul p, a
    $P1 = mul q, a1
    ta1 =  add $P0, $P1
    $P2 = mul p, b
    $P3 = mul q, b1
    tb1 =  add $P2, $P3
    a = ta
    b = tb
    a1 = ta1
    b1 = tb1
    .local pmc d, d1
    d = fdiv a, b
    d1 = fdiv a1, b1
yield_loop:
    unless d == d1 goto end_yield
    .yield(d)
    $P4 = mod a, b
    a = mul $P4, 10
    $P5 = mod a1, b1
    a1 = mul $P5, 10
    d = fdiv a, b
    d1 = fdiv a1, b1
    goto yield_loop
end_yield:
    goto forever
.end

.sub main :main
    .local int i
    .local pmc d
    null i
loop:
    d = PI()
    print d
    inc i
    $I0 = i % 50
    if $I0 goto no_nl
    print "\n"
no_nl:
    if i < 1000 goto loop
    print "\n"
.end

=begin python

class PI(object):
    def __iter__(self):
        k, a, b, a1, b1 = 2, 4, 1, 12, 4
        while 1:
            p, q, k = k*k, 2*k+1, k+1
            a, b, a1, b1 = a1, b1, p*a+q*a1, p*b+q*b1
            d, d1 = a//b, a1//b1
            while d == d1:
                yield d
                a, a1 = 10*(a%b), 10*(a1%b1)
                d, d1 = a//b, a1//b1

pi = iter(PI())
ds = ""
for i in xrange(1, 1001):
    d = pi.next()
    ds += str(d)
    im = i % 50
    if im == 0:
        print ds
        ds = ""

print ds

=end python

=cut

CODE
31415926535897932384626433832795028841971693993751
05820974944592307816406286208998628034825342117067
98214808651328230664709384460955058223172535940812
84811174502841027019385211055596446229489549303819
64428810975665933446128475648233786783165271201909
14564856692346034861045432664821339360726024914127
37245870066063155881748815209209628292540917153643
67892590360011330530548820466521384146951941511609
43305727036575959195309218611738193261179310511854
80744623799627495673518857527248912279381830119491
29833673362440656643086021394946395224737190702179
86094370277053921717629317675238467481846766940513
20005681271452635608277857713427577896091736371787
21468440901224953430146549585371050792279689258923
54201995611212902196086403441815981362977477130996
05187072113499999983729780499510597317328160963185
95024459455346908302642522308253344685035261931188
17101000313783875288658753320838142061717766914730
35982534904287554687311595628638823537875937519577
81857780532171226806613001927876611195909216420198

OUTPUT

pasm_output_is( <<'CODE', <<'OUT', "shl_bigint" );
   new P0, ['BigInt']
   set P0, "2"
   new P1, ['BigInt']
   set P1, 2
   new P2, ['BigInt']
   shl P2, P0, P1
   set S0, P2
   print S0
   print "\n"
   set P0, "100000000000"
   set P1, 10
   shl P2, P0, P1
   set S0, P2
   print S0
   print "\n"
   end
CODE
8
102400000000000
OUT

pir_output_is( <<'CODE', <<'OUT', "shl_bigint with a negative shift" );
## cf the shr_bigint case.
.sub main :main
   $P0 = new ['BigInt']
   set $P0, 8
   $P1 = new ['BigInt']
   set $P1, -2
   $P2 = new ['BigInt']
   shl $P2, $P0, $P1
   say $P2
   set $P0, "102400000000000"
   set $P1, -10
   shl $P2, $P0, $P1
   say $P2
.end
CODE
2
100000000000
OUT

pasm_output_is( <<'CODE', <<'OUT', "shl_int" );
   new P0, ['BigInt']
   set P0, 2
   new P1, ['Integer']
   set P1, 1
   new P2, ['BigInt']
   shl P2, P0, P1
   set S0, P2
   print S0
   print "\n"
   set P0, "100000000000"
   set P1, 1
   shl P2, P0, P1
   set S0, P2
   print S0
   print "\n"
   set P0, "100000000000"
   set P1, 10
   shl P2, P0, P1
   set S0, P2
   print S0
   print "\n"
   end
CODE
4
200000000000
102400000000000
OUT

pir_output_is( <<'CODE', <<'OUT', "shl_int with a negative shift" );
## cf the shr_int case.
.sub main :main
   $P0 = new ['BigInt']
   set $P0, 4
   $P1 =  new ['Integer']
   set $P1, -1
   $P2 = new ['BigInt']
   shl $P2, $P0, $P1
   say $P2
   set $P0, "200000000000"
   set $P1, -1
   shl $P2, $P0, $P1
   say $P2
   set $P0, "102400000000000"
   set $P1, -10
   shl $P2, $P0, $P1
   say $P2
.end
CODE
2
100000000000
100000000000
OUT

pir_output_like( <<'CODE', <<'OUT', "shl_int and i_shl_int promote Integer to Bigint" );
## The result on the second line is a BigInt on 32-bit systems and still an
## Integer on 64-bit systems.
.sub main :main
   $P0 = new ['Integer']
   set $P0, 1000001
   $P1 = new ['Integer']
   set $P1, 10
   $P2 = new ['Integer']
   ## shift by 10 bits . . .
   shl $P2, $P0, $P1
   $S2 = typeof $P2
   print $S2
   print ' '
   say $P2
   ## then by 20 bits . . .
   $P1 = 20
   $P3 = new ['Integer']
   $P3 = 1000001
   shl $P3, $P0, $P1
   $S2 = typeof $P3
   print $S2
   print ' '
   say $P3
   ## then by another 40 bits (total 60) in place.
   $P1 = 40
   shl $P3, $P3, $P1
   $S2 = typeof $P3
   print $S2
   print ' '
   say $P3
.end
CODE
/Integer 1024001024
(Integer|BigInt) 1048577048576
BigInt 1152922657528351582846976
/
OUT

pir_error_output_like( <<'CODE', <<'OUT', "shl_int throws an error when promotion is disabled" );
.include "errors.pasm"
.sub main :main
   errorson .PARROT_ERRORS_OVERFLOW_FLAG
   $P0 = new ['Integer']
   set $P0, 1000001
   $P1 = new ['Integer']
   set $P1, 10
   $P2 = new ['Integer']
   ## shift by 10 bits . . .
   shl $P2, $P0, $P1
   $S2 = typeof $P2
   print $S2
   print ' '
   say $P2
   ## then by 60 bits.
   $P1 = 60
   $P0 = 1000001
   shl $P3, $P0, $P1
   $S2 = typeof $P3
   print $S2
   print ' '
   say $P3
.end
CODE
/Integer 1024001024
Integer overflow
current instr/
OUT

pir_output_is( <<'CODE', <<'OUT', "shl_int by 64 bits also promotes to Bigint" );
## The C << and >> ops take the right arg modulo the word size in bits (at least
## on all the systems I have available), so both 32- and 64-bit systems treat
## shifting by 64 bits as shifting by zero.
.sub main :main
   $P0 = new ['Integer']
   set $P0, 1000001
   $P1 = new ['Integer']
   set $P1, 64
   shl $P2, $P0, $P1
   $S2 = typeof $P2
   print $S2
   print ' '
   say $P2
.end
CODE
BigInt 18446762520453625325551616
OUT

pir_output_is(
    <<'CODE', <<'OUT', "shr_int and i_shr_int with a neg shift promote Integer to Bigint" );
.sub main :main
   $P0 = new ['Integer']
   set $P0, 1000001
   $P1 = new ['Integer']
   set $P1, -10
   $P2 = new ['Integer']
   ## shift by 10 bits . . .
   shr $P2, $P0, $P1
   $S2 = typeof $P2
   print $S2
   print ' '
   say $P2
   ## then by another 50 bits (total 60) in place.
   $P1 = -50
   shr $P2, $P1
   $S2 = typeof $P2
   print $S2
   print ' '
   say $P2
.end
CODE
Integer 1024001024
BigInt 1152922657528351582846976
OUT

pasm_output_is( <<'CODE', <<'OUT', "shr_bigint" );
   new P0, ['BigInt']
   set P0, 8
   new P1, ['BigInt']
   set P1, 2
   new P2, ['BigInt']
   shr P2, P0, P1
   set S0, P2
   print S0
   print "\n"
   set P0, "102400000000000"
   set P1, 10
   shr P2, P0, P1
   set S0, P2
   print S0
   print "\n"
   end
CODE
2
100000000000
OUT

pir_output_is( <<'CODE', <<'OUT', "shr_bigint with a negative shift" );
## cf the shl_bigint case.
.sub main :main
   $P0 = new ['BigInt']
   set $P0, 2
   $P1 = new['BigInt']
   set $P1, -2
   $P2 = new ['BigInt']
   shr $P2, $P0, $P1
   say $P2
   set $P0, "100000000000"
   set $P1, -10
   shr $P2, $P0, $P1
   say $P2
.end
CODE
8
102400000000000
OUT

pasm_output_is( <<'CODE', <<'OUT', "shr_int" );
   new P0, ['BigInt']
   set P0, 4
   new P1, ['Integer']
   set P1, 1
   new P2, ['BigInt']
   shr P2, P0, P1
   set S0, P2
   print S0
   print "\n"
   set P0, "200000000000"
   set P1, 1
   shr P2, P0, P1
   set S0, P2
   print S0
   print "\n"
   set P0, "102400000000000"
   set P1, 10
   shr P2, P0, P1
   set S0, P2
   print S0
   print "\n"
   end
CODE
2
100000000000
100000000000
OUT

pir_output_is( <<'CODE', <<'OUT', "shr_int with a negative shift" );
## cf the shl_int case.
.sub main :main
   $P0 = new ['BigInt']
   set $P0, 2
   $P1 = new ['Integer']
   set $P1, -1
   $P2 = new ['BigInt']
   shr $P2, $P0, $P1
   say $P2
   set $P0, "100000000000"
   set $P1, -1
   shr $P2, $P0, $P1
   say $P2
   set $P1, -10
   shr $P2, $P0, $P1
   say $P2
.end
CODE
4
200000000000
102400000000000
OUT

pir_output_is( <<'CODE', <<'OUT', "BUG #34949 gt" );
.sub main :main
    .local pmc b
    b = new ['BigInt']
    b = 1e10
    if b > 4 goto ok
    print "never\n"
    end
ok:
    print "ok\n"
.end
CODE
ok
OUT

pir_output_is( <<'CODE', <<'OUT', "BUG #34949 ge" );
.sub main :main
    .local pmc b
    b = new ['BigInt']
    b = 1e10
    if b >= 4 goto ok
    print "never\n"
    end
ok:
    print "ok\n"
.end
CODE
ok
OUT

pir_output_is( <<'CODE', <<'OUT', "BUG #34949 ne" );
.sub main :main
    .local pmc b
    b = new ['BigInt']
    b = 1e10
    if b != 4 goto ok
    print "never\n"
    end
ok:
    print "ok\n"
.end
CODE
ok
OUT

pir_output_is( <<'CODE', <<'OUT', "BUG #34949 eq" );
.sub main :main
    .local pmc b
    b = new ['BigInt']
    b = 1e10
    if b == 4 goto nok
    print "ok\n"
    end
nok:
    print "nok\n"
.end
CODE
ok
OUT

pir_output_is( <<'CODE', <<'OUT', "BUG #34949 le" );
.sub main :main
    .local pmc b
    b = new ['BigInt']
    b = 1e10
    if b <= 4 goto nok
    print "ok\n"
    end
nok:
    print "nok\n"
.end
CODE
ok
OUT

pir_output_is( <<'CODE', <<'OUT', "BUG #34949 lt" );
.sub main :main
    .local pmc b
    b = new ['BigInt']
    b = 1e10
    if b < 4 goto nok
    print "ok\n"
    end
nok:
    print "nok\n"
.end
CODE
ok
OUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
