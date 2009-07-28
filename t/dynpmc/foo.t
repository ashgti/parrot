#! perl
# Copyright (C) 2005, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 9;
use Parrot::Config;

=head1 NAME

t/dynpmc/foo.t - Test for a very simple dynamic PMC

=head1 SYNOPSIS

        % prove t/dynpmc/foo.t

=head1 DESCRIPTION

Tests the Foo PMC.

=cut

pir_output_is( << 'CODE', << 'OUTPUT', "get_integer" );

.sub main :main
    loadlib $P1, "foo_group"
    $P1 = new "Foo"

    $I1 = $P1
    print $I1
    print "\n"
.end
CODE
42
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', "loadlib with relative pathname, no ext" );
.sub main :main
    ## load a relative pathname without the extension.  loadlib will convert the
    ## '/' characters to '\\' on windows.
    $S0 = "runtime/parrot/dynext/foo_group"
    loadlib $P1, $S0

    ## ensure that we can still make Foo instances.
    $P1 = new "Foo"
    $I1 = $P1
    print $I1
    print "\n"
.end
CODE
42
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', "loadlib with absolute pathname, no ext" );
.sub main :main
    ## get cwd in $S0.
    .include "iglobals.pasm"
    $P11 = getinterp
    $P12 = $P11[.IGLOBALS_CONFIG_HASH]
    $S0 = $P12["prefix"]

    ## convert cwd to an absolute pathname without the extension, and load it.
    ## this should always find the version in the build directory, since that's
    ## the only place "make test" will work.
    $S0 = concat "/runtime/parrot/dynext/foo_group"
    loadlib $P1, $S0

    ## ensure that we can still make Foo instances.
    $P1 = new "Foo"
    $I1 = $P1
    print $I1
    print "\n"
.end
CODE
42
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', "loadlib with relative pathname & ext" );
.sub main :main
    ## get load_ext in $S0.
    .include "iglobals.pasm"
    $P11 = getinterp
    $P12 = $P11[.IGLOBALS_CONFIG_HASH]
    $S0 = $P12["load_ext"]

    ## load a relative pathname with an extension.
    $S0 = concat "runtime/parrot/dynext/foo_group", $S0
    loadlib $P1, $S0

    ## ensure that we can still make Foo instances.
    $P1 = new "Foo"
    $I1 = $P1
    print $I1
    print "\n"
.end
CODE
42
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', "loadlib with absolute pathname & ext" );
.sub main :main
    ## get cwd in $S0, load_ext in $S1.
    .include "iglobals.pasm"
    $P11 = getinterp
    $P12 = $P11[.IGLOBALS_CONFIG_HASH]
    $S0 = $P12["prefix"]
    $S1 = $P12["load_ext"]

    ## convert $S0 to an absolute pathname with extension, and load it.
    ## this should always find the version in the build directory, since that's
    ## the only place "make test" will work.
    $S0 = concat $S0, "/runtime/parrot/dynext/foo_group"
    $S0 = concat $S0, $S1
    loadlib $P1, $S0

    ## ensure that we can still make Foo instances.
    $P1 = new "Foo"
    $I1 = $P1
    print $I1
    print "\n"
.end
CODE
42
OUTPUT

SKIP: {
    skip( "No BigInt Lib configured", 1 ) if !$PConfig{gmp};

    pir_output_is( << 'CODE', << 'OUTPUT', "inherited add" );
.sub _main :main
    .local pmc d, l, r
    $P0 = loadlib "foo_group"
    print "ok\n"
    l = new "Foo"
    l = 42
    r = new 'BigInt'
    r = 0x7ffffff
    d = new 'Undef'
    add d, l, r
    print d
    print "\n"
    $S0 = typeof d
    print $S0
    print "\n"
.end
CODE
ok
134217769
BigInt
OUTPUT

}

pir_output_is( <<'CODE', <<'OUTPUT', "Foo subclass isa Integer" );
.sub main :main
    .local pmc F, f, d, r
    loadlib F, "foo_group"
    f = new "Foo"
    f = 1
    d = new 'Integer'
    r = new 'Integer'
    r = 2
    d = f - r
    print d
    print "\n"
.end
CODE
144
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', ".HLL 1" );
# load our Foo test (pseudo) language
# it defines one PMC type "Foo"
.HLL "Fool"
.loadlib "foo_group"
.sub main :main
    new $P1, "Foo"      # load by name
    $I1 = $P1
    print $I1
    print "\n"
.end
CODE
42
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', ".HLL 2" );
.HLL "Fool"
.loadlib "foo_group"
.sub main :main
    new $P1, 'Foo'       # load by index
    $I1 = $P1
    print $I1
    print "\n"
.end
CODE
42
OUTPUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
