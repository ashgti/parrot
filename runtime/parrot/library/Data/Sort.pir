=head1 TITLE

Data::Sort - simple sort algorithm implemented in PIR

=head1 SYNOPSIS

    ...

    load_bytecode "library/Data/Sort.pir"

    .local pmc sort

    sort = find_global "Data::Sort", "simple"

    sort( array_pmc )

    ...

=head1 DESCRIPTION

Sort the specified array.

B<Note:> The C<cmp> op is used to compare the array elements.
If the sorting is done, "cmp val, array[i], array[i+1]"
results in -1, or 0 if both values are equal,
for all values of i in range 0 <= i < array_size-1.

=cut

=head1 FUNCTIONS

This library provides the following function:

=over 4

=item simple( array )

A simple array sort function.

=over 4

=item C<array>

This is the array pmc to be sorted.

=back

This function returns nothing.

=cut

.namespace ["Data::Sort"]

.sub simple
    .param pmc array
    .param int start         :optional
    .param int has_start     :opt_flag
    .local int i
    .local int size
    .local pmc tmp
    .local int minpos
    .local pmc minval
    .local int v

    # check number of INT args
    if has_start == 1 goto OK
    set start, 0
OK:
    size = array
    # already sorted
    if size <= 1 goto END
REDO:
    if start >= size goto END
    i = start
    minpos = -1
LOOP:
    tmp = array[i]
    if minpos == -1 goto DO

    cmp_str v, tmp, minval
    if v > 0 goto SKIP
DO:
    minval = tmp
    minpos = i
SKIP:
    inc i
    if i < size goto LOOP
    if start >= minpos goto SKIP2
    tmp = array[start]
    array[start] = minval
    array[minpos] = tmp
SKIP2:
    inc start
    branch REDO
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

Copyright (C) 2004, The Perl Foundation.

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
