# Copyright (C) 2001-2003, Parrot Foundation.
# $Id$

=head1 NAME

examples/benchmarks/gc_header_new.pasm - GC Benchmark

=head1 SYNOPSIS

    % ./parrot examples/benchmarks/gc_header_new.pasm

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

=head1 SEE ALSO

F<examples/benchmarks/bench_newp.pasm>,
F<examples/benchmarks/gc_alloc_new.pasm>,
F<examples/benchmarks/gc_alloc_reuse.pasm>,
F<examples/benchmarks/gc_generations.pasm>,
F<examples/benchmarks/gc_header_reuse.pasm>,
F<examples/benchmarks/gc_waves_headers.pasm>,
F<examples/benchmarks/gc_waves_sizeable_data.pasm>,
F<examples/benchmarks/gc_waves_sizeable_headers.pasm>.

=cut

	set I0, 40000
	set I1, 0
	set S0,  ""
	time N5
loop:
	concat S1, S0, S0
	concat S2, S0, S0
	concat S3, S0, S0
	concat S4, S0, S0
	concat S5, S0, S0
	concat S6, S0, S0
	concat S7, S0, S0
	concat S8, S0, S0
	concat S9, S0, S0
	concat S10, S0, S0
	concat S11, S0, S0
	concat S12, S0, S0
	concat S13, S0, S0
	concat S14, S0, S0
	concat S15, S0, S0
	concat S16, S0, S0
	concat S17, S0, S0
	concat S18, S0, S0
	concat S19, S0, S0
	concat S20, S0, S0
	concat S20, S0, S0
	concat S21, S0, S0
	concat S22, S0, S0
	concat S23, S0, S0
	concat S24, S0, S0
	concat S25, S0, S0
	concat S26, S0, S0
	concat S27, S0, S0
	concat S28, S0, S0
	concat S29, S0, S0
	concat S30, S0, S0
	concat S31, S0, S0
	inc I1
	lt I1, I0, loop


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

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
