=head1 TITLE

Stream::Base - Stream library base class

=head1 VERSION

version 0.1

=head1 SYNOPSIS

This is an abstract baseclass that is not supposed to be used directly.

=head1 DESCRIPTION

TBD

=head1 METHODS

=over 4

=cut

.namespace ["Stream::Base"]

.const int aSource = 0
.const int aIncludes = 1
.const int aBuffer = 2

.sub onload :load, :anon
    find_type $I0, "Stream::Base"
    if $I0 > 1 goto END

    load_bytecode "library/Data/Escape.pir"

    newclass $P0, "Stream::Base"
    addattribute $P0, 'source'
    addattribute $P0, 'includes'
    addattribute $P0, 'buffer'
END:
.end

.sub __init method
    .local pmc close

    # call our own close
    close = find_global "Stream::Base", "close"
    close(self)
.end

=item assign stream, source

=cut

.sub __set_pmc method
    .param pmc source

    classoffset $I0, self, "Stream::Base"
    setattribute self, $I0, source
    null source
.end

.sub setSource method
    .param pmc source

    classoffset $I0, self, "Stream::Base"
    setattribute self, $I0, source
    null source
.end

=item close

=cut

.sub close method
    .local pmc temp

    # reset source
    classoffset $I0, self, "Stream::Base"
    new temp, .PerlUndef
    setattribute self, $I0, temp

    # reset includes
    inc $I0
    new temp, .ResizablePMCArray
    setattribute self, $I0, temp
.end

=item stream."dump"() (debug aid)

Dumps the content of the stream.

Returns nothing.

=cut

.sub dump method
    .local string str
    .local int i
    .local pmc escape

    escape = find_global "Data::Escape", "String"
LOOP:
    i = self."connected"()
    unless i goto END
    str = self."read"()
    if_null str, END
    print "read:["
    str = escape( str )
    print str
    print "]\n"
    branch LOOP
END:
.end

=item copyTo

=cut

.sub copyTo method
    .param pmc to
    .local string str
    .local int i

LOOP:
    i = self."connected"()
    unless i goto END
    str = self."read"()
    if_null str, END
    print to, str
    branch LOOP
END:
.end

=item source = stream."source"()

Returns the currently set source.

=cut

.sub source method
    .local pmc ret

    classoffset $I0, self, "Stream::Base"
    add $I0, aSource
    getattribute ret, self, $I0

    .return(ret)
ERROR:
    print "Stream::Base::source(): parameters passed\n"
    end
.end

=item is = stream."connected"()

Returns 1 if the stream is connected, 0 otherwise.

It is connected until the source sub returns.

=cut

.sub connected method
    .local pmc stream

    classoffset $I0, self, "Stream::Base"
    add $I0, aSource
    getattribute $P0, self, $I0
    typeof $I0, $P0
    if $I0 == .PerlUndef goto NOT

    can $I0, $P0, "connected"
    unless $I0 goto SKIP
    $I0 = $P0."connected"()
    unless $I0 goto NOT
SKIP:
    .return(1)
NOT:
    .return(0)
.end

=item str = stream."read"()

Returns the read string, or a null string if the stream end has been reached.

=cut

.sub read method
    .local string ret
    .local pmc includes

    classoffset $I0, self, "Stream::Base"
    add $I0, aBuffer
    getattribute $P0, self, $I0
    if_null $P0, NO_BUFFER
    ret = $P0
    null $P0
    setattribute self, $I0, $P0
    branch DONE
NO_BUFFER:

    classoffset $I0, self, "Stream::Base"
    add $I0, aIncludes
    getattribute includes, self, $I0
    if_null includes, SELF

    $I0 = includes
    if $I0 > 0 goto INCLUDE

SELF:
    ret = self."rawRead"()

    if_null includes, DONE
    if_null ret, INCLUDE_CHECK

DONE:
    .return(ret)

INCLUDE_CHECK:
    $I0 = includes
    if $I0 == 0 goto DONE

INCLUDE:
    .local pmc stream

    stream = includes[0]

    ret = stream."read"()

    if_null ret, INCLUDE_DEL
    branch DONE

INCLUDE_DEL:
    shift $P0, includes
    $I0 = includes
    if $I0 > 0 goto INCLUDE
    branch SELF
.end

=item stream."include"( stream2 )

Include stream2 in this stream. The next read on this stream will
act on the included stream.

Please have a look at F<examples/streams/Include.pir> to see how it works.

=cut

.sub include method
    .param pmc stream
    .local pmc includes

    classoffset $I0, self, "Stream::Base"
    add $I0, aIncludes
    getattribute includes, self, $I0
    push includes, stream

    self."flush"()
.end

=item stream."flush"() (B<internal>)

Used to flush the stream when including another stream.

=cut

.sub flush method
    .local int i
    .local string str
    .local pmc includes

    can i, self, "write"
    unless i goto CANT

    classoffset $I0, self, "Stream::Base"
    add $I0, aIncludes
    getattribute includes, self, $I0

    i = includes
    if i == 0 goto CANT

    null str
    i = self."write"( str )

CANT:
    .return(i)
.end

=item stream."read_bytes"( number )

Reads the specified number of bytes from the stream.

=cut

.sub read_bytes method
    .param int requested
    .local string ret
    .local pmc buffer

    classoffset $I0, self, "Stream::Base"
    add $I0, aBuffer

    # get buffer
    getattribute buffer, self, $I0

    # no buffer set?
    if_null buffer, CREATE

    # unset buffer
    null $P0
    setattribute self, $I0, $P0
    branch OK

CREATE:
    buffer = new .PerlString

OK:
    ret = buffer

LOOP:
    $I0 = length ret
    if $I0 >= requested goto DONE
    $S0 = self."read"()
    if_null $S0, DONE
    concat ret, $S0
    branch LOOP
DONE:

    $I0 = length ret
    if $I0 <= requested goto END

    $S0 = ret
    substr ret, $S0, 0, requested
    substr $S0, $S0, requested
    ret = clone ret
    $I0 = length $S0
    unless $I0 goto END

    assign buffer, $S0
    classoffset $I0, self, "Stream::Base"
    add $I0, aBuffer
    setattribute self, $I0, buffer

END:
    .return(ret)
.end

=item stream."byte_buffer"() (B<internal>)

Returns or sets the byte buffer.

=cut

.sub byte_buffer method
    .param pmc buf :optional
    .param int has_buf :opt_flag

    classoffset $I0, self, "Stream::Base"
    add $I0, aBuffer

    if has_buf goto SET

    getattribute $P0, self, $I0

    if_null $P0, END
    branch END
SET:
    setattribute self, $I0, buf
END:
    .return($P0)
.end

=back

=head1 AUTHOR

Jens Rieks E<lt>parrot at jensbeimsurfen dot deE<gt> is the author
and maintainer.
Please send patches and suggestions to the Perl 6 Internals mailing list.

=head1 COPYRIGHT

Copyright (c) 2004, the Perl Foundation.

=cut
