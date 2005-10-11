#! perl -w
# Copyright: 2005 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

t/pmc/foo.t - Test for a very simple dynamic PMC

=head1 SYNOPSIS

	% perl -Ilib t/dynclass/foo.t

=head1 DESCRIPTION

Tests the Foo PMC.

=cut

use Parrot::Test tests => 9;
use Parrot::Config;

pir_output_is(<< 'CODE', << 'OUTPUT', "get_integer");

.sub main :main
    loadlib P1, "foo"
    find_type $I0, "Foo"
    new $P1, $I0

    $I1 = $P1
    print $I1
    print "\n"
.end
CODE
42
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "loadlib with relative pathname, no ext");
.sub main :main
    ## load a relative pathname without the extension.  loadlib will convert the
    ## '/' characters to '\\' on windows.
    $S0 = "runtime/parrot/dynext/foo"
    loadlib P1, $S0

    ## ensure that we can still make Foo instances.
    find_type $I0, "Foo"
    new $P1, $I0
    $I1 = $P1
    print $I1
    print "\n"
.end
CODE
42
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "loadlib with absolute pathname, no ext");
.sub main :main
    ## get cwd in $S0.
    .include "iglobals.pasm"
    $P11 = getinterp
    $P12 = $P11[.IGLOBALS_CONFIG_HASH]
    $S0 = $P12["prefix"]

    ## convert cwd to an absolute pathname without the extension, and load it.
    ## this should always find the version in the build directory, since that's
    ## the only place "make test" will work.
    $S0 = concat "/runtime/parrot/dynext/foo"
    loadlib P1, $S0

    ## ensure that we can still make Foo instances.
    find_type $I0, "Foo"
    new $P1, $I0
    $I1 = $P1
    print $I1
    print "\n"
.end
CODE
42
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "loadlib with relative pathname & ext");
.sub main :main
    ## get load_ext in $S0.
    .include "iglobals.pasm"
    $P11 = getinterp
    $P12 = $P11[.IGLOBALS_CONFIG_HASH]
    $S0 = $P12["load_ext"]

    ## load a relative pathname with an extension.
    $S0 = concat "runtime/parrot/dynext/foo", $S0
    loadlib P1, $S0

    ## ensure that we can still make Foo instances.
    find_type $I0, "Foo"
    new $P1, $I0
    $I1 = $P1
    print $I1
    print "\n"
.end
CODE
42
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', "loadlib with absolute pathname & ext");
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
    $S0 = concat $S0, "/runtime/parrot/dynext/foo"
    $S0 = concat $S0, $S1
    loadlib P1, $S0

    ## ensure that we can still make Foo instances.
    find_type $I0, "Foo"
    new $P1, $I0
    $I1 = $P1
    print $I1
    print "\n"
.end
CODE
42
OUTPUT

SKIP: { skip("No BigInt Lib configured", 1) if !$PConfig{gmp};

pir_output_is(<< 'CODE', << 'OUTPUT', "inherited add");
.sub _main :main
    .local pmc d, l, r
    $P0 = loadlib "foo"
    print "ok\n"
    l = new "Foo"
    l = 42
    r = new BigInt
    r = 0x7ffffff
    d = new Undef
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

pir_output_is(<<'CODE', <<'OUTPUT', "Foo subclass isa Integer");
.sub main :main
    .local pmc F, f, d, r
    loadlib F, "foo"
    f = new "Foo"
    f = 1
    d = new Integer
    r = new Integer
    r = 2
    d = f - r
    print d
    print "\n"
.end
CODE
144
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', ".HLL 1");
# load our Foo test (pseudo) language
# it defines one PMC type "Foo"
.HLL "Fool", "foo"
.sub main :main
    new $P1, "Foo"      # load by name
    $I1 = $P1
    print $I1
    print "\n"
.end
CODE
42
OUTPUT

pir_output_is(<< 'CODE', << 'OUTPUT', ".HLL 2");
.HLL "Fool", "foo"
.sub main :main
    new $P1, .Foo       # load by index
    $I1 = $P1
    print $I1
    print "\n"
.end
CODE
42
OUTPUT
