#! perl
# Copyright (C) 2001-2007, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );

use Test::More;
use Parrot::Test tests => 62;
use Parrot::Config;

=head1 NAME

t/pmc/sub.t - Subroutine PMCs

=head1 SYNOPSIS

    % prove t/pmc/sub.t

=head1 DESCRIPTION

Tests the creation and invocation of C<Sub>, C<Closure> and
C<Continuation> PMCs.

=cut

my $temp = "temp.pasm";

END {
    unlink( $temp, 'temp.pbc', 'temp.pasm' );
}

pasm_output_is( <<'CODE', <<'OUTPUT', "PASM subs - invokecc" );
    .const .Sub P0 = "func"

    set I5, 3
    set_args "0", I5
    invokecc P0
    print I5
    print "\n"
    end

.pcc_sub func:
    get_params "0", I5
    print I5
    print "\n"

    eq I5, 0, endfunc
    dec I5

.include "interpinfo.pasm"
    interpinfo P0, .INTERPINFO_CURRENT_SUB
    set_args "0", I5
    invokecc P0  # recursive invoke

endfunc:
    returncc
CODE
3
2
1
0
3
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "Continuation" );
    new P5, 'Integer'
    set P5, 3
    store_global "foo", P5
    new P1, 'Continuation'
    set_addr P1, endcont
endcont:
    get_global P4, "foo"
    print "here "
    print P4
    print "\n"
    unless P4, done
    dec P4
    store_global "foo", P4
    print "going to cont\n"
    clone P0, P1
    invokecc P0
done:
    print "done\n"
    end

CODE
here 3
going to cont
here 2
going to cont
here 1
going to cont
here 0
done
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "definedness of Continuation" );
    new P1, 'Continuation'
    defined I1, P1
    print I1
    print "\n"
    set_addr P1, cont
    defined I1, P1
    print I1
    print "\n"
    end

cont:
    print "I'm a very boring continuation"
    end

CODE
0
1
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "pcc sub" );
    get_global P0, "_the_sub"
    defined I0, P0
    if I0, ok
    print "not "
ok:
    print "ok 1\n"
    invokecc P0
    print "back\n"
    end
.pcc_sub _the_sub:
    print "in sub\n"
    returncc
CODE
ok 1
in sub
back
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "pcc sub, tail call" );
    get_global P0, "_the_sub"
    defined I0, P0
    if I0, ok
    print "not "
ok:
    print "ok 1\n"
    invokecc P0
    print "back\n"
    end

.pcc_sub _the_sub:
    print "in sub\n"
    get_global P0, "_next_sub"
    get_addr I0, P0
    jump I0
    print "never here\n"

.pcc_sub _next_sub:
    print "in next sub\n"
    returncc
    print "never here\n"
    end
CODE
ok 1
in sub
in next sub
back
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "pcc sub perl::syn::tax" );
    get_global P0, "_the::sub::some::where"
    defined I0, P0
    if I0, ok
    print "not "
ok:
    print "ok 1\n"
    invokecc P0
    print "back\n"
    end
.pcc_sub _the::sub::some::where:
    print "in sub\n"
    returncc
CODE
ok 1
in sub
back
OUTPUT

open my $S, '>', "$temp" or die "Can't write $temp";
print $S <<'EOF';
  .pcc_sub _sub1:
  print "in sub1\n"
  end
EOF
close $S;

pasm_output_is( <<'CODE', <<'OUTPUT', "load_bytecode call sub" );
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pasm"
    print "loaded\n"
    get_global P0, "_sub1"
    defined I0, P0
    if I0, ok1
    print "not "
ok1:
    print "found sub\n"
    invokecc P0
    print "never\n"
    end
CODE
main
loaded
found sub
in sub1
OUTPUT

open $S, '>', "$temp" or die "Can't write $temp";
print $S <<'EOF';
  .pcc_sub _sub1:
  print "in sub1\n"
  returncc
EOF
close $S;

pasm_output_is( <<'CODE', <<'OUTPUT', "load_bytecode call sub, ret" );
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pasm"
    print "loaded\n"
    get_global P0, "_sub1"
    defined I0, P0
    if I0, ok1
    print "not "
