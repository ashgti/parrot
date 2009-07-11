#! perl
# Copyright (C) 2007, Parrot Foundation.
# $Id$
# 018-revision_to_cache.t

use strict;
use warnings;

use Test::More;
plan( skip_all =>
    "\nRelevant only when working in checkout from repository and during configuration" )
    unless (-e 'DEVELOPING' and ! -e 'Makefile');
plan( tests =>  8 );
use Carp;
use Cwd;
use File::Copy;
use File::Path ();
use File::Temp qw| tempdir |;
use lib qw( lib );

my $cwd = cwd();
{
    my $tdir = tempdir( CLEANUP => 1 );
    ok( chdir $tdir, "Changed to temporary directory for testing" );
    my $libdir = qq{$tdir/lib};
    ok( (File::Path::mkpath( [ $libdir ], 0, 0777 )), "Able to make libdir");
    local @INC;
    unshift @INC, $libdir;
    ok( (File::Path::mkpath( [ qq{$libdir/Parrot} ], 0, 0777 )), "Able to make Parrot dir");
    ok( (copy qq{$cwd/lib/Parrot/Revision.pm},
            qq{$libdir/Parrot}), "Able to copy Parrot::Revision");
    require Parrot::Revision;
    no warnings 'once';
    like($Parrot::Revision::current, qr/^\d+$/,
        "Got numeric value for revision number");
    use warnings;
    my $cache = q{.parrot_current_rev};
    ok( ( -e $cache ), "Cache for revision number was created");
    unlink qq{$libdir/Parrot/Revision.pm}
        or croak "Unable to delete file after testing";
    ok( chdir $cwd, "Able to change back to starting directory");
}

pass("Completed all tests in $0");

################### DOCUMENTATION ###################

=head1 NAME

018-revision_to_cache.t - test Parrot::Revision

=head1 SYNOPSIS

    % prove t/configure/018-revision_to_cache.t

=head1 DESCRIPTION

The files in this directory test functionality used by F<Configure.pl>.

The tests in this file test Parrot::Revision (F<lib/Parrot/Revision.pm>).

=head1 AUTHOR

James E Keenan

=head1 SEE ALSO

Parrot::Configure, F<Configure.pl>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
