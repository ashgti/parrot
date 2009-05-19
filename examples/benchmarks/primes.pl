#! perl
# Copyright (C) 2001-2003, Parrot Foundation.
# $Id$

=head1 NAME

examples/benchmarks/primes.pl - Calculate prime numbers < 10000

=head1 SYNOPSIS

    % time perl examples/benchmarks/primes.pl

=head1 DESCRIPTION

Calculates all the prime numbers up to 50000 and prints out the number
of primes and the last one found.

=cut

use strict;
use warnings;

my $i1 = 1;
my $i2 = 1000;
my $i3;
my $i4;
my $i5;
my $i6 = 0;
my $i7;
printf("N primes up to ");
printf( "%d", $i2 );
printf(" is: ");

REDO:
$i3 = 2;
$i4 = $i1 / 2;
LOOP:
$i5 = $i1 % $i3;
if ($i5) { goto OK; }
goto NEXT;
OK:
$i3++;
if ( $i3 <= $i4 ) { goto LOOP; }
$i6++;
$i7 = $i1;
NEXT:
$i1++;
if ( $i1 <= $i2 ) { goto REDO; }
print $i6, "\nlast is: $i7\n";

=head1 SEE ALSO

F<examples/benchmarks/primes.c>,
F<examples/benchmarks/primes.pasm>,
F<examples/benchmarks/primes2_p.pasm>,
F<examples/benchmarks/primes2.c>,
F<examples/benchmarks/primes2.pir>,
F<examples/benchmarks/primes2.py>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