ok1:
    print "found sub\n"
    invokecc P0
    print "back\n"
    end
CODE
main
loaded
found sub
in sub1
back
OUTPUT

open $S, '>', "$temp" or die "Can't write $temp";
print $S <<'EOF';
  .pcc_sub _sub1:
  print "in sub1\n"
  returncc
  .pcc_sub _sub2:
  print "in sub2\n"
  returncc
EOF
close $S;

pasm_output_is( <<'CODE', <<'OUTPUT', "load_bytecode call different subs, ret" );
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pasm"
    print "loaded\n"
    get_global P0, "_sub1"
    defined I0, P0
    if I0, ok1
    print "not "
ok1:
    print "found sub1\n"
    set P10, P0
    invokecc P0
    print "back\n"
    get_global P0, "_sub2"
    defined I0, P0
    if I0, ok2
    print "not "
ok2:
    print "found sub2\n"
    invokecc P0
    print "back\n"
    set P0, P10
    invokecc P0
    print "back\n"
    end
CODE
main
loaded
found sub1
in sub1
back
found sub2
in sub2
back
in sub1
back
OUTPUT

system(".$PConfig{slash}parrot$PConfig{exe} -o temp.pbc $temp");

pir_output_is( <<'CODE', <<'OUTPUT', "load_bytecode Sx" );
.sub main :main
    $S0 = 'temp.pasm'
    load_bytecode $S0
    _sub1()
    $S0 = 'temp.pbc'
    load_bytecode $S0
    _sub2()
.end
CODE
in sub1
in sub2
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "load_bytecode PBC call different subs, ret" );
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pbc"
    print "loaded\n"
    get_global P0, "_sub1"
    defined I0, P0
    if I0, ok1
    print "not "
ok1:
    print "found sub1\n"
    set P10, P0
    invokecc P0
    print "back\n"
    get_global P0, "_sub2"
    defined I0, P0
    if I0, ok2
    print "not "
ok2:
    print "found sub2\n"
    invokecc P0
    print "back\n"
    set P0, P10
    invokecc P0
    print "back\n"
    end
CODE
main
loaded
found sub1
in sub1
back
found sub2
in sub2
back
in sub1
back
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "equality of closures" );
.pcc_sub main:
      .const .Sub P3 = "f1"
      newclosure P0, P3
      clone P1, P0
      eq P0, P1, OK1
      print "not "
OK1:  print "ok 1\n"

      .const .Sub P4 = "f2"
      newclosure P2, P4
      eq P0, P2, BAD2
      branch OK2
BAD2: print "not "
OK2:  print "ok 2\n"
      end

.pcc_sub :outer(main) f1:
      print "Test\n"
      end

.pcc_sub :outer(main) f2:
      new P1, 'Undef'
      end
CODE
ok 1
ok 2
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "equality of subs" );
      .const .Sub P0 = "f1"
      clone P1, P0
      eq P0, P1, OK1
      print "not "
OK1:  print "ok 1\n"

      .const .Sub P2 = "f2"
      clone P1, P0
      eq P0, P2, BAD2
      branch OK2
BAD2: print "not "
OK2:  print "ok 2\n"
      end

.pcc_sub f1:
      print "Test\n"
      end

.pcc_sub f2:
      new P1, 'Undef'
      end
CODE
ok 1
ok 2
OUTPUT

pasm_output_is( <<'CODE', <<'OUT', "MAIN pragma, syntax only" );
.pcc_sub :main _main:
    print "ok\n"
    end
CODE
ok
OUT

open $S, '>', "$temp" or die "Can't write $temp";
print $S <<'EOF';
  .pcc_sub :load _sub1:
  print "in sub1\n"
  returncc
EOF
close $S;

pasm_output_is( <<'CODE', <<'OUTPUT', 'load_bytecode :load' );
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pasm"
    print "back\n"
    end
CODE
main
in sub1
back
OUTPUT

open $S, '>', "$temp" or die "Can't write $temp";
print $S <<'EOF';
  .pcc_sub _error:
  print "error\n"
  .pcc_sub :load _sub1:
  print "in sub1\n"
  returncc
EOF
close $S;

pasm_output_is( <<'CODE', <<'OUTPUT', 'load_bytecode :load second sub' );
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pasm"
    print "back\n"
    end
