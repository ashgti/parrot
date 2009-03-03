#! perl
################################################################################
# Copyright (C) 2001-2008, Parrot Foundation.
# $Id$
################################################################################

=head1 TITLE

tools/dev/install_files.pl - Copy files to their correct locations

=head1 SYNOPSIS

    % perl tools/dev/install_files.pl [options]

=head1 DESCRIPTION

Use a detailed MANIFEST to install a set of files.

=head2 Options

=over 4

=item C<buildprefix>

The build prefix. Defaults to ''.

=item C<prefix>

The install prefix. Defaults to '/usr'.

=item C<exec_prefix>

The exec prefix. Defaults to '/usr'.

=item C<bindir>

The executables directory. Defaults to '/usr/bin'.

=item C<libdir>

The library directory. Defaults to '/usr/lib'.

=item C<includedir>

The header directory. Defaults to '/usr/include'.

=back

=head2 MANIFEST Format

The format of the MANIFEST (currently MANIFEST and MANIFEST.generated
are used) is:

    source_path <whitespace> [package]meta1,meta2,...

or you may optionally specify a different destination path:

    source_path <whitespace> [package]meta1,meta2,... <whitespace> destination

Additionally, there may be a * in front of the whole line to designate
a generated file:

    source_path <whitespace> *[package]meta1,meta2,... <whitespace> destination

The square brackets around C<package> are literal. C<package> gives
the name of the RPM that the given file will be installed for, and is
only used by this script to skip files that are not members of any
package.

The various meta flags recognized are:

=over 4

=item C<doc>

Tag this file with %doc in the RPM, and omit the leading path (because
rpm will put it into a directory of its choosing)

=item C<include>

Write this file to the location given by the C<--includedir> option

=item C<lib>

Write this file to the location given by the C<--libdir> option

=item C<bin>

Write this file to the location given by the C<--bindir> option

=back

The optional C<destination> field provides a general way to change
where a file will be written to. It will be applied before any
metadata tags.

Example: if this line is in the MANIFEST.generated file

  languages/snorkfest/snork-compile        [main]bin

and the --bindir=/usr/parroty/bin, then the generated
parrot-<VERSION>-1.<arch>.rpm file will contain the file
/usr/parroty/bin/snork-compile.

=head1 SEE ALSO

F<tools/dev/mk_manifests.pl>

=cut

################################################################################

use strict;
use warnings;
use File::Basename qw(dirname basename);
use File::Copy;
use File::Spec;

# When run from the makefile, which is probably the only time this
# script will ever be used, all of these defaults will get overridden.
my %options = (
    buildprefix => '',
    prefix      => '/usr',
    destdir     => '',
    exec_prefix => '/usr',
    bindir      => '/usr/bin',
    libdir      => '/usr/lib',       # parrot/ subdir added below
    includedir  => '/usr/include',   # parrot/ subdir added below
    docdir      => '/usr/share/doc', # parrot/ subdir added below
    version     => '',
    'dry-run'   => 0,
);

my @manifests;
foreach (@ARGV) {
    if (/^--([^=]+)=(.*)/) {
        $options{$1} = $2;
    }
    else {
        push @manifests, $_;
    }
}

my $parrotdir = $options{versiondir};

# We'll report multiple occurrences of the same file
my %seen;

my @files;
my @installable_exe;
my %directories;
@ARGV = @manifests;
while (<>) {
    chomp;

    s/\#.*//;    # Ignore comments
    next if /^\s*$/;    # Skip blank lines

    my ( $src, $meta, $dest ) = split( /\s+/, $_ );
    $dest ||= $src;

    if ( $seen{$src}++ ) {
        print STDERR "$ARGV:$.: Duplicate entry $src\n";
    }

    # Parse out metadata
    die "Malformed line in MANIFEST: $_" if not defined $meta;
    my $generated = $meta =~ s/^\*//;
    my ($package) = $meta =~ /^\[(.*?)\]/;
    $meta =~ s/^\[(.*?)\]//;
    next unless $package;    # Skip if this file belongs to no package

    next unless $package =~ /main|library|pge/;

    my %meta;
    @meta{ split( /,/, $meta ) } = ();
    $meta{$_} = 1 for ( keys %meta );          # Laziness

    if ( /^runtime/ ) {
         # have to catch this case early.
        $dest =~ s/^runtime\/parrot\///;
        $dest = File::Spec->catdir( $options{libdir}, $parrotdir, $dest );
    }
    elsif ( $meta{lib} ) {
        if ( $dest =~ /^install_/ ) {
            $dest =~ s/^install_//;            # parrot with different config
            $dest = File::Spec->catdir( $options{libdir}, $parrotdir, 'include', $dest );
        }
        else {
            # don't allow libraries to be installed into subdirs of libdir
            $dest = File::Spec->catdir( $options{libdir}, basename($dest) );
        }
    }
    elsif ( $meta{bin} ) {
        my $copy = $dest;
        $dest =~ s/^installable_//;            # parrot with different config
        $dest = File::Spec->catdir( $options{bindir}, $dest );
        if ( $copy =~ /^installable/ ) {
            push @installable_exe, [ $src, $dest ];
            next;
        }
    }
    elsif ( $meta{include} ) {
        $dest =~ s/^include//;
        $dest = File::Spec->catdir( $options{includedir}, $parrotdir, $dest );
    }
    elsif ( $meta{doc} ) {
        $dest =~ s/^docs\/resources/resources/; # resources go in the top level of docs
        $dest =~ s/^docs/pod/; # other docs are actually raw Pod
        $dest = File::Spec->catdir( $options{docdir}, $parrotdir, $dest );
    }
    elsif ( $meta{pkgconfig} ) {

        # For the time being this is hardcoded as being installed under libdir
        # as it is typically done with automake installed packages.  If there
        # is a use case to make this configurable we'll add a seperate
        # --pkgconfigdir option.
        $dest = File::Spec->catdir( $options{libdir}, 'pkgconfig', $parrotdir, $dest );
    }
    elsif ( /^compilers/ ) {
        $dest =~ s/^compilers/languages/;
        $dest = File::Spec->catdir( $options{libdir}, $parrotdir, $dest );
    }
    else {
        die "Unknown install location in MANIFEST: $_";
    }

    $dest = File::Spec->catdir( $options{buildprefix}, $dest )
        if $options{buildprefix};

    $directories{ dirname($dest) } = 1;
    push( @files, [ $src => $dest ] );
}
continue {
    close ARGV if eof;    # Reset line numbering for each input file
}

unless ( $options{'dry-run'} ) {
    for my $dir ( map { $options{destdir} . $_ } keys %directories ) {
        unless ( -d $dir ) {

            # Make full path to the directory $dir
            my @dirs;
            while ( !-d $dir ) {    # Scan up to nearest existing ancestor
                unshift @dirs, $dir;
                $dir = dirname($dir);
            }
            foreach (@dirs) {
                mkdir( $_, 0777 ) or die "mkdir $_: $!\n";
            }
        }
    }
}
print("Installing ...\n");
foreach ( @files, @installable_exe ) {
    my ( $src, $dest ) = @$_;
    $dest = $options{destdir} . $dest;
    if ( $options{'dry-run'} ) {
        print "$src -> $dest\n";
        next;
    }
    else {
        next unless -e $src;
        next if $^O eq 'cygwin' and -e "$src.exe"; # stat works, copy not
        copy( $src, $dest ) or die "copy $src to $dest: $!\n";
        print "$dest\n";
    }
    my $mode = ( stat($src) )[2];
    chmod $mode, $dest;
}

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
