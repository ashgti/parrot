#! perl -w
# Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

t/pmc/io.t - IO Ops

=head1 SYNOPSIS

	% perl t/pmc/io.t

=head1 DECSRIPTION

Tests the Parrot IO operations.

=cut

use Parrot::Test tests => 21;
use Test::More;

output_is(<<'CODE', <<'OUTPUT', "open/close");
	open P0, "temp.file", ">"
	print P0, "a line\n"
	close P0
	open P0, "temp.file", "<"
	read S0, P0, 20
	print S0
	end
CODE
a line
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "timely destruction");
	interpinfo I0, 2	# DOD runs
	open P0, "temp.file", ">"
        needs_destroy P0
	print P0, "a line\n"
	null P0			# kill it
	sweep 0			# a lazy DOD has to close the PIO
	open P0, "temp.file", "<"
	read S0, P0, 20
	print S0
	interpinfo I1, 2	# DOD runs
	sub I1, I0
	print I1
	print "\n"
	end
CODE
a line
1
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "getfd/fdopen");
        getstdout P0
        getfd I0, P0
	fdopen P1, I0, ">"
	defined I0, P1
	unless I0, nok
	print P1, "ok\n"
	close P1
	end
nok:
	print "fdopen failed\n"
	end
CODE
ok
OUTPUT

output_is(<<'CODE', <<'OUTPUT', "fdopen - no close");
        getstdout P0
        getfd I0, P0
	fdopen P1, I0, ">"
	defined I0, P1
	unless I0, nok
	print P1, "ok\n"
	end
nok:
	print "fdopen failed\n"
	end
CODE
ok
OUTPUT

unlink "no_such_file" if (-e "no_such_file");

output_is(<<'CODE', <<'OUTPUT', "get_bool");
	open P0, "no_such_file", "<"
	unless P0, ok1
	print "Huh: 'no_such_file' exists? - not "
ok1:
	print "ok 1\n"
	open P0, "temp.file", "<"
	if P0, ok2
	print "not "
ok2:	print "ok 2\n"
	read S0, P0, 1024
	read S0, P0, 1024
	unless P0, ok3
	print "not "
ok3:	print "ok 3\n"
	defined I0, P0
	if I0, ok4
	print "not "
ok4:	print "ok 4\n"
	close P0
	defined I0, P0		# closed file is still defined
	if I0, ok5
	print "not "
ok5:	print "ok 5\n"
	unless P0, ok6		# but false
	print "not "
ok6:	print "ok 6\n"
	end
CODE
ok 1
ok 2
ok 3
ok 4
ok 5
ok 6
OUTPUT


output_is(<<'CODE', <<'OUTPUT', "read on invalid fh should throw exception");
	open P0, "no_such_file", "<"
	unless P0, ok1
	print "Huh: 'no_such_file' exists? - not "
ok1:
	print "ok 1\n"

        newsub P20, .Exception_Handler, _readline_handler
        set_eh P20
        readline S0, P0 # Currently segfaults
        print "not "
_readline_handler:
        print "ok 2\n"
	branch fin

        newsub P20, .Exception_Handler, _read_handler
        set_eh P20
        read S0, P0, 1
        print "not "
_read_handler:
        print "ok 3\n"

        newsub P20, .Exception_Handler, _print_handler
        set_eh P20
        print P0, "kill me now\n"
        print "not "
_print_handler:
fin:
        print "ok 4\n"

        end
CODE
ok 1
ok 2
ok 4
OUTPUT

SKIP: {
    skip ("clone not finished yet", 1);
output_is(<<'CODE', <<'OUTPUT', "clone");
	open P0, "temp.file", "<"
	clone P1, P0
	read S0, P1, 1024
	print S0
	end
CODE
a line
OUTPUT
}

# It would be very embarrassing if these didnt work...
open FOO, ">temp.file";
print FOO "2\n1\n";
close FOO;
output_is(<<'CODE', <<'OUTPUT', "open and readline");
	open P0, "temp.file"
	set S0, ""
	set S1, ""
	readline S0, P0
	readline S1, P0
	print S1
	print S0
	end
CODE
1
2
OUTPUT

