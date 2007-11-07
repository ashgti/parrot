#! perl
# Copyright (C) 2006-2007, The Perl Foundation.
# $Id$

=head1 NAME

t/codingstd/linelength.t - Test code lines length

=head1 SYNOPSIS

    # test all files
    % prove t/codingstd/linelength.t

    # test specific files
    % perl t/codingstd/linelength.t src/foo.c include/parrot/bar.h

=head1 DESCRIPTION

Tests source files for the line length limit as defined in
F<pdd07_codingstd.pod>.  Only files for some language implementations are
checked.

=head1 SEE ALSO

L<docs/pdds/pdd07_codingstd.pod>

=cut

use strict;
use warnings;

my $num_col_limit = 100;

use lib qw( . lib ../lib ../../lib );

use File::Spec;
use Test::More tests => 1;
use Parrot::Config;
use ExtUtils::Manifest qw( maniread );

# a list of languages where we want to test line length
my %lang_is_checked = map { $_ => 1 } qw{
    APL
    WMLScript
    amber
    cardinal
    dotnet
    lua
    perl6
    pugs
    python
    plumhead
    tcl
};

# RT#44437 this should really be using src_dir instead of build_dir but it
# doesn't exist (yet)
my $build_dir = $PConfig{build_dir};
my $manifest = maniread( File::Spec->catfile( $build_dir, 'MANIFEST' ) );

# skip files listed in the __DATA__ section
my %skip_files;
while (<DATA>) {
    next if m{^#};
    next if m{^\s*$};
    chomp;
    $skip_files{$_}++;
}

# find the files that we need to check
my @files = @ARGV ? @ARGV : source_files();

# check all the files and keep a list of those failing
my @lines_too_long;
foreach (@files) {
    my $lineinfo = info_for_first_long_line($_);
    next unless $lineinfo;
    push @lines_too_long => $lineinfo;
}

## L<PDD07/Code Formatting/"Source line length is limited to 100 characters">
ok( !@lines_too_long, 'Line length ok' )
    or diag( "Lines longer than coding standard limit ($num_col_limit columns) in "
        . scalar @lines_too_long
        . " files:\n"
        . join( "\n", @lines_too_long ) );

sub info_for_first_long_line {
    my $file = shift;

    open my $fh, '<', $file or die "Can't open file '$file'";
    while ( my $line = <$fh> ) {
        chomp $line;
        $line =~ s/\t/' ' x (1 + length($`) % 8)/eg;    # expand \t
        next if $line =~ m/https?:\/\//;                # skip long web addresses
        return sprintf '%s:%d: %d cols', $file, $., length($line)
            if length($line) > $num_col_limit;
    }
    return;
}

sub source_files {
    my @files;
    foreach my $file ( sort keys(%$manifest) ) {
        my $full_path = File::Spec->catfile( $build_dir, $file );

        # skip binary files (including .pbc files)
        next if -B $full_path;

        # skip files specified in __DATA__ section
        next if exists $skip_files{$file};

        # skip languages files, unless specifically included above
        next if $file =~ m{^languages/([^/]+)/} && !$lang_is_checked{$1};

        push @files => $full_path
            if $file =~ m{\.(c|pmc|ops|pod)$};
    }
    return @files;
}

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:

__DATA__
# Lex and Bison generated
compilers/ast/astlexer.c
compilers/ast/astparser.c
compilers/imcc/imclexer.c
compilers/imcc/imcparser.c
compilers/pirc/new/pirlexer.c
compilers/pirc/new/pirlexer.h
compilers/pirc/new/pirparser.c
compilers/pirc/macro/macro.h
compilers/pirc/macro/macrolexer.c
compilers/pirc/macro/macrolexer.h
compilers/pirc/macro/macroparser.c
compilers/pirc/macro/macroparser.h
compilers/pirc/heredoc/hdocprep.l
compilers/pirc/heredoc/hdocprep.c
languages/plumhead/src/yacc/plumhead_lexer.c
languages/plumhead/src/yacc/plumhead_parser.c
languages/plumhead/src/yacc/plumhead_parser.h
# Generators with big strings
tools/build/jit2c.pl
tools/build/nativecall.pl
tools/dev/lib_deps.pl
tools/dev/mk_inno.pl
tools/dev/parrot_coverage.pl
# these ones include a big URL
cage/todo.pod
docs/dev/pmc_obj_design_meeting_notes.pod
docs/gettingstarted.pod
docs/glossary.pod
languages/LANGUAGES_STATUS.pod
