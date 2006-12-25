# Copyright (C) 2001-2006, The Perl Foundation.
# $Id$

=head1 NAME

lib/Parrot/BuildUtil.pm - Utilities for building Parrot

=head1 DESCRIPTION

For now only a sub for getting the current version.

=head2 Functions

=over 4

=cut

package Parrot::BuildUtil;

use strict;
use warnings;

=item C<parrot_version()>

Determine the current version number for Parrot from the VERSION file
and returns it.

=cut

# cache for repeated calls
# XXX this could be in BEGIN block
my ( $parrot_version, @parrot_version );

sub parrot_version {
    if ( defined $parrot_version ) {
        return wantarray ? @parrot_version : $parrot_version;
    }

    # Obtain the official version number from the VERSION file.
    open my $VERSION, '<', 'VERSION' or die "Could not open VERSION file!";
    $parrot_version = <$VERSION>;
    close $VERSION;

    chomp $parrot_version;
    $parrot_version =~ s/\s+//g;
    @parrot_version = split( /\./, $parrot_version );

    if ( scalar(@parrot_version) < 3 ) {
        die "Too few components to VERSION file contents: '$parrot_version' (should be 3 or 4)!";
    }

    if ( scalar(@parrot_version) > 4 ) {
        die "Too many components to VERSION file contents: '$parrot_version' (should be 3 or 4)!";
    }

    foreach (@parrot_version) {
        die "Illegal version component: '$_' in VERSION file!" unless m/^[1-9]*\w*$/;
    }

    if ( @parrot_version == 4 ) {

        #    $parrot_version[2] = $parrot_version[2] . "_" . $parrot_version[3];
        $#parrot_version = 3;
    }
    $parrot_version = join( '.', @parrot_version );

    return wantarray ? @parrot_version : $parrot_version;
}

=back

=head1 AUTHOR

Gregor N. Purdy

=cut

1;


# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
