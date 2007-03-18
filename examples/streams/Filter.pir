=head1 INFORMATION

This example shows the usage of C<Stream::Filter>.

Filter streams are used to alter and/or filter strings read
from another stream. The strings are passed to a filter sub
that will return the altered string, or null if the string should
be filtered out.

=head1 FUNCTIONS

=over 4

=item _main

Creates a counter stream that generates numbers from 0 to 9.

=cut

.sub _main
    .local pmc stream
    .local pmc filter

    load_bytecode "library/Stream/Sub.pir"
    load_bytecode "library/Stream/Filter.pir"
    
    # create the counter stream
    find_type $I0, "Stream::Sub"
    new stream, $I0
    # assign its source
    .const .Sub temp = "_counter"
    assign stream, temp

    # create the filter stream
    find_type $I0, "Stream::Filter"
    new filter, $I0
    # assign its source
    assign filter, stream
    # set the filter sub
    .const .Sub temp = "_filter"
    filter."filter"( temp )
    
    # dump the stream
    filter."dump"()
    
    end
.end

=item _counter

This sub is the source of the counter stream.
It generates numbers from 0 to 9 in string form.

=cut

.sub _counter
    .param pmc stream
    .local string str
    .local int i
    
    i = 0

LOOP:
    str = i
    stream."write"( str )

    inc i
    if i < 10 goto LOOP
.end

=item _filter

This sub is the source of the filter stream.

It doubles the read number and appends it together with " * 2 = " to the string.
It also filters the value 4 out, which will be missing in the output.

=cut

.sub _filter
    .param string str
    .local string tmp
    .local int i

    i = str

    # skip 4 * 2
    if i == 4 goto SKIP

    mul i, 2
    set tmp, i
    concat str, " * 2 = "
    concat str, tmp

    .return(str)

SKIP:
    null str
    .return(str)
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