CODE
main
in sub1
back
OUTPUT

system(".$PConfig{slash}parrot$PConfig{exe} -o temp.pbc $temp");

pasm_output_is( <<'CODE', <<'OUTPUT', 'load_bytecode :load in pbc' );
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pbc"
    print "back\n"
    end
CODE
main
in sub1
back
OUTPUT

open $S, '>', "$temp" or die "Can't write $temp";
print $S <<'EOF';
  .pcc_sub :load _sub1:
  print "in sub1\n"
  returncc
  .pcc_sub _sub2:
  print "in sub2\n"
  returncc
EOF
close $S;

pasm_output_is( <<'CODE', <<'OUTPUT', "load_bytecode autorun first" );
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pasm"
    print "loaded\n"
    get_global P0, "_sub2"
    invokecc P0
    print "back\n"
    end
CODE
main
in sub1
loaded
in sub2
back
OUTPUT

system(".$PConfig{slash}parrot$PConfig{exe} -o temp.pbc $temp");

pasm_output_is( <<'CODE', <<'OUTPUT', "load_bytecode autorun first in pbc" );
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pbc"
    print "loaded\n"
    get_global P0, "_sub2"
    invokecc P0
    print "back\n"
    end
CODE
main
in sub1
loaded
in sub2
back
OUTPUT

open $S, '>', "$temp" or die "Can't write $temp";
print $S <<'EOF';
  .pcc_sub _sub1:
  print "in sub1\n"
  returncc
  .pcc_sub :load _sub2:
  print "in sub2\n"
  returncc
EOF
close $S;

pasm_output_is( <<'CODE', <<'OUTPUT', "load_bytecode autorun second" );
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pasm"
    print "loaded\n"
    get_global P0, "_sub1"
    invokecc P0
    print "back\n"
    end
CODE
main
in sub2
loaded
in sub1
back
OUTPUT

system(".$PConfig{slash}parrot$PConfig{exe} -o temp.pbc $temp");

pasm_output_is( <<'CODE', <<'OUTPUT', "load_bytecode autorun second in pbc" );
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pbc"
    print "loaded\n"
    get_global P0, "_sub1"
    invokecc P0
    print "back\n"
    end
CODE
main
in sub2
loaded
in sub1
back
OUTPUT

open $S, '>', "$temp" or die "Can't write $temp";
print $S <<'EOF';
  .pcc_sub :load _sub1:
  print "in sub1\n"
  returncc
  .pcc_sub :load _sub2:
  print "in sub2\n"
  returncc
EOF
close $S;

pasm_output_is( <<'CODE', <<'OUTPUT', "load_bytecode autorun both" );
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pasm"
    print "loaded\n"
    get_global P0, "_sub1"
    invokecc P0
    print "back\n"
    end
CODE
main
in sub1
in sub2
loaded
in sub1
back
OUTPUT

system(".$PConfig{slash}parrot$PConfig{exe} -o temp.pbc $temp");

pasm_output_is( <<'CODE', <<'OUTPUT', "load_bytecode autorun both in pbc" );
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pbc"
    print "loaded\n"
    get_global P0, "_sub1"
    invokecc P0
    print "back\n"
    end
CODE
main
in sub1
in sub2
loaded
in sub1
back
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', ':main pragma' );
.pcc_sub _first:
    print "first\n"
    returncc
.pcc_sub :main _main:
    print "main\n"
    end
CODE
main
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', 'two :main pragmas' );
.pcc_sub _first:
    print "first\n"
    returncc
.pcc_sub :main _main:
    print "main\n"
    end
.pcc_sub :main _second:
    print "second\n"
    returncc
CODE
main
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', ':main pragma call subs' );
.pcc_sub _first:
    print "first\n"
    returncc
.pcc_sub _second:
    print "second\n"
    returncc
.pcc_sub :main _main:
    print "main\n"
    get_global P0, "_first"
    invokecc P0
    get_global P0, "_second"
    invokecc P0
    end
CODE
main
first
second
OUTPUT

pir_error_output_like( <<'CODE', <<'OUTPUT', "implicit :main with wrong # args." );
.sub a
  .param int op1
  .param int op2
