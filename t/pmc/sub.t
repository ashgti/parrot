#! perl -w

use Parrot::Test tests => 47;
use Test::More;
use Parrot::Config;

output_is(<<'CODE', <<'OUTPUT', "PASM subs - newsub");
    print "main\n"
    newsub .Sub, .RetContinuation, _func, _ret
    invoke
_ret:
    print "back\n"
    end
_func:
    print "func\n"
    invoke P1
CODE
main
func
back
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "PASM subs - newsub 2");
    print "main\n"
    newsub P0, .Sub, _func
    invokecc
    print "back\n"
    end
_func:
    print "func\n"
    invoke P1
CODE
main
func
back
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "PASM subs - invokecc");
    new P0, .Sub
    set_addr P0, func

    set I5, 3
    save I5

    invokecc

    restore I5
    print I5
    print "\n"
    end

func:
    print I5
    print "\n"

    eq I5, 0, endfunc
    dec I5

    save P1
    invokecc   # recursive invoke
    restore P1

endfunc:
    invoke P1
CODE
3
2
1
0
3
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "PASM subs - invokecc_p");
    new P0, .Sub
    set_addr P0, func

    set I5, 3
    save I5

    invokecc P0

    restore I5
    print I5
    print "\n"
    end

func:
    print I5
    print "\n"

    eq I5, 0, endfunc
    dec I5

    save P1
    invokecc P0  # recursive invoke
    restore P1

endfunc:
    invoke P1
CODE
3
2
1
0
3
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "Continuation");
    new P5, .PerlInt
    set P5, 3
    store_global "foo", P5
    new P1, .Continuation
    set_addr P1, endcont
endcont:
    find_global P4, "foo"
    print "here "
    print P4
    print "\n"
    unless P4, done
    dec P4
    store_global "foo", P4
    print "going to cont\n"
    clone P0, P1
    invoke
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

output_is(<<'CODE', <<'OUTPUT', "definedness of Continuation");
    new P1, .Continuation
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

output_is(<<'CODE', <<'OUTPUT', "PASM sub as closure");
    # sub foo {
    #     my ($n) = @_;
    #     sub {$n += shift}
    # }
    # my $f = foo(5);
    # print &$f(3), "\n";
    # print &$f(3), "\n";
    # print &$f(3), "\n";
main:

    new P0, .Sub
    set_addr P0, foo

    new P5, .PerlInt
    set P5, 5

    invokecc
    set P0, P5	# the closure

    new P5, .PerlInt
    set P5, 3

    pushbottomp	# preserve P regs
    invokecc
    save P5	# result in P5
    popbottomp
    restore P2

    print P2
    print "\n"

    pushbottomp	# preserve P regs
    invokecc
    save P5	# result in P5
    popbottomp
    restore P2

    print P2
    print "\n"

    pushbottomp	# preserve P regs
    invokecc
    save P5	# result in P5
    popbottomp
    restore P2

    print P2
    print "\n"

    end

# foo takes a number n (P5) and returns a sub (in P5) that takes
# a number i (P5) and returns n incremented by i.
foo:
    new_pad 0
    store_lex 0, "n", P5
    new P5, .Closure	# P5 has now the lexical "n" in the pad
    set_addr P5, f
    invoke P1		# ret

# expects arg in P5, returns incremented result in P5
f:
    find_lex P2, "n"	# invoke-ing the Sub pushes the lexical pad
    			# of the closure on the pad stack
    add P2, P5		# n += shift, the lexical is incremented
    new P5, .PerlInt
    set P5, P2
    invoke P1		# ret

CODE
8
11
14
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "PASM subs - tail invoke");
    new P0, .Sub
    set_addr P0, func1

    invokecc
    print "done\n"
    end

func1:
    print "in func1\n"

    new P0, .Sub
    set_addr P0, func2

    invoke    # tail invoke (reuses context already in P1)
    print "this should not be called\n"
    end

func2:
    print "in func2\n"
    invoke P1

CODE
in func1
in func2
done
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "PASM subs - tail invoke with newsub");
    newsub P0, .Sub, func1

    invokecc
    print "done\n"
    end

