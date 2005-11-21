#! perl -w
# Copyright: 2005 The Perl Foundation.  All Rights Reserved.
# $Id:$

=head1 NAME

t/dynclass/dynlexpad.t - test the DynLexPad PMC 

=head1 SYNOPSIS

	% perl -Ilib t/dynclass/dynlexpad.t

=head1 DESCRIPTION

Tests the C<DynLexPad> PMC.

=cut

use Parrot::Test;
use Test::More;
use Parrot::Config;
plan tests => 2;

my $loadlib = <<EOC;
.HLL "Some", ""
.sub 'test' :main
    .local pmc lib
    lib = loadlib "dynlexpad" 
EOC

pir_output_is($loadlib . << 'CODE', << 'OUTPUT', "loadlib");
    # see loadlib
    unless lib goto not_loaded
    print "ok\n"
    end
not_loaded:
    print "not loaded\n"
.end
CODE
ok
OUTPUT

pir_output_is($loadlib . << 'CODE', << 'OUTPUT', "store_lex");
    # see loadlib
    foo()
.end
.sub foo :lex
    $P1 = new .Integer
    $P1 = 13013
    # XXX hack ahead - use different opcode to create lex (as with pads)
    #     the pad depth is ignored
    store_lex 0, 'a', $P1
    print "ok 1\n"
    $P2 = find_lex 'a'
    print "ok 2\n"
    print $P2
    print "\n"
    end
.end
CODE
ok 1
ok 2
13013
OUTPUT
