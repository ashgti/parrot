# $Id: Manifest.pm 19888 2007-07-15 13:03:16Z jkeenan $
package Parrot::Manifest;
use strict;
use warnings;
use Carp;

sub new {
    my $class = shift;
    my $script = shift;

    my $self = bless( {}, $class );

    my %data = (
        id          => '$' . 'Id$',
        time        => scalar gmtime,
        cmd         => -d '.svn' ? 'svn' : 'svk',
        script      => $script,
    );

    my @status_output = qx($data{cmd} status -v);
    
    # grab the versioned resources:
    my @versioned_files = ();
    my @dirs = ();
    my @versioned_output = grep !/^[?D]/, @status_output;
    for my $line (@versioned_output) {
        my @line_info = split( /\s+/, $line );
    
        # the file is the last item in the @line_info array
        my $filename = $line_info[-1];
        $filename =~ s/\\/\//g;
        # ignore the debian directory
        next if $filename =~ m[/\.svn|blib|debian];
        if ( -d $filename ) {
            push @dirs, $filename;
        } else {
            push @versioned_files, $filename;
        }
    }
    $data{dirs} = \@dirs;
    $data{versioned_files} = \@versioned_files;

    # initialize the object from the prepared values (Damian, p. 98)
    %$self = %data;
    return $self;
}

sub prepare_manifest {
    my $self = shift;
    my @manifest_lines;
    
    for my $file (@{ $self->{versioned_files} }) {
        push @manifest_lines, _get_manifest_entry($file);
    }
    return \@manifest_lines;
}

sub print_manifest {
    my $self = shift;
    my $manifest_lines_ref = shift;
    open my $MANIFEST, '>', 'MANIFEST'
        or croak "Unable to open MANIFEST for writing";
    print $MANIFEST <<"END_HEADER";
# ex: set ro:
# $self->{id}
#
# generated by $self->{script} $self->{time} UT
#
# See tools/dev/install_files.pl for documentation on the
# format of this file.
# See docs/submissions.pod on how to recreate this file after SVN
# has been told about new or deleted files.
END_HEADER

    print $MANIFEST $_ for ( sort @{ $manifest_lines_ref } );
    close $MANIFEST or croak "Unable to close MANIFEST after writing";
}

sub _get_manifest_entry {
    my $file = shift;
    # XXX Most of these can probably be cleaned up
    
    my $special = _get_special();
    my $loc  = '[]';
    for ($file) {
        $loc =
              exists( $special->{$_} ) ? $special->{$_}
            : !m[/]                  ? '[]'
            : m[^LICENSES/]          ? '[main]doc'
            : m[^docs/]              ? '[main]doc'
            : m[^editor/]            ? '[devel]'
            : m[^examples/]          ? '[main]doc'
            : m[^include/]           ? '[main]include'
            : ( m[^languages/(\w+)/] and $1 ne 'conversion' ) ? "[$1]"
            : m[^lib/]        ? '[devel]'
            : m[^runtime/]    ? '[library]'
            : m[^tools/docs/] ? '[devel]'
            : m[^tools/dev/]  ? '[devel]'
            : m[^(apps/\w+)/] ? "[$1]"
            :                   '[]';
    }
    return sprintf( "%- 59s %s\n", $file, $loc );
}

sub _get_special {
    my %special = qw(
        LICENSE                                         [main]doc
        NEWS                                            [devel]doc
        PBC_COMPAT                                      [devel]doc
        PLATFORMS                                       [devel]doc
        README                                          [devel]doc
        README.win32.pod                                [devel]doc
        README.win32.pod                                [devel]doc
        RESPONSIBLE_PARTIES                             [main]doc
        TODO                                            [main]doc
        parrot-config                                   [main]bin
        docs/ROADMAP.pod                                [devel]doc
        docs/compiler_faq.pod                           [devel]doc
        docs/configuration.pod                          [devel]doc
        docs/debug.pod                                  [devel]doc
        docs/dev/dod.pod                                [devel]doc
        docs/dev/events.pod                             [devel]doc
        docs/dev/fhs.pod                                [devel]doc
        docs/dev/infant.pod                             [devel]doc
        docs/dev/pmc_freeze.pod                         [devel]doc
        examples/sdl/anim_image.pir                     [devel]
        examples/sdl/anim_image_dblbuf.pir              [devel]
        examples/sdl/blue_font.pir                      [devel]
        examples/sdl/blue_rect.pir                      [devel]
        examples/sdl/bounce_parrot_logo.pir             [devel]
        examples/sdl/lcd/clock.pir                      [devel]
        examples/sdl/move_parrot_logo.pir               [devel]
        examples/sdl/parrot_small.png                   [devel]
        examples/sdl/raw_pixels.pir                     [devel]
        languages/t/harness                             []
        runtime/parrot/dynext/README                    [devel]doc
        runtime/parrot/include/DWIM.pir                 [devel]doc
        runtime/parrot/include/README                   [devel]doc
        src/call_list.txt                               [devel]doc
        src/ops/ops.num                                 [devel]
        tools/build/ops2c.pl                            [devel]
        tools/build/ops2pm.pl                           [devel]
        tools/build/pbc2c.pl                            [devel]
        tools/build/revision_c.pl                       [devel]
        vtable.tbl                                      [devel]
    );
    return \%special;
}