func1:
    print "in func1\n"

    newsub P0, .Sub, func2

    invoke    # tail invoke (reuses context already in P1)
    print "this should not be called\n"

func2:
    print "in func2\n"
    invoke P1

CODE
in func1
in func2
done
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "sub calling a sub");
    print "main\n"
    newsub .Sub, .RetContinuation, _func1, ret1
    invoke
ret1:
    print "back\n"
    end

_func1:
    print "func1\n"
    pushbottomp
    newsub .Sub, .RetContinuation, _func2, ret2
    invoke
ret2:
    popbottomp
    print "func1\n"
    invoke P1

_func2:
    print "func2\n"
    invoke P1

CODE
main
func1
func2
func1
back
OUTPUT

for my $R (0..4) {
for my $N (254..258) {
output_is(<<"CODE", <<'OUTPUT', "test COW");
    print "main\\n"
    set I16, 0
    set I17, $N	 	#~1 chunk full
lp:
    save I16
    inc I16
    le I16, I17, lp
    newsub .Continuation, .RetContinuation, _func, ret
    pushbottomi
    invoke
    popbottomi
ret:
lp2:
    restore I16
    eq I16, I17, ok
    print "nok I16: "
    print I16
    print " I17: "
    print I17
    print "\\n"
    end
ok:
    dec I17
    if I17, lp2
    print "back\\n"
    end
_func:
    print "func\\n"
    set I0, $R
lp3:
    unless I0, cont
    save I0
    dec I0
    branch lp3
cont:
    invoke P1
CODE
main
func
back
OUTPUT
}
}

output_like(<<'CODE', <<'OUTPUT', "interp - warnings");
    new P0, .PerlUndef
    set I0, P0
    printerr "main:"
    newsub .Sub, .RetContinuation, _func, _ret
    invoke
_ret:
    printerr ":back"
    new P0, .PerlUndef
    set I0, P0
    end
_func:
    warningson 1
    new P0, .PerlUndef
    set I0, P0
    invoke P1
CODE
/^main:Use of uninitialized value in integer context at.*:back$/sm
OUTPUT

output_like(<<'CODE', <<'OUTPUT', "interp - warnings 2");
    warningson 1
    newsub .Sub, .RetContinuation, _func, ret
    new P10, .PerlUndef
    set I0, P10
    printerr ":main"
    invoke
ret:
    printerr ":back:"
    new P10, .PerlUndef
    set I0, P10
    printerr ":end"
    end
_func:
    warningsoff 1
    new P0, .PerlUndef
    set I0, P0
    invoke P1
CODE
/^Use of uninitialized value in integer context at.*:main:back:Use of un.*$/sm
OUTPUT

output_like(<<'CODE', <<'OUTPUT', "interp - warnings 2 - updatecc");
    # the RetContinuation in P1 is created with warnings off
    newsub .Sub, .RetContinuation, _func, ret
    # turn warnings on in main
    warningson 1
    new P10, .PerlUndef
    set I0, P10
    printerr ":main"
    # update the state of the return continuation
    updatecc
    invoke
ret:
    printerr ":back:"
    new P10, .PerlUndef
    set I0, P10
    printerr ":end"
    end
_func:
    # turn off warnings in the sub
    warningsoff 1
    new P0, .PerlUndef
    set I0, P0
    invoke P1
CODE
/^Use of uninitialized value in integer context at.*:main:back:Use of un.*$/sm
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "pcc sub");
    find_global P0, "_the_sub"
    defined I0, P0
    if I0, ok
    print "not "
ok:
    print "ok 1\n"
    invokecc
    print "back\n"
    end
.pcc_sub _the_sub:
    print "in sub\n"
    invoke P1
CODE
ok 1
in sub
back
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "pcc sub, tail call");
    find_global P0, "_the_sub"
    defined I0, P0
    if I0, ok
    print "not "
ok:
    print "ok 1\n"
    invokecc
    print "back\n"
    end

.pcc_sub _the_sub:
    print "in sub\n"
    find_global P0, "_next_sub"
    get_addr I0, P0
    jump I0
    print "never here\n"

