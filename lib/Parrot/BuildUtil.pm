# Copyright (C) 2001-2009, Parrot Foundation.
# $Id$

package Parrot::BuildUtil;

use strict;
use warnings;

=head1 NAME

Parrot::BuildUtil - Utilities for building Parrot

=head1 DESCRIPTION

This package holds three subroutines:  C<parrot_version()>, C<slurp_file>,
and C<generated_file_header>. Subroutines are not exported--each must be
requested by using a fully qualified name.

=head1 SUBROUTINES

=over 4

=item C<parrot_version()>

Determines the current version number for Parrot from the VERSION file
and returns it in a context-appropriate manner.

    $parrot_version = Parrot::BuildUtil::parrot_version();
    # $parrot_version is '0.4.11'

    @parrot_version = Parrot::BuildUtil::parrot_version();
    # @parrot_version is (0, 4, 11)

=cut

# cache for repeated calls
my ( $parrot_version, @parrot_version );

sub parrot_version {
    if ( defined $parrot_version ) {
        return wantarray ? @parrot_version : $parrot_version;
    }

    # Obtain the official version number from the VERSION file.
    if (-e 'VERSION') {
        open my $VERSION, '<', 'VERSION' or die 'Could not open VERSION file!';
        chomp( $parrot_version = <$VERSION> );
        close $VERSION or die $!;
    }
    else { # we're in an installed copy of Parrot
        my $path = shift;
        $path = '' unless $path;
        open my $VERSION, '<', "$path/VERSION" or die 'Could not open VERSION file!';
        chomp( $parrot_version = <$VERSION> );
        close $VERSION or die $!;
    }

    $parrot_version =~ s/\s+//g;
    @parrot_version = split( /\./, $parrot_version );

    if ( scalar(@parrot_version) < 3 ) {
        die "Too few components to VERSION file contents: '$parrot_version' (should be 3 or 4)!";
    }

    if ( scalar(@parrot_version) > 4 ) {
        die "Too many components to VERSION file contents: '$parrot_version' (should be 3 or 4)!";
    }

    foreach my $component (@parrot_version) {
        die "Illegal version component: '$component' in VERSION file!"
            unless $component =~ m/^\d+$/;
    }

    $parrot_version = join( '.', @parrot_version );
    return wantarray ? @parrot_version : $parrot_version;
}

=item C<slurp_file($filename)>

Slurps up the filename and returns the content as one string.  While
doing so, it converts all DOS-style line endings to newlines.

=cut

sub slurp_file {
    my ($file_name) = @_;

    open( my $SLURP, '<', $file_name ) or die "open '$file_name': $!";
    local $/ = undef;
    my $file = <$SLURP> . '';
    $file =~ s/\cM\cJ/\n/g;
    close $SLURP or die $!;

    return $file;
}

=item C<generated_file_header($filename, $style)>

Returns a comment to mark a generated file and detail how it was created.
C<$filename> is the name of the file on which the generated file is based,
C<$style> is the style of comment--C<'perl'> and C<'c'> are permitted, other
values produce an error.

=cut

sub generated_file_header {
    my ( $filename, $style ) = @_;

    die qq{unknown style "$style"}
        if $style !~ m/\A(?:perl|c)\z/;

    require File::Spec;
    my $script = File::Spec->abs2rel($0);
    $script =~ s/\\/\//g;

    my $header = <<"END_HEADER";
/* ex: set ro ft=c:
 * !!!!!!!   DO NOT EDIT THIS FILE   !!!!!!!
 *
 * This file is generated automatically from '$filename'
 * by $script.
 *
 * Any changes made here will be lost!
 *
 */
END_HEADER

    if ( $style eq 'perl' ) {
        $header =~ s/^\/\*(.*?)ft=c:/# $1ft=perl:/;
        $header =~ s/\n \*\n \*\///;
        $header =~ s/^ \* ?/#  /msg;
    }

    return $header;
}

1;

=back

=head1 AUTHOR

Gregor N. Purdy.  Revised by James E Keenan.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
