#! perl
# Copyright (C) 2008, The Perl Foundation.
# $Id$

=head1 NAME

t/php/md5.t - Standard Library md5

=head1 SYNOPSIS

    % perl -I../lib plumhead/t/php/md5.t

=head1 DESCRIPTION

Tests PHP Standard Library md5
(implemented in F<languages/plumhead/src/common/php_md5.pir>).

See L<http://www.php.net/manual/en/ref.strings.php>.

=cut

use strict;
use warnings;

use FindBin;
use lib "$FindBin::Bin/../../lib";

use Test::More     tests => 5;
use Parrot::Test;


language_output_is( 'Plumhead', <<'CODE', <<'OUTPUT', 'md5(msg)' );
<?php
  echo md5('message digest'), "\n";
?>
CODE
f96b697d7cb7938d525a2f31aaf161d0
OUTPUT

language_output_is( 'Plumhead', <<'CODE', <<'OUTPUT', 'md5(msg, FALSE)' );
<?php
  echo md5('message digest', FALSE), "\n";
?>
CODE
f96b697d7cb7938d525a2f31aaf161d0
OUTPUT

language_output_is( 'Plumhead', <<'CODE', <<'OUTPUT', 'md5(msg, TRUE)' );
<?php
  $md = md5('message digest', TRUE);
  echo gettype($md), "\n";
  echo strlen($md), "\n";
?>
CODE
string
16
OUTPUT

unlink '../file.txt' if (-f '../file.txt');
open my $X, '>', '../file.txt';
print {$X} 'message digest';
close $X;

language_output_is( 'Plumhead', <<'CODE', <<'OUTPUT', 'md5_file(file)' );
<?php
  echo md5_file('file.txt'), "\n";
?>
CODE
f96b697d7cb7938d525a2f31aaf161d0
OUTPUT

unlink '../file.txt' if (-f '../file.txt');

language_output_is( 'Plumhead', <<'CODE', <<'OUTPUT', 'md5_file(nofile)' );
<?php
  echo md5_file('nofile.txt'), "\n";
?>
CODE

OUTPUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