.end
CODE
/argument count mismatch in main \(more than 1 param\)/
OUTPUT

pir_error_output_like( <<'CODE', <<'OUTPUT', "explicit :main with wrong # args." );
.sub a :main
  .param int op1
  .param int op2
.end
CODE
/argument count mismatch in main \(more than 1 param\)/
OUTPUT

$temp = "temp.pir";
open $S, '>', "$temp" or die "Can't write $temp";
print $S <<'EOF';
.sub _sub1 :load
  print "in sub1\n"
  returncc
.end
EOF
close $S;

pasm_output_is( <<'CODE', <<'OUTPUT', 'load_bytecode :load first sub - pir' );
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pir"
    print "back\n"
    end
CODE
main
in sub1
back
OUTPUT

open $S, '>', "$temp" or die "Can't write $temp";
print $S <<'EOF';
.sub _foo
  print "error\n"
.end

# :load or other pragmas are only evaluated on the first
# instruction of a compilation unit
.sub _sub1 :load
  print "in sub1\n"
  returncc
.end
EOF
close $S;

pasm_output_is( <<'CODE', <<'OUTPUT', 'load_bytecode :load second sub - pir' );
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pir"
    print "back\n"
    end
CODE
main
in sub1
back
OUTPUT

open $S, '>', "$temp" or die "Can't write $temp";
print $S <<'EOF';
.sub _foo
  print "error\n"
.end
.sub _sub1
  print "in sub1\n"
  returncc
.end
EOF
close $S;

pasm_output_is( <<'CODE', <<'OUTPUT', 'load_bytecode no :load - pir' );
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pir"
    print "back\n"
    end
CODE
main
back
OUTPUT

# This is the behavior of Parrot 0.4.3
# RT#46817 Should there be a warning ?
pir_output_is( <<'CODE', '', 'warn on in main' );
.sub _main :main
.include "warnings.pasm"
    warningson .PARROT_WARNINGS_UNDEF_FLAG
    _f1()
.end
.sub _f1
    $P0 = new 'Undef'
    print $P0
.end
CODE

pir_output_is( <<'CODE', <<'OUTPUT', "warn on in sub" );
.sub _main :main
.include "warnings.pasm"
    _f1()
    $P0 = new 'Undef'
    print $P0
    print "ok\n"
.end
.sub _f1
    warningson .PARROT_WARNINGS_UNDEF_FLAG
.end
CODE
ok
OUTPUT

# RT#46819 This is the behavior of Parrot 0.4.3
# It looks like core PMCs never emit warning.
# Look in perlundef.t for a more sane test of 'warningson' in subs
pir_output_is( <<'CODE', <<'OUTPUT', "warn on in sub, turn off in f2" );
.sub _main :main
.include "warnings.pasm"
    _f1()
    $P0 = new 'Undef'
    print "back\n"
    print $P0
    print "ok\n"
.end
.sub _f1
    warningson .PARROT_WARNINGS_UNDEF_FLAG
    _f2()
    $P0 = new 'Undef'
    print $P0
.end
.sub _f2
    warningsoff .PARROT_WARNINGS_UNDEF_FLAG
.end
CODE
back
ok
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "sub names" );
.pcc_sub main:
    .include "interpinfo.pasm"
    interpinfo P20, .INTERPINFO_CURRENT_SUB
    print P20
    print "\n"
    get_global P0, "the_sub"
    invokecc P0
    interpinfo P20, .INTERPINFO_CURRENT_SUB
    print P20
    print "\n"
    end
.pcc_sub the_sub:
    interpinfo P20, .INTERPINFO_CURRENT_SUB
    print P20
    print "\n"
    interpinfo P1, .INTERPINFO_CURRENT_CONT
    returncc
CODE
main
the_sub
main
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "sub names w MAIN" );
.pcc_sub dummy:
    print "never\n"
    noop
.pcc_sub :main main:
    .include "interpinfo.pasm"
    interpinfo P20, .INTERPINFO_CURRENT_SUB
    print P20
    print "\n"
    get_global P0, "the_sub"
    invokecc P0
    interpinfo P20, .INTERPINFO_CURRENT_SUB
    print P20
    print "\n"
    end
