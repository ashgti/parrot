#!perl
use strict;
use TestCompiler tests => 19;

##############################
# Parrot Calling Conventions

output_is(<<'CODE', <<'OUT', "basic syntax - invokecc, constants");
.sub _main
    .local Sub sub
    newsub sub, .Sub, _sub
    .const int y = 20
    .pcc_begin prototyped
    .arg 10
    .arg y
    .pcc_call sub
    ret:
    .pcc_end
    end
.end
.pcc_sub _sub prototyped
    .param int a
    .param int b
    print a
    print "\n"
    print b
    print "\n"
    end
.end
CODE
10
20
OUT

##############################
# tail recursion - caller saves - parrot calling convention
output_is(<<'CODE', <<'OUT', "tail recursive sub");
.sub _main
    .local int count
    count = 5
    .local int product
    product = 1
    .local Sub sub
    .local Continuation cc
    newsub sub, .Sub, _fact
    newsub cc, .Continuation, ret
   .pcc_begin prototyped
   .arg product
   .arg count
   .pcc_call sub, cc
 ret:
   .local int result
    .result result
    .pcc_end
    print result
    print "\n"
    end
.end

.pcc_sub _fact prototyped
   .param int product
   .param int count
   if count <= 1 goto fin
   product = product * count
   dec count
   invoke
fin:
   .pcc_begin_return
    .return product
   .pcc_end_return
.end

CODE
120
OUT

output_is(<<'CODE', <<'OUT', "proto call, proto sub, invokecc, P param");
.sub _main
    .local Sub sub
    newsub sub, .Sub, _sub
    $P0 = new PerlUndef
    $P0 = "ok 1\n"
    $P1 = new PerlUndef
    $P1 = "ok 2\n"
    .pcc_begin prototyped
    .arg $P0
    .arg $P1
    .pcc_call sub
    ret:
    .pcc_end
    print "back\n"
    end
.end
.pcc_sub _sub prototyped
    .param PerlUndef a
    .param PerlUndef b
    print a
    print b
   .pcc_begin_return
   .pcc_end_return
.end
CODE
ok 1
ok 2
back
OUT
output_is(<<'CODE', <<'OUT', "proto call, un proto sub, invokecc, P param");
.sub _main
    .local Sub sub
    newsub sub, .Sub, _sub
    $P0 = new PerlUndef
    $P0 = "ok 1\n"
    $P1 = new PerlUndef
    $P1 = "ok 2\n"
    .pcc_begin prototyped
    .arg $P0
    .arg $P1
    .pcc_call sub
    ret:
    .pcc_end
    print "back\n"
    end
.end
.pcc_sub _sub
    .param PerlUndef a
    .param PerlUndef b
    print a
    print b
   .pcc_begin_return
   .pcc_end_return
.end
CODE
ok 1
ok 2
back
OUT

output_is(<<'CODE', <<'OUT', "proto call, proto sub, invokecc, S param");
.sub _main
    .local Sub sub
    newsub sub, .Sub, _sub
    $S0 = "ok 1\n"
    $S1 = "ok 2\n"
    .pcc_begin prototyped
    .arg $S0
    .arg $S1
    .pcc_call sub
    ret:
    .pcc_end
    print "back\n"
    end
.end
.pcc_sub _sub prototyped
    .param string a
    .param string b
    print a
    print b
   .pcc_begin_return
   .pcc_end_return
.end
CODE
ok 1
ok 2
back
OUT

output_is(<<'CODE', <<'OUT', "proto call, nonproto sub, invokecc, S param");
.sub _main
    .local Sub sub
    newsub sub, .Sub, _sub
    $S0 = "ok 1\n"
    $S1 = "ok 2\n"
    .pcc_begin non_prototyped
    .arg $S0
    .arg $S1
    .pcc_call sub
    ret:
    .pcc_end
    print "back\n"
    end
.end
.pcc_sub _sub non_prototyped
    .param string a
    .param string b
    print a
    print b
   .pcc_begin_return
   .pcc_end_return
.end
CODE
ok 1
ok 2
back
OUT

output_is(<<'CODE', <<'OUT', "proto call, unproto sub, invokecc, S param");
.sub _main
    .local Sub sub
    newsub sub, .Sub, _sub
    $S0 = "ok 1\n"
    $S1 = "ok 2\n"
    .pcc_begin prototyped
    .arg $S0
    .arg $S1
    .pcc_call sub
    ret:
    .pcc_end
    print "back\n"
    end
