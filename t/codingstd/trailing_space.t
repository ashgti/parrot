#! perl
# Copyright (C) 2006-2007, The Perl Foundation.
# $Id$

use strict;
use warnings;

use lib qw( . lib ../lib ../../lib );
use Parrot::Distribution;
use Test::More tests => 1;

=head1 NAME

t/codingstd/trailing_space.t - checks for superfluous trailing space or tab characters

=head1 SYNOPSIS

    # test all files
    % prove t/codingstd/trailing_space.t

    # test specific files
    % perl t/codingstd/trailing_space.t src/foo.c include/parrot/bar.h

=head1 DESCRIPTION

Checks that files don't have trailing space or tab characters between the
last nominal character on the line and the end of line character.

=head1 SEE ALSO

L<docs/pdds/pdd07_codingstd.pod>

=cut

my $DIST = Parrot::Distribution->new;

my $skip_files = $DIST->generated_files();
my @files = @ARGV ? @ARGV : (
    $DIST->get_c_language_files(),
    $DIST->get_perl_language_files(),
);
my @failed_files;

foreach my $file (@files) {

    # if we have command line arguments, the file is the full path
    # otherwise, use the relevant Parrot:: path method
    my $path = @ARGV ? $file : $file->path;

    next if exists $skip_files->{$path};

    open my $fh, '<', $path
        or die "Cannot open '$path' for reading: $!\n";

    my $spacecount = 0;

    my $message = qq<  $path:>;
    while (<$fh>) {
        next unless m{.?[ \t]+$}m;
        $message .= " $.";
        $spacecount++;
    }
    push @failed_files => "$message\n"
        if $spacecount;
    
}

# check the file
ok( !scalar(@failed_files), 'No trailing spaces or tabs' )
    or diag(
    join
        $/ => "Trailing space or tab char found in " . scalar @failed_files . " files:",
    @failed_files
    );

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
