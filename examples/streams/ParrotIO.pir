=head1 INFORMATION

This small example shows the usage of C<Stream::ParrotIO>.

It reads this file with a default block size.

You can specify another block size with the C<blockSize> method.
C<blockSize> called without an integer parameter will return the
current block size.

Each time the C<read> method is called, the next block is read from
the underlying ParrotIO, until EOF, where the stream will be disconnected.

Instead of using the C<open> method, you can also assign your own ParrotIO
PMC to the stream with the C<assign> op.

=cut

.sub _main :main
    .local pmc stream

    load_bytecode "library/Stream/ParrotIO.pir"

    # create the ParrotIO stream    
    find_type $I0, "Stream::ParrotIO"
    new stream, $I0

    # open this file
    stream."open"( "examples/streams/ParrotIO.pir", "<" )
    
    # you can specifiy a custom block size with
    # stream."blockSize"( 10 )
    
    # dump the stream
    stream."dump"()
    
    end
.end

=head1 AUTHOR

Jens Rieks E<lt>parrot at jensbeimsurfen dot deE<gt> is the author
and maintainer.
Please send patches and suggestions to the Perl 6 Internals mailing list.

=head1 COPYRIGHT

Copyright (C) 2004, The Perl Foundation.

=cut
