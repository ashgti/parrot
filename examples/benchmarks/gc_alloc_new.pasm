# Copyright (C) 2001-2003, Parrot Foundation.
# $Id$

=head1 NAME

examples/benchmarks/gc_alloc_new.pasm - GC Benchmark

=head1 SYNOPSIS

    % ./parrot examples/benchmarks/gc_alloc_new.pasm

=head1 DESCRIPTION

Makes a big string with C<repeat>, then prints out some statistics
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

# a rather useless test to check if your system does swapping
# if sizes are too big
	set I0, 6
	set I1, 0
	set S1,  "1234567890"
	time N5
loop:
	repeat S1, S1, 10
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

=head1 SEE ALSO

F<examples/benchmarks/bench_newp.pasm>,
F<examples/benchmarks/gc_alloc_reuse.pasm>,
F<examples/benchmarks/gc_generations.pasm>,
F<examples/benchmarks/gc_header_new.pasm>,
F<examples/benchmarks/gc_header_reuse.pasm>,
F<examples/benchmarks/gc_waves_headers.pasm>,
F<examples/benchmarks/gc_waves_sizeable_data.pasm>,
F<examples/benchmarks/gc_waves_sizeable_headers.pasm>.

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
