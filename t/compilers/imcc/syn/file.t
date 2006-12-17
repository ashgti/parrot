#!perl
# Copyright (C) 2001-2005, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use File::Spec;
use Test::More;
use Parrot::Config;
use Parrot::Test tests => 13;

=head1 NAME

syn/file.t - test inclusion of files

=head1 SYNOPSIS

A test script which is supposed to be called by Test::Harness.

=cut

use 5;

# Do not assume that . is in $PATH
my $PARROT = ".$PConfig{slash}parrot$PConfig{exe}";
my $PERL5  = $PConfig{perl};

##############################
open FOO, ">", "temp.pasm" or die "Can't write temp.pasm\n";
print FOO <<'ENDF';
  .constant BAR 42
ENDF
close FOO;

pir_output_is( <<'CODE', <<'OUT', "include pasm" );
.sub test :main
    print "before\n"
    .include "temp.pasm"
    print .BAR
    print "\nafter\n"
    end
.end
CODE
before
42
after
OUT
unlink "temp.pasm";

##############################
open FOO, ">", "temp.pir" or die "Can't write temp.pir\n";
print FOO <<'ENDF';
  .const int BAR = 42
ENDF
close FOO;

pir_output_is( <<'CODE', <<'OUT', "include pir" );
.sub test :main
    print "before\n"
    .include "temp.pir"
    print BAR
    print "\nafter\n"
    end
.end
CODE
before
42
after
OUT
unlink "temp.pir";

##############################
open FOO, ">", "temp.inc" or die "Can't write temp.inc\n";
print FOO <<'ENDF';
  .const int BAR = 42
ENDF
close FOO;

pir_output_is( <<'CODE', <<'OUT', "include .inc" );
.sub test :main
    print "before\n"
    .include "temp.inc"
    print BAR
    print "\nafter\n"
    end
.end
CODE
before
42
after
OUT
unlink "temp.inc";

##############################
my $file = '_test.inc';
open F, ">", "$file";
print F <<'EOF';
.sub _foo		# sub foo(int a, int b)
   .param int a
   .param int b
   print "a = "
   print a
   print "\n"
   print "b = "
   print b
   print "\n"
   .local int pl
   .local int mi
   pl = a + b
   mi = a - b
   .return (pl, mi)
.end
EOF
close F;

pir_output_is( <<'CODE', <<'OUT', "subroutine in external file" );
.sub test :main
   .local int x
   x = 10
   .const int y = 20

   .local int r
   .local int s
   (r, s) = _foo(x,y)

   print "r = "
   print r
   print "\n"
   print "s = "
   print s
   print "\n"
   end
.end
.include "_test.inc"
CODE
a = 10
b = 20
r = 30
s = -10
OUT

# test load_bytecode branches and subs

# write sub2
open FOO, ">", "temp.pir" or die "Can't write temp.pir\n";
print FOO <<'ENDF';
.pcc_sub _sub2
    print "sub2\n"
    end
.end
ENDF
close FOO;

# compile it

system("$PARROT -o temp.pbc temp.pir");

pir_output_is( <<'CODE', <<'OUT', "call sub in external pbc" );
.pcc_sub _sub1
    print "sub1\n"
    load_bytecode "temp.pbc"
    print "loaded\n"
    $P0 = global "_sub2"
    .pcc_begin
    .pcc_call $P0
    ret:
    .pcc_end
    end
.end
CODE
sub1
loaded
sub2
OUT

# write sub2
open FOO, ">", "temp.pir" or die "Can't write temp.pir\n";
print FOO <<'ENDF';
.pcc_sub _sub2
    print "sub2\n"
   .pcc_begin_return
   .pcc_end_return
    end
.end
ENDF
close FOO;

# compile it

system("$PARROT -o temp.pbc temp.pir");

pir_output_is( <<'CODE', <<'OUT', "call sub in external pbc, return" );
.pcc_sub _sub1
    print "sub1\n"
    load_bytecode "temp.pbc"
    print "loaded\n"
    $P0 = global "_sub2"
    .pcc_begin
    .pcc_call $P0
    ret:
    .pcc_end
    print "back\n"
    end
.end
CODE
sub1
loaded
sub2
back
OUT

# write sub2
open FOO, ">", "temp.pir" or die "Can't write temp.pir\n";
print FOO <<'ENDF';
.pcc_sub _not_sub2
    print "not sub2\n"
    end
.end

.pcc_sub _sub2
    print "sub2\n"
    end
.end
ENDF
close FOO;

# compile it

system("$PARROT -o temp.pbc temp.pir");

pir_output_is( <<'CODE', <<'OUT', "call sub in external pbc with 2 subs" );
.pcc_sub _sub1
    print "sub1\n"
    load_bytecode "temp.pbc"
    print "loaded\n"
    $P0 = global "_sub2"
    .pcc_begin
    .pcc_call $P0
    ret:
    .pcc_end
    end
.end
CODE
sub1
loaded
sub2
OUT

# write sub2
open FOO, ">", "temp.pir" or die "Can't write temp.pir\n";
print FOO <<'ENDF';
.pcc_sub _sub2
    print "sub2\n"
   .pcc_begin_return
   .pcc_end_return
    end
.end
ENDF
close FOO;

# compile it

pir_output_is( <<'CODE', <<'OUT', "call sub in external pir, return" );
.pcc_sub _sub1
    print "sub1\n"
    load_bytecode "temp.pir"
    print "loaded\n"
    $P0 = global "_sub2"
    .pcc_begin
    .pcc_call $P0
    ret:
    .pcc_end
    print "back\n"
    end
