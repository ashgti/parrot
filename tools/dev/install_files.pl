#!perl

=head1 TITLE

tools/dev/install_files.pl - Copy files to their correct locations

=head1 DESCRIPTION

Use a detailed MANIFEST to install a set of files. The format of the
MANIFEST.detailed is:

    [package]meta1,meta2,... <whitespace> source_path

or you may optionally specify a different destination path:

    [package]meta1,meta2,... <whitespace> source_path <whitespace> destination

Additionally, there may be a * in front of the whole line to designate
a generated file:

    *[package]meta1,meta2,... <whitespace> source_path <whitespace> destination

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

Write this file to the location given by the --includedir option

=item C<lib>

Write this file to the location given by the --libdir option

=item C<bin>

Write this file to the location given by the --bindir option

=back

The optional C<destination> field provides a general way to change
where a file will be written to. It will be applied before any
metadata tags.

Example: if this line is in the MANIFEST.detailed file

  *[main]bin	languages/imcc/imcc     imcc

and the --bindir=/usr/parroty/bin, then the generated
parrot-<VERSION>-1.<arch>.rpm file will contain the file
/usr/parroty/bin/imcc.

=head1 SEE ALSO

mk_manifests.pl

=cut

use File::Basename qw(dirname);
use strict;

my %options = ( buildprefix => '',
                prefix => '/usr',
                exec_prefix => '/usr',
                bindir => '/usr/bin',
                libdir => '/usr/lib',
                includedir => '/usr/include',
              );

my $manifest;
foreach (@ARGV) {
    if (/^--([^=]+)=(.*)/) {
        $options{$1} = $2;
    } else {
        $manifest = $_;
    }
}

my @files;
my %directories;
@ARGV = ($manifest);
while(<>) {
    chomp;
    my ($meta, $src, $dest) = split(/\s+/, $_);
    $dest ||= $src;

    # Parse out metadata
    my $generated = $meta =~ s/^\*//;
    my ($package) = $meta =~ /^\[(.*?)\]/;
    $meta =~ s/^\[(.*?)\]//;
    next if $package eq ""; # Skip if this file belongs to no package
    my %meta;
    @meta{split(/,/, $meta)} = ();
    $meta{$_} = 1 for (keys %meta); # Laziness

    if ($meta{lib}) {
        $dest = "$options{libdir}/$dest";
    } elsif ($meta{bin}) {
        $dest = "$options{bindir}/$dest";
    } elsif ($meta{include}) {
        $dest = "$options{includedir}/$dest";
    } else {
        $dest = "$options{prefix}/$dest";
    }

    $dest = "$options{buildprefix}/$dest" if $options{buildprefix};

    $directories{dirname($dest)} = 1;
    push(@files, [ $src => $dest ]);
}

foreach (sort keys %directories) {
    print "install -d $_\n";
}

foreach (@files) {
    my ($src, $dest) = @$_;
    print "install -c $src $dest\n";
}