.pcc_sub _next_sub:
    print "in next sub\n"
    invoke P1
    print "never here\n"
    end
CODE
ok 1
in sub
in next sub
back
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "pcc sub perl::syn::tax");
    find_global P0, "_the::sub::some::where"
    defined I0, P0
    if I0, ok
    print "not "
ok:
    print "ok 1\n"
    invokecc
    print "back\n"
    end
.pcc_sub _the::sub::some::where:
    print "in sub\n"
    invoke P1
CODE
ok 1
in sub
back
OUTPUT

my $temp = "temp.pasm";
open S, ">$temp" or die "Can't write $temp";
print S <<'EOF';
  .pcc_sub _sub1:
  print "in sub1\n"
  end
EOF
close S;

output_is(<<'CODE', <<'OUTPUT', "load_bytecode call sub");
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pasm"
    print "loaded\n"
    find_global P0, "_sub1"
    defined I0, P0
    if I0, ok1
    print "not "
ok1:
    print "found sub\n"
    invoke
    print "never\n"
    end
CODE
main
loaded
found sub
in sub1
OUTPUT

open S, ">$temp" or die "Can't write $temp";
print S <<'EOF';
  .pcc_sub _sub1:
  print "in sub1\n"
  invoke P1
EOF
close S;

output_is(<<'CODE', <<'OUTPUT', "load_bytecode call sub, ret");
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pasm"
    print "loaded\n"
    find_global P0, "_sub1"
    defined I0, P0
    if I0, ok1
    print "not "
ok1:
    print "found sub\n"
    invokecc
    print "back\n"
    end
CODE
main
loaded
found sub
in sub1
back
OUTPUT

open S, ">$temp" or die "Can't write $temp";
print S <<'EOF';
  .pcc_sub _sub1:
  print "in sub1\n"
  invoke P1
  .pcc_sub _sub2:
  print "in sub2\n"
  invoke P1
EOF
close S;

output_is(<<'CODE', <<'OUTPUT', "load_bytecode call different subs, ret");
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pasm"
    print "loaded\n"
    find_global P0, "_sub1"
    defined I0, P0
    if I0, ok1
    print "not "
ok1:
    print "found sub1\n"
    set P10, P0
    invokecc
    print "back\n"
    find_global P0, "_sub2"
    defined I0, P0
    if I0, ok2
    print "not "
ok2:
    print "found sub2\n"
    invokecc
    print "back\n"
    set P0, P10
    invokecc
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

output_is(<<'CODE', <<'OUTPUT', "load_bytecode PBC call different subs, ret");
.pcc_sub _main:
    print "main\n"
    load_bytecode "temp.pbc"
    print "loaded\n"
    find_global P0, "_sub1"
    defined I0, P0
    if I0, ok1
    print "not "
ok1:
    print "found sub1\n"
    set P10, P0
    invokecc
    print "back\n"
    find_global P0, "_sub2"
    defined I0, P0
    if I0, ok2
    print "not "
ok2:
    print "found sub2\n"
    invokecc
    print "back\n"
    set P0, P10
    invokecc
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

output_is(<<'CODE', <<'OUTPUT', "equality of closures");
      new P0, .Closure
      set_addr P0, f1
      clone P1, P0
      eq P0, P1, OK1
      print "not "
OK1:  print "ok 1\n"

      new P2, .Closure
      set_addr P2, f2
      eq P0, P2, BAD2
      branch OK2
BAD2: print "not "
OK2:  print "ok 2\n"
      end

f1:
      print "Test\n"
      end

f2:
      new P1, .PerlUndef
      end
CODE
ok 1
ok 2
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "equality of subs");
      new P0, .Sub
      set_addr P0, f1
      clone P1, P0
      eq P0, P1, OK1
      print "not "
OK1:  print "ok 1\n"

      new P2, .Sub
      set_addr P2, f2
      eq P0, P2, BAD2
      branch OK2
BAD2: print "not "
OK2:  print "ok 2\n"
      end

f1:
      print "Test\n"
      end

f2:
      new P1, .PerlUndef
      end
CODE
ok 1
ok 2
OUTPUT

unlink($temp, 'temp.pbc');


