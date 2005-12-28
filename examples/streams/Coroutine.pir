=head1 INFORMATION

This example shows the usage of C<Stream::Coroutine>.

You can only write to the stream with yielding.

Stream::Sub has the advantage that the stream object can be passed to other
functions that in turn can write to the stream, which is not possible with
coroutine streams.

On the other hand, Stream::Coroutine is more lightweight than Stream::Sub.

=head1 FUNCTIONS

=over 4

=item _main

Creates a coroutine stream and dumps it.

=cut

.sub _main
    .local pmc stream
    .local pmc temp

    load_bytecode "library/Stream/Base.pir"
    load_bytecode "library/Stream/Coroutine.pir"

    # create the coroutine stream    
    find_type $I0, "Stream::Coroutine"
    new stream, $I0

    # set the stream's source coroutine
    # A .Sub is a coroutine when there is a yield?
    .const .Sub temp = "_coro"
    assign stream, temp
    #stream."source"( temp )
    
    # dump the stream
    stream."dump"()
    
    end
.end

=item _coro

This sub is a coroutine that is used as the source of the stream.
It just writes the numbers 0 to 9 to the stream.

=cut

.sub _coro
    .param pmc stream
    .local int i
    .local string str

    i = 0
LOOP:
    str = i
    
    .yield(str)

    inc i
    if i < 10 goto LOOP

    # the stream is automatically closed if you return a null string
    # you can also close it explicitly with
    # stream."close"()
    # in which case it doesn't matter what you are returning.
    
    null str
    .return(str)
.end

=back

=head1 AUTHOR

Jens Rieks E<lt>parrot at jensbeimsurfen dot deE<gt> is the author
and maintainer.
Please send patches and suggestions to the Perl 6 Internals mailing list.

=head1 COPYRIGHT

Copyright (c) 2004, the Perl Foundation.

=cut
