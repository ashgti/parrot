#
# Declaration.pm
#
# Copyright (C) 2002-2003 Gregor N. Purdy. All rights reserved.
# This program is free software. It is subject to the same license
# as the Parrot interpreter.
#
# $Id$
#

use strict;
use warnings;

package Jako::Construct::Declaration;

use base qw(Jako::Construct);

sub access { return shift->{ACCESS}; }

1;