.pcc_sub the_sub:
    interpinfo P20, .INTERPINFO_CURRENT_SUB
    print P20
    print "\n"
    interpinfo P1, .INTERPINFO_CURRENT_CONT
    returncc
CODE
main
the_sub
main
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "caller introspection via interp" );
.sub main :main
.include "interpinfo.pasm"
    # this test will fail when run with -Oc
    # as the call chain is cut down with tail calls
    foo()
    
    $P0 = get_hll_global ["Bar"], "foo"
    $P0()
    print "ok\n"
.end
.sub foo
    print "main foo\n"
    $P0 = get_hll_global ["Bar"], "bar"
    $P0()
.end
.namespace ["Bar"]
.sub bar
    print "Bar bar\n"
    $P1 = getinterp
    $P0 = $P1["sub"]
    print "subname: "
    print $P0
    print "\n"
    foo()
.end
.sub foo
    print "Bar foo\n"
    $P1 = getinterp
    $I0 = 0
    push_eh tb_end
tb_loop:
    $P0 = $P1["sub"; $I0]
    print "caller "
    print $I0
    print " "
    print $P0
    print "\n"
    inc $I0
    goto tb_loop
tb_end:
.end
CODE
main foo
Bar bar
subname: bar
Bar foo
caller 0 foo
caller 1 bar
caller 2 foo
caller 3 main
Bar foo
caller 0 foo
caller 1 main
ok
OUTPUT

{

    # the test has different output when run with --run-pbc (make testr)
    # actually not - compiling creates 2 'initial'
    #                running emts 'main'

    my $code = <<'CODE';
.sub optc :immediate :postcomp
    print "initial\n"
.end
.sub _main :main
    print "main\n"
.end
CODE
    my $descr = ':immediate :postcomp';
    if ( exists $ENV{TEST_PROG_ARGS} and $ENV{TEST_PROG_ARGS} =~ m/-r/ ) {
        pir_output_is( $code, <<'OUT', $descr );
initial
initial
main
OUT
    }
    else {
        pir_output_is( $code, <<'OUT', $descr );
initial
initial
main
OUT
    }
}

pir_output_like( <<'CODE', <<'OUTPUT', ':anon' );
.sub main :main
    "foo"()
    print "ok\n"
    $P0 = global "new"
    $I0 = defined $P0
    print $I0
    print "\n"
    $P0 = global "foo"
    unless null $P0 goto foo
    print "nofoo\n"
  foo:
.end

.sub "foo" :anon
    print "foo\n"
    "new"()
.end

.sub "new"
    print "new\n"
.end
CODE
/^foo
new
ok
1
nofoo/
OUTPUT

open $S, '>', "test_l1.pir" or die "Can't write test_l1.pir";
print $S <<'EOF';
.sub l11 :load
    print "l11\n"
.end

.sub l12 :load
    print "l12\n"
.end
EOF
close $S;

open $S, '>', "test_l2.pir" or die "Can't write test_l2.pir";
print $S <<'EOF';
.sub l21 :load
    print "l21\n"
.end

.sub l22 :load
    print "l22\n"
.end
EOF
close $S;

system(".$PConfig{slash}parrot$PConfig{exe} -o test_l1.pbc test_l1.pir");
system(".$PConfig{slash}parrot$PConfig{exe} -o test_l2.pbc test_l2.pir");

END { unlink(qw/ test_l1.pir test_l2.pir test_l1.pbc test_l2.pbc /); }

pir_output_is( <<'CODE', <<'OUTPUT', 'multiple :load' );
.sub main :main
    print "main 1\n"
    load_bytecode "test_l1.pir"
    load_bytecode "test_l2.pir"
    print "main 2\n"
    load_bytecode "test_l1.pbc"	# these have to be ignored
    load_bytecode "test_l2.pbc"
    print "main 3\n"
.end
CODE
main 1
l11
l12
l21
l22
main 2
main 3
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "immediate code as const" );
.sub make_pi :immediate :anon
    $N0 = atan 1.0, 1.0
    $N0 *= 4
    $P0 = new 'Float'
    $P0 = $N0
    .return ($P0)
.end

.sub main :main
    .const .Sub pi = "make_pi"
    print pi
    print "\n"
