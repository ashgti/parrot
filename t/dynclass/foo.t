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

use Parrot::Test tests => 2;

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

pir_output_is(<< 'CODE', << 'OUTPUT', "inherited add");
.sub _main @MAIN
    .local pmc d, l, r
    $P0 = loadlib "foo"
    print "ok\n"
    l = new "Foo"
    l = 3
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
134217730
BigInt
OUTPUT