.end
.pcc_sub _sub
    .param string a
    .param string b
    print a
    print b
   .pcc_begin_return
   .pcc_end_return
.end
CODE
ok 1
ok 2
back
OUT

output_is(<<'CODE', <<'OUT', "non_proto call, unproto sub, invokecc, S param");
.sub _main
    .local Sub sub
    newsub sub, .Sub, _sub
    $S0 = "ok 1\n"
    $S1 = "ok 2\n"
    .pcc_begin non_prototyped
    .arg $S0
    .arg $S1
    .pcc_call sub
    ret:
    .pcc_end
    print "back\n"
    end
.end
.pcc_sub _sub
    .param string a
    .param string b
    print a
    print b
   .pcc_begin_return
   .pcc_end_return
.end
CODE
ok 1
ok 2
back
OUT

output_is(<<'CODE', <<'OUT', "wrong param count exception");
.sub _main
    .local Sub sub
    newsub sub, .Sub, _sub
    .pcc_begin non_prototyped
    .arg $S0
    .pcc_call sub
    ret:
    .pcc_end
    print "back\n"
    end
.end

.pcc_sub _sub
    .param string k
    .param string l
    print k
    print l
   .pcc_begin_return
   .pcc_end_return
.end
CODE
wrong param count
OUT

output_is(<<'CODE', <<'OUT', "wrong param count exception, call 2 subs");
.sub _main
    .local Sub sub
    newsub sub, .Sub, _sub
    $S0 = "ok 1\n"
    $S1 = "ok 2\n"
    .pcc_begin non_prototyped
    .arg $S0
    .arg $S1
    .pcc_call sub
    ret:
    .pcc_end
    newsub sub, .Sub, _sub2
    .pcc_begin non_prototyped
    .arg $S0
    .pcc_call sub
    ret2:
    .pcc_end
    print "back\n"
    end
.end

.pcc_sub _sub
    .param string k
    .param string l
    print k
    print l
   .pcc_begin_return
   .pcc_end_return
.end

.pcc_sub _sub2
    .param string k
    .param string l
    print k
    print l
   .pcc_begin_return
   .pcc_end_return
.end
CODE
ok 1
ok 2
wrong param count
OUT


output_is(<<'CODE', <<'OUT', "wrong param count exception, catch it");
.sub _main
    .local Sub ex_handler
    newsub ex_handler, .Exception_Handler, _handler
    set_eh ex_handler
    .local Sub sub
    newsub sub, .Sub, _sub
    .pcc_begin non_prototyped
    .arg $S0
    .pcc_call sub
    ret:
    .pcc_end
    print "back\n"
    end
.end

.sub _handler
    set S0, P5["_message"]	# P5 is the exception object
    eq S0, "wrong param count", ok
    print "not "
ok:
    print "ok\n"
    end
.end

.pcc_sub _sub
    .param string k
    .param string l
    print k
    print l
   .pcc_begin_return
   .pcc_end_return
.end
CODE
ok
OUT

output_is(<<'CODE', <<'OUT', "wrong param type exception");
.sub _main
    .local Sub sub
    newsub sub, .Sub, _sub
    .pcc_begin non_prototyped
    .arg $S0
    .pcc_call sub
    ret:
    .pcc_end
    print "back\n"
    end
.end

.pcc_sub _sub
    .param int k
    print k
    print "n"
   .pcc_begin_return
   .pcc_end_return
.end
CODE
wrong param type
OUT

output_is(<<'CODE', <<'OUT', "wrong param type exception - 2 params");
.sub _main
    .local Sub sub
    $S0 = "ok 1\n"
    newsub sub, .Sub, _sub
    .pcc_begin non_prototyped
    .arg $S0
    .arg $I0
    .pcc_call sub
    ret:
    .pcc_end
    print "back\n"
    end
.end

.pcc_sub _sub
    .param string k
    .param string l
    print k
    print l
   .pcc_begin_return
   .pcc_end_return
.end
CODE
wrong param type
OUT

####################
# coroutine iterator
#

