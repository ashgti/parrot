#!/usr/bin/perl

use strict;
use lib qw(t . lib ../lib ../../lib ../../../lib);
use Parrot::Test tests => 4;

language_output_is('cardinal', <<'CODE', <<'OUT', 'simple conditional test');
if true
1
end

CODE
OUT

language_output_is('cardinal', <<'CODE', <<'OUT', 'simple conditional test');
if true then puts "TRUE" end
CODE
OUT

language_output_is('cardinal', <<'CODE', <<'OUT', 'simple conditional test');
puts("Condition is True") if true
CODE
OUT

language_output_is('cardinal', <<'CODE', <<'OUT', 'simple conditional test');
if true
  puts("Its true")
end
CODE
OUT
