#! perl
# Copyright: 2001-2005 The Perl Foundation.  All Rights Reserved.
# $Id$

use strict;
use warnings;
use lib qw( t . lib ../lib ../../lib );
use Test::More;
use Parrot::Test;
use Parrot::Test::PGE;


=head1 NAME

t/library/pge_examples.t - Parrot Grammar Engine tests of examples

=head1 SYNOPSIS

	% prove -Ilib t/library/pge_examples.t

=cut


# 1 
pir_output_is(<<'CODE', <<'OUT', "This made Parrot m4 fail");

.sub 'test' :main
  load_bytecode "PGE.pbc"

  .local pmc p6rule
  p6rule = compreg "PGE::P6Rule"

  .local pmc rulesub_a, rulesub_b
  rulesub_a  = p6rule( "a" )
  rulesub_b  = p6rule( "^(<[b]>)" )

  .local string input_string    
  input_string    = "_____________________________________________________________________"

  rulesub_b( input_string ) 

  print "ok1\n"
  # end

  rulesub_a( input_string ) 
  print "ok2\n"

.end

CODE
ok1
ok2
OUT


# 2 
pir_output_is(<<'CODE', <<'OUT', "parse FASTA");

# Grok fasta files, which usually contain DNA, RNA or protein sequences.
# http://en.wikipedia.org/wiki/FASTA_format

# TODO: Compose rules out of subrules

.include "library/dumper.imc"

.sub "example" :main
    load_bytecode 'PGE.pbc'
    load_bytecode 'PGE/Util.pir'

    .local string fasta_grammar
    fasta_grammar = <<'END_FASTA_GRAMMAR'
grammar Bio::Fasta;

rule file        { <entry>+ }
rule start_entry { \> }
rule desc_line   { <start_entry> <id> } 
rule entry       { <desc_line> \s+ <desc> } 
rule id          { (\S+) }
rule desc        { (\N*) }
rule sequence    { (<-[>]>*) }

END_FASTA_GRAMMAR

    .local string fasta
    fasta = <<'END_FASTA'
>gi|5524211|gb|AAD44166.1| cytochrome b [Elephas maximus maximus]
LCLYTHIGRNIYYGSYLYSETWNTGIMLLLITMATAFMGYVLPWGQMSFWGATVITNLFSAIPYIGTNLV
EWIWGGFSVDKATLNRFFAFHFILPFTMVALAGVHLTFLHETGSNNPLGLTSDSDKIPFHPYYTIKDFLG
LLILILLLLLLALLSPDMLGDPDNHMPADPLNTPLHIKPEWYFLFAYAILRSVPNKLGGVLALFLSIVIL
GLMPFLHTSKHRSMMLRPLSQALFWTLTMDLLTLTWIGSQPVEYPYTIIGQMASILYFSIILAFLPIAGX
IENY
END_FASTA

    .local pmc p6grammar
    p6grammar = compreg "PGE::P6Grammar"
    .local pmc code
    ( code ) = p6grammar(fasta_grammar)
    # print code

    .local pmc fasta_rule
    fasta_rule = find_global "Bio::Fasta", "id"
    .local pmc match
    ( match ) = fasta_rule( fasta )
    
    # TODO: Extract named or positional captures
    print match
    print "\n"

.end

CODE
>gi|5524211|gb|AAD44166.1|
OUT


# remember to change the number of tests :-)
BEGIN { plan tests => 2; }
