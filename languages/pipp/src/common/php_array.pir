# Copyright (C) 2008, The Perl Foundation.
# $Id$

=head1 NAME

php_array.pir - PHP array Standard Library

=head1 DESCRIPTION

=head2 Functions

=over 4

=cut

=item C<array array_change_key_case(array input [, int case=CASE_LOWER])>

Retuns an array with all string keys lowercased [or uppercased]

NOT IMPLEMENTED.

=cut

.sub 'array_change_key_case'
    not_implemented()
.end

=item C<array array_chunk(array input, int size [, bool preserve_keys])>

Split array into chunks

NOT IMPLEMENTED.

=cut

.sub 'array_chunk'
    not_implemented()
.end

=item C<array array_combine(array keys, array values)>

Creates an array by using the elements of the first parameter as keys and the elements of the second as the corresponding values

NOT IMPLEMENTED.

=cut

.sub 'array_combine'
    not_implemented()
.end

=item C<array array_count_values(array input)>

Return the value as key and the frequency of that value in input as value

NOT IMPLEMENTED.

=cut

.sub 'array_count_values'
    not_implemented()
.end

=item C<array array_diff(array arr1, array arr2 [, array ...])>

Returns the entries of arr1 that have values which are not present in any of the others arguments.

NOT IMPLEMENTED.

=cut

.sub 'array_diff'
    not_implemented()
.end

=item C<array array_diff_assoc(array arr1, array arr2 [, array ...])>

Returns the entries of arr1 that have values which are not present in any of the others arguments but do additional checks whether the keys are equal

NOT IMPLEMENTED.

=cut

.sub 'array_diff_assoc'
    not_implemented()
.end

=item C<array array_diff_key(array arr1, array arr2 [, array ...])>

Returns the entries of arr1 that have keys which are not present in any of the others arguments. This function is like array_diff() but works on the keys instead of the values. The associativity is preserved.

NOT IMPLEMENTED.

=cut

.sub 'array_diff_key'
    not_implemented()
.end

=item C<array array_diff_uassoc(array arr1, array arr2 [, array ...], callback data_comp_func)>

Returns the entries of arr1 that have values which are not present in any of the others arguments but do additional checks whether the keys are equal. Elements are compared by user supplied function.

NOT IMPLEMENTED.

=cut

.sub 'array_diff_uassoc'
    not_implemented()
.end

=item C<array array_diff_ukey(array arr1, array arr2 [, array ...], callback key_comp_func)>

Returns the entries of arr1 that have keys which are not present in any of the others arguments. User supplied function is used for comparing the keys. This function is like array_udiff() but works on the keys instead of the values. The associativity is preserved.

NOT IMPLEMENTED.

=cut

.sub 'array_diff_ukey'
    not_implemented()
.end

=item C<array array_fill(int start_key, int num, mixed val)>

Create an array containing num elements starting with index start_key each initialized to val

NOT IMPLEMENTED.

=cut

.sub 'array_fill'
    not_implemented()
.end

=item C<array array_fill_keys(array keys, mixed val)>

Create an array using the elements of the first parameter as keys each initialized to val

NOT IMPLEMENTED.

=cut

.sub 'array_fill_keys'
    not_implemented()
.end

=item C<array array_filter(array input [, mixed callback])>

Filters elements from the array via the callback.

NOT IMPLEMENTED.

=cut

.sub 'array_filter'
    not_implemented()
.end

=item C<array array_flip(array input)>

Return array with key <-> value flipped

NOT IMPLEMENTED.

=cut

.sub 'array_flip'
    not_implemented()
.end

=item C<array array_intersect(array arr1, array arr2 [, array ...])>

Returns the entries of arr1 that have values which are present in all the other arguments

NOT IMPLEMENTED.

=cut

.sub 'array_intersect'
    not_implemented()
.end

=item C<array array_intersect_assoc(array arr1, array arr2 [, array ...])>

Returns the entries of arr1 that have values which are present in all the other arguments. Keys are used to do more restrictive check

NOT IMPLEMENTED.

=cut

.sub 'array_intersect_assoc'
    not_implemented()
.end

=item C<array array_intersect_key(array arr1, array arr2 [, array ...])>

Returns the entries of arr1 that have keys which are present in all the other arguments. Kind of equivalent to array_diff(array_keys($arr1), array_keys($arr2)[,array_keys(...)]). Equivalent of array_intersect_assoc() but does not do compare of the data.

