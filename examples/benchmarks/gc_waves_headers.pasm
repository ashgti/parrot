# Copyright (C) 2001-2003, Parrot Foundation.
# $Id$

=head1 NAME

examples/benchmarks/gc_waves_headers.pasm - GC Benchmark

=head1 SYNOPSIS

    % ./parrot examples/benchmarks/gc_waves_headers.pasm

=head1 DESCRIPTION

Makes some empty strings with C<concat>, then prints out some statistics
indicating:

=over 4

=item * the time taken

=item * the total number of bytes allocated

=item * the total of GC runs made

=item * the total number of collection runs made

=item * the total number of bytes copied

=item * the number of active C<Buffer> C<struct>s

=item * the total number of C<Buffer> C<struct>s

=back

=cut

	set I0, 200
	set I2, 500
	set S0,  ""
	time N5
	set I1, 0

mainloop:

loopup:
	concat S1, S0, S0
	concat S2, S0, S0
	concat S3, S0, S0
	concat S4, S0, S0
	concat S5, S0, S0
	concat S6, S0, S0
	concat S7, S0, S0
	concat S8, S0, S0

	inc I1
	lt I1, I0, loopup

loopdown:
	dec I1
	gt I1, 0, loopdown

	inc I3
	lt I3, I2, mainloop


getout:	time N6

	sub N7, N6, N5
	print N7
 	print " seconds.\n"

	interpinfo I1, 1
	print "A total of "
	print I1
	print " bytes were allocated\n"

	interpinfo I1, 2
	print "A total of "
	print I1
	print " GC runs were made\n"

	interpinfo I1, 3
	print "A total of "
	print I1
	print " collection runs were made\n"

	interpinfo I1, 10
	print "Copying a total of "
	print I1
	print " bytes\n"

	interpinfo I1, 5
	print "There are "
	print I1
	print " active Buffer structs\n"

	interpinfo I1, 7
	print "There are "
	print I1
	print " total Buffer structs\n"

	end

=head1 SEE ALSO

F<examples/benchmarks/bench_newp.pasm>,
F<examples/benchmarks/gc_alloc_new.pasm>,
F<examples/benchmarks/gc_alloc_reuse.pasm>,
F<examples/benchmarks/gc_generations.pasm>,
F<examples/benchmarks/gc_header_new.pasm>,
F<examples/benchmarks/gc_header_reuse.pasm>,
F<examples/benchmarks/gc_waves_sizeable_data.pasm>,
F<examples/benchmarks/gc_waves_sizeable_headers.pasm>.

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
