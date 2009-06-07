# Copyright (C) 2004-2008, Parrot Foundation.
# $Id$

=head1 TITLE

libpcre.pir - NCI interface to Perl-Compatible Regular Expression library

=head1 DESCRIPTION

See 'library/pcre.pir' for details on the user interface.

=cut


.namespace ['PCRE::NCI']


.sub compile
    .param string pat
    .param int options

    .local string error
    .local pmc PCRE_NCI_compile
    .local int error_size

    .local pmc NULL
    null NULL

    .local pmc errptr
    errptr= new 'Integer'

    ## error message string size
    error_size= 500

    ## allocate space in string for error message
    repeat error, " ", error_size

    PCRE_NCI_compile = get_hll_global ['PCRE::NCI'], 'PCRE_compile'

    .local pmc code

    code = PCRE_NCI_compile( pat, options, error, errptr, NULL )

    .local int is_code_defined
    is_code_defined = defined code
    unless is_code_defined goto RETURN

    error = ""

RETURN:
    .return( code, error, errptr )
.end


.sub exec
    .param pmc regex
    .param string s
    .param int start
    .param int options

    .local int len
    length len, s

    .local pmc NULL
    null NULL

    ## osize -- 1/(2/3) * 4 * 2
    .local int osize
    osize = 12

    ## number of result pairs
    .local int num_result_pairs
    num_result_pairs = 10

    .local int ovector_length
    ovector_length = osize * num_result_pairs

    .local pmc ovector
    ovector = new 'ManagedStruct'
    ovector = ovector_length

    ## on 32 bit systems
    .local pmc PCRE_NCI_exec
    PCRE_NCI_exec = get_hll_global ['PCRE::NCI'], 'PCRE_exec'

    .local int ok

    ok = PCRE_NCI_exec( regex, NULL, s, len, start, options, ovector, 10 )

    .return( ok, ovector )
.end


.sub result
    .param string s
    .param int ok
    .param pmc ovector
    .param int n

    .local string match
    match= ""
    if ok <= 0 goto NOMATCH

    .local int ovecs
    .local int ovece

    .local pmc struct
    struct = new 'FixedPMCArray'
    struct = 3

    .include "datatypes.pasm"

    struct[0] = .DATATYPE_INT
    $I0 = ok * 2
    struct[1] = $I0
    struct[2] = 0
    assign ovector, struct
    $I0 = n * 2
    ovecs = ovector[0;$I0]
    inc $I0
    ovece = ovector[0;$I0]
    $I0 = ovece - ovecs
    if ovecs >= 0 goto M1
    match = ""
    goto M0
M1:
    substr match, s, ovecs, $I0
M0:
NOMATCH:
    .return( match )
.end

=for todo
    # or use convenience function
    print "copy_substring\n"
    i = 0
    repeat match, " ", 500
loop:
    .begin_call
    .set_arg s
    .set_arg ovector
    .set_arg ok
    .set_arg i
    .set_arg match
    .set_arg 500
    .nci_call COPY_SUBSTRING
    .end_call
    if i goto subp
    print "all "
    goto all
subp:
    print "("
    print i
    print ") "
all:
    print "matched: '"
    print match
    print "'\n"
    inc i
    if i < ok goto loop
    end
.end

=head1 FILES

pcre.pir, libpcre.pir

=head1 SEE ALSO

pcre(3)

=head1 AUTHORS

Original code by Leo Toetsch, updated by Jerry Gay
E<lt>jerry dot gay at gmail dot com<gt>

=cut


# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
