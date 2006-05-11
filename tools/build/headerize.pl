#! perl
# Copyright: 2001-2006 The Perl Foundation.  All Rights Reserved.
# $Id: pmc2c.pl 12524 2006-05-05 21:50:26Z petdance $

=head1 NAME

tools/build/headerizer.pl - Generates the function header parts of .h files from .c files

=head1 SYNOPSIS

Update the headers in F<include/parrot> with the function declarations in
the F<*.pmc> or F<*.c> files that correspond to the F<*.o> files passed
on the command line.

    % perl tools/build/headerizer.pl OBJFILES

=head1 DESCRIPTION

=head1 TODO

* Tell if there are funcs without docs

* Generate docs from funcs

* Test the POD of the stuff we're parsing.

=head1 COMMAND-LINE OPTIONS

=over 4

=item C<--verbose>

Verbose status along the way.

=back

=head1 COMMAND-LINE ARGUMENTS

=over 4

=item C<OBJFILES>

One or more object file names.

=back

=cut

use strict;
use warnings;

use Data::Dumper;
use Getopt::Long;
use lib qw( lib );
use Parrot::Config;

my %opt;

main();

=head1 FUNCTIONS

=cut

sub open_file {
    my $direction = shift;
    my $filename = shift;

    my %actions = (
        "<" => "Reading",
        ">" => "Writing",
        ">>" => "Appending",
    );

    my $action = $actions{$direction} or die "Invalid direction '$direction'";
    print "$action $filename\n" if $opt{verbose};
    open my $fh, $direction, $filename or die "$action $filename: $!\n";
    return $fh;
}


sub extract_functions {
    my $text = shift;

    # Strip blocks of comments
    $text =~ s[^/\*.*?\*/][]mxsg;

    # Strip # compiler directives (Thanks, Audrey!)
    $text =~ s[^#(\\\n|.)*][]mg;

    # Strip code blocks
    $text =~ s[^{.+?^}][]msg;

    # Split on paragraphs
    my @funcs = split /\n{2,}/, $text;

    # If it doesn't start in the left column, it's not a func
    @funcs = grep /^\S/, @funcs;

    # Typedefs and structs are no good
    @funcs = grep !/^(typedef|struct|enum|extern)/, @funcs;

    # Variables are of no use to us
    @funcs = grep !/=/, @funcs;

    # Get rid of any blocks at the end
    s/\s*{.*//s for @funcs;

    # Toast anything non-whitespace
    @funcs = grep /\S/, @funcs;

    # If it's got a semicolon, it's not a function header
    @funcs = grep !/;/, @funcs;

    chomp @funcs;

    return @funcs;
}


sub main {
    GetOptions(
        "verbose"   => \$opt{verbose},
    ) or exit(1);

    my @ofiles = @ARGV;
    for my $ofile ( @ofiles ) {
        next if $ofile =~ m/^\Qsrc$PConfig{slash}ops\E/;

        my $cfile = $ofile;
        $cfile =~ s/\Q$PConfig{o}\E$/.c/;

        my $pmcfile = $ofile;
        $pmcfile =~ s/\Q$PConfig{o}\E$/.pmc/;

        my $sourcefile = -f $pmcfile ? $pmcfile : $cfile;

        my $fh = open_file( "<", $cfile );
        my $source = do { local $/; <$fh> };
        close $fh;

        print "\n=== $cfile ===\n";
        my @funcs = extract_functions( $source );

        for my $func ( @funcs ) {
            print "---\n$func\n";
        }
    } # for @cfiles
}

# vim: expandtab shiftwidth=4:
