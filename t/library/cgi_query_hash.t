#!./parrot
# Copyright (C) 2006-2007, The Perl Foundation.

=head1 NAME

t/library/cgi_query_hash.t - test [ 'CGI'; 'QueryHash' ]

=head1 SYNOPSIS

    % prove t/library/cgi_query_hash.t

=head1 DESCRIPTION

Test  [ 'CGI'; 'QueryHash' ]

=cut

.include "library/dumper.pir"

.sub test :main

    load_bytecode 'Test/More.pir'
    load_bytecode 'CGI/QueryHash.pbc'

    .local pmc plan, is, ok
    plan = get_hll_global ['Test::More'], 'plan'
    is   = get_hll_global ['Test::More'], 'is'
    ok   = get_hll_global ['Test::More'], 'ok'

    plan(7)

    .local int    is_defined
    .local pmc    query_hash
    .local string val

    .local pmc parse_get_sub
    parse_get_sub = get_global [ 'CGI'; 'QueryHash' ], 'parse_get'
    is_defined = defined parse_get_sub
    ok( is_defined, 'got the sub "parse_get"' )

    # set up environment
    .local pmc my_env
    my_env = new .Env
    ok( my_env, 'Got an .Env pmc' )
    my_env['REQUEST_TYPE'] = 'GET'

    my_env['QUERY_STRING'] = 'as=df'
    query_hash = parse_get_sub()
    val = query_hash['as']
    is( val, 'df', 'single GET param' )

    my_env['QUERY_STRING'] = 'sky=blue;grass=green&water=wet'
    query_hash = parse_get_sub()
    val = query_hash['sky']
    is( val, 'blue', 'first of three GET params' )
    val = query_hash['grass']
    is( val, 'green', 'second of three GET params' )
    val = query_hash['water']
    is( val, 'wet', 'third of three GET params' )

    my_env['QUERY_STRING'] = 'a=1'
    query_hash = parse_get_sub()
    val = query_hash['a']
    is( val, '1', 'numeric value' )

.end   

=head1 AUTHOR

Bernhard Schmalhofer <Bernhard Schmalhofer@gmx.de>

=cut