.end
CODE
3.14159
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "immediate code as const - obj" );
.sub make_obj :immediate :anon
    .local pmc cl, o
    cl = newclass "Foo"
    addattribute cl, 'x'
    o = new 'Foo'
    $P0 = new 'String'
    $P0 = "ok 1\n"
    setattribute o, 'x', $P0
    .return (o)
.end

.sub main :main
    .const .Sub o = "make_obj"
    $P0 = getattribute o, 'x'
    print $P0
.end

CODE
ok 1
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "__get_regs_used 1" );
.sub main :main
    .local pmc m
    .include "interpinfo.pasm"
    m = interpinfo .INTERPINFO_CURRENT_SUB
    $I0 = m."__get_regs_used"('N')
    print $I0
    $I0 = m."__get_regs_used"('I')
    print $I0
    $I0 = m."__get_regs_used"('S')
    print $I0
    $I0 = m."__get_regs_used"('P')
    print $I0
    print "\n"
.end

CODE
0101
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "__get_regs_used 2" );
.sub main :main
    foo()
.end
.sub foo
    .local pmc m
    .include "interpinfo.pasm"
    m = interpinfo .INTERPINFO_CURRENT_SUB
    set N0, 1.0
    set N7, 1.0
    add N7, N7, N0
    set I9, 1
    add I10, I9, I9
    $I0 = m."__get_regs_used"('N')
    print $I0
    $I0 = m."__get_regs_used"('I')
    print $I0
    $I0 = m."__get_regs_used"('S')
    print $I0
    $I0 = m."__get_regs_used"('P')
    print $I0
    print "\n"
.end


CODE
2301
OUTPUT

pir_output_like(
    <<"CODE", <<'OUTPUT', 'warn on in main', todo => "RT#46819 core undef doesn't warn here. Should it?" );
.sub 'test' :main
.include "warnings.pasm"
    warningson .PARROT_WARNINGS_UNDEF_FLAG
    _f1()
.end
.sub _f1
    P0 = new 'Undef'
    print P0
.end
CODE
/uninit/
OUTPUT

pir_output_is( <<"CODE", <<'OUTPUT', 'warn on in sub' );
.sub 'test' :main
.include "warnings.pasm"
    _f1()
    P0 = new 'Undef'
    print P0
    print "ok\\n"
.end
.sub _f1
    warningson .PARROT_WARNINGS_UNDEF_FLAG
.end
CODE
ok
OUTPUT

pir_output_like(
    <<"CODE", <<'OUTPUT', 'warn on in sub, turn off in f2', todo => "RT#46819 core undef doesn't warn here. Should it?" );
.sub 'test' :main
.include "warnings.pasm"
    _f1()
    P0 = new 'Undef'
    print "back\\n"
    print P0
    print "ok\\n"
.end
.sub _f1
    warningson .PARROT_WARNINGS_UNDEF_FLAG
    _f2()
    P0 = new 'Undef'
    print P0
.end
.sub _f2
    warningsoff .PARROT_WARNINGS_UNDEF_FLAG
.end
CODE
/uninit.*\n.*\nback\nok/
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', ':postcomp' );
.sub main :main
    print "main\n"
.end
.sub pc :postcomp
    print "pc\n"
.end
.sub im :immediate
    print "im\n"
.end
.sub pc2 :postcomp
    print "pc2\n"
.end
.sub im2 :immediate
    print "im2\n"
.end
CODE
im
im2
pc
pc2
main
OUTPUT

# see also #38964
pir_output_is( <<'CODE', <<'OUTPUT', 'unicode sub names, compilation' );
.sub unicode:"\u7777"
   print "ok\n"
.end
CODE
ok
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'unicode sub names, invocation' );
.sub unicode:"\u7777"
    print "ok\n"
.end

.sub test :main
    unicode:"\u7777"()
.end
CODE
ok
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'unicode sub names, dynamic' );
.sub unicode:"\u7777"
    print "ok\n"
.end

.sub test :main
    $P1 = find_name unicode:"\u7777"
    $P1()
.end
CODE
ok
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'unicode sub names' );
.sub unicode:"\u7777"
    print "ok\n"
.end

