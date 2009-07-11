# Copyright (C) 2005-2008, Parrot Foundation.
# $Id$

=head1 NAME

YAML/Parser/Syck.pir - Interface to libsyck

=head1 SYNOPSIS

This won't work yet with a standard libsyck, as libsyck needs to be
aware of Parrot.

=head1 DESCRIPTION

Ask libsyck to do some work.
Uses the native call interface.
Needs synchronous callbacks.

=head1 METHODS

=cut

=head2 load

Parses a YAML string and returns a data structure.

=cut

.include "datatypes.pasm"

.sub load
    .param string yaml_string

    # prepare user data
    .local pmc user_data
    user_data = new 'String'

    .local string library_name
    library_name = 'libsyck'
    .local pmc libsyck
    libsyck = loadlib library_name
    unless libsyck goto NOT_LOADED

    .local int is_defined

    # getting a handle on a YAML parser
    .local pmc syck_new_parser
    syck_new_parser = dlfunc libsyck, "syck_new_parser", "p"
    .local pmc parser
    ( parser ) = syck_new_parser( )
    is_defined = defined parser
    unless is_defined goto NOT_DEFINED

    # A Sub that can be given to the library
    # this callback function will eventually by called by the library
    .const 'Sub' yaml_handler = "_yaml_handler"
    .local pmc yaml_handler_wrapped
    yaml_handler_wrapped = new_callback yaml_handler, user_data, "vUp"	# Z in pdd16

    .local pmc synchronous
    synchronous = new 'Integer'
    synchronous = 1
    setprop user_data, "_synchronous", synchronous

    # call syck_parser_handler
    .local pmc syck_parser_handler
    syck_parser_handler = dlfunc libsyck, "syck_parser_handler", "vpp"
    syck_parser_handler( parser, yaml_handler_wrapped )

    # tell about the user_data
    .local pmc syck_parser_bonus
    syck_parser_bonus = dlfunc libsyck, "syck_parser_bonus", "vpP"
    syck_parser_bonus( parser, user_data )

    # tell about the string
    .local pmc syck_parser_str, read
    read = new 'UnManagedStruct'
    pin yaml_string
    syck_parser_str = dlfunc libsyck, "syck_parser_str", "vpbip"
    .local int yaml_string_len
    yaml_string_len = length yaml_string
    syck_parser_str( parser, yaml_string, yaml_string_len, read )

    # call syck_parse
    .local pmc syck_parse
    .local int sym_id
    syck_parse = dlfunc libsyck, "syck_parse", "ip"
    ( sym_id ) = syck_parse( parser )

    # cleaning up a handle on a YAML parser
    .local pmc syck_free_parser
    syck_free_parser = dlfunc libsyck, "syck_free_parser", "vp"
    syck_free_parser( parser )

NOT_DEFINED:
NOT_LOADED:
    .return( user_data )
.end


.sub _yaml_handler
    .param pmc user_data
    .param pmc external_data

    # struct _syck_node {
    #     /* Symbol table ID */
    #     SYMID id;
    #     /* Underlying kind */
    #     enum syck_kind_tag kind;
    #     /* Fully qualified tag-uri for type */
    #     char *type_id;
    #     /* Anchor name */
    #     char *anchor;
    #     union {
    #         /* Storage for map data */
    #         struct SyckMap {
    #             SYMID *keys;
    #             SYMID *values;
    #             long capa;
    #             long idx;
    #         } *pairs;
    #         /* Storage for sequence data */
    #         struct SyckSeq {
    #             SYMID *items;
    #             long capa;
    #             long idx;
    #         } *list;
    #         /* Storage for string data */
    #         struct SyckStr {
    #             enum scalar_style style;
    #             char *ptr;
    #             long len;
    #         } *str;
    #     } data;
    #     /* Shortcut node */
    #     void *shortcut;
    # };


    # external_data is an UnManagedStruct PMC containing node info
    .local pmc external_data_decl, data_str_decl, data_str
    external_data_decl = new 'ResizablePMCArray'
    data_str_decl      = new 'ResizablePMCArray'

    # id
    push external_data_decl, .DATATYPE_INT
    push external_data_decl, 0
    push external_data_decl, 0
    # kind
    push external_data_decl, .DATATYPE_INT
    push external_data_decl, 0
    push external_data_decl, 0
    # type_id
    push external_data_decl, .DATATYPE_CSTR
    push external_data_decl, 0
    push external_data_decl, 0
    # anchor
    push external_data_decl, .DATATYPE_CSTR
    push external_data_decl, 0
    push external_data_decl, 0

    # style
    push data_str_decl, .DATATYPE_INT
    push data_str_decl, 0
    push data_str_decl, 0
    # ptr
    push data_str_decl, .DATATYPE_CSTR
    push data_str_decl, 0
    push data_str_decl, 0
    # len
    push data_str_decl, .DATATYPE_INT
    push data_str_decl, 0
    push data_str_decl, 0
    data_str = new 'UnManagedStruct', data_str_decl

    push  external_data_decl, .DATATYPE_STRUCT_PTR
    $P9 = external_data_decl[-1]
    setprop $P9, "_struct", data_str
    push external_data_decl, 0
    push external_data_decl, 0

    assign external_data, external_data_decl

    .local int id, kind, data_str_len
    .local string type_id, anchor, data_str_ptr
    .local string cloned
    id      = external_data[0]
    kind    = external_data[1]
    #    type_id = external_data[2]     # This might be NULL
    #    anchor = external_data[3]      # This might be NULL
    data_str_len = external_data[4;2]

    # For now just save all the strings
    unless kind == 2 goto IGNORE_NON_STRING
        data_str_ptr = external_data[4;1]
        concat user_data, user_data, data_str_ptr
        concat user_data, user_data, "\n"
IGNORE_NON_STRING:

    # debug output
    print user_data
    print "\n"

    .return()
.end

=head1 TODO

Do some object stuff.
Be nice and put stuff into a namespace.
Provide a patch for libsyck.

=head1 AUTHOR

Bernhard Schmalhofer - <Bernhard.Schmalhofer@gmx.de>

=head1 SEE ALSO

  YAML::Parser::Syck.pm
  yaml.kwiki.org

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
