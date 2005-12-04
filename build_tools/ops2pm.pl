#! perl -w

# Copyright: 2001-2004 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

build_tools/ops2pm.pl - Generate Perl module from operation definitions

=head1 SYNOPSIS

 $ perl build_tools/ops2pm.pl [--help] [--no-lines] input.ops [input2.ops ...]
 $ perl build_tools/ops2pm.pl [--renum]  input.ops [input2.ops ...]

=head1 DESCRIPTION

Reads the ops files listed on the command line and outputs a
C<Parrot::OpLib::core> module containing information about the ops.

=head2 Options

=over 4

=item C<--help>

Print synopsis.

=item C<--no-lines>

Do not generate C<#line> directives in the generated C code.

=item C<--renum>

Renumber opcodes according to existing ops in ops/num and natural
order in the given ops files. See also F<tools/dev/ops_renum.mak>.

=back

=head2 WARNING

Generating a C<Parrot::OpLib::core> module for a set of ops files that
you do not later turn into C code (see F<build_tools/ops2c.pl>) with the
same op content and order is a recipe for disaster. But as long as you
just run these tools in the standard build process via C<make> there
shouldn't be a problem.

=head1 TODO

The original design of the ops processing code was intended to be
a read-only representation of what was in a particular ops file. It was
not originally intended that it was a mechanism for building a bigger
virtual ops file from multiple physical ops files.

This code does half of that job (the other half is getting them to
compile together instead of separately in a F<*_ops.c> file).

You can see evidence of this by the way this code reaches in to the
internal C<OPS> hash key to do its concatenation, and the way it
twiddles each op's C<CODE> hash key after that.

If the op and oplib Perl modules are going to be used for modifying
information read from ops files in addition to reading it, they should
be changed to make the above operations explicitly supported.

Otherwise, the Parrot build and interpreter start-up logic should be
modified so that it doesn't need to concatenate separate ops files.

=head1 SEE ALSO

=over 4

=item F<build_tools/ops2c.pl>.

=back

=cut

use strict;
use lib 'lib';

use Data::Dumper;
$Data::Dumper::Useqq  = 1;
#$Data::Dumper::Terse  = 1;
#$Data::Dumper::Indent = 0;
use Getopt::Long;

use Parrot::OpsFile;

#
# Look at the command line options
#

# TODO: Use Pod::Usage
my ( $nolines_flag, $help_flag, $renum_flag );
GetOptions( "no-lines"      => \$nolines_flag,
            "help"          => \$help_flag,
            "renum"          => \$renum_flag,
          );

sub Usage {
    print STDERR <<_EOF_;
usage: $0 [--help] [--no-lines] input.ops [input2.ops ...]
_EOF_
    exit;
}

Usage() if $help_flag;
Usage() unless @ARGV;


#
# Read in the first ops file.
#

my $package = "core";
my $moddir  = "lib/Parrot/OpLib";
my $module  = "$moddir/core.pm";

my $file = shift @ARGV;
die "$0: Could not find ops file '$file'!\n" unless -e $file;
my $ops = Parrot::OpsFile->new( [ $file ], $nolines_flag );
die "$0: Could not read ops file '$file'!\n" unless defined $ops;

#
# Copy the ops from the remaining .ops files to the object just created.
#

my %seen;

for $file (@ARGV) {
    if ($seen{$file}) {
	print STDERR "$0: Ops file '$file' mentioned more than once!\n";
	next;
    }
    $seen{$file} = 1;

    die "$0: Could not find ops file '$file'!\n" unless -e $file;
    my $temp_ops = Parrot::OpsFile->new( [ $file ], $nolines_flag );
    die "$0: Could not read ops file '$file'!\n" unless defined $temp_ops;

    die "OPS invalid for $file" unless ref $temp_ops->{OPS};

    my $experimental = $file =~ /experimental/;

    # mark experimental ops
    if ($experimental) {
	for $_ (@{$temp_ops->{OPS}}) {
	    $_->{experimental} = 1;
	}
    }

    push @{$ops->{OPS}}, @{$temp_ops->{OPS}};
    $ops->{PREAMBLE} .= "\n" . $temp_ops->{PREAMBLE};
}


# Renumber ops/num based on old ops.num and *.ops
if ($renum_flag) {
    renum_op_map_file($ops);
    exit 0;
}    
# else check strictly against ops.num and renumber
else {
    load_op_map_file();

    my $cur_code = 0;
    for(@{$ops->{OPS}}) {
        $_->{CODE} = find_op_number($_->full_name, $_->{experimental});
    }

    @{$ops->{OPS}} = sort { $a->{CODE} <=> $b->{CODE} } (@{$ops->{OPS}} );
}

# create opsfile with valid ops from ops.num
# or from experimental

my $real_ops = Parrot::OpsFile->new( [ ], $nolines_flag );
$real_ops->{PREAMBLE} = $ops->{PREAMBLE};
$real_ops->version($ops->version);

# verify opcode numbers
my $seq = 0;
for(@{$ops->{OPS}}) {
    next if ($_->{CODE} < 0);	# skip
    my $opname = $_->full_name;
    my $n = $ParrotOps::optable{$opname} ;
    if ($n != $_->{CODE}) {
	die "op number mismatch ops.num: $n core.ops: $_->{CODE}";
    }
    if ($seq != $_->{CODE}) {
	die "op sequence mismatch ops.num: $seq core.ops: $_->{CODE}";
    }
    push @{$real_ops->{OPS}}, $_;
    ++$seq;
}
 
