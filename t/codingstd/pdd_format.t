#! perl
# Copyright (C) 2001-2005, The Perl Foundation.
# $Id$

use strict;
use warnings;

use Test::More tests =>  1;
use Carp;
use Cwd;
use Tie::File;

my $cwd = cwd();
my @pdddirs = (
    qq{$cwd/docs/pdds},
    qq{$cwd/docs/pdds/draft},
);

my @pddfiles = ();
foreach my $dir (@pdddirs) {
    my @pdds;
    opendir my $DIRH, $dir
        or croak "Unable to open directory handle: $!";
    @pdds = map { qq|$dir/$_| } grep { m/^pdd\d{2,}_.*\.pod$/ }
        readdir $DIRH;
    closedir $DIRH or croak "Unable to close directory handle: $!";
    @pddfiles = (@pddfiles, @pdds);
}

my @diagnostics = ();
foreach my $pdd (@pddfiles) {
    my $diag = check_pdd_formatting($pdd);
    if ( $diag ) {
        push @diagnostics, $diag;
    }
}

my $errmsg = q{};
if ( @diagnostics ) {
    $errmsg = join ("\n" => @diagnostics) . "\n";
}

$errmsg ? fail( qq{\n$errmsg} )
        : pass( q{All PDDs are formatted correctly} );

sub check_pdd_formatting {
    my $pdd = shift;
    my $base = $pdd;
    if ($pdd =~ m{((draft/)?[^/]+)$}) {
        $base = $1;
    }
    my $diag = q{};
    my @toolong = ();
    my @sections_needed = qw(
        NAME
        VERSION
        ABSTRACT
        DESCRIPTION
        IMPLEMENTATION
        REFERENCES
    );
    my %sections_seen = map { $_, 0 } @sections_needed;
    my @lines;
    tie @lines, 'Tie::File', $pdd
        or croak "Unable to tie to $pdd: $!";
    for (my $i=0; $i<=$#lines; $i++) {
        if (
            ( length( $lines[$i] ) > 78 )
            and
            ( $lines[$i] !~ m/^(?:L?<)?http/ ) 
        ) {
            push @toolong, ($i + 1);
        }
        foreach my $need ( @sections_needed ) {
            $sections_seen{$need}++ if $lines[$i] =~ m{^=head1\s+$need};
        }
    }
    untie @lines or croak "Unable to untie from $pdd: $!";
    if ( @toolong ) {
        $diag .=
            qq{$base has } .
            scalar(@toolong) .
            qq{ lines > 78 chars:  @toolong\n};
    }
    foreach my $need ( keys %sections_seen ) {
        if ( ! $sections_seen{$need} ) {
            $diag .= qq{$base lacks 'head1' $need section\n};
        }
    }
    return $diag;
}

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
