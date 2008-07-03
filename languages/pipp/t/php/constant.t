#! perl
# Copyright (C) 2008, The Perl Foundation.
# $Id$

=head1 NAME

t/php/var.t - Test for constants

=head1 SYNOPSIS

    % perl -I../lib pipp/t/php/constants.t

=head1 DESCRIPTION

Tests support for constants.

See L<http://www.php.net/manual/en/language.constants.php>.

=cut

use strict;
use warnings;

use FindBin;
use lib "$FindBin::Bin/../../lib";

use Test::More     tests => 9;
use Parrot::Test;

language_output_is( 'Pipp', <<'END_CODE', <<'END_OUT', 'define() and constant(), string' );
<?php

define( "THIS_IS", "it" );
echo constant("THIS_IS"), "\n";
END_CODE
it
END_OUT

language_output_is( 'Pipp', <<'END_CODE', <<'END_OUT', 'define() and constant(), integer' );
<?php

define( "TEN_TIMES_TEN", 100 );
echo constant("TEN_TIMES_TEN"), "\n";
END_CODE
100
END_OUT

language_output_is( 'Pipp', <<'END_CODE', <<'END_OUT', 'define() and constant(), TRUE' );
<?php

define( "FAUX", FALSE );
echo constant("FAUX"), "\n";
END_CODE

END_OUT

language_output_is( 'Pipp', <<'END_CODE', <<'END_OUT', 'define() and constant(), NULL' );
<?php

define( "NUL", NULL );
echo constant("NUL"), "\n";
END_CODE

END_OUT

language_output_is( 'Pipp', <<'END_CODE', <<'END_OUT', 'define() and constant(), Float' );
<?php

define( "PI", 3.14159 );
echo constant("PI"), "\n";
END_CODE
3.14159
END_OUT

language_output_is( 'Pipp', <<'END_CODE', <<'END_OUT', 'define() and constant(), FALSE' );
<?php

define( "VRAI", TRUE );
echo constant("VRAI"), "\n";
END_CODE
1
END_OUT

language_output_is( 'Pipp', <<'END_CODE', <<'END_OUT', 'define() and echo, String' );
<?php

define( "THIS_IS", "it" );
echo THIS_IS;
echo "\n";
END_CODE
it
END_OUT

language_output_is( 'Pipp', <<'END_CODE', <<'END_OUT', 'define() and echo, Float' );
<?php

define( "PI", 3.14159 );
echo PI, "\n";
END_CODE
3.14159
END_OUT

language_output_is( 'Pipp', <<'END_CODE', <<'END_OUT', 'define() and defined()' );
<?php

define( "PI", 3.14159 );
echo defined("PI"), "\n";
echo defined("UNDEF_PI"), "\n";
END_CODE
1

END_OUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
