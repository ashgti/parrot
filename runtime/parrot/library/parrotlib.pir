=head1 INFORMATION

This is the parrot bytecode library.

=head1 FUNCTIONS

=over 4

=cut

.namespace ["_parrotlib"]

=item __onload

__onload is the initialization function. It sets the signatures of
parrotlib's interface functions.

=cut

.sub __onload :load
    .local pmc paths
    .local pmc includes
    .local string root


    # XXX todo: get root from config
    $P0 = new .Env
    root = $P0["PARROT_RUNTIME_ROOT"]
    length $I0, root
    if $I0 == 0 goto DEFAULT
    branch OKAY
DEFAULT:
    root = "runtime/parrot"
OKAY:

    # XXX: get include paths from config
    $S0 = clone root
    concat $S0, "/include"
    paths = new .ResizableStringArray
    push paths, "."
    push paths, $S0
    push paths, root

    # create includes array
    includes = new .ResizablePMCArray
    store_global "_parrotlib", "include_paths", includes

    # get the directory handler
    $P0 = find_global "_parrotlib", "handle_directory"

    # fill the includes array
LOOP:
    $P1 = clone $P0
    $P2 = new .PerlString
    $S0 = shift paths
    concat $S0, "/"
    $P2 = $S0
    setprop $P1, "path", $P2
    push includes, $P1
    if paths goto LOOP

    # setup the signatures
    set_signature( "include_file_location", "SS" )
    set_signature( "imcc_compile_file_location", "SS" )
    set_signature( "dynext_location", "SSSP" )
.end

.sub set_signature
    .param string name
    .param string sig

    $P1 = new .PerlString
    $P1 = sig
    find_global $P0, "_parrotlib", name
    setprop $P0, "signature", $P1
    store_global "_parrotlib", name, $P0
.end

=item STRING = include_file_location( STRING )

Is called by IMCC when it encounters an C<.include> statement.
The string parameter the argument of this statement.

This function returns the absolute filename of the requested file.

=cut

.sub include_file_location
    .param string name

    find_global $P0, "_parrotlib", "include_paths"
    $S0 = find_file_path( name, $P0 )

    .pcc_begin_return
    .return $S0
    .pcc_end_return
.end

=item STRING = bytecode_location( STRING )

Is called by Parrot_load_bytecode.
The string parameter is the name of the file to be loaded.

This function returns the absolute filename of the requested file.

=cut

.sub imcc_compile_file_location
    .param string name

    find_global $P0, "_parrotlib", "include_paths"
    $S0 = find_file_path( name, $P0 )

    .pcc_begin_return
    .return $S0
    .pcc_end_return
.end

=item STRING = dynext_location( STRING )

Returns the location of a dynamic extension.

=cut

.sub dynext_location
    .param string request
    .param string ext
    .local string name

    name = request
    stat $I0, name, 0
    if $I0 goto END

    name = clone request
    concat name, ext
    stat $I0, name, 0
    if $I0 goto END

    name = "runtime/parrot/dynext/"
    concat name, request
    stat $I0, name, 0
    if $I0 goto END

    name = "runtime/parrot/dynext/"
    concat name, request
    concat name, ext
    stat $I0, name, 0
    if $I0 goto END

    # file not found, give the OS a chance to locate it
    name = clone request
    concat name, ext

END:
    .pcc_begin_return
    .return name
    .pcc_end_return
.end


.sub find_file_path
    .param string name
    .param pmc array
    .local string ret
    .local pmc iter

    iter = new .Iterator, array
    iter = 0 #ITERATE_FROM_START

NEXT:
    null ret
    unless iter goto END

    $P0 = shift iter
    ret = $P0( name )
    if_null ret, NEXT
END:
    .pcc_begin_return
    .return ret
    .pcc_end_return
.end

.sub handle_directory
    .param string name
    .local string path

.include "interpinfo.pasm"
    interpinfo P0, .INTERPINFO_CURRENT_SUB
    getprop $P0, "path", P0
    path = $P0

    $S0 = clone path
    concat $S0, name
    stat $I0, $S0, 0
    if $I0 goto OK
    null $S0
OK:
    .pcc_begin_return
    .return $S0
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
