=head1 TITLE

Stream::Base - Stream library base class

=head1 VERSION

version 0.1

=head1 SYNOPSIS

please see F<examples/streams/Coroutine.pir>

=head1 DESCRIPTION

TBD

=head1 METHODS

=over 4

=cut

.sub onload :load, :anon
    .local pmc base
    .local pmc coro
    .local int i
    
    find_type i, "Stream::Coroutine"
    if i > 1 goto END
    
    load_bytecode "library/Stream/Base.pir"

    getclass base, "Stream::Base"
    subclass coro, base, "Stream::Coroutine"
    
END:
.end

.namespace ["Stream::Coroutine"]

=item source."rawRead"() (B<internal>)

...

=cut

.sub rawRead :method
    .local pmc coro
    .local string str

    null str
    coro = self."source"()
    typeof $I0, coro
    if $I0 == .Undef goto END

    str = coro( self )
END:

    .return(str)
.end

=back

=head1 AUTHOR

Jens Rieks E<lt>parrot at jensbeimsurfen dot deE<gt> is the author
and maintainer.
Please send patches and suggestions to the Perl 6 Internals mailing list.

=head1 COPYRIGHT

Copyright (c) 2004-2006, the Perl Foundation.

=cut