sub prepare_manifest_skip {
    my $self = shift;
    my $svnignore = `$self->{cmd} propget svn:ignore @{ $self->{dirs} }`;
    # cope with trailing newlines in svn:ignore output
    $svnignore =~ s/\n{3,}/\n\n/g;
    my %ignore;
    my @ignore = split( /\n\n/, $svnignore );
    foreach (@ignore) {
        my @cnt = m/( - )/g;
        if ($#cnt) {
            my @a = split /\n(?=(?:.*?) - )/, $_;
            foreach (@a) {
                m/^\s*(.*?) - (.+)/sm;
                $ignore{$1} = $2 if $2;
            }
        }
        else {
            m/^(.*) - (.+)/sm;
            $ignore{$1} = $2 if $2;
        }
    }
    return \%ignore;
}

sub print_manifest_skip {
    my $self = shift;
    my $ignore_ref = shift;
    my %ignore = %{ $ignore_ref };
    my $print_str = <<"END_HEADER";
# ex: set ro:
# $self->{id}
# generated by $self->{script} $self->{time} UT
#
# This file should contain a transcript of the svn:ignore properties
# of the directories in the Parrot subversion repository. (Needed for
# distributions or in general when svn is not available).
# See docs/submissions.pod on how to recreate this file after SVN
# has been told about new generated files.
#
# Ignore the SVN directories
\\B\\.svn\\b

# debian/ should not go into release tarballs
^debian\$
^debian/
END_HEADER

    foreach my $directory ( sort keys %ignore ) {
        my $dir = $directory;
        $dir =~ s!\\!/!g;
        $print_str .= "# generated from svn:ignore of '$dir/'\n";
        foreach ( sort split /\n/, $ignore{$directory} ) {
            s/\./\\./g;
            s/\*/.*/g;
            $print_str .= ($dir ne '.')
                ? "^$dir/$_\$\n^$dir/$_/\n"
                : "^$_\$\n^$_/\n";
        }
    }
    my $current_skips_ref = _get_current_skips();
    my $proposed_skips_ref = _get_proposed_skips($print_str);
    my $different_patterns_count = 0;
    foreach my $cur (keys %{ $current_skips_ref }) {
        $different_patterns_count++ unless $proposed_skips_ref->{$cur};
    }
    foreach my $pro (keys %{ $proposed_skips_ref }) {
        $different_patterns_count++ unless $current_skips_ref->{$pro};
    }
    if ( $different_patterns_count or (! -f 'MANIFEST.SKIP') ) {
        open my $MANIFEST_SKIP, '>', 'MANIFEST.SKIP'
            or die "Unable to open MANIFEST.SKIP for writing";
        print $MANIFEST_SKIP $print_str;
        close $MANIFEST_SKIP
            or die "Unable to close MANIFEST.SKIP after writing";
    }
    return 1;
}

sub _get_current_skips {
    my $sk = q{MANIFEST.SKIP};
    return {} unless -f $sk;
    my %current_skips = ();
    open my $SKIP, "<", $sk or die "Unable to open $sk for reading";
    while (my $line = <$SKIP>) {
        chomp $line;
        next if $line =~ /^\s*$/o;
        next if $line =~ /^#/o;
        $current_skips{$line}++;
    }
    close $SKIP or die "Unable to close $sk after reading";
    return \%current_skips;
}

sub _get_proposed_skips {
    my $print_str = shift;
    my @proposed_lines = split /\n/, $print_str;
    my %proposed_skips = ();
    for my $line (@proposed_lines) {
        next if $line =~ /^\s*$/o;
        next if $line =~ /^#/o;
        $proposed_skips{$line}++;
    }
    return \%proposed_skips;
}

1;

#################### DOCUMENTATION ####################

=head1 NAME

Parrot::Manifest - Re-create MANIFEST and MANIFEST.SKIP

=head1 SYNOPSIS

    use Parrot::Manifest;

    $mani = Parrot::Manifest->new($0);

    $manifest_lines_ref = $mani->prepare_manifest();
    $mani->print_manifest($manifest_lines_ref);

    $ignore_ref = $mani->prepare_manifest_skip();
    $mani->print_manifest_skip($ignore_ref);

=head1 SEE ALSO

F<tools/dev/mk_manifest_and_skip.pl>.

=head1 AUTHOR

James E. Keenan (jkeenan@cpan.org) refactored code from earlier versions of
F<tools/dev/mk_manifest_and_skip.pl>.

=head1 LICENSE

This is free software which you may distribute under the same terms as Perl
itself.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
