#! perl
# $Id$

# Copyright (C) 2004-2007, The Perl Foundation.

=head1 NAME

tools/build/c2str.pl - constant string support

=cut

use warnings;
use strict;
use lib 'lib';

use Fcntl qw( :DEFAULT :flock );
use Text::Balanced qw(extract_delimited);
use Math::BigInt;
use Getopt::Long;

my $outfile          = 'all_cstring.str';
my $string_private_h = 'src/string_private_cstring.h';
my $lockfile         = "$outfile.lck";

# add read/write permissions even if we don't read/write the file
# for example, Solaris requires write permissions for exclusive locks
sysopen( my $lock, $lockfile, O_CREAT | O_RDWR ) or die "Can't write '$lockfile': $!\n";

flock( $lock, LOCK_EX ) or die "Can't lock '$lockfile': $!\n";

END { close $lock; unlink $lockfile; }

my ( $result, $do_all, $do_init, $file );
$result = GetOptions(
    "all"  => \$do_all,
    "init" => \$do_init,
);

$do_all and do {
    read_all();
    create_c_include();
    exit;
};
$do_init and do {
    unlink $outfile;
    exit;
};

$file = shift @ARGV;
$file =~ s/\.c$//;
my $infile = $file . '.c';
die "$0: $infile: $!" unless -e $infile;

my %known_strings = ();
my @all_strings;

read_all();
open my $ALL, '>>', $outfile or die "Can't write '$outfile': $!";
process_cfile();
close $ALL;

sub hash_val {
    my $h = Math::BigInt->new('+0');
    my $s = shift;
    for ( my $i = 0 ; $i < length($s) ; ++$i ) {
        $h += $h << 5;
        $h &= 0xffffffff;
        $h += ord substr( $s, $i, 1 );
        $h &= 0xffffffff;
    }
    return sprintf( "0x%x", $h );
}

sub read_all {
    if ( -e $outfile ) {
        open my $IN, '<', $outfile;
        while (<$IN>) {

            # len hashval "string"
            if (/(\d+)\s+(0x[\da-hA-H]+)\s+"(.*)"/) {
                push @all_strings, [ $1, $2, $3 ];
                $known_strings{$3} = scalar @all_strings;
            }
        }
        close($IN);
    }
    return;
}

