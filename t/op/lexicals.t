#! perl -w

use Parrot::Test tests => 6;

output_is(<<CODE, <<OUTPUT, "simple store and fetch");
	new_pad 0
	new P0, .PerlInt
	new P1, .PerlInt
	set P0, 12
	set P1, 7
	store_lex 0, "Integer", P0
	find_lex P1, "Integer"
	print P1
        print "\\n"
	find_lex P1, 0, "Integer"
	print P1
        print "\\n"
	end
CODE
12
12
OUTPUT

output_is(<<CODE, <<OUTPUT, "Repeated stores with the same key");
	new_pad 0
	new P0, .PerlInt
	new P1, .PerlInt
        set I0, 0
LOOP:
	set P0, I0
	store_lex 0, "a", P0
	find_lex P1, "a"
	print P1
        print "\\n"
        inc I0
        lt I0, 10, LOOP
	end
CODE
0
1
2
3
4
5
6
7
8
9
OUTPUT

output_is(<<CODE, <<OUTPUT, "nested scopes");
  new P0, .PerlInt
  new P1, .PerlInt
  new P2, .PerlInt
  new P3, .PerlInt
  set P0, 0
  set P1, 1
  set P2, 2
  
  # outer most lexical scope
  new_pad 0
  store_lex 0, "a", P0
  find_lex P3, "a"
  print P3
  print "\\n"

    new_pad 1
    store_lex 1, "a", P1

      new_pad 2
      store_lex 2, "c", P2
      find_lex P3, "c"
      print P3
      print "\\n"
      pop_pad

    find_lex P3, "a"
    print P3
    print "\\n"

    pop_pad

  find_lex P3, "a"
  print P3
  print "\\n"
  end
CODE
0
2
1
0
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "Replacing a scope at the same depth.");
# sub f($x) {
#     sub g($y) {
#         my $foo = $y
#         if $y > 0 { f $y - 1 }
#         $outer::bar = $foo - 1
#     }
#     $foo = $x + 1
#     g $x
#     print $foo, $bar
# }

	new P3, .PerlInt
	set P3, 2
	save P3
	bsr sub1
	end

sub2:
	saveall
	new_pad 1
	restore P1
	new P2, .PerlInt
	set P2, 0
	clone P3, P1
	dec P3
	store_lex 1, "foo", P3
	save P3
	eq P1, P2, done
	bsr sub1
done:
	find_lex P2, "foo"
	clone P3, P2
	store_lex 0, "bar", P3
	pop_pad
	restoreall
	ret

sub1:
	saveall
	new_pad 0
	restore P0
	clone P3, P0
	inc P3
	store_lex 0, "foo", P3
	save P0
	find_lex P2, "foo"
	print P2
	print " before sub2\n"
	bsr sub2
	find_lex P2, "foo"
	print P2
	print " = foo after sub2\n"
	find_lex P2, "bar"
	print P2
	print " = bar after sub2\n"
	pop_pad
	restoreall
	ret

CODE
3 before sub2
2 before sub2
1 before sub2
1 = foo after sub2
-1 = bar after sub2
2 = foo after sub2
0 = bar after sub2
3 = foo after sub2
1 = bar after sub2
OUTPUT

output_is(<<CODE, <<OUTPUT, "pad stack ops");
	new_pad P10, 0
        new_pad P11, 0
        new_pad P12, 0

	new P0, .PerlInt
	new P1, .PerlInt
	new P2, .PerlInt
	set P0, 12
	set P1, 7
	set P2, 46

        set P10[0;"a"], P0
        set P11[0;"a"], P1
        set P12[0;"a"], P2

        push_pad P10
        find_lex P20, "a"
        print P20
        print "\\n"

        push_pad P11
        find_lex P20, "a"
        print P20
        print "\\n"

        push_pad P12
        find_lex P20, "a"
        print P20
        print "\\n"

        pop_pad
        find_lex P20, "a"
        print P20
        print "\\n"

        peek_pad P22
        pop_pad
        find_lex P20, "a"
        print P20
        print "\\n"

        set P20, P22["a"]
        print P20
        print "\\n"

	end
CODE
12
7
46
7
12
7
OUTPUT
 
output_is(<<CODE, <<OUTPUT, "access by position");
	new_pad 0
        new_pad 1
        new_pad 2

	new P0, .PerlInt
	new P1, .PerlInt
	new P2, .PerlInt
	new P3, .PerlInt
	new P4, .PerlInt
	new P5, .PerlInt
	set P0, 10
	set P1, 11
	set P2, 12
	set P3, 13
	set P4, 14
	set P5, 15

        store_lex 0, "a", P0
        store_lex -3, 1, P1

        store_lex 1, 0, P2
        store_lex -2, 1, P3

        store_lex 2, 0, P4
        store_lex -1, "x", P5

        find_lex P6, "a"
        print P6
        print "\\n"
        find_lex P6, 0, "a"
        print P6
        print "\\n"

        find_lex P6, 0
        print P6
        print "\\n"
        find_lex P6, -1, 0
        print P6
        print "\\n"
        find_lex P6, 2, 1
        print P6
        print "\\n"

        find_lex P6, 1, 0
        print P6
        print "\\n"
        find_lex P6, -2, 0
        print P6
        print "\\n"
        find_lex P6, -2, 1
        print P6
        print "\\n"

        find_lex P6, -3, 0
        print P6
        print "\\n"
        find_lex P6, 0, 1
        print P6
        print "\\n"

        store_lex "a", P5
        find_lex P6, "a"
        print P6
        print "\\n"
        find_lex P6, 0, "a"
        print P6
        print "\\n"
        find_lex P6, 0, 0
        print P6
        print "\\n"

     

        end
CODE
10
10
14
14
15
12
12
13
10
11
15
15
15
OUTPUT

1;

