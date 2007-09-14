#! perl
# $Id$

# Copyright (C) 2001-2007, The Perl Foundation.

use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../../lib";

#use Scheme::Test tests => 7;
use Scheme::Test skip_all => 'Using obsolete calling conventions';

###
### If
###

output_is( <<'CODE', 1, 'basic if - Fail' );
(write (if (= 1 1) 1 0))
CODE

output_is( <<'CODE', 1, 'basic if - Pass' );
(write (if (= 0 1) 0 1))
CODE

output_is( <<'CODE', 7, 'slightly more complex if' );
(write (if (= 9 9) 7 -23))
CODE

###
### And
###

output_is( <<'CODE', 1, 'basic and - Pass' );
(write (and 1 1))
CODE

output_is( <<'CODE', 0, 'basic and - Fail' );
(write (and 0 1))
CODE

###
### Or
###

output_is( <<'CODE', 1, 'basic or - Pass' );
(write (or 1 1))
CODE

output_is( <<'CODE', 0, 'basic or - Fail' );
(write (or 0 0))
CODE


# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
