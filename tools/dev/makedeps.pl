#!/usr/bin/perl

use strict;
use warnings;

# This dependency builder is about as brute-force as it gets, but it's
# better than nothing.

my @objects = @ARGV;

for my $object ( @objects ) {
    my $source = $object;

    $source =~ s{[.]o$}{.c} or die "Got an object that doesn't end in .o";

    open( my $fh, '<', $source ) or die "Can't read $source: $!";

    while ( my $line = <$fh> ) {
        if ( $line =~ /^\s*#include\s+"([^"]+)"/ ) {
            my $include_file = $1;
            print "$source: $include_file\n";
        }
    }

    close $fh or die $!;


}
