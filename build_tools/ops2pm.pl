#! perl -w
# Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

build_tools/ops2pm.pl - Generate Perl module from operation definitions

=head1 SYNOPSIS

    % perl build_tools/ops2pm.pl ops/core.ops ops/bit.ops ...

=head1 DESCRIPTION

Reads the ops files listed on the command line and outputs a
C<Parrot::OpLib::core> module containing information about the ops.

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
use Parrot::OpsFile;

use Data::Dumper;
$Data::Dumper::Useqq  = 1;
#$Data::Dumper::Terse  = 1;
#$Data::Dumper::Indent = 0;

my $moddir  = "lib/Parrot/OpLib";

sub Usage {
    print STDERR <<_EOF_;
usage: $0 input.ops [input2.ops ...]
_EOF_
    exit;
}

Usage() unless @ARGV;


#
# Read in the first ops file.
#

my $file = shift @ARGV;
my $package = "core";
my $module  = "lib/Parrot/OpLib/core.pm";

die "$0: Could not find ops file '$file'!\n" unless -e $file;
my $ops = new Parrot::OpsFile $file;
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
    my $temp_ops = new Parrot::OpsFile $file;
    die "$0: Could not read ops file '$file'!\n" unless defined $temp_ops;

    die "OPS invalid for $file" unless ref $temp_ops->{OPS};
    push @{$ops->{OPS}}, @{$temp_ops->{OPS}};
    $ops->{PREAMBLE} .= "\n" . $temp_ops->{PREAMBLE};
}


# Renumber the ops based on ops.num
#

&load_op_map_file;

my $cur_code = 0;
for(@{$ops->{OPS}}) {
    $_->{CODE} = find_op_number($_->full_name);
}

my @sorted = sort { $a->{CODE} <=> $b->{CODE} } (@{$ops->{OPS}} );
@{$ops->{OPS}} = @sorted;

# verify opcode numbers
my $seq = 0;
for(@{$ops->{OPS}}) {
	my $opname = $_->full_name;
	my $n = $ParrotOps::optable{$opname} ;
	if ($n != $_->{CODE}) {
		die "op number mismatch ops.num: $n core.ops: $_->{CODE}";
	}
	if ($seq != $_->{CODE}) {
		die "op sequence mismatch ops.num: $seq core.ops: $_->{CODE}";
	}
	++$seq;
}
#
# Open the output file:
#

if (! -d $moddir) {
    mkdir($moddir, 0755) or die "$0: Could not mkdir $moddir: $!!\n";
}
open MODULE, ">$module"
  or die "$0: Could not open module file '$module' for writing: $!!\n";


#
# Print the preamble for the MODULE file:
#

my $version = $ops->version;

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
print MODULE Data::Dumper->Dump([ $ops->preamble, [$ops->ops ]],
          [ qw($preamble $ops) ]);

print MODULE <<END_C;

1;
END_C

sub find_op_number {
  my $opname = shift;
  if (exists $ParrotOps::optable{$opname}) {
    return $ParrotOps::optable{$opname};
  } else {
    my $n = $ParrotOps::optable{$opname} = ++$ParrotOps::max_op_num;
    warn "$opname\t$n\tnot mentioned in ops.num\n" if -e "DEVELOPING";
    return $n;
  }
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
  warn "last opcode in ops.num: $number\n";
  return;
}

exit 0;

