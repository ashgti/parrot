#!perl
# Copyright (C) 2001-2007, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Config;
use Parrot::Test tests => 3;

##############################
pir_output_is( <<'CODE', <<'OUT', "goto 1" );
.sub test :main
    goto foo
    end
foo:
    print "ok 1\n"
    end
.end

CODE
ok 1
OUT

##############################
pir_output_is( <<'CODE', <<'OUT', "goto 2" );
.sub test :main
    goto foo
bar:    print "ok 2\n"
    end
foo:
    print "ok 1\n"
    goto bar
.end

CODE
ok 1
ok 2
OUT

##############################
pir_error_output_like( <<'CODE', <<'OUT', "illegal label" );
.sub bogus
    goto _function
    print "never\n"
    end
.end
.sub _function
    print "in function\n"
    .return()
.end
CODE
/no label offset defined/
OUT


# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
