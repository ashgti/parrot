#! perl

# Copyright (C) 2009, Parrot Foundation.
# $Id$

use strict;
use warnings;

use Cwd qw(abs_path getcwd realpath);
use Fatal qw(open);
use File::Spec;
use Test::More;
use Test::Harness;

=head1 NAME

tools/dev/checkdepend.pl

=head1 DESCRIPTION

A braindead script to check that every file has makefile deps
on its includes.

 checkdepend.pl [--dump]

If called with C<--dump>, no tests are run, and the pre-processed makefile
is dumped (in lieu of having C<cc>'s C<-E> for C<make>.

=head1 REQUIREMENTS

A built parrot (Configure and make) to generate all files so we can analyze
them. Ack is used to find the files. We are not currently requiring ack
for development, so this is an optional test.

=head1 BUGS

The pre-processing of the makefile doesn't follow make's behavior: variables
should have no value until they are defined; in our pre-processing, their
values are propagated throughout the makefile regardless of order; This could
cause false positives in the test output.

=cut

die 'no Makefile found; This tool requires a full build for analysis.'
    unless -e 'Makefile';

my @incfiles = split /\n/, `ack -fa src compilers include  | ack '\\.(c|h|pir)\$'`;

our %deps;

foreach my $file (sort grep /\.[hc]$/, @incfiles) {
    # For now, skip any files that have generated dependencies
    next if $file =~ m{src/(ops|dynoplibs|dynpmc|pmc)/};
    next if $file =~ m{src/string/(charset|encoding)/};

    open my $fh, '<', $file;
    my $guts;
    {
        local $/;
        $guts = <$fh>;
    }

    # Ignore anything inside a c-style comment.
    $guts =~ s{\Q/*\E.*?\Q*/}{}gm;

    my @includes = $guts =~ m/#include "(.*)"/g;

    # Canonicalize each of these includes.

    $deps{$file} = [ ];
    foreach my $include (@includes) {
        # same dir as file?
        my $file_dir = (File::Spec->splitpath($file))[1];
        my $make_dep = collapse_path(File::Spec->catfile($file_dir,$include));
        if (defined($make_dep) && -f $make_dep) {
            push @{$deps{$file}}, $make_dep;
            next;
        }

        # global 'include' dir?
        $make_dep = collapse_path(File::Spec->catfile('include',$include));
        if (defined($make_dep) && -f $make_dep) {
            push @{$deps{$file}}, $make_dep;
            next;
        }

        diag "couldn't find $include, included from $file";
    }
}

foreach my $file (sort grep /\.pir$/, @incfiles) {
    open my $fh, '<', $file;
    my $guts;
    {
        local $/;
        $guts = <$fh>;
    }

    # Ignore anything inside a # - comment.
    $guts =~ s{^#.*$}{}gm;
    # Ignore anything inside pod.
    $guts =~ s{^=.*^=cut$}{}gsm;
    # Ignore anything inside strings that are assigned to a variable.
    # (Avoid clobbering the strings used in .include 'foo.pir', etc.)
    $guts =~ s{=\s*'[^']*'\s*$}{}gm;
    $guts =~ s{=\s*"(?:[^"\\]|\\.)*"\s*$}{}gm;
    # XXX also, heredocs (wheeeee!)

    my @includes;
    while ($guts =~ m/(?:\.include|\bload_bytecode)\s+(["'])(.*)\1/g) {
        push @includes, $2;
    }

    # Canonicalize each of these includes.

    $deps{$file} = [ ];
    foreach my $include (@includes) {
        # same dir as file?
        my $file_dir = (File::Spec->splitpath($file))[1];
        my $make_dep = collapse_path(File::Spec->catfile($file_dir,$include));
        if (defined($make_dep) && -f $make_dep) {
            push @{$deps{$file}}, $make_dep;
            next;
        }

        # global 'runtime' dir?
        $make_dep = collapse_path(File::Spec->catfile('runtime/parrot/include',$include));
        if (defined($make_dep) && -f $make_dep) {
            push @{$deps{$file}}, $make_dep;
            next;
        }
        $make_dep = collapse_path(File::Spec->catfile('runtime/parrot/library',$include));
        if (defined($make_dep) && -f $make_dep) {
            push @{$deps{$file}}, $make_dep;
            next;
        }

        # relative to top level?
        $make_dep = collapse_path(File::Spec->catfile($include));
        if (defined($make_dep) && -f $make_dep) {
            push @{$deps{$file}}, $make_dep;
            next;
        }

        diag "couldn't find $include, included from $file";
    }
}


open my $mf, '<', "Makefile";
my $rules;
{
    local $/;
    $rules = <$mf>;
}
close $mf;

# inline all "include"'d Makefiles;
while ($rules =~ /^include\s+(.*)$/m) {
    my $sub_makefile = $1;
    open my $smf, '<', $sub_makefile;
    my $subrules;
    {
        local $/;
        $subrules = <$smf>;
    }
    close $smf;
    $rules =~ s/^include\s+$sub_makefile$/$subrules/m;
}

# convert all \-newline continuations into single lines for ease of processing.
$rules =~ s/\\\n//g;

# replace all _DIR variables with their expansions.
while ($rules =~ s/^([A-Z_]+_DIR)\s*=\s*(\S*)$//m) {
    my ($var,$val) = ($1, $2);
    $rules =~ s/\$\($var\)/$val/g;
}

# expand PARROT_H_HEADERS
$rules =~ m/^PARROT_H_HEADERS\s*=\s*(.*)$/m;
my $phh = $1;

$rules =~ s/\Q$(PARROT_H_HEADERS)/$phh/g;

if (@ARGV && $ARGV[0] eq '--dump') {
    print $rules;
    exit 0;
}

my $test_count = grep {/\.(c|pir)$/} (keys %deps);

plan( tests => $test_count );

foreach my $header (sort grep {/\.h$/} (keys %deps)) {
    # static headers shouldn't depend on anything else.
    if ($rules =~ /^$header\s*:\s*(.*)\s*$/m) {
        #is("", $1, "$header should have no dependencies");
    }
}

my @files = keys %deps;
@files = @ARGV if @ARGV;

check_files(\@files, '.c',   '\$\(O\)');
check_files(\@files, '.pir', '.pbc');

sub check_files {
    
    my ($possible_files, $src_ext, $obj_ext) = @_;

    foreach my $file (sort grep {/$src_ext$/} @$possible_files) {
        my $rule = $file;
        $rule =~ s/$src_ext$//;

        $rules =~ /^${rule}${obj_ext}\s*:\s*(.*)\s*$/m;
        my $rule_deps = defined $1 ? $1 : '';

        my $expected_deps = join ' ', sort (get_deps($file));
        $rule_deps        = join ' ', sort split /\s+/, $rule_deps;

        is($rule_deps, $expected_deps, "$file has correct dependencies.");
    }
}

sub collapse_path {
    my $path = shift;
    return $path unless defined $path;

    my $cwd = getcwd();
    $path = abs_path($path);
    return $path unless defined $path;

    $path =~ s/^\Q$cwd\E\///;
    return $path;
}

sub get_deps {
    my $file = shift;
    my %all_deps;

    if (! exists $deps{$file}) {
        return;
    }

    # add explicit deps
    my @pending_deps = @{$deps{$file}};

    while (@pending_deps) {
        my $dep = shift @pending_deps;
        if (!exists $all_deps{$dep}) {
            $all_deps{$dep} = 1;
            if (exists $deps{$dep}) {
                push @pending_deps, @{$deps{$dep}};
            }
        }
    }

    return keys %all_deps;
}

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