# pseudo source code:
#	main () {
#	  int i=5;
#	  foreach addtwo(i) {
#	    print $_, "\n";
#	  }
#	  print "done in main\n";
#	}
#
#	addtwo (int a) {
#	  int i;
#	  for (i=0; i<10; i++) {
#	    yield a+i;
#	  }
#	  print "done in coroutine\n";
#	}

output_is(<<'CODE', <<'OUT', "coroutine iterator");
.sub _main
  .local int i
  i=5
  newsub $P0, .Coroutine, _addtwo
  newsub $P1, .Continuation, after_loop
  .pcc_begin prototyped
  .arg $P1
  .arg i
  .pcc_call $P0
 ret_addr:
  .result $I2
  .pcc_end
    print $I2
    print "\n"
    savetop
    invoke
    goto ret_addr
    restoretop
 after_loop:
  print "done in main\n"
  end
.end

.pcc_sub _addtwo
  .param Continuation when_done
  .param int a
  .local int i
  i = 0
 loop:
    if i >= 10 goto done
    $I5 = a+i
    .pcc_begin_yield
    .return $I5
    .pcc_end_yield
    i = i + 1
    goto loop
 done:
  print "done in coroutine\n"
  invoke when_done
  end
.end
CODE
5
6
7
8
9
10
11
12
13
14
done in coroutine
done in main
OUT

output_is(<<'CODE', <<'OUT', "sub calling another sub, SRegs");
.sub _main
    .local Sub sub
    newsub sub, .Sub, _sub
    $S0 = "ok 1\n"
    $S1 = "ok 2\n"
    .pcc_begin prototyped
    .arg $S0
    .arg $S1
    .pcc_call sub
    ret:
    .pcc_end
    print "back\n"
    end
.end
.pcc_sub _sub
    .param string a
    .param string b
    print a
    print b
    .local Sub sub
    newsub sub, .Sub, _sub2
    $S0 = "ok 3\n"
    $S1 = "ok 4\n"
    .pcc_begin prototyped
    .arg $S0
    .arg $S1
    .pcc_call sub
    ret:
    .pcc_end
    print a
    print b
   .pcc_begin_return
   .pcc_end_return
.end
.pcc_sub _sub2
    .param string a
    .param string b
    print a
    print b
   .pcc_begin_return
   .pcc_end_return
.end
CODE
ok 1
ok 2
ok 3
ok 4
ok 1
ok 2
back
OUT

output_is(<<'CODE', <<'OUT', "sub calling another sub, PRegs");
.sub _main
    .local Sub sub
    newsub sub, .Sub, _sub
    $P0 = new PerlUndef
    $P1 = new PerlUndef
    $P0 = "ok 1\n"
    $P1 = "ok 2\n"
    .pcc_begin prototyped
    .arg $P0
    .arg $P1
    .pcc_call sub
    ret:
    .pcc_end
    print "back\n"
    end
.end
.pcc_sub _sub
    .param PerlUndef a
    .param PerlUndef b
    print a
    print b
    .local Sub sub
    newsub sub, .Sub, _sub2
    $P0 = new PerlUndef
    $P1 = new PerlUndef
    $P0 = "ok 3\n"
    $P1 = "ok 4\n"
    .pcc_begin prototyped
    .arg $P0
    .arg $P1
    .pcc_call sub
    ret:
    .pcc_end
    print a
    print b
   .pcc_begin_return
   .pcc_end_return
.end
.pcc_sub _sub2
    .param PerlUndef a
    .param PerlUndef b
    print a
    print b
   .pcc_begin_return
   .pcc_end_return
.end
CODE
ok 1
ok 2
ok 3
ok 4
ok 1
ok 2
back
OUT

output_is(<<'CODE', <<'OUT', "in, out different P param, 2 subs");
.sub _main
    .local Sub sub
    .local PerlUndef x
    x = new PerlUndef
    x = 42
    newsub sub, .Sub, _sub
    .pcc_begin prototyped
    .arg x
    .pcc_call sub
    ret:
    .local PerlUndef y
    .result y
    .pcc_end
    .local PerlUndef z
    z = y
    .pcc_begin prototyped
    .arg y
    .pcc_call sub
    ret2:
    .result y
    .pcc_end
    print x
    print "\n"
    print y
    print "\n"
    print z
    print "\n"
    end
.end
.pcc_sub _sub prototyped
    .param PerlUndef a
    .local PerlUndef res
    res = new PerlUndef
    res = a + 1
    .pcc_begin_return
    .return res
    .pcc_end_return
