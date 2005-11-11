# Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

config/auto/byteorder.pl - Native Byteorder

=head1 DESCRIPTION

Computes the native byteorder for Parrot's wordsize.

=cut

package Configure::Step;

use strict;
use vars qw($description @args);

$description="Computing native byteorder for Parrot's wordsize...";

@args=();

sub runstep {
  cc_gen('config/auto/byteorder/test_c.in');
  cc_build();
  my $byteorder=cc_run() or die "Can't run the byteorder testing program: $!";
  cc_clean();

  chomp $byteorder;

  if($byteorder =~ /^1234/) {
    Configure::Data->set(
      byteorder => $byteorder,
      bigendian => 0
    );
    $Configure::Step::result = 'little-endian';
  }
  elsif($byteorder =~ /^(8765|4321)/) {
    Configure::Data->set(
      byteorder => $byteorder,
      bigendian => 1
    );
    $Configure::Step::result = 'big-endian';
  }
  else {
    die "Unsupported byte-order [$byteorder]!";
  }
}

1;
