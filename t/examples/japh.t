#! perl -w
# Copyright: 2005 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

t/examples/japh.t - Test some JAPHs

=head1 SYNOPSIS

	% perl -Ilib t/examples/japh.t

        % perl t/harness t/examples/japh.t

=head1 DESCRIPTION

Test the JAPHs in 'examples/japh'.
For now there are only JAPHs in PASM.

=cut

use strict;
use Parrot::Test tests => 15;
use Test::More;

{
  # these JAPHs should work
  foreach my $japh_num ( 3, 8, 10, 12, 13 ) {
    test_japh($japh_num);
  } 
} 

TODO:
{
  # these JAPHs are known to fail
  local $TODO = 'various reasons';
  foreach my $japh_num ( 1, 2, 4, 5, 6, 7, 9, 11, 14, 15 ) {
    test_japh($japh_num);
  } 
};

sub test_japh
{
  my ( $japh_num ) = @_;

  my $pasm_fn   = "examples/japh/japh${japh_num}.pasm";
  my $pasm_code = Parrot::Test::slurp_file($pasm_fn); 

  pasm_output_is($pasm_code, "Just another Parrot Hacker\n", $pasm_fn); 
}
