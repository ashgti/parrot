#! perl
# Copyright (C) 2004-2007, Parrot Foundation.
# $Id$

=head1 NAME

tools/dev/parrot_api.pl - Verify Parrot API (symbols)

=head1 SYNOPSIS

    % perl tools/dev/parrot_api.pl [libfile]

=head1 DESCRIPTION

Displays the API (the visible symbols, code or data) of the Parrot lib.

First lists the Parrot public embedding API as described in the public
headers F<include/parrot/embed.h> and F<include/parrot/extend.h> (the
API is detected using pattern C</^\w+\s+(Parrot_\w+)\(/>), then finds
out the visible symbols in the Parrot lib (by default
F<blib/lib/libparrot.a>), and then cross-references the dubious API
symbols according to the below categories.  Each symbol is listed with
the object file it was found in.

Good things are listed with C<+++> (at the moment the only good thing
is Parrot APIs definitions their declarations), bad things are listed
with C<--->.

=over 4

=item Missing Parrot API

The API is listed in the public headers but not defined in the Parrot lib.

Either the API listing is wrong or the implementation is missing.

=item No Parrotish Prefix

The API is implemented but has no C<Parrot_> prefix (or prefix deemed
Parroty enough, like C<PDB_>, C<PF_>, C<PIO_>, and C<PackFile>).

If code: see L</"Public or Private">.

If data: at least consider limiting its scope by making it file
static or function static, but see L</"Data is not an API">.

=item No Parrot API

The API is defined in the lib but not defined in the public headers.

If code: see L</"Public or Private">.

If data: see L</"Data is not an API">.

=item Uninitialized Modifiable Data

Data symbol that is not initialized with any data.

See L</"Data is not an API">.

=item Initialized Modifiable Data

Data symbol that is initialized with data, but modifiable.

See L</"Data is not an API">.

=back

=head1 RULES

You can sometimes use C preprocessor defines to shorten the API names
but please do carefully contain the effect so that only the Parrot
itself sees those shortened definitions, the defines must not leak
to the outside world.

=head2 Public or Private

If the API is really meant to be public, prefix it with <Parrot_>,
or something else specific enough, preferably specific to Parrot,
not some generic term.  Currently acceptable prefixes are
C</^(Parrot|PDB|PF|PIO|PackFile)_/>.

If the API is not meant to be public, considering making it private
(file static, and prefix it with C<S_>, but still do have a prototype
for it).

=head2 Data is not an API

Consider making the data const(ant), or moving it into the heap (and
accessed through a real API that takes care of synchronization, data
as such is not a good API unless it's constant).

Think multithreaded access, or just plain reentrancy (think recursion).

Often you can collect a group of data fields into a "context" (or
"descriptor", "handle", "session", "state") structure that is either
explicitly passed between functions or implicitly retrieved from a
global API.  Encapsulating data like this is good: it allows passing,
synchronzing, and duplicating state much easier, and makes it clearer
into which context the data belongs.

For APIs purely internal to Parrot, try using C<const> in function
prototypes as often as possible to catch inadvertent data modification
by callers.  For APIs that cross into the operating system and/or
external libraries, you usually cannot go overly C<const> because of
portability issues.

Make your strings and arrays of strings (or similar inlined data)
C<const> -- for the latter, remember that you need two consts:

  const char* const foo[] = { "foo", "bar" };

=head1 DEPENDENCIES

Uses F<tools/dev/nm.pl> to list the symbols.

=head1 TODO

=over 4

=item *

Write a pollution detector also for the C preprocessor: also in that
namespace, Parrot should only present Parrot_ (and similar) symbols to
the outside world.  Be careful to only scan Parrot's defines, not
system's or third parties'.

=back

=head1 HISTORY

Author: Jarkko Hietaniemi.

=cut

use strict;
use warnings;

my $Obj;

$Obj = shift(@ARGV)           unless defined $Obj;
$Obj = 'blib/lib/libparrot.a' unless defined $Obj;
die "$0: '$Obj': No such file\n" unless -f $Obj;

my %ParrotAPI;

$| = 1;

my @H = qw(include/parrot/embed.h include/parrot/extend.h);

for my $h (@H) {
    if ( open( my $H, '<', $h ) ) {
        while (<$H>) {
            if (/^\w+\s+(Parrot_\w+)\(/) {
                $ParrotAPI{$1}++;
            }
        }
        close($H);
    }
    else {
        die "$0: Header '$h': $!\n";
    }
}

my @ParrotAPI = sort keys %ParrotAPI;

die "$0: No API found in @H\n" unless @ParrotAPI;

printf "=== @H: %d interfaces ===\n", scalar @ParrotAPI;

my %Code;
my %DataB;
my %DataD;
my %DataR;
my %Undef;
my %API;

if ( open( my $NM, '<', "perl tools/dev/nm.pl -BDo '$Obj' |" ) ) {
    while (<$NM>) {
        my ( $o, $s, $v ) = split;
        $API{$s} = $o;
        if ( $v eq 'T' ) {
            $Code{$s} = $o;
        }
        elsif ( $v =~ /[BDR]/ ) {
            if ( $v eq 'B' ) {
                $DataB{$s} = $o;
            }
            elsif ( $v eq 'D' ) {
                $DataD{$s} = $o;
            }
            elsif ( $v eq 'R' ) {
                $DataR{$s} = $o;
            }
        }
        elsif ( $v eq 'U' ) {
            $Undef{$s} = $o;
        }
    }
    close($NM);
}
else {
    die "$0: nm.pl -Bgo '$Obj': $!\n";
}
for my $api ( keys %API ) {
    delete $API{$api} unless exists $Code{$api};    # Not ours.
}

printf "+++ Parrot API: %d +++\n", scalar @ParrotAPI;
if (@ParrotAPI) {
    for my $api (@ParrotAPI) {
        printf "%s\t%s\tOK\n", $api, $API{$api} || "-";
    }
}

printf "=== $Obj: %d interfaces ===\n", scalar keys %API;

my @API = sort keys %API;

my @NoParrotAPI = grep { !exists $API{$_} } sort keys %ParrotAPI;
my @NoParrotPrefix;
my @UnParrotAPI;

my $ParrotPrefix = qr/^(Parrot|PDB|PF|PIO|PackFile)_/;

for my $api (@API) {
    unless ( $api =~ $ParrotPrefix ) {
        push @NoParrotPrefix, $api;
    }
    unless ( exists $ParrotAPI{$api} || $api =~ $ParrotPrefix ) {
        push @UnParrotAPI, $api;
    }
}

if (@NoParrotAPI) {
    printf "--- Missing Parrot API: %d ---\n", scalar @NoParrotAPI;
    for my $api (@NoParrotAPI) {
        printf "%s\t%s\tMISSING_API\n", $api, "-";
    }
}

if (@NoParrotPrefix) {
    printf "--- No Parrot prefix: %d ---\n", scalar @NoParrotPrefix;
    for my $api (@NoParrotPrefix) {
        printf "%s\t%s\tBAD_PREFIX\n", $api, $API{$api};
    }
}

if (@UnParrotAPI) {
    printf "--- No Parrot API: %d ---\n", scalar @UnParrotAPI;
    for my $api (@UnParrotAPI) {
        printf "%s\t%s\tNO_API\n", $api, $API{$api};
    }
}

if ( keys %DataB ) {
    printf "--- Uninitialized Modifiable Data: %d ---\n", scalar keys %DataB;
    for my $api ( sort keys %DataB ) {
        printf "%s\t%s\tUMD\n", $api, $DataB{$api};
    }
}

if ( keys %DataD ) {
    printf "--- Initialized Modifiable Data: %d ---\n", scalar keys %DataD;
    for my $api ( sort keys %DataD ) {
        printf "%s\t%s\tIMD\n", $api, $DataD{$api};
    }
}

exit(0);

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
