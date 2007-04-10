=head1 INFORMATION

This is an advanced example.

It uses a file stream (Stream::ParrotIO) that is processed linewise with
Stream::Lines. A counter stream creates line numbers, both are combined to
one stream and then dumped.

=head1 FUNCTIONS

=over 4

=item _main

Opens this file (or the one specified at the command line) and creates a lines
stream for it. Then it combines the stream with a stream providing line numbers.

=cut

.sub _main
    .param pmc argv
    .local int argc
    .local pmc file
    .local pmc lines
    .local pmc counter
    .local pmc combiner
    .local string name

    # get the name of the file to open
    name = "examples/streams/FileLines.pir"
    argc = argv
    if argc < 2 goto NO_NAME
    name = argv[1]
NO_NAME:

    load_bytecode "library/Stream/ParrotIO.pir"
    load_bytecode "library/Stream/Lines.pir"
    load_bytecode "library/Stream/Sub.pir"
    load_bytecode "library/Stream/Combiner.pir"

    # create a file stream
    find_type $I0, "Stream::ParrotIO"
    new file, $I0
    file."open"( name, "<" )

    # process it one line per read
    find_type $I0, "Stream::Lines"
    new lines, $I0
    assign lines, file

    # endless counter
    find_type $I0, "Stream::Sub"
    new counter, $I0
    .const .Sub temp = "_counter"
    assign counter, temp

    # combine the counter and the file's lines
    find_type $I0, "Stream::Combiner"
    new combiner, $I0
    assign combiner, counter
    assign combiner, lines

    # dump the stream
    combiner."dump"()

    end
.end

=item _counter

This sub is the source of the counter stream. It just endlessly writes
line numbers followed by a space to its stream.

=cut

.sub _counter
    .param pmc stream
    .local int i
    .local string str
    .local pmc array

    i = 0
    array = new .ResizablePMCArray

LOOP:
    inc i
    array[0] = i
    sprintf str, "%5d ", array
    stream."write"( str )
    branch LOOP
.end

=back

=head1 AUTHOR

Jens Rieks E<lt>parrot at jensbeimsurfen dot deE<gt> is the author
and maintainer.
Please send patches and suggestions to the Perl 6 Internals mailing list.

=head1 COPYRIGHT

Copyright (C) 2004, The Perl Foundation.

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
