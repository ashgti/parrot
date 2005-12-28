=head1 TITLE

Stream::Writer - a PIR sub as target for a Stream

=head1 VERSION

version 0.1

=head1 SYNOPSIS

    # create the stream
    find_type $I0, "Stream::Writer"
    new stream, $I0

    # set the source sub
    .const .Sub temp = "_reader"
    stream."source"( temp )

    stream."write"( "hello, world" )

    ...

    .sub _reader method
	.local string str
	str = self."read"()
    .end

=head1 DESCRIPTION

=cut

.include "interpinfo.pasm"
.namespace ["Stream::Writer"]

.sub __onload :load
    find_type $I0, "Stream::Writer"
    if $I0 > 1 goto END

    load_bytecode "library/Stream/Base.pir"

    getclass $P0, "Stream::Base"
    subclass $P1, $P0, "Stream::Writer"

    addattribute $P1, "writer"
    addattribute $P1, "status"
END:
.end

=head1 METHODS

=over 4

=cut

.sub _reader_stub method
    .local pmc source
    .local pmc mysub
    #.local pmc myself

    interpinfo mysub, .INTERPINFO_CURRENT_SUB
    #interpinfo myself, .INTERPINFO_CURRENT_OBJECT
    getprop source, "CALL", mysub
    source()

    # close the source
    source = find_global "Stream::Base", "close"
    self."setSource"()

    # mark it as closed
    classoffset $I0, self, "Stream::Writer"
    inc $I0
    .local pmc status
    interpinfo self, .INTERPINFO_CURRENT_OBJECT
    getattribute status, self, $I0
    status = 0
.end

.sub __init method
    .local pmc status

    new status, .PerlInt
    set status, 0
    classoffset $I0, self, "Stream::Writer"
    inc $I0
    setattribute self, $I0, status
.end

.sub __set_pmc method
    .param pmc source
    .local pmc status

    .const .Sub stub = "_reader_stub"
    setprop stub, "CALL", source
    self."setSource"( stub )

    classoffset $I0, self, "Stream::Writer"
    inc $I0
    getattribute status, self, $I0
    status = 1
.end

.sub close method
    .local string str

    null str
LOOP:
    $I0 = self."connected"()
    unless $I0 goto END
    self."write"( str )
    branch LOOP
END:
.end

.sub connected method
    .local pmc status
    .local int ret

    classoffset $I0, self, "Stream::Writer"
    inc $I0
    getattribute status, self, $I0
    ret = status
    .return(ret)
.end

=item source."write"()

...

=cut

.sub write method
    .param string str
    .local pmc source
    .local pmc status

    classoffset $I0, self, "Stream::Writer"
    .include "interpinfo.pasm"
    $P0 = interpinfo .INTERPINFO_CURRENT_CONT
    setattribute self, $I0, $P0
    inc $I0
    getattribute status, self, $I0

    if status == 0 goto END
    if status == 2 goto WRITE
    status = 2
    self."write"( "" )
WRITE:

    classoffset $I0, self, "Stream::Writer"
    .include "interpinfo.pasm"
    $P0 = interpinfo .INTERPINFO_CURRENT_CONT
    setattribute self, $I0, $P0

    source = self."source"()
    if_null source, END
    typeof $I0, source
    if $I0 == .PerlUndef goto END

    source( str )
END:
    .return()
    goto WRITE	# XXX else self get's overwritten by source
.end

=item source."rawRead"() (B<internal>)

...

=cut

.sub rawRead method
    .local string str
    .local pmc writer
    .local pmc cont

    cont = self."source"()
    if_null cont, END_OF_STREAM

    classoffset $I0, self, "Stream::Writer"
    getattribute writer, self, $I0
    str = self."_call_writer"(writer)
    .return(str)
END_OF_STREAM:
    null writer
    classoffset $I0, self, "Stream::Writer"
    setattribute self, $I0, writer
    null str

    .return(str)
.end

.sub _call_writer method
    .param pmc writer
    .local pmc cont
    .local string str

    cont = interpinfo .INTERPINFO_CURRENT_CONT
    self."setSource"( cont )
    str = writer()
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
