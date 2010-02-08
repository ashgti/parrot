# Copyright (C) 2004-2009, Parrot Foundation.
# $Id$

=head1 TITLE

bench_op - Benchmark one or more opcodes

=head1 SYNOPSIS

  parrot bench_op.pir 'add $I0, $I1, $I2'
  parrot bench_op.pir --preops='newclass $P0, "Foo"' file_w_ops.pir
  parrot bench_op.pir --help

=head1 DESCRIPTION

The given opcode(s) are compiled into a sequence:

  .sub _entry0
    .param int N
    .local int i
    #
    # preops go here
    #
    null i
  loop:
    #
    # ops go here
    #
    inc i
    lt i, N, loop
  .end

so they should conform to Parrot Calling Conventions.
The code gets executed with the PIR compiler - using symbolic variables is always ok.

Output is the time in seconds for 1.000.000 instruction(s).

=head1 OPTIONS

=over 4

=item I<--times=N>

Run the given opcode(s) in a loop I<N> time.
If no I<--times> options is given 100.000 is used.

=item I<--verbose=i>

Set I<--verbose> to 1 to see the total time.
Set I<--verbose> to 2 to see the compiled programs.

=item I<--preops='opcode(s)'>

Execute the given opcodes in front of the loop. This is needed for ops,
that have side effects like B<newclass>.

=item I<--help>

Print a short description and exit.

=item I<--version>

Print program version and exit.

=back

=cut

.const string VERSION = "0.1.1"

.sub main :main
    .param pmc argv

    load_bytecode "Getopt/Obj.pbc"

    .local int times
    times = 100000

    .local int verbose
    verbose = 0

    # Specification of command line arguments.
    .local pmc getopts
    getopts = new ["Getopt";"Obj"]
    # getopts."notOptStop"(1)
    push getopts, "version"
    push getopts, "verbose=i"
    push getopts, "help"
    push getopts, "times=i"
    push getopts, "preops=s"

    .local string program
    shift program, argv

    .local pmc opt
    opt = getopts."get_options"(argv)

    .local int argc
    argc = argv
    unless argc goto do_help

    .local int def
    def = defined opt
    unless def goto do_help
    def = defined opt["version"]
    if def goto do_def
    def = defined opt["help"]
    if def goto do_help
    def = defined opt["times"]
    unless def goto default_times
    times = opt["times"]
default_times:
    def = defined opt["verbose"]
    unless def goto default_verbose
        verbose = opt["verbose"]
default_verbose:
    .local string preops
    def = defined opt["preops"]
    unless def goto default_preops
        preops = opt["preops"]
default_preops:
    _run( argv, times, verbose, preops)
    end

do_def:
    print program
    print ": Version "
    print VERSION
    print "\n"
    end

do_help:
    print program
    print " [--help] [--version] [--verbose=i]] [--times=N] \\ \n"
    print "\t[--preops='op(s)'] file | opcode\n"
    print "\nRun opcode on commandline or from file <N> times.\n"
    print "s. perldoc -F "
    print program
    print "\n"
    end
.end

.sub _run
    .param pmc argv
    .param int times
    .param int verbose
    .param string preops

    .local string op
    shift op, argv
    unless verbose goto no_v1
    	print "Running '"
	print op
	print "' "
	print times
	print " times\n"
no_v1:
    # op may be a file or an opcode - try to open it
    .local pmc F
    open F, op, 'r'
    .local int def
    def = defined F
    unless def goto op_is_op
	read op, F, 10000	# TODO use stat
	close F
op_is_op:
    .local num empty
    empty = _bench(times, verbose, '', '')
    unless verbose goto no_v2
        print "Empty "
        print empty
        print "\n"
no_v2:
    .local num test
    .local num diff
    test = _bench(times, verbose, op, preops)
    diff = test - empty
    unless verbose goto no_v3
        print "Total "
        print test
        print "\n"
        print "Prog  "
        print diff
        print "\n"
no_v3:
    test = diff * 1.0E6
    test = test / times
    print "Time for 1 million instructions: "
    print test
    print "\n"
.end

.sub _bench
    .param int n
    .param int verbose
    .param string ops
    .param string preops

    .local pmc compiler
    .local pmc compiled
    .local int l
    .local string ls
    .local string prog

    length l, ops
    ls = l
    .local string entry_label
    entry_label = "_entry" . ls
    compreg compiler, "PIR"	# TODO PASM option
    prog = ".sub " . entry_label
    prog = prog . "\n.param int N\n.local int i\nnull i\n"
    prog = prog . preops
    prog = prog . "\nloop:\n"
    prog = prog . ops
    prog = prog . "\ninc i\nlt i, N, loop\n.end\n"
    if verbose < 2 goto no_v2
	print "\n#---------\n"
	print prog
	print "#---------\n"
no_v2:
    compiled = compiler(prog)
    .local num now
    time now
    .local pmc entry
    find_global entry, entry_label
    entry(n)
    .local num later
    time later
    later = later - now

    .return(later)
.end

=head1 BUGS

You can't use the variables I<i> and I<N> nor the labels I<loop:> and
I<_entry\d> in your opcodes.

=head1 AUTHOR

Leopold Toetsch <lt@toetsch.at>

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
