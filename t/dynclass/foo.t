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

use Parrot::Test tests => 3;
use Parrot::Config;

pir_output_is(<< 'CODE', << 'OUTPUT', "get_integer");

.sub main @MAIN
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

SKIP: { skip("No BigInt Lib configured", 1) if !$PConfig{gmp};

pir_output_is(<< 'CODE', << 'OUTPUT', "inherited add");
.sub _main @MAIN
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
.sub main @MAIN
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