.end
CODE
42
44
43
OUT

output_is(<<'CODE', <<'OUT', "sub calling another");
# sub g() { return 42; }
# sub f() { return g(); }
# print f(), "\n"
# mostly generated from pirate.py

.sub __main__
    new_pad 0
    newsub $P0, .Sub, _sub0       # (genFunction:378)
    store_lex -1, 'g', $P0        # (genFunction:380)
    newsub $P1, .Sub, _sub1       # (genFunction:378)
    store_lex -1, 'f', $P1        # (genFunction:380)
    find_lex $P5, 'f'             # (callingExpression:325)
    newsub $P6, .Continuation, ret1# (callingExpression:331)
    .pcc_begin non_prototyped     # (callingExpression:332)
    .pcc_call $P5, $P6            # (callingExpression:335)
ret1:
    .result $P4                   # (callingExpression:338)
    .pcc_end                      # (callingExpression:339)
    print $P4                      # (visitPrint:394)
    print "\n"                    # (visitPrintnl:403)
    end                           # (compile:574)
.end

# g from line 1
.pcc_sub _sub0 non_prototyped
    .local object res0            # (visitReturn:528)
    res0 = new PerlInt            # (expressConstant:153)
    res0 = 42                     # (expressConstant:154)
    .pcc_begin_return             # (visitReturn:530)
    .return res0                  # (visitReturn:531)
    .pcc_end_return               # (visitReturn:532)
.end


# f from line 3
.pcc_sub _sub1 non_prototyped
    .local object res1            # (visitReturn:528)
    find_lex $P2, 'g'             # (callingExpression:325)
    newsub $P3, .Continuation, ret0# (callingExpression:331)
    .pcc_begin non_prototyped     # (callingExpression:332)
    .pcc_call $P2, $P3            # (callingExpression:335)
ret0:
    .result res1                  # (callingExpression:338)
    .pcc_end                      # (callingExpression:339)
    .pcc_begin_return             # (visitReturn:530)
    .return res1                  # (visitReturn:531)
    .pcc_end_return               # (visitReturn:532)
.end
CODE
42
OUT


output_is(<<'CODE', <<'OUT', "coroutine generator throwing exception");
## this is mainly from Michal Wallace
## generator_try_bug.imc

## this exposes a bug in parrot exception handling
## a function returns a function and that function
## throws an error. The error is not caught.

# count returns a generator, which counts down to zero and then
# throws an exception, which is caught in main

.sub __main__
    new_pad 0

    # count() returns a generator
    .local Sub count
    newsub count, .Sub, _count

    # here's where we'll store it
    .local object generator

    # call count and get the generator
    .local PerlInt start
    start = new PerlInt
    start = 3
    .pcc_begin non_prototyped
    .arg start
    .pcc_call count
ret0:
    .result generator
    .pcc_end


    ## HERE IS where we want the try block to start ####
    .local Sub handler
    newsub handler, .Exception_Handler, catch0
    set_eh handler


    # now call the generator, until that throws 'StopIteration'
loop:
    .pcc_begin non_prototyped
    .arg $P0
    .pcc_call generator
ret1:
    .result $P0
    .pcc_end
    print $P0
    print "\n"
    goto loop

    # end the "try" block (we never get here)
    clear_eh
    goto endtry0
catch0:
    print "caught it!\n"
endtry0:
    end
.end

# here is count(), which returns the generator
.pcc_sub _count non_prototyped
   .param PerlInt start
   .local object gen_fun
   .local object gen_obj
   store_lex -1, "start", start
   newsub gen_fun, .Coroutine, _count_g
   .pcc_begin_return
   .return gen_fun
   .pcc_end_return
.end

# here is the generator itself
# all it does is throw StopIteration
.pcc_sub _count_g non_prototyped
    .local PerlInt c
    find_lex c, -1, "start"
    lt c, 0, stop
    .pcc_begin_yield
    .return c
    .pcc_end_yield
    c = c - 1
    goto _count_g

stop:
    .local object ex0
    .local object msg0
    ex0 = new Exception
    msg0 = new PerlString
    msg0 = 'StopIteration'
    ex0['_message'] = msg0
    throw ex0
.end

## end of test case ###############################

CODE
3
2
1
0
caught it!
OUT

