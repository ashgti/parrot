#!perl
use strict;
use TestCompiler tests => 4;

# these tests are run with -O2 by TestCompiler and show
# generated PASM code for various optimizations at level 2

##############################
output_is(<<'CODE', <<'OUT', "used once lhs");
.sub _main
	$I1 = 1
	$I2 = 2
	print $I2
	end
.end
CODE
_main:
	print 2
	end
OUT

##############################
output_is(<<'CODE', <<'OUT', "constant propogation and resulting dead code");
.sub _main
       set I0, 5
loop:
       set I1, 2
       add I0, I1
       lt I0, 20, loop
       print I0
       end
.end
CODE
_main:
  set I0, 5
loop:
  add I0, 2
  lt I0, 20, loop
  print I0
  end
OUT

##############################
output_is(<<'CODE', <<'OUT', "don't move constant past a label");
.sub _main
  set I1, 10
  set I0, 5
  lt I1, 20, nxt
add:
  add I0, I1, I1
  print I0
nxt:
  set I1, 20
  branch add
.end
CODE
_main:
  set I1, 10
  set I0, 5
  lt 10, 20, nxt
add:
  add I0, I1, I1
  print I0
nxt:
  set I1, 20
  branch add
OUT

##############################
output_is(<<'CODE', <<'OUT', "remove invariant from loop");
.sub _main
       set I0, 5
loop:
       set I1, 2
       add I0, I1
       lt I0, 20, loop
next:
       print I0
       add I0, I1
       print I0
       lt I1, 4, next
       end
.end
CODE
_main:
	set I0, 5
	set I1, 2
loop:
	add I0, 2
	lt I0, 20, loop
next:
	print I0
	add I0, I1
	print I0
	lt I1, 4, next
	end
OUT

