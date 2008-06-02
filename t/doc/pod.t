#! perl
# Copyright (C) 2001-2005, The Perl Foundation.
# $Id$

=head1 NAME

t/doc/pod.t - Pod document syntax tests

=head1 SYNOPSIS

    # test all files
    % prove t/doc/pod.t

    # test specific files
    % perl t/doc/pod.t perl_module.pm perl_file.pl

=head1 DESCRIPTION

Tests the Pod syntax for all files listed in F<MANIFEST> and
F<MANIFEST.generated> that appear to contain Pod markup. If any files
contain invalid POD markup, they are reported in the test output.
Use C<podchecker> to ferret out individual issues.

=cut

use strict;
use warnings;

use lib qw( . lib ../lib ../../lib );

use Test::More;
use Parrot::Config;
use ExtUtils::Manifest qw(maniread);

use vars qw(@failed);

BEGIN {
    eval 'use Pod::Find';
    if ($@) {
        plan skip_all => 'Pod::Find not installed';
        exit;
    }
    eval 'use Pod::Simple';
    if ($@) {
        plan skip_all => 'Pod::Simple not installed';
        exit;
    }
}

plan tests => 1;

# RT#44437 this should really be using src_dir instead of build_dir but it
# does not exist (yet)
my $build_dir    = $PConfig{build_dir};
my $manifest     = maniread("$build_dir/MANIFEST");
my $manifest_gen = maniread("$build_dir/MANIFEST.generated");

# if we have files passed in at the command line, use them
my @files;
if (@ARGV) {
    @files = @ARGV;
}
else {
    diag "finding files with POD, this may take a minute.";
    @files = ( keys(%$manifest), keys(%$manifest_gen) );
}

foreach my $file (@files) {
    $file = "$build_dir/$file";

    # skip missing MANIFEST.generated files
    next unless -e $file;

    # Skip the book, because it uses extended O'Reilly-specific POD
    next if $file =~ m{docs/book/};

    # skip binary files (including .pbc files)
    next if -B $file;

    # skip files without POD
    next unless Pod::Find::contains_pod( $file, 0 );

    # skip POD generating scripts
    next if $file =~ m/ops_summary\.pl/;

    # skip file which includes malformed POD for other testing purposes
    next if $file =~ m{t/tools/dev/searchops/samples\.pm};

    # skip files with valid POD
    next if file_pod_ok($file);
    push @failed, $file;
}

my $bad_files = join( "\n", @failed );
is( $bad_files, q{}, 'Pod syntax correct' );    # only ok if everything passed

diag("You should use podchecker to check the failed files.\n")
    if $bad_files;

# Pulled from Test::Pod
sub file_pod_ok {
    my $file    = shift;
    my $checker = Pod::Simple->new;

    $checker->output_string( \my $trash );      # Ignore any output
    $checker->parse_file($file);

    return !$checker->any_errata_seen;
}

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
