#! perl
# $Id$

use strict;
use warnings;

chdir '../..';
`$^X -Ilib tools/dev/reconfigure.pl --step=gen::languages --languages=pheme`;