NOT IMPLEMENTED.

=cut

.sub 'array_intersect_key'
    not_implemented()
.end

=item C<array array_intersect_uassoc(array arr1, array arr2 [, array ...], callback key_compare_func)>

Returns the entries of arr1 that have values which are present in all the other arguments. Keys are used to do more restrictive check and they are compared by using an user-supplied callback.

NOT IMPLEMENTED.

=cut

.sub 'array_intersect_uassoc'
    not_implemented()
.end

=item C<array array_intersect_ukey(array arr1, array arr2 [, array ...], callback key_compare_func)>

Returns the entries of arr1 that have keys which are present in all the other arguments. Kind of equivalent to array_diff(array_keys($arr1), array_keys($arr2)[,array_keys(...)]). The comparison of the keys is performed by a user supplied function. Equivalent of array_intersect_uassoc() but does not do compare of the data.

NOT IMPLEMENTED.

=cut

.sub 'array_intersect_ukey'
    not_implemented()
.end

=item C<bool array_key_exists(mixed key, array search)>

Checks if the given key or index exists in the array

NOT IMPLEMENTED.

=cut

.sub 'array_key_exists'
    not_implemented()
.end

=item C<array array_keys(array input [, mixed search_value[, bool strict]])>

Return just the keys from the input array, optionally only for the specified search_value

NOT IMPLEMENTED.

=cut

.sub 'array_keys'
    not_implemented()
.end

=item C<array array_map(mixed callback, array input1 [, array input2 ,...])>

Applies the callback to the elements in given arrays.

NOT IMPLEMENTED.

=cut

.sub 'array_map'
    not_implemented()
.end

=item C<array array_merge(array arr1, array arr2 [, array ...])>

Merges elements from passed arrays into one array

NOT IMPLEMENTED.

=cut

.sub 'array_merge'
    not_implemented()
.end

=item C<array array_merge_recursive(array arr1, array arr2 [, array ...])>

Recursively merges elements from passed arrays into one array

NOT IMPLEMENTED.

=cut

.sub 'array_merge_recursive'
    not_implemented()
.end

=item C<bool array_multisort(array ar1 [, SORT_ASC|SORT_DESC [, SORT_REGULAR|SORT_NUMERIC|SORT_STRING]] [, array ar2 [, SORT_ASC|SORT_DESC [, SORT_REGULAR|SORT_NUMERIC|SORT_STRING]], ...])>

Sort multiple arrays at once similar to how ORDER BY clause works in SQL

NOT IMPLEMENTED.

=cut

.sub 'array_multisort'
    not_implemented()
.end

=item C<array array_pad(array input, int pad_size, mixed pad_value)>

Returns a copy of input array padded with pad_value to size pad_size

NOT IMPLEMENTED.

=cut

.sub 'array_pad'
    not_implemented()
.end

=item C<mixed array_pop(array stack)>

Pops an element off the end of the array

NOT IMPLEMENTED.

=cut

.sub 'array_pop'
    not_implemented()
.end

=item C<mixed array_product(array input)>

Returns the product of the array entries

NOT IMPLEMENTED.

=cut

.sub 'array_product'
    not_implemented()
.end

=item C<int array_push(array stack, mixed var [, mixed ...])>

Pushes elements onto the end of the array

NOT IMPLEMENTED.

=cut

.sub 'array_push'
    not_implemented()
.end

=item C<mixed array_rand(array input [, int num_req])>

Return key/keys for random entry/entries in the array

NOT IMPLEMENTED.

=cut

.sub 'array_rand'
    not_implemented()
.end

=item C<mixed array_reduce(array input, mixed callback [, int initial])>

Iteratively reduce the array to a single value via the callback.

NOT IMPLEMENTED.

=cut

.sub 'array_reduce'
    not_implemented()
.end

=item C<array array_reverse(array input [, bool preserve keys])>

Return input as a new array with the order of the entries reversed

NOT IMPLEMENTED.

=cut

.sub 'array_reverse'
    not_implemented()
.end

=item C<mixed array_search(mixed needle, array haystack [, bool strict])>

Searches the array for a given value and returns the corresponding key if successful

NOT IMPLEMENTED.

=cut

.sub 'array_search'
    not_implemented()
.end

=item C<mixed array_shift(array stack)>

Pops an element off the beginning of the array

NOT IMPLEMENTED.

