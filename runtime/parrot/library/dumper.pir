=head1 TITLE

dumper.pir - PIR version of Data::Dumper

=head1 VERSION

version 0.10

=head1 SYNOPSIS

	...
	# dump the P0 register
	_dumper( P0 )

	# dump the P0 register, with "name"
	_dumper( P0, "name" )
	...

	END
	.include "library/dumper.pir"


=head1 DESCRIPTION

    PIR implementation of Perl's Data::Dumper module.

=cut

# first method prints usage information
.sub __library_dumper_onload
    print "usage:"
    print "\tload_bytecode \"library/Data/Dumper.pir\"\n"
    print "\t...\n"
    print "\tnew dumper, \"Data::Dumper\"\n"
    print "\tdumper.\"dumper\"( foo, \"foo\" )\n\n"
    end
.end

.include "errors.pasm"

=head1 FUNCTIONS

This library provides the following functions:

=over 4

=item _dumper( pmc, ?name, ?indent] )

This is the public (non object) interface to the dumper library.

=over 4

=item pmc

Required. The PMC to dump.

=item name

Optional. The name of the PMC.

=item indent

Optional. The indent used at the start of each line printed.

=back

B<Note:> This function currently returns nothing. It should return
the dumped data as a string, like Perl's Data::Dumper. Instead,
everything is printed out using C<print>.

B<Note: #2> Hash keys are now sorted using C<_sort()> (library/sort.pir)

=cut

.sub _dumper
    .param pmc p
    .param string name    :optional
    .param int has_name   :opt_flag
    .param string ident   :optional
    .param int has_ident  :opt_flag
    $P2 = _global_dumper()
    if has_ident goto w_ident
    unless has_name goto wo_name        # XXX argument order, opt 1st
    $P2."dumper"(p, name)
    goto ex
wo_name:
    $P2."dumper"(p)
    goto ex
w_ident:
    $P2."dumper"(p, name, ident)
ex:
.end

=item _register_dumper( id, sub )

Registers a dumper for new PMC type. B<UNIMPLEMENTED>
But see B<method __dunp> below.

=over 4

=item id

the PMC id, as returned by the C<typeof> op.

=item sub

a Sub pmc, that gets called in order to dump the content of the given PMC

=back

For example:

	sub = find_name "_dump_PerlArray"
	_register_dumper( .PerlArray, sub )

This function returns nothing.

=cut

.sub _register_dumper
    .param int id
    .param pmc s
    $P2 = _global_dumper()
    $P2."registerDumper"(id, s)
.end

=item __dump(pmc dumper, str label) method

If a method C<__dump> exists in the namespace of the class, it will be
called with the current dumper object and the label of the PMC.

=item dumper =_global_dumper() B<(internal)>

Internal helper function.

Returns the global dumper instance used by the non object interface.

=cut

.sub _global_dumper
    .local pmc self
    .local int mytype
    .local int type

    find_type mytype, "Data::Dumper"
    if mytype != 0 goto TYPE_OK
    load_bytecode "library/Data/Dumper.pir"
    find_type mytype, "Data::Dumper"
    if mytype != 0 goto TYPE_OK
    print "fatal error: failure while loading library/Data/Dumper.pir\n"
    end
TYPE_OK:

    errorsoff .PARROT_ERRORS_GLOBALS_FLAG
    find_global self, "Data::Dumper", "global"
    errorson .PARROT_ERRORS_GLOBALS_FLAG
    if null self goto create_type

    typeof type, self
    if type == mytype goto END
create_type:
    new self, mytype
    store_global "Data::Dumper", "global", self

END:
    .pcc_begin_return
    .return self
    .pcc_end_return
.end

=back

=head1 AUTHOR

Jens Rieks E<lt>parrot at jensbeimsurfen dot deE<gt> is the author
and maintainer.
Please send patches and suggestions to the Perl 6 Internals mailing list.

=head1 COPYRIGHT

Copyright (c) 2004, the Perl Foundation.

=cut
