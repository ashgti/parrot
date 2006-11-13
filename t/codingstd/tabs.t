#! perl
# Copyright (C) 2006, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More tests => 1;
use Parrot::Distribution;

=head1 NAME

t/codingstd/tabs.t - checks for tab indents in C source and headers

=head1 SYNOPSIS

    # test all files
    % prove t/codingstd/tabs.t

    # test specific files
    % perl t/codingstd/tabs.t src/foo.c include/parrot/bar.h

=head1 DESCRIPTION

Checks that files do not use tabs to indent.

=head1 SEE ALSO

L<docs/pdds/pdd07_codingstd.pod>

=cut

my $DIST = Parrot::Distribution->new;
my @files = @ARGV ? @ARGV : $DIST->get_c_language_files();
my @tabs;

foreach my $file (@files) {
    my $path;

    ## get the full path of the file
    # if we have the files on the command line, the file is the full path
    if (@ARGV) {
        $path = $file;
    }

    # otherwise, use the Parrot::Doc::File::path method
    else {
        $path = $file->path;
    }

    open my $fh, '<', $path
        or die "Cannot open '$path' for reading: $!\n";

    # search each line for leading tabs
    while (<$fh>) {
        if ($_ =~ m/^ *\t/) {
            push @tabs => "$path\n";
            last;
        }
    }
    close $fh;
}

ok( !scalar(@tabs), "tabs in leading whitespace" )
    or diag("Found tab in leading whitespace in " . scalar(@tabs)
        . " files.  Files affected:\n@tabs");

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