=cut

.sub 'array_shift'
    not_implemented()
.end

=item C<array array_slice(array input, int offset [, int length [, bool preserve_keys]])>

Returns elements specified by offset and length

NOT IMPLEMENTED.

=cut

.sub 'array_slice'
    not_implemented()
.end

=item C<array array_splice(array input, int offset [, int length [, array replacement]])>

Removes the elements designated by offset and length and replace them with supplied array

NOT IMPLEMENTED.

=cut

.sub 'array_splice'
    not_implemented()
.end

=item C<mixed array_sum(array input)>

Returns the sum of the array entries

NOT IMPLEMENTED.

=cut

.sub 'array_sum'
    not_implemented()
.end

=item C<array array_udiff(array arr1, array arr2 [, array ...], callback data_comp_func)>

Returns the entries of arr1 that have values which are not present in any of the others arguments. Elements are compared by user supplied function.

NOT IMPLEMENTED.

=cut

.sub 'array_udiff'
    not_implemented()
.end

=item C<array array_udiff_assoc(array arr1, array arr2 [, array ...], callback key_comp_func)>

Returns the entries of arr1 that have values which are not present in any of the others arguments but do additional checks whether the keys are equal. Keys are compared by user supplied function.

NOT IMPLEMENTED.

=cut

.sub 'array_udiff_assoc'
    not_implemented()
.end

=item C<array array_udiff_uassoc(array arr1, array arr2 [, array ...], callback data_comp_func, callback key_comp_func)>

Returns the entries of arr1 that have values which are not present in any of the others arguments but do additional checks whether the keys are equal. Keys and elements are compared by user supplied functions.

NOT IMPLEMENTED.

=cut

.sub 'array_udiff_uassoc'
    not_implemented()
.end

=item C<array array_uintersect(array arr1, array arr2 [, array ...], callback data_compare_func)>

Returns the entries of arr1 that have values which are present in all the other arguments. Data is compared by using an user-supplied callback.

NOT IMPLEMENTED.

=cut

.sub 'array_uintersect'
    not_implemented()
.end

=item C<array array_uintersect_assoc(array arr1, array arr2 [, array ...], callback data_compare_func)>

Returns the entries of arr1 that have values which are present in all the other arguments. Keys are used to do more restrictive check. Data is compared by using an user-supplied callback.

NOT IMPLEMENTED.

=cut

.sub 'array_uintersect_assoc'
    not_implemented()
.end

=item C<array array_uintersect_uassoc(array arr1, array arr2 [, array ...], callback data_compare_func, callback key_compare_func)>

Returns the entries of arr1 that have values which are present in all the other arguments. Keys are used to do more restrictive check. Both data and keys are compared by using user-supplied callbacks.

NOT IMPLEMENTED.

=cut

.sub 'array_uintersect_uassoc'
    not_implemented()
.end

=item C<array array_unique(array input)>

Removes duplicate values from array

NOT IMPLEMENTED.

=cut

.sub 'array_unique'
    not_implemented()
.end

=item C<int array_unshift(array stack, mixed var [, mixed ...])>

Pushes elements onto the beginning of the array

NOT IMPLEMENTED.

=cut

.sub 'array_unshift'
    not_implemented()
.end

=item C<array array_values(array input)>

Return just the values from the input array

NOT IMPLEMENTED.

=cut

.sub 'array_values'
    not_implemented()
.end

=item C<bool array_walk(array input, string funcname [, mixed userdata])>

Apply a user function to every member of an array

NOT IMPLEMENTED.

=cut

.sub 'array_walk'
    not_implemented()
.end

=item C<bool array_walk_recursive(array input, string funcname [, mixed userdata])>

Apply a user function recursively to every member of an array

NOT IMPLEMENTED.

=cut

.sub 'array_walk_recursive'
    not_implemented()
.end

=item C<bool arsort(array &array_arg [, int sort_flags])>

Sort an array in reverse order and maintain index association

NOT IMPLEMENTED.

=cut

.sub 'arsort'
    not_implemented()
.end

=item C<bool asort(array &array_arg [, int sort_flags])>

Sort an array and maintain index association

NOT IMPLEMENTED.

=cut

.sub 'asort'
    not_implemented()
.end

=item C<array compact(mixed var_names [, mixed ...])>

Creates a hash containing variables and their values

NOT IMPLEMENTED.

=cut