.sub test :main
    # unicode:"\u7777" ends up as a string nicode:"\u7777
    # (or it did, in r12860)
    $P1 = find_name 'nicode:"\u7777'
    unless null $P1 goto bad
    print "not found\n"
  bad:
.end
CODE
not found
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'unicode sub constant' );
.sub main :main
    .const .Sub s = unicode:"\u7777"
    s()
.end

.sub unicode:"\u7777"
   print "ok\n"
.end
CODE
ok
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'literal \u in sub name (not unicode)' );
.sub '\u2193'
    say 'ok'
.end
CODE
ok
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'load_bytecode with .pir (RT #39807)' );
.sub main :main
    load_bytecode 'PGE.pbc'
    load_bytecode 'dumper.pir'
    load_bytecode 'PGE/Dumper.pir'

    $P0 = compreg 'PGE::P5Regex'
    $P1 = $P0('aabb*')
    $P2 = $P1('fooaabbbar')

    _dumper($P2)
.end
CODE
"VAR1" => PMC 'PGE::Match' => "aabbb" @ 3
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'load_bytecode with .pbc (RT #39807)' );
.sub main :main
    load_bytecode 'PGE.pbc'
    load_bytecode 'dumper.pbc'
    load_bytecode 'PGE/Dumper.pbc'

    $P0 = compreg 'PGE::P5Regex'
    $P1 = $P0('aabb*')
    $P2 = $P1('fooaabbbar')

    _dumper($P2)
.end
CODE
"VAR1" => PMC 'PGE::Match' => "aabbb" @ 3
OUTPUT

pir_error_output_like( <<'CODE', qr/Null PMC access in invoke()/, 'invoking null pmc' );
.sub main :main
    null $P0
    $P0()
.end
CODE

pir_output_is( <<'CODE', <<'OUTPUT', ":init" );
.sub a :init
    print "in inited\n"
.end

.sub main :main
    print "main\n"
.end
CODE
in inited
main
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'assign' );
.sub main :main
    $P0 = get_global 'ok'

    $P1 = new 'Undef'
    assign $P1, $P0

    $P1()
.end

.sub ok
    say "ok"
.end
CODE
ok
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'assign w/:outer' );
.sub main :main
    $P0 = get_global 'ok'

    $P1 = new 'Undef'
    assign $P1, $P0

    $P1()
.end

.sub ok :outer('main')
    say "ok"
.end
CODE
ok
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'get_namespace()' );
.sub main :main
    $P0 = get_global 'main'
    $P0 = $P0.'get_namespace'()
    $P0 = $P0.'get_name'()
    $S0 = join ';', $P0
    say $S0

    $P0 = get_global ['Foo'; 'Bar'], 'foo'
    $P0 = $P0.'get_namespace'()
    $P0 = $P0.'get_name'()
    $S0 = join ';', $P0
    say $S0
.end

.namespace ['Foo'; 'Bar']
.sub foo
    noop
.end
CODE
parrot
parrot;Foo;Bar
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', 'arity()' );
.sub main :main
    $P0 = get_global 'none'
    $I0 = $P0.arity()
    say $I0

    $P0 = get_global 'one'
    $I0 = $P0.arity()
    say $I0

    $P0 = get_global 'four'
    $I0 = $P0.arity()
    say $I0

    $P0 = get_global 'all_slurpy'
    $I0 = $P0.arity()
    say $I0

    $P0 = get_global 'some_optional'
    $I0 = $P0.arity()
    say $I0

    $P0 = get_global 'some_named'
    $I0 = $P0.arity()
    say $I0

    $P0 = get_global 'allsorts'
    $I0 = $P0.arity()
    say $I0
.end

.sub none
.end

.sub one
    .param int a
.end

.sub four
    .param int a
    .param pmc b
    .param string c
    .param num d
.end

.sub all_slurpy
    .param pmc s :slurpy
.end

.sub some_optional
    .param int a
    .param int b :optional
    .param int bo :opt_flag
    .param int c
.end

.sub some_named
    .param int a :named
    .param int b
    .param int c :named
.end

.sub allsorts
    .param int a :named
    .param int b :optional
    .param int bo :opt_flag
    .param int c
    .param pmc s :slurpy
.end
CODE
0
1
4
0
2
3
2
OUTPUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
