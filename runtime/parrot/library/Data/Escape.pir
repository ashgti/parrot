=head1 TITLE

Data::Escape - simple string escaping implemented in PIR

=head1 SYNOPSIS

    ...

    load_bytecode "library/Data/Escape.pir"

    .local pmc escape

    escape = find_global "Data::Escape", "String"

    str = escape( str, "'" )

    ...


=head1 DESCRIPTION

This library escapes data to make it usable in PIR and C code.

=cut

=head1 FUNCTIONS

This library provides the following function:

=over 4

=item str = String( str, [char] )

A simple string escaping function.

=over 4

=item C<str>

The string to escape.

=item C<char>

The quote character used for printing, either ' or ".

=back

This function returns the quoted string.

=cut

.namespace ["Data::Escape"]


.sub String
    .param string str
    .param string quote_char   :optional
    .param int has_quote       :opt_flag

    .local string return
    .local int index
    .local int str_len
    .local int test_char_ord
    .local int quote_char_ord
    .local int is_alphanum
    .local string tmp
    .local int mask

    .include 'cclass.pasm'
    mask = .CCLASS_PRINTING

    index = 0
    str_len = length str
    return = ''
LOOP:
    if index >= str_len goto END

    tmp = str[index]

    ord test_char_ord, tmp

    ## allow spaces as is
    if 32 == test_char_ord goto DONE

t_backslash:
    ## escape backslash to double backslash
    if 92 != test_char_ord goto t_newline
    tmp = '\\'
    branch DONE

t_newline:
    ## escape newline to \n
    if 10 != test_char_ord goto t_tab
    tmp = '\n'
    branch DONE

t_tab:
    ## escape tab to \t
    if 9 != test_char_ord goto t_quote
    tmp = '\t'
    branch DONE

t_quote:
    ## escape quote character
    unless has_quote goto escape_qu
    ord quote_char_ord, quote_char

    if quote_char_ord == test_char_ord goto escape_quote_char
escape_qu:
    if 34 == test_char_ord goto DONE
    if 39 == test_char_ord goto DONE
    branch default

escape_quote_char:
    tmp = '\'
    concat tmp, quote_char
    branch DONE

default:
    if test_char_ord >= 256 goto esc_unicode
    if test_char_ord >= 128 goto esc_oct
    is_alphanum = is_cclass mask, tmp, 0
    if is_alphanum goto DONE
esc_oct:
    ## otherwise encode the value as an octal number
    $P0 = new .ResizableIntegerArray
    push $P0, test_char_ord
    tmp = sprintf "\\%03o", $P0
    branch DONE
esc_unicode:
    $P0 = new .ResizableIntegerArray
    push $P0, test_char_ord
    tmp = sprintf "\\x{%x}", $P0
    # fall though to DONE
DONE:
    concat return, tmp
    inc index
    branch LOOP
END:
    .return( return )
.end

=back

=head1 AUTHOR

Jens Rieks E<lt>parrot at jensbeimsurfen dot deE<gt> is the author
and maintainer.
Please send patches and suggestions to the Perl 6 Internals mailing list.

=head1 HISTORY

Modifications by Jerry Gay to escape backslash and use character class ops.

=head1 COPYRIGHT

Copyright (C) 2004, The Perl Foundation.

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