.sub 'compact'
    not_implemented()
.end

=item C<int count(mixed var [, int mode])>

Count the number of elements in a variable (usually an array)

NOT IMPLEMENTED.

=cut

.sub 'count'
    not_implemented()
.end

=item C<mixed current(array array_arg)>

Return the element currently pointed to by the internal array pointer

NOT IMPLEMENTED.

=cut

.sub 'current'
    not_implemented()
.end

=item C<mixed end(array array_arg)>

Advances array argument's internal pointer to the last element and return it

NOT IMPLEMENTED.

=cut

.sub 'end'
    not_implemented()
.end

=item C<int extract(array var_array [, int extract_type [, string prefix]])>

Imports variables into symbol table from an array

NOT IMPLEMENTED.

=cut

.sub 'extract'
    not_implemented()
.end

=item C<bool in_array(mixed needle, array haystack [, bool strict])>

Checks if the given value exists in the array

NOT IMPLEMENTED.

=cut

.sub 'in_array'
    not_implemented()
.end

=item C<mixed key(array array_arg)>

Return the key of the element currently pointed to by the internal array pointer

NOT IMPLEMENTED.

=cut

.sub 'key'
    not_implemented()
.end

=item C<bool krsort(array &array_arg [, int sort_flags])>

Sort an array by key value in reverse order

NOT IMPLEMENTED.

=cut

.sub 'krsort'
    not_implemented()
.end

=item C<bool ksort(array &array_arg [, int sort_flags])>

Sort an array by key

NOT IMPLEMENTED.

=cut

.sub 'ksort'
    not_implemented()
.end

=item C<mixed max(mixed arg1 [, mixed arg2 [, mixed ...]])>

Return the highest value in an array or a series of arguments

NOT IMPLEMENTED.

=cut

.sub 'max'
    not_implemented()
.end

=item C<mixed min(mixed arg1 [, mixed arg2 [, mixed ...]])>

Return the lowest value in an array or a series of arguments

NOT IMPLEMENTED.

=cut

.sub 'min'
    not_implemented()
.end

=item C<void natcasesort(array &array_arg)>

Sort an array using case-insensitive natural sort

NOT IMPLEMENTED.

=cut

.sub 'natcasesort'
    not_implemented()
.end

=item C<void natsort(array &array_arg)>

Sort an array using natural sort

NOT IMPLEMENTED.

=cut

.sub 'natsort'
    not_implemented()
.end

=item C<mixed next(array array_arg)>

Move array argument's internal pointer to the next element and return it

NOT IMPLEMENTED.

=cut

.sub 'next'
    not_implemented()
.end

=item C<mixed prev(array array_arg)>

Move array argument's internal pointer to the previous element and return it

NOT IMPLEMENTED.

=cut

.sub 'prev'
    not_implemented()
.end

=item C<array range(mixed low, mixed high[, int step])>

Create an array containing the range of integers or characters from low to high (inclusive)

NOT IMPLEMENTED.

=cut

.sub 'range'
    not_implemented()
.end

=item C<mixed reset(array array_arg)>

Set array argument's internal pointer to the first element and return it

NOT IMPLEMENTED.

=cut

.sub 'reset'
    not_implemented()
.end

=item C<bool rsort(array &array_arg [, int sort_flags])>

Sort an array in reverse order

NOT IMPLEMENTED.

=cut

.sub 'rsort'
    not_implemented()
.end

=item C<bool shuffle(array array_arg)>

Randomly shuffle the contents of an array

NOT IMPLEMENTED.

=cut

.sub 'shuffle'
    not_implemented()
.end

=item C<bool sort(array &array_arg [, int sort_flags])>

Sort an array

NOT IMPLEMENTED.

=cut

.sub 'sort'
    not_implemented()
.end

=item C<bool uasort(array array_arg, string cmp_function)>

Sort an array with a user-defined comparison function and maintain index association

NOT IMPLEMENTED.

=cut

.sub 'uasort'
    not_implemented()
.end

=item C<bool uksort(array array_arg, string cmp_function)>

Sort an array by keys using a user-defined comparison function

NOT IMPLEMENTED.

=cut

.sub 'uksort'
    not_implemented()
.end

=item C<bool usort(array array_arg, string cmp_function)>

Sort an array by values using a user-defined comparison function

NOT IMPLEMENTED.

=cut

.sub 'usort'
    not_implemented()
.end

=back

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
