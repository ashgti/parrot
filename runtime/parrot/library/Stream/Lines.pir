=head1 TITLE

Stream::Lines - process a stream one line per read

=head1 VERSION

version 0.1

=head1 SYNOPSIS

please see F<examples/streams/Lines.pir> and F<examples/streams/FileLines.pir>

=head1 DESCRIPTION

TBD

=head1 METHODS

=over 4

=cut

.include "library/Data/Escape.pir"

.sub onload :load, :anon
    .local int i
    .local pmc base
    .local pmc lines

    find_type i, "Stream::Lines"
    if i > 1 goto END

    load_bytecode "library/Stream/Base.pir"

    getclass base, "Stream::Base"
    subclass lines, base, "Stream::Lines"

    addattribute lines, "buffer"
END:
.end

.namespace ["Stream::Lines"]

.sub __init :method
    .local pmc temp

    classoffset $I0, self, "Stream::Lines"
    temp = new .PerlString
    setattribute self, $I0, temp
.end

=item is = stream."connected"()

...

=cut

.sub connected :method
    # XXX: check if the buffer is empty if the source stream is not connected
    classoffset $I0, self, "Stream::Base"
    getattribute $P0, self, $I0
    if_null $P0, NOT_CONNECTED
    typeof $I0, $P0
    if $I0 == .PerlUndef goto NOT_CONNECTED
    $I0 = $P0."connected"()
    .return($I0)
NOT_CONNECTED:
    .return(0)
.end

=item is = stream."rawRead"() (B<internal>)

...

=cut

.sub rawRead :method
    .local pmc temp
    .local string buffer
    .local string ret
    .local int i

    buffer = self."buffer"()
    i = length buffer
    if i > 0 goto BUFFER_OK
BUFFER_FILL:
    (buffer, i) = self."fillBuffer"()
    ret = buffer
    if i == 0 goto BUFFER_END
#    if i != 0 goto BUFFER_OK
#    self."close"()
#    branch BUFFER_END
BUFFER_OK:

    index i, buffer, "\n"

    if i == -1 goto BUFFER_FILL

    substr ret, buffer, 0, i
    inc i
    substr buffer, buffer, i

    self."setBuffer"( buffer )

BUFFER_END:
    .return(ret)
.end

=item is = stream."buffer"() (B<internal>)

...

=cut

.sub buffer :method
    .local pmc temp
    .local string _buffer

    classoffset $I0, self, "Stream::Lines"
    getattribute temp, self, $I0
    _buffer = temp

    .return(temp,_buffer)
.end

=item is = stream."setBuffer"() (B<internal>)

...

=cut

.sub setBuffer :method
    .param string buffer
    .local pmc temp

    new temp, .PerlString
    temp = buffer
    classoffset $I0, self, "Stream::Lines"
    setattribute self, $I0, temp
.end

=item stream."fillBuffer"() (B<internal>)

...

=cut

.sub fillBuffer :method
    .local pmc temp
    .local string buffer
    .local string str
    .local int code

    # read
    temp = self."source"()
    str = temp."read"()

    classoffset $I0, self, "Stream::Lines"
    getattribute temp, self, $I0
    buffer = temp
    code = 0

    if_null str, END

    # concat to the buffer
    concat buffer, str
    temp = buffer
    setattribute self, $I0, temp
    code = 1

END:
    if code == 1 goto NO_CLOSE
    self = self."source"()
    self."close"()
    null buffer
    null code
NO_CLOSE:
    .return(buffer, code)
.end

=back

=head1 AUTHOR

Jens Rieks E<lt>parrot at jensbeimsurfen dot deE<gt> is the author
and maintainer.
Please send patches and suggestions to the Perl 6 Internals mailing list.

=head1 COPYRIGHT

Copyright (c) 2004, the Perl Foundation.

=cut
