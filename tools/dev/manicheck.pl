#! perl
################################################################################
# Copyright (C) 2001-2005, Parrot Foundation.
# $Id$
################################################################################

=head1 NAME

tools/dev/manicheck.pl - Check the MANIFEST file

=head1 SYNOPSIS

    % perl tools/dev/manicheck.pl

=head1 DESCRIPTION

Check the contents of the F<MANIFEST> file against the files present in
this directory tree, accounting for .svn dirs. Prints out the
number of I<missing>, I<expected> and I<extra> files, and
then any extra files are listed.

Files that match the patterns in MANIFEST.SKIP are not reported as extra
files.

=cut

################################################################################

use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../../lib";

use ExtUtils::Manifest;

$ExtUtils::Manifest::Quiet = 1;
my $manifest  = ExtUtils::Manifest::maniread();
my $file_list = ExtUtils::Manifest::manifind();
my @missing   = ExtUtils::Manifest::manicheck();
my @extra     = ExtUtils::Manifest::filecheck();

# my @ignored   = ExtUtils::Manifest::skipcheck();

# strip '~' backup files from the extra list
@extra = grep !m/~$/, @extra;

printf "Found %d distinct files among MANIFEST and directory contents.\n\n",
    scalar( keys %{$file_list} );

printf "  %5d missing\n", scalar @missing;
printf "  %5d extra\n",   scalar @extra;

if (@missing) {
    print "\n";
    print "Missing files:\n";
    foreach (@missing) {
        print "  $_\n";
    }
}

if (@extra) {
    print "\n";
    print "Extra files:\n";
    foreach (@extra) {
        print "  $_\n";
    }
}

exit scalar(@missing) or scalar(@extra) ? 1 : 0;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
