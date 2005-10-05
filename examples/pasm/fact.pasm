# Copyright (C) 2001-2003 The Perl Foundation.  All rights reserved.
# $Id$

=head1 NAME

examples/assembly/fact.pasm - Mmmm, beer good

=head1 SYNOPSIS

    % ./parrot examples/assembly/fact.pasm

=head1 DESCRIPTION

Compute the factorial recursively for 0! to 6! and print the results.

=head1 HISTORY

=over 4

=item 20020316 bdwheele@indiana.edu 

Changed local labels to global for new assembler. Use C<pushi> instead
of the now missing C<clonei>.

=back

=cut

main:
	set 	I1,0

loop:
	print	"fact of "
	print	I1
	print	" is: "
	set	I0,I1
	bsr	fact
	print	I0
	print	"\n"
	inc	I1
	eq	I1,7,done
	branch	loop
done:
	end

# I0 is the number to compute
fact:
	pushi
	lt	I0,2,is_one
	set	I1,I0
	dec	I0
	bsr	fact
	mul	I0,I0,I1
	save	I0
	branch	fact_done
is_one:
	set	I0,1
	save	I0
fact_done:
	popi
	restore	I0
	ret
