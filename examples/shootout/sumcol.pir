#!./parrot
# Copyright (C) 2005-2009, Parrot Foundation.
# $Id$
#
# ./parrot -R jit sumcol.pir < sum8M
# by Joshua Isom
# N.B. it is called the sum-file benchmark on the computer shootout (KF)

.sub main :main
	.local pmc stdin
	.local string line
	.local int count, tmp, linelen
	count = 0
	stdin = getstdin
beginwhile:
	line = readline stdin
	linelen = length line
	unless linelen goto endwhile
	tmp	= line
	count += tmp
	goto beginwhile
endwhile:
	print count
	print "\n"
.end


# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