open FOO, ">temp.file";  # Clobber previous contents
close FOO;

output_is(<<'CODE', <<'OUTPUT', "open & print");
       set I0, -12
       set N0, 2.2
       set S0, "Foo"
       new P0, .PerlString
       set P0, "Bar\n"

       open P1, "temp.file"
       print P1, I0
       print P1, N0
       print P1, S0
       print P1, P0
       close P1

       open P2, "temp.file"
       readline S1, P2
       close P2

       print S1
       end
CODE
-122.200000FooBar
OUTPUT

open FOO, ">temp.file";  # Clobber previous contents
close FOO;

# write to file opened for reading
output_is(<<'CODE', <<'OUTPUT', "3-arg open");
       open P1, "temp.file", "<"
       print P1, "Foobar\n"
       close P1

       open P3, "temp.file", "<"
       readline S1, P3
       close P3

       print S1
       print "writing to file opened for reading\n"
       end
CODE
writing to file opened for reading
OUTPUT

unlink("temp.file");

output_is(<<'CODE', <<'OUTPUT', 'open and close');
       open P1, "temp.file"
       print P1, "Hello, World!\n"
       close P1
       print "done\n"
       end
CODE
done
OUTPUT

$/=undef; # slurp mode
open FOO, "temp.file";

is(<FOO>, <<'OUTPUT', 'file contents');
Hello, World!
OUTPUT

close FOO;

output_is(<<'CODE', '', 'append');
       open P1, "temp.file", ">>"
       print P1, "Parrot flies\n"
       close P1
       end
CODE

open FOO, "temp.file";

is(<FOO>, <<'OUTPUT', 'append file contents');
Hello, World!
Parrot flies
OUTPUT

close FOO;

output_is(<<'CODE', '', 'write to file');
       open P1, "temp.file", ">"
       print P1, "Parrot overwrites\n"
       close P1
       end
CODE

open FOO, "temp.file";

is(<FOO>, <<'OUTPUT', 'file contents');
Parrot overwrites
OUTPUT

close FOO;

unlink("temp.file");

output_is(<<'CODE', <<'OUT', 'standard file descriptors');
       getstdin P0
       getfd I0, P0
       # I0 is 0 on Unix and non-Null on stdio and win32
       print "ok 1\n"
       getstdout P1
       getfd I1, P1
       if I1, OK_2
       print "not "
OK_2:  print "ok 2\n"
       getstderr P2
       getfd I2, P2
       if I2, OK_3
       print "not "
OK_3:  print "ok 3\n"
       end
CODE
ok 1
ok 2
ok 3
OUT

output_is(<<'CODE', <<'OUTPUT', 'printerr');
       new P0, .PerlString
       set P0, "This is a test\n"
       printerr 10
       printerr "\n"
       printerr 1.0
       printerr "\n"
       printerr "foo"
       printerr "\n"
       printerr P0
       end
CODE
10
1.000000
foo
This is a test
OUTPUT

output_is(<<'CODE', <<'OUTPUT', 'puts method');
       set S5, "ok 2\n"
       getstdout P5
       can I0, P5, "puts"
       if I0, ok1
       print "not "
ok1:   print "ok 1\n"
       find_method P0, P5, "puts"
       invoke
       end
CODE
ok 1
ok 2
OUTPUT

output_is(<<'CODE', <<'OUTPUT', 'callmethod puts');
       getstderr P2	# the object
       set S0, "puts"	# method
       set P5, P2	# first param
       set S5, "ok 1\n"	# 2nd param
       callmethod
       set S5, "ok 2\n"
       callmethod
       end
CODE
ok 1
ok 2
OUTPUT

output_is(<<'CODE', <<'OUTPUT', 'seek/tell');
       open P0, "temp.file", ">"
       print P0, "Hello "
       tell I0, P0
       print P0, "World!"
       seek P0, I0, 0
       print P0, "Parrot!\n"
       close P0
       print "ok 1\n"
       open P0, "temp.file", "<"
       read S0, P0, 65635
       print S0
       end
CODE
ok 1
Hello Parrot!
OUTPUT

unlink "temp.file";
