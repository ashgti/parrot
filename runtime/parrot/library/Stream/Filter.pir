=head1 TITLE

Stream::Filter - filter and/or alter a stream

=head1 VERSION

version 0.1

=head1 SYNOPSIS

please see F<examples/streams/Filter.pir>

=head1 DESCRIPTION

TBD

=head1 METHODS

=over 4

=cut

.sub onload :load, :anon
    .local int i
    .local pmc base
    .local pmc filter

    find_type i, "Stream::Filter"
    if i > 1 goto END

    load_bytecode "library/Stream/Base.pir"

    getclass base, "Stream::Base"
    subclass filter, base, "Stream::Filter"

    addattribute filter, "filter"
END:
.end

.namespace ["Stream::Filter"]

=item source."filter"( filter )

Sets or returns the filter sub.

=cut

.sub filter method
    .param pmc _filter :optional
    .param int has_filter :opt_flag
    .local pmc ret

    classoffset $I0, self, "Stream::Filter"
    unless has_filter goto GET
    ret = _filter
    setattribute self, $I0, _filter
    branch END
GET:
    getattribute ret, self, $I0
END:
    .return(ret)
.end

=item source."rawRead"() (B<internal>)

...

=cut

.sub rawRead method
    .local pmc source
    .local pmc filter
    .local string str

    source = self."source"()
    filter = self."filter"()
REDO:
    str = source."read"()
    if_null str, END
    str = filter( str )
    if_null str, REDO
END:
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
