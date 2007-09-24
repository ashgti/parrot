#! perl
# Copyright (C) 2001-2006, The Perl Foundation.
# $Id$

use strict;
use warnings;

use lib qw( . lib ../lib ../../lib );
use Test::More tests => 1;
use Parrot::Distribution;

=head1 NAME

t/codingstd/cppcomments.t - checks for C++ style comments

=head1 SYNOPSIS

    # test all files
    % prove t/codingstd/cppcomments.t

    # test specific files
    % perl t/codingstd/cppcoments.t src/foo.t include/parrot/bar.h

=head1 DESCRIPTION

Checks that no source file in the distribution uses C++ style comments.

=head1 SEE ALSO

L<docs/pdds/pdd07_codingstd.pod>

=cut

my $DIST = Parrot::Distribution->new();
my @files = @ARGV ? @ARGV : $DIST->get_c_language_files();

check_cppcomments(@files);

sub check_cppcomments {
    my @files = @_;

    my @comments;
    foreach my $file (@files) {
        my $path = @ARGV ? $file : $file->path();
        my $buf = $DIST->slurp( $path );
        $buf =~ s{ (?:
                       (?: ' (?: \\\\ | \\' | [^'] )* ' )  # remove ' string
                     | (?: " (?: \\\\ | \\" | [^"] )* " )  # remove " string
                     | /\* .*? \*/                         # remove C comment
                   )
                }{}gsx;

        if ( $buf =~ m{ ( .*? // .* ) }x ) {
            push( @comments, "$path: $1\n" );
        }
    }

    ok( !scalar(@comments), 'C++ comments' )
        or diag( "C++ comments found in " . scalar @comments . " files:\n@comments" );
}

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
