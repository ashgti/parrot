#!perl
# Copyright (C) 2005-2009, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw(t . lib ../lib ../../lib ../../../lib);
use Test::More;
use Parrot::Test tests => 2;

=head1 NAME

t/parser.t - TGE::Parser tests

=head1 SYNOPSIS

        $ prove t/compilers/tge/parser.t

=cut

pir_output_is( <<'CODE', <<'OUT', "parse a basic attribute grammar" );

.sub _main :main
    load_bytecode 'TGE.pbc'

    .local string source
    source = <<'GRAMMAR'
    transform min (Leaf) {
        $P1 = getattribute node, "value"
        .return ($P1)
    }
    # A test comment
    transform gmin (Branch) :applyto('left') {
        .local pmc gmin
        gmin = tree.get('gmin', node)
        .return (gmin)
    }
GRAMMAR

    # Match against the source
    .local pmc match
    .local pmc start_rule
    start_rule = get_global ['TGE';'Parser'], "start"
    print "loaded start rule\n"
    match = start_rule(source)
    print "matched start rule\n"

    # Verify the match
    unless match goto match_fail           # if match fails stop
    print "parse succeeded\n"
    goto cleanup

  match_fail:
    print "parse failed\n"

  cleanup:
    end
.end

CODE
loaded start rule
matched start rule
parse succeeded
OUT

pir_error_output_like( <<'CODE', qr/Syntax error at line 4, near "transform "/, "parse failure" );

.sub _main :main
    load_bytecode 'TGE.pbc'

    .local string source
    source = <<'GRAMMAR'
    transform min (Leaf) {
      # nothing to see here.
    }
    transform max {  # missing ()'s
    }

GRAMMAR
    .local pmc match
    .local pmc start_rule
    start_rule = get_global ['TGE';'Parser'], "start"
    match = start_rule(source, 'grammar'=>'TGE::Parser') # should throw.
.end

CODE

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
