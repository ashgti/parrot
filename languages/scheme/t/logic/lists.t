#! perl
# $Id$

# Copyright (C) 2002-2007, The Perl Foundation.

use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../../lib";

use Test::More tests => 23;
use Parrot::Test;

language_output_is( 'Scheme', <<'CODE', '(2 . 5)', 'cons' );
(write (cons 2 5))
CODE

language_output_is( 'Scheme', <<'CODE', '((2 . 3) . 4)', 'cons car' );
(write (cons (cons 2 3) 4))
CODE

language_output_is( 'Scheme', <<'CODE', '(2 3 . 4)', 'cons cdr' );
(write (cons 2 (cons 3 4)))
CODE

language_output_is( 'Scheme', <<'CODE', '((1 . 2) 3 . 4)', 'complex cons' );
(write 
  (cons 
    (cons 1 2) 
    (cons 3 4)))
CODE

language_output_is( 'Scheme', <<'CODE', '(3 2 1 0)', 'list' );
(write
  (list 3 2 1 0))
CODE

language_output_is( 'Scheme', <<'CODE', '(1 2 3)', 'lists the hard way' );
(write
  (cons 1
    (cons 2
      (cons 3
        (list)))))
CODE

language_output_is( 'Scheme', <<'CODE', '4', 'length' );
(write
  (length (list 3 2 1 0)))
CODE

language_output_is( 'Scheme', <<'CODE', '2', 'car' );
(write
  (car (list 2 1 0)))
CODE

language_output_is( 'Scheme', <<'CODE', '(1 0)', 'cdr' );
(write
  (cdr (list 2 1 0)))
CODE

language_output_is( 'Scheme', <<'CODE', '(4 2 3)', 'set-car!' );
(write
  (set-car! (list 1 2 3) 4))
CODE

language_output_is( 'Scheme', <<'CODE', '((4 . 2) 2 3)', 'set-car! II' );
(write
  (set-car! (list 1 2 3) (cons 4 2)))
CODE

language_output_is( 'Scheme', <<'CODE', '(1 4 2)', 'set-cdr!' );
(write
  (set-cdr! (list 1 2 3) (list 4 2)))
CODE

language_output_is( 'Scheme', <<'CODE', '(1 2 3 4)', 'quoted list' );
(write '(1 2 3 4)) ; for emacs ')
CODE

language_output_is( 'Scheme', <<'CODE', '#t', 'null?' );
(write
  (null? (list)))
CODE

language_output_is( 'Scheme', <<'CODE', '()', "'()" );
(write '()) ; for emacs ')
CODE

language_output_is( 'Scheme', <<'CODE', '#f', 'failed null?' );
(write
  (null? (list 1)))
CODE

language_output_is( 'Scheme', <<'CODE', '(1 2 (3 4))', 'complex list' );
(write
  '(1 2 (3 4))) ; for emacs ')
CODE

language_output_is( 'Scheme', <<'CODE', '(1 2 (3 4))', 'complex list II' );
(write
  (list 1 2 (list 3 4)))
CODE

language_output_is( 'Scheme', <<'CODE', '(list 3 4)', 'quasiquote' );
(write
  `(list ,(+ 1 2) 4))
CODE

language_output_is( 'Scheme', <<'CODE', '(quasiquote (list (unquote (+ 1 2)) 4))', 'quoted quasiquote' );
(write
  '`(list ,(+ 1 2) 4))
CODE

language_output_is( 'Scheme', <<'CODE', '(list 1 2 3)', 'unquote-splicing' );
(write
  `(list ,@(list 1 2 3)))
CODE

language_output_is( 'Scheme', <<'CODE', '(list)', 'splicing empty list' );
(write
  `(list ,@(list)))
CODE

language_output_is( 'Scheme', <<'CODE', '(list 1 2 3 (4 5))', 'complex quasiquote' );
(write
  `(list ,@(list 1 2) ,(+ 1 2) ,(list 4 5)))
CODE

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
