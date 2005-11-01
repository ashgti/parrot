# $Id$

use strict;
use warnings;
use Parrot::Test;
use Parrot::Test::PGE;


## tests based on http://dev.perl.org/perl6/doc/design/syn/S05.html, ver. 7
##   in the 'Modifiers' section


## :i
## TODO lexical scoping of :i
p6rule_is  ('abcdef', ':i bcd', 'ignorecase (:i)');
p6rule_is  ('aBcdef', ':i bcd', 'ignorecase (:i)');
p6rule_is  ('abCdef', ':i bcd', 'ignorecase (:i)');
p6rule_is  ('abcDef', ':i bcd', 'ignorecase (:i)');
p6rule_isnt('abc-ef', ':i bcd', 'ignorecase (:i)');
p6rule_is  ('abcdef', ':ignorecase bcd', 'ignorecase (:ignorecase)');
p6rule_is  ('aBCDef', ':ignorecase bcd', 'ignorecase (:ignorecase)');
p6rule_isnt('abc-ef', ':ignorecase bcd', 'ignorecase (:ignorecase)');


## :w
## TODO lexical scoping of :w
p6rule_is  ('a bcdef', ':w bcd', 'words (:w)');
p6rule_is  ('a bcd ef', ':w bcd', 'words (:w)');
p6rule_isnt('abcdef', ':w bcd', 'words (:w)');
p6rule_isnt('abcd ef', ':w bcd', 'words (:w)');
p6rule_isnt('ab cdef', ':w bcd', 'words (:w)');
p6rule_is  ('a b c d ef', ':w b c d', 'words (:w)');
p6rule_is  ('a b c def', ':w b c d', 'words (:w)');
p6rule_isnt('ab c d ef', ':w b c d', 'words (:w)');
p6rule_isnt('a bcdef', ':w b c d', 'words (:w)');
p6rule_isnt('abcdef', ':w b c d', 'words (:w)');
p6rule_is  ('a bcdef', ':words bcd', 'words (:words)');
p6rule_is  ('a bcd ef', ':words bcd', 'words (:words)');
p6rule_isnt('abcdef', ':words bcd', 'words (:words)');
p6rule_is  ('a b c d ef', ':words b c d', 'words (:words)');
p6rule_is  ('a b c def', ':words b c d', 'words (:words)');
p6rule_isnt('ab c d ef', ':words b c d', 'words (:words)');


## :once


## TODO :c, :p, :g, :bytes, :codes, :graphs, :langs, :perl5,
##   integer modifiers, Nth occurance, :ov, :ex, :rw, :keepall
##   user-defined modifiers


## remember to change the number of tests :-)
BEGIN { plan tests => 24; }
