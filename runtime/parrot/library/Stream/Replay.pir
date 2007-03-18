# $Id$

=head1 TITLE

Stream::Replay - replayable Stream

=head1 VERSION

version 0.1

=head1 SYNOPSIS

    load_bytecode "library/Stream/Replay.pir"

    $I0 = find_type "Stream::Replay"
    $P0 = new $I0
    assign $P0, other_stream

    # .. read from $P0 ..

    $P1 = clone $P0

    # .. read further ..

    $P0 = $P1
    # now, if you read from $P0, you get the same data as after the clone

=head1 DESCRIPTION

By using C<clone>, you can read data from a stream as often as you want.

=head1 METHODS

=over 4

=cut

.namespace ["Stream::Replay"]

.sub onload :load, :anon
    find_type $I0, "Stream::Replay"
    if $I0 > 1 goto END
    
    load_bytecode "library/Stream/Base.pir"
    
    # Stream::Replay
    getclass $P0, "Stream::Base"
    subclass $P0, $P0, "Stream::Replay"
    addattribute $P0, "buffer"
    addattribute $P0, "pos"

    # Stream::Replay::Buffer
    newclass $P0, "Stream::Replay::Buffer"
    addattribute $P0, "strings"
    addattribute $P0, "clones"
END:
.end

=item __init

...

=cut

.sub __init :method
    .local pmc temp

    classoffset $I0, self, "Stream::Replay"

    inc $I0
    temp = new .Integer
    setattribute self, $I0, temp
.end

=item assign stream, source

...

=cut

.sub __set_pmc :method
    .param pmc val
    .local pmc buffer
    
    isa $I0, val, "Stream::Replay"
    unless $I0 goto NOTA
    
    # get the buffer
    classoffset $I0, val, "Stream::Replay"
    getattribute buffer, val, $I0
    
    # add us to the buffer
    buffer."add"( self )
    
    # store the buffer
    classoffset $I0, self, "Stream::Replay"
    setattribute self, $I0, buffer

    # get the position
    classoffset $I0, val, "Stream::Replay"
    inc $I0
    getattribute $P0, val, $I0
    
    $P0 = clone $P0
    
    # set the position
    classoffset $I0, val, "Stream::Replay"
    inc $I0
    setattribute val, $I0, $P0
    
    # assign the source
    val = val."source"()
    branch ASSIGN
NOTA:
    find_type $I0, "Stream::Replay::Buffer"
    new buffer, $I0
    classoffset $I0, self, "Stream::Replay"
    setattribute self, $I0, buffer
ASSIGN:
    self."setSource"( val )
.end

=item stream."rawRead"() (B<internal>)

...

=cut

.sub rawRead :method
    .local pmc source
    .local pmc buffer
    .local pmc pos
    .local string ret
    
    null ret
    source = self."source"()
    if_null source, END
    classoffset $I0, self, "Stream::Replay"
    getattribute buffer, self, $I0
    if_null buffer, END
    inc $I0
    getattribute pos, self, $I0

    ret = buffer."read"( pos, source )
END:
    .return(ret)
.end

=item stream2 = clone stream

...

=cut

.sub __clone :method
    .local pmc ret
    .local pmc temp

    find_type $I0, "Stream::Replay"
    ret = new $I0
    
    assign ret, self
    
    $P0 = self."byte_buffer"()
    if_null $P0, END
    $P0 = clone $P0
    ret."byte_buffer"( $P0 )
END:
    .return(ret)
.end


.namespace ["Stream::Replay::Buffer"]

.sub __init :method
    .local pmc temp
    
    classoffset $I0, self, "Stream::Replay::Buffer"

    temp = new ResizableStringArray
    setattribute self, $I0, temp
    
    inc $I0
    temp = new ResizablePMCArray
    setattribute self, $I0, temp
.end

.sub compact :method
    # XXX check the begin of the queue for stale entries
    noop
.end

.sub read :method
    .param pmc pos
    .param pmc source
    .local string ret
    .local pmc strings
    .local pmc clones
    
    null ret
    typeof $I0, source
    if $I0==.Undef goto END
    
    classoffset $I0, self, "Stream::Replay::Buffer"
    getattribute strings, self, $I0
    inc $I0
    getattribute clones, self, $I0
    
    $I0 = clones
    if $I0 <= 1 goto READ_SOURCE

    $I0 = pos
    $I1 = strings

    if $I0 >= $I1 goto READ_SOURCE

    ret = strings[$I0]
    inc pos
    branch END
    
READ_SOURCE:
    ret = source."read"()
    
    $I0 = clones
    if $I0 <= 1 goto END
    inc pos
    push strings, ret
END:   
 
    .return(ret)
.end

.sub add :method
    .param pmc stream
    .local pmc clones
    
    classoffset $I0, self, "Stream::Replay::Buffer"
    inc $I0
    getattribute clones, self, $I0
    push clones, stream
.end

.sub remove :method
    .param pmc stream
    .local pmc clones
    .local pmc entry
    .local int i
    .local int j
    
    classoffset i, self, "Stream::Replay::Buffer"
    inc i
    getattribute clones, self, i

    i = 0
    j = clones
LOOP:
    if i >= j goto END
    entry = clones[i]
    null entry
    clones[i] = entry
    ne_addr entry, stream, NEXT
NEXT:
    inc i
    branch LOOP
END:
.end

=back

=head1 AUTHOR

Jens Rieks E<lt>parrot at jensbeimsurfen dot deE<gt> is the author
and maintainer.
Please send patches and suggestions to the Perl 6 Internals mailing list.

=head1 COPYRIGHT

Copyright (C) 2004-2006, The Perl Foundation.

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
