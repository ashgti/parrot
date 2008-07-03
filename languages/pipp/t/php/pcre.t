#! perl
# Copyright (C) 2008, The Perl Foundation.
# $Id$

=head1 NAME

t/php/pcre.t - Library pcre

=head1 SYNOPSIS

    % perl -I../lib pipp/t/php/pcre.t

=head1 DESCRIPTION

Tests PHP Library pcre
(implemented in F<languages/pipp/src/common/php_pcre.pir>).

See L<http://www.php.net/manual/en/ref.pcre.php>.

=cut

use strict;
use warnings;

use FindBin;
use lib "$FindBin::Bin/../../lib";

use Test::More;
use Parrot::Test;
use Parrot::Config;

if ( $PConfig{HAS_PCRE} ) {
    plan tests => 7;
}
else {
    plan skip_all => "no PCRE";
}


language_output_is( 'Pipp', <<'CODE', <<'OUTPUT', 'constants', todo => 'compiler supports constant');
<?php
  echo PREG_NO_ERROR, "\n";
  echo PREG_INTERNAL_ERROR, "\n";
  echo PREG_BACKTRACK_LIMIT_ERROR, "\n";
  echo PREG_RECURSION_LIMIT_ERROR, "\n";
  echo PREG_BAD_UTF8_ERROR, "\n";
?>
CODE
0
1
2
3
4
OUTPUT

language_output_is( 'Pipp', <<'CODE', <<'OUTPUT', 'preg_match()' );
<?php
  echo preg_match('/b/', 'abc'), "\n";
  echo preg_match('/b/', 'aaa'), "\n";
  echo preg_match('{b}', 'abc'), "\n";
  echo preg_match('/b/i', 'ABC'), "\n";
?>
CODE
1
0
1
1
OUTPUT

language_output_like( 'Pipp', <<'CODE', <<'OUTPUT', 'preg_match() empty regex' );
<?php
  echo preg_match('   ', 'abc'), "\n";
?>
CODE
/Empty regular expression/
OUTPUT

language_output_like( 'Pipp', <<'CODE', <<'OUTPUT', 'preg_match() bad delim' );
<?php
  echo preg_match(' 7b7', 'abc'), "\n";
?>
CODE
/Delimiter must not be alphanumeric or backslash/
OUTPUT

language_output_like( 'Pipp', <<'CODE', <<'OUTPUT', 'preg_match() no end' );
<?php
  echo preg_match(' |b', 'abc'), "\n";
?>
CODE
/ No ending delimiter '|' found/
OUTPUT

language_output_like( 'Pipp', <<'CODE', <<'OUTPUT', 'preg_match() unknown modif' );
<?php
  echo preg_match(' /b/ia', 'abc'), "\n";
?>
CODE
/Unknown modifier 'a'/
OUTPUT

language_output_is( 'Pipp', <<'CODE', <<'OUTPUT', 'preg_quote()' );
<?php
  echo preg_quote('{}[]()'), "\n";
?>
CODE
\{\}\[\]\(\)
OUTPUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
