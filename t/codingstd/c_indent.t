#! perl
# Copyright (C) 2006-2007, The Perl Foundation.
# $Id$

use strict;
use warnings;

use lib qw( . lib ../lib ../../lib );
use Test::More tests => 2;
use Parrot::Distribution;

=head1 NAME

t/codingstd/c_indent.t - checks for rules related to indenting in C source

=head1 SYNOPSIS

    # test all files
    % prove t/codingstd/c_indent.t

    # test specific files
    % perl t/codingstd/c_indent.t src/foo.c include/parrot/bar.h

=head1 DESCRIPTION

Checks that all C language source files have the proper use of indentation,
as specified in PDD07.

=head1 SEE ALSO

L<docs/pdds/pdd07_codingstd.pod>

=cut

my @files = @ARGV ?
                @ARGV
                :
                map { $_->path() } Parrot::Distribution->new()->get_c_language_files();

check_indent(@files);

sub check_indent {
    my ( @pp_indent, @c_indent );
    my ( %pp_failed, %c_failed );

    foreach my $path (@_) {
        my @source;
        open my $fh, '<', $path
            or die "Can not open '$path' for reading!\n";
        @source = <$fh>;

        my @stack;                   # for tracking indention level
        my $line_cnt       = 0;
        my $f              = undef;
        my $prev_last_char = '';
        my $last_char      = '';
        my $in_comment     = 0;

        foreach (@source) {
            $line_cnt++;
            next unless defined $_;
            chomp;

            $prev_last_char = $last_char;
            $last_char = substr($_, -1, 1);

            # ignore multi-line comments (except the first line)
            $in_comment = 0, next if $in_comment && m{\*/} && $' !~ m{/\*};
            next if $in_comment;
            $in_comment = 1 if m{/\*} && $' !~ m{\*/};

            ## preprocessor scan
            if ( m/ ^ \s* \#
                    (\s*)
                    ( ifndef | ifdef | if )
                    \s+(.*)
                  /x ) {
                next if ( m/PARROT_IN_CORE|_GUARD/ );

                my $indent = q{  } x @stack;
                if ( $1 ne $indent ) {
                    push @pp_indent => "$path:$line_cnt\n"
                        . "     got: $_"
                        . "expected: #$indent$2 $3'\n";
                    $pp_failed{"$path\n"} = 1;
                }
                push @stack, "#$2 $3";
                next;
            }
            if ( m/ ^ \s* \#
                    (\s*)
                    ( else | elif )
                  /x ) {

                # stay where we are, but indenting should be
                # back even with the opening brace.
                my $indent = q{  } x ( @stack - 1 );
                if ( $1 ne $indent ) {
                    push @pp_indent => "$path:$line_cnt\n"
                        . "     got: $_"
                        . "expected: #$indent$2 -- it's inside of "
                        . ( join ' > ', @stack ) . "\n";
                    $pp_failed{"$path\n"} = 1;
                }
                next;
            }
            if ( m/ ^ \s* \#
                    (\s*)
                    (endif)
                  /x ) {
                my $indent = q{  } x ( @stack - 1 );
                if ( $1 ne $indent ) {
                    push @pp_indent => "$path:$line_cnt\n"
                        . "     got: $_"
                        . "expected: #$indent$2 --  it's inside of "
                        . ( join ' > ', @stack ) . "\n";
                    $pp_failed{"$path\n"} = 1;
                }
                pop @stack;
                next;
            }
            next unless @stack;

            if ( m/ ^ \s* \#
                    (\s*)
                    (.*)
                  /x ) {
                my $indent = q{  } x (@stack);
                if ( $1 ne $indent ) {
                    push @pp_indent => "$path:$line_cnt\n"
                        . "     got: $_"
                        . "expected: #$indent$2 -- it's inside of "
                        . ( join ' > ', @stack ) . "\n";
                    $pp_failed{"$path\n"} = 1;
                }
                next;
            }

            ## c source scan
            # for now just try to catch glaring errors.  A real parser is
            # probably overkill for this task.  For now we just check the
            # first line of a function, and assume that more likely than not
            # indenting is consistent within a func body.
            if (/^(\s*).*\{\s*$/) {

                # note the beginning of a block, and its indent depth.
                $f = length($1);
                next;
            }

            if (/^\s*([\#\}])/) {

                # skip the last line of the func or cpp directives.
                $f = undef if ( $1 eq "}" );
                next;
            }

            if ( defined($f) ) {

                # first line of a block
                if ( $f == 0 ) {

                    # first line of a top-level block (first line of a function,
                    # in other words)
                    my ($indent) = /^(\s*)/;
                    if ( length($indent) != 4 ) {
                        push @c_indent => "$path:$line_cnt\n"
                            . "    apparent non-4 space indenting ("
                            . length($indent)
                            . " spaces)\n";
                        $c_failed{"$path\n"} = 1;
                    }
                }
                $f = undef;
            }

            my ($indent) = /^(\s+)/ or next;
            $indent = length($indent);

            # Ignore the indentation of the current line if that
            # previous line's last character was anything but a ;.
            #
            # The indentation of the previous line is not considered.
            # Check sanity by verifying that the indentation of the current line
            # is divisible by four.
            if ($indent % 4 && !$in_comment && $prev_last_char eq ';')
            {
                push @c_indent => "$path:$line_cnt\n"
                    . "    apparent non-4 space indenting ($indent space"
                    . ($indent == 1 ? '' : 's')
                    . ")\n";
                $c_failed{"$path\n"} = 1;
            }
        }
    }

    # get the lists of files failing the test
    my @c_failed_files  = keys %c_failed;
    my @pp_failed_files = keys %pp_failed;

## L<PDD07/Code Formatting/"Preprocessor #directives must be indented two columns per nesting level, with two exceptions: neither PARROT_IN_CORE nor the outermost _GUARD #ifdefs cause the level of indenting to increase">
    ok( !scalar(@pp_indent), 'Correctly indented preprocessor directives' )
        or diag( "incorrect indenting in preprocessor directive found "
            . scalar @pp_indent
            . " occurrences in "
            . scalar @pp_failed_files
            . " files:\n@pp_indent" );

    ok( !scalar(@c_indent), 'Correctly indented C files' )
        or diag( "incorrect indenting in C file found "
            . scalar @c_indent
            . " occurrences in "
            . scalar @c_failed_files
            . " files:\n@c_indent" );
}

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
