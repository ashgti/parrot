=head1 TITLE

Data::Replace - data replacing implemented in PIR

=head1 SYNOPSIS

    ...

    load_bytecode "library/Data/Replace.pir"

    .local pmc replace

    replace = find_global "Data::Replace", "replace"

    replace( array, old, new )

    ...


=head1 DESCRIPTION

Replaces every occurrence of a specified PMC with another PMC
in a nested, possible self-referential data structure.

=head1 FUNCTIONS

This library provides the following function:

=over 4

=cut

.namespace ["Data::Replace"]

=item replace( where, old, new )

Replaces every "old" with "new" inside the aggregate "where".

=cut

.sub replace
    .param pmc where
    .param pmc oldVal
    .param pmc newVal
    .local pmc cache
    
    new cache, .PerlArray
    __do_replace( where, oldVal, newVal, cache )
.end


.sub __in_cache
    .param pmc val
    .param pmc cache
    .local int i
    .local pmc temp
    
    set i, cache
LOOP:
    dec i
    if i < 0 goto END
    temp = cache[i]
    eq_addr val, temp, IS
    branch LOOP
END:
    .pcc_begin_return
    .return 0
    .pcc_end_return
IS:
    .pcc_begin_return
    .return 1
    .pcc_end_return
.end
    
.sub __do_replace
    .param pmc where
    .param pmc oldVal
    .param pmc newVal
    .param pmc cache
    .local pmc temp
    .local string name
    
    if_null where, END
    
    # call __replace if the PMC supports it
    can $I0, where, "__replace"
    unless $I0 goto CANT_REPLACE
    where."__replace"( oldVal, newVal, cache )
    branch REPLACE_PROPS
CANT_REPLACE:

    # find a Data::Replace method with the name of the type to replace
    typeof name, where
    errorsoff .PARROT_ERRORS_GLOBALS_FLAG
    find_global temp, "Data::Replace", name
    errorson .PARROT_ERRORS_GLOBALS_FLAG
    # invoke it if found
    typeof $I0, temp
    if $I0 == .Undef goto REPLACE_PROPS
    temp( where, oldVal, newVal, cache )

REPLACE_PROPS:
    prophash temp, where
    set $I0, temp
    if $I0 == 0 goto END
    __do_replace( temp, oldVal, newVal, cache )
END:
.end

.sub __onload :load
    $P0 = find_global "Data::Replace", "PerlArray"
    store_global "Data::Replace", "PMCArray", $P0
    store_global "Data::Replace", "StringArray", $P0
.end

.sub PerlArray
    .param pmc where
    .param pmc oldVal
    .param pmc newVal
    .param pmc cache
    .local int i
    .local pmc val
    
    set i, where
LOOP:
    dec i
    if i < 0 goto END

    val = where[i]
    
    ne_addr val, oldVal, SKIP
    where[i] = newVal
SKIP:
    I0 = __in_cache( val, cache )
    if I0 goto LOOP
    
    push cache, val
    __do_replace( val, oldVal, newVal, cache )
    branch LOOP
END:
    .pcc_begin_return
    .pcc_end_return
.end

.sub PerlHash method
    .param pmc where
    .param pmc oldVal
    .param pmc newVal
    .param pmc cache
    .local pmc iter
    .local string key
    .local pmc val
    
    new iter, .Iterator, where
    set iter, 0
    
LOOP:
    unless iter, END

    shift key, iter
    
    val = where[key]
    
    ne_addr val, oldVal, SKIP
    where[key] = newVal
SKIP:
    I0 = __in_cache( val, cache )
    if I0 goto LOOP
    
    push cache, val
    __do_replace( val, oldVal, newVal, cache )
    branch LOOP
    
END:
    .pcc_begin_return
    .pcc_end_return
.end

.sub Hash method
    .param pmc where
    .param pmc oldVal
    .param pmc newVal
    .param pmc cache
    .local pmc iter
    .local string key
    .local pmc val
    
    new iter, .Iterator, where
    set iter, 0
    
LOOP:
    unless iter, END

    shift key, iter
    
    val = where[key]
    
    ne_addr val, oldVal, SKIP
    where[key] = newVal
SKIP:
    I0 = __in_cache( val, cache )
    if I0 goto LOOP
    
    push cache, val
    __do_replace( val, oldVal, newVal, cache )
    branch LOOP
    
END:
    .pcc_begin_return
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
