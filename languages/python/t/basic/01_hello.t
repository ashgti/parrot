# $Id$

use strict;

use Parrot::Test tests => 5;

language_output_is( 'python', <<'CODE', '', 'hello' );
print "hello pie-thon"
CODE

language_output_is( 'python', <<'CODE', '', 'print_i 1' );
print 1
CODE

language_output_is( 'python', <<'CODE', '', 'print_item' );
print "hello", "pie-thon"
CODE

language_output_is( 'python', <<'CODE', '', 'print_i 1, 2' );
print 1, 2
CODE

language_output_is( 'python', <<'CODE', '', 'print_n' );
print 0.1, 0.2
CODE
