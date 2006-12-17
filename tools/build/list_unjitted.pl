#! perl -l

use warnings;
use strict;

unless ( 1 == scalar @ARGV ) {
    print "Must give a directory as an argument.\nPossible options are:";
    opendir my $DIR, 'jit' or die $!;
    while ( my $dir = readdir $DIR ) {
        next if $dir =~ /^\./ or $dir eq '\.svn';
        print $dir;
    }
    exit 1;
}

my @ops;
my @jitted;

open my $OPS, '<', 'ops/ops.num' or die "Damn can not open 'ops/ops.num': $!";

while (<$OPS>) {
    next if /^\s*#/ or /^\s*$/;
    if ( my ($op) = /(\S+)\s+\d+/ ) {
        push @ops, $op;
    }
    else { die "Misformated line: $."; }
}

close $OPS or die "Noo.. $!";

open my $JIT, '<', "jit/$ARGV[0]/core.jit"
    or die "Could not open jit/$ARGV[0]/core.jit: $!";

while (<$JIT>) {
    if ( my ($jit) = /^Parrot_(\S+)\s*\{\s*$/ ) {
        push @jitted, $jit;
    }
}
close $JIT or die "Noo.. $!";

my @not_jitted;

OP: foreach my $op (@ops) {
    foreach my $jit (@jitted) {
        next OP if $jit eq $op;
    }
    push @not_jitted, $op;
}

print foreach @not_jitted;

print 'Not jitted: ', scalar @not_jitted;
print 'Jitted: ',     scalar @jitted;
print 'Total ops: ',  scalar @ops;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