# Open the output file:
if (! -d $moddir) {
    mkdir($moddir, 0755) or die "$0: Could not mkdir $moddir: $!!\n";
}
open MODULE, ">$module"
  or die "$0: Could not open module file '$module' for writing: $!!\n";


#
# Print the preamble for the MODULE file:
#

my $version = $real_ops->version();

# Hide the pod.

(my $pod = <<"END_POD") =~ s/^    //osmg;
    =head1 NAME

    Parrot::OpLib::$package - Parrot Op Info

    =head1 DESCRIPTION

    This is an autogenerated file, created by F<$0>.

    It contains Parrot version info, a preamble for inclusion in C code,
    and an array of C<Parrot::Op> instances representing the Parrot ops.

    =cut
END_POD

my $preamble = <<END_C;
#! perl -w
#
# !!!!!!!   DO NOT EDIT THIS FILE   !!!!!!!
#
# This file is generated automatically from '$file'.
# Any changes made here will be lost!
#

$pod

use strict;

package Parrot::OpLib::$package;

use vars qw(\$VERSION \$ops \$preamble);

\$VERSION = "$version";

END_C

print MODULE $preamble;
print MODULE Data::Dumper->Dump([ $real_ops->preamble, [$real_ops->ops ]],
          [ qw($preamble $ops) ]);

print MODULE <<END_C;

1;
END_C

close MODULE;

# finally create an include file with opcode number

my $inc_f = "include/parrot/oplib/ops.h";
my $inc_dir = "include/parrot/oplib";
if (! -d $inc_dir) {
    mkdir($inc_dir, 0755) or die "ops2pm.pl: Could not mkdir $inc_dir: $!\n";
}
open OUT, ">$inc_f" or die "Can't write $inc_f: $!";

print OUT <<END_C;
/* !!!!!!!   DO NOT EDIT THIS FILE   !!!!!!!
 *
 * This file is generated automatically from '$file'
 * by $0.
 *
 * Any changes made here will be lost!
 */

#ifndef PARROT_OPS_H_GUARD
#define PARROT_OPS_H_GUARD

typedef enum {
END_C

for(@{$real_ops->{OPS}}) {
    my $opname = $_->full_name;
    my $n = $_->{CODE};
    my $comma = $n < @{$real_ops->{OPS}} -1 ? "," : "";
    $opname = "PARROT_OP_$opname$comma";

    printf OUT "\t%-30s\t/* %4d */\n", $opname, $n;
}

print OUT <<END_C;
} parrot_opcode_enums;

#endif

END_C
close OUT;


# if opcode numer is not in ops.num create one but
# only for *experimental* ops
# other opcodes are just skipped
sub find_op_number {
    my ($opname, $experimental) = @_;
    if (exists $ParrotOps::optable{$opname}) {
	return $ParrotOps::optable{$opname};
    } elsif (!$experimental) {
	warn "$opname\t\t* skipped not in ops.num\n"
	    if -e "DEVELOPING" and exists $ENV{PARROT_OPS_DEBUG};
	return -1;
    } else {
	my $n = $ParrotOps::optable{$opname} = ++$ParrotOps::max_op_num;
	warn "$opname\t$n\tnot mentioned in ops.num\n" if -e "DEVELOPING";
	return $n;
    }
}

sub renum_op_map_file {
    my $ops = shift;
    my $file = shift;

    if (!defined $file) {
        $file = "ops/ops.num";
    }
    my ($name, $number, @lines, %seen, %fixed, $fix);
    local *OP;
    $fix = 1;
    open OP, "< $file" or die "Can't open $file, error $!";
    while (<OP>) {
        push @lines, $_ if $fix;
        chomp;
        $fix = 0 if /^###DYNAMIC###/;
        s/#.*$//;
        s/\s*$//;
        s/^\s*//;
        next unless $_;
        ($name, $number) = split(/\s+/, $_);
        $seen{$name} = $number;
        $fixed{$name} = $number if ($fix);
    }
    close OP;
    open OP, "> $file" or die "Can't open $file, error $!";
    print OP @lines;
    my ($n);
    #
    # we can't use all autogenerated ops from oplib/core
    # there are unwanted permutations like 'add_i_ic_ic
    # which aren't opcodes but calced at compile-time
    #
    
    for(@{$ops->{OPS}}) {
        if (defined $fixed{$_->full_name}) {
            $n = $fixed{$_->full_name};
        }
        elsif ($seen{$_->full_name}) {
            printf OP "%-31s%4d\n", $_->full_name, ++$n;
        }
    }
    close OP;
}
  
sub load_op_map_file {
  my $file = shift;

  if (!defined $file) {
    $file = "ops/ops.num";
  }

  my ($name, $number, $prev);

  if (!defined $ParrotOps::max_op_num) {
    $ParrotOps::max_op_num = 0;
  }

  local *OP;
  open OP, "< $file" or die "Can't open $file, error $!";
  $prev = -1;

  while (<OP>) {
    chomp;
    s/#.*$//;
    s/\s*$//;
    s/^\s*//;
    next unless $_;
    ($name, $number) = split(/\s+/, $_);
    if ($prev + 1 != $number) {
	    die "hole in ops.num opcode # $number";
    }
    if (exists $ParrotOps::optable{$name}) {
	    die "duplicate opcode $name and $number";
    }
    $prev = $number;
    $ParrotOps::optable{$name} = $number;
    if ($number > $ParrotOps::max_op_num) {
      $ParrotOps::max_op_num = $number;
    }
  }
  close OP;
  return;
}

exit 0;

# vim: expandtab shiftwidth=4:
