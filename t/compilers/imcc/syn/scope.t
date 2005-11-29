#!perl -w
# Copyright: 2001-2005 The Perl Foundation.  All Rights Reserved.
# $Id$

use strict;
use Parrot::Test tests => 1;

##############################
pir_output_is(<<'CODE', <<'OUT', "global const");
.sub test :main
	.globalconst string ok = "ok\n"
	print ok
	_sub()
	end
.end
.sub _sub
	print ok
.end
CODE
ok
ok
OUT