.end
CODE
sub1
loaded
sub2
back
OUT

pir_output_is( <<'CODE', <<'OUT', "call internal sub like external" );
.pcc_sub _sub1
    print "sub1\n"
    $P0 = global "_sub2"
    .pcc_begin
    .pcc_call $P0
    ret:
    .pcc_end
    print "back\n"
    end
.end

.pcc_sub _sub2
    print "sub2\n"
   .pcc_begin_return
   .pcc_end_return
    end
.end
CODE
sub1
sub2
back
OUT

# write subs
open FOO, ">", "temp.pir" or die "Can't write temp.pir\n";
print FOO <<'ENDF';
.pcc_sub _sub1
    print "sub1\n"
    $P0 = global "_sub2"
    .pcc_begin
    .pcc_call $P0
    ret:
    .pcc_end
    print "back\n"
    end
.end

.pcc_sub _sub2
    print "sub2\n"
    .pcc_begin_return
    .pcc_end_return
    end
.end
ENDF
close FOO;

# compile it

system("$PARROT -o temp.pbc temp.pir");

use Test::More;
is( `$PARROT temp.pbc`, <<OUT, "call internal sub like external, precompiled" );
sub1
sub2
back
OUT

{

    # include a non-existent file and catch the error message
    my $err_msg;
    {
        open FOO, ">", "temp.pir" or die "Can't write temp.pir\n";
        print FOO << 'END_PIR';
# Including a non-existent file should produce an error
.include "non_existent.pir"
# An error should have been raised
.sub test :main
  # dummy, because a main function is expected
  end
.end
END_PIR
        close FOO;
        local *OLDERR;
        open OLDERR, ">&", "STDERR"   or die "Can't save STDERR\n";
        open STDERR, ">",  "temp.out" or die "Can't write temp.out\n";
        system "$PARROT temp.pir";
        open FOO, "<", "temp.out" or die "Can't read temp.out\n";
        { local $/; $err_msg = <FOO>; }
        close FOO;
        open STDERR, ">&", "OLDERR" or die "Can't restore STDERR\n";
        unlink "temp.out";
    }

    # read a non-existent file and catch the error message
    my $enoent_err_msg;
    {
        open FOO, "<", "non_existent.file";
        my $ENOENT = $! + 0;
        open FOO, ">", "temp.pir" or die "Can't write temp.pir\n";
        print FOO << "END_PIR";
.sub test \:main
  # run a OS command, and get the errmessge for the exit code
  .local string enoent_err_msg
  err enoent_err_msg, $ENOENT
  print enoent_err_msg
  end
.end
END_PIR
        close FOO;
        $enoent_err_msg = qx{$PARROT temp.pir}
    }

    $err_msg =~ s/\r//g if $^O =~ /^(MSWin32|msys)$/i;
    is( $err_msg, << "OUT", "including a non-existent file" );
error:imcc:$enoent_err_msg
\tin file 'temp.pir' line 2
OUT
    unlink "temp.pir";
}

SKIP:
{
    skip( "multiple loading not speced - failing", 1 );

    pir_output_is( <<'CODE', <<'OUT', "twice call sub in external pir, return" );
.pcc_sub _sub1
    print "sub1\n"
    load_bytecode "temp.pir"
    print "loaded\n"
    $P0 = global "_sub2"
    .pcc_begin
    .pcc_call $P0
    ret:
    .pcc_end
    print "back\n"
    print "sub1 again\n"
    load_bytecode "temp.pir"
    print "loaded again\n"
    $P0 = global "_sub2"
    .pcc_begin
    .pcc_call $P0
    ret_again:
    .pcc_end
    print "back again\n"
    end
.end
CODE
sub1
loaded
sub2
back
sub1 again
loaded again
sub2
back again
OUT
}

my @temp_files;
SKIP:
{
    my $temp_dir = File::Spec->tmpdir();
    my $td2 = File::Spec->catfile( $temp_dir, '.' );
    substr( $td2, -1, 1, '' );

    for my $file (qw( with_slash without_slash )) {
        push @temp_files, File::Spec->catfile( $temp_dir, "${file}.pir" );

        open( my $out_fh, '>', $temp_files[-1] )
            or skip( "Cannot write temporary file to $temp_files[-1]", 2 );

        print {$out_fh} <<"TEMP_PIR";
.sub $file
    print "$file() called!\\n"
.end
TEMP_PIR
    }

    pir_output_is( <<"CODE", <<'OUT', "load PIR from added paths, minding slash" );
  .include 'iglobals.pasm'

  .sub main :main
      .local pmc interp
      getinterp interp

      .local pmc lib_paths
      lib_paths = interp[.IGLOBALS_LIB_PATHS]

      # XXX - hard-coded magic constant (should be PARROT_LIB_PATH_LIBRARY)
      .local pmc include_paths
      include_paths = lib_paths[1]

      unshift include_paths, '$temp_dir'
      load_bytecode 'with_slash.pir'

      .local pmc dummy
      dummy = shift include_paths
      unshift include_paths, '$td2'
      load_bytecode 'without_slash.pir'

      with_slash()
      without_slash()
  .end
CODE
with_slash() called!
without_slash() called!
OUT
}

END {
    unlink $file;
    unlink "temp.pir";
    unlink "temp.pbc";
    unlink @temp_files;
}
