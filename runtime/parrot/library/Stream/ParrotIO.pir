=head1 TITLE

Stream::ParrotIO - a ParrotIO PMC as source for a Stream

=head1 VERSION

version 0.1

=head1 SYNOPSIS

please see F<examples/streams/ParrotIO.pir> and F<examples/streams/FileLines.pir>

=head1 DESCRIPTION

TDB

=head1 METHODS

=over 4

=cut

.sub onload :load, :anon
    .local int i
    .local pmc base
    .local pmc io

    find_type i, "Stream::ParrotIO"
    if i > 1 goto END

    load_bytecode "library/Stream/Base.pir"

    getclass base, "Stream::Base"
    subclass io, base, "Stream::ParrotIO"

    addattribute io, "blocksize"
END:
.end

.namespace ["Stream::ParrotIO"]

.sub __init :method
    self."blockSize"( 50 )
.end

=item stream."open"( name, mode )

Uses the open op to create a ParrotIO which is used as the source.

=cut

.sub open :method
    .param string name
    .param string mode
    .local pmc pio

    open pio, name, mode
    assign self, pio
.end

=item size = stream."blockSize"( [size] )

Sets or returns the current block size.

=cut

.sub blockSize :method
    .param int bs :optional
    .param int has_bs :opt_flag

    .local pmc temp
    .local int ret

    classoffset $I0, self, "Stream::ParrotIO"
    unless has_bs goto GET

    temp = new .PerlInt
    temp = bs
    setattribute self, $I0, temp
    branch RET
GET:
    getattribute temp, self, $I0
RET:
    ret = temp

    .return(ret)
.end

=item source."rawRead"() (B<internal>)

...

=cut

.sub rawRead :method
    .local string str
    .local pmc pio
    .local int bs

    bs = self."blockSize"()
    pio = self."source"()
    read str, pio, bs
    length $I0, str
    if $I0 > 0 goto OK
    self."close"()
    null str
OK:
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