sub process_cfile {
    open my $IN, '<', $infile or die "Can't read '$infile': $!";

    my $line = 0;
    print <<"HEADER";
/* ex: set ro:
 * !!!!!!!   DO NOT EDIT THIS FILE   !!!!!!!
 *
 * This file is generated automatically from '$infile'
 * by $0.
 *
 * Any changes made here will be lost!
 *
 */

/* HEADERIZER HFILE: none */
/* HEADERIZER STOP */

#define CONCAT(a,b) a##b
#define _CONST_STRING(i, l) (i)->const_cstring_table[CONCAT(_CONST_STRING_, l)]
#define CONST_STRING(i, s) _CONST_STRING(i, __LINE__)

HEADER
    print $ALL "# $infile\n";
    my %this_file_seen;

    # NOTE: when CONST_STRING gets used in a .pmc it *should not* be split
    # across more than one line, because some compilers generate line
    # numbers in such cases differently to the way gcc does this (a case in
    # point is Intel's C compiler, icc) and hence the #defined CONST_STRING
    # won't be found by the compiler.

    # There is a chance that the same __LINE__ will reoccur if #line directives
    # are used.
    my %lines_seen;
    while (<$IN>) {
        if (m/^\s*#\s*line\s+(\d+)/) {

            # #line directive
            $line = $1 - 1;
            next;
        }
        $line++;
        next if m/^\s*#/;    # otherwise ignore preprocessor
        next unless s/.*\bCONST_STRING\s*\(\w+\s*,//;

        if ( $lines_seen{$line}++ ) {
            die "Seen line $line before in $infile - can't continue";
        }

        # TODO maybe cope with escaped \"
        my $cnt = tr/"/"/;
        die "bogus CONST_STRING at line $line" unless $cnt == 2;

        my $str = extract_delimited;    # $_, '"';
        $str = substr $str, 1, -1;
        ## print STDERR "** '$str' $line\n";
        my $n;
        if ( $n = $known_strings{$str} ) {
            if ( $this_file_seen{$str} ) {
                print "#define _CONST_STRING_$line _CONST_STRING_", $this_file_seen{$str}, "\n";
            }
            else {
                print "#define _CONST_STRING_$line $n\n";
            }
            $this_file_seen{$str} = $line;
            next;
        }
        my $len     = length $str;
        my $hashval = hash_val($str);
        push @all_strings, [ $len, $hashval, $str ];
        $n                    = scalar @all_strings;
        $known_strings{$str}  = $n;
        $this_file_seen{$str} = $line;
        print "#define _CONST_STRING_$line $n\n";
        print $ALL qq!$len\t$hashval\t"$str"\n!;
    }
    close($IN);
    return;
}

sub create_c_include {
    open my $OUT, '>', $string_private_h
        or die "Can't write '$string_private_h': $!";
    print $OUT <<"HEADER";
/* ex: set ro:
 * !!!!!!!   DO NOT EDIT THIS FILE   !!!!!!!
 *
 * This file is generated automatically from '$outfile'
 * by $0.
 *
 * Any changes made here will be lost!
 *
 */

/* HEADERIZER HFILE: none */
/* HEADERIZER STOP */

#ifndef PARROT_SRC_STRING_PRIVATE_CSTRING_H_GUARD
#define PARROT_SRC_STRING_PRIVATE_CSTRING_H_GUARD

static const struct _cstrings {
    UINTVAL len;
    Parrot_UInt4 hash_val;
    const char *string;
} parrot_cstrings[] = {
    { 0, 0, "" },
HEADER
    my @all;
    for my $s (@all_strings) {
        push @all, qq!    {$s->[0], $s->[1], "$s->[2]"}!;
    }
    print $OUT join( ",\n", @all );
    print $OUT <<HEADER;

};

HEADER

    # append the C code coda
    print $OUT <<HEADER;

#endif /* PARROT_SRC_STRING_PRIVATE_CSTRING_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
HEADER

    close $OUT;
    return;
}

=for never

print <<"HEADER";
/*
 * !!!!!!!   DO NOT EDIT THIS FILE   !!!!!!!
 *
 * This file is generated automatically from '$infile'
 * by $0.
 *
 * Any changes made here will be lost!
 *
 */

#define CONCAT(a,b) a##b
#define _S(name) (__PARROT_STATIC_STR(__LINE__))
#define __PARROT_STATIC_STR(line) CONCAT(&static_string_, line)

#if ! DISABLE_GC_DEBUG
#  define GC_DEBUG_VERSION ,0
#else
#  define GC_DEBUG_VERSION
#endif

HEADER

# currently unused true const strings
sub output_string {
  my ($text, $line) = @_;

  if (exists $known_strings{$text}) {
    <<"DATA";
#define static_string_${line} static_string_$known_strings{$text}

DATA
  }
  else {
    $known_strings{$text} = $line;
    my $h = hash_val($text);
    <<"DATA";
static /*const*/ char static_string_${line}_data\[\] = $text;
static /*const*/ struct parrot_string_t static_string_${line} = {
  { /* pobj_t */
    {{
      static_string_${line}_data,
      sizeof(static_string_${line}_data)
    }},
    (PObj_constant_FLAG|PObj_external_FLAG)
    GC_DEBUG_VERSION
  },
  sizeof(static_string_${line}_data),
  static_string_${line}_data,
  sizeof(static_string_${line}_data) - 1,
  1,
  $h
};

DATA
  }
}

open IN, '<', $infile;

my $line = 0;
while (<IN>) {
  $line++;
  next if m/^\s*#/; # ignore preprocessor
  next unless s/.*\b_S\b//;

  my $str = extract_bracketed $_, '(")';

  print output_string (substr($str,1,-1), $line);
}

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
