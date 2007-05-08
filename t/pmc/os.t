#! perl
# Copyright (C) 2001-2006, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 15;
use Parrot::Config;
use Cwd;
use File::Spec;

my $MSWin32 = $^O =~ m!MSWin32!;
my $cygwin  = $^O =~ m!cygwin!;
my $MSVC = grep { $PConfig{cc} eq $_ } (qw(cl cl.exe));

=head1 NAME

t/pmc/os.t - Files and Dirs

=head1 SYNOPSIS

    % prove t/pmc/os.t

=head1 DESCRIPTION

Tests the C<OS> PMC.

=cut

END {

    # Clean up environment on exit
    rmdir "xpto"  if -d "xpto";
    unlink "xpto" if -f "xpto";
}

# test 'cwd'
my $cwd = File::Spec->canonpath(getcwd);
pir_output_is( <<'CODE', <<"OUT", 'Test cwd' );
.sub main :main
        $P1 = new .OS
        $S1 = $P1."cwd"()
        print $S1
        print "\n"
        end
.end
CODE
$cwd
OUT

#  TEST chdir
chdir "src";
my $upcwd = File::Spec->canonpath(getcwd);
chdir '..';

pir_output_is( <<'CODE', <<"OUT", 'Test chdir' );
.sub main :main
        $P1 = new .OS

        $S1 = "src"
        $P1."chdir"($S1)

        $S1 = $P1."cwd"()
        print $S1
        print "\n"

        $S1 = ".."
        $P1."chdir"($S1)

        $S1 = $P1."cwd"()
        print $S1
        print "\n"

        end
.end
CODE
$upcwd
$cwd
OUT

# Test mkdir

my $xpto = $upcwd;
$xpto =~ s/src([\/\\]?)$/xpto$1/;

pir_output_is( <<'CODE', <<"OUT", 'Test mkdir' );
.sub main :main
        $P1 = new .OS

        $S1 = "xpto"
        $I1 = 0o555
        $P1."mkdir"($S1,$I1)
        $P1."chdir"($S1)

        $S1 = $P1."cwd"()
        print $S1
        print "\n"

        $S1 = ".."
        $P1."chdir"($S1)

        $S1 = $P1."cwd"()
        print $S1
        print "\n"

        end
.end
CODE
$xpto
$cwd
OUT

# Test remove on a directory
mkdir "xpto" unless -d "xpto";

pir_output_is( <<'CODE', <<'OUT', 'Test rm call in a directory' );
.sub main :main
        $P1 = new .OS

        $S1 = "xpto"
        $P1."rm"($S1)

        print "ok\n"

        end
.end
CODE
ok
OUT

ok( !-d $xpto, "Test that rm removed the directory" );
rmdir $xpto if -d $xpto;    # this way next test doesn't fail if this one does

# test stat

open my $X, '>', "xpto";
print $X "xpto";
close $X;

my $stat;

if ( $cygwin || $MSWin32 ) {

    # Skip inode number
    my @s = stat('xpto');
    $stat = join( "\n", $s[0], @s[ 2 .. 10 ] ) . "\n";
    pir_output_is( <<'CODE', $stat, 'Test OS.stat' );
.sub main :main
        $P1 = new .OS
        $S1 = "xpto"
        $P2 = $P1."stat"($S1)

        $I1 = 0
loop:
        $S1 = $P2[$I1]
        print $S1
        print "\n"
        $I1 += 1
        if $I1 == 1 goto inc
        if $I1 == 11 goto done
        goto loop
inc:
        $I1 += 1
        goto loop

done:
        end
.end
CODE
}
else {
    $stat = join( "\n", stat("xpto") ) . "\n";
    pir_output_is( <<'CODE', $stat, 'Test OS.stat' );
.sub main :main
        $P1 = new .OS
        $S1 = "xpto"
        $P2 = $P1."stat"($S1)

        $I1 = 0
loop:
        $S1 = $P2[$I1]
        print $S1
        print "\n"
        $I1 += 1
        if $I1 == 13 goto done
        goto loop
done:
        end
.end
CODE

}

# test readdir
SKIP: {
    skip 'not implemented on windows yet', 1 if ( $MSWin32 && $MSVC );

    opendir IN, '.';
    my @entries = readdir IN;
    closedir IN;
    my $entries = join( ' ', @entries ) . "\n";
    pir_output_is( <<'CODE', $entries, 'Test OS.readdir' );
.sub main :main
    $P1 = new .OS
    $P2 = $P1.readdir('.')

    $S0 = join ' ', $P2
    print $S0
    print "\n"
.end
CODE
}

# test rename
SKIP: {
    open FILE, ">", "____some_test_file";
    close FILE;
    pir_output_is( <<'CODE', <<"OUT", 'Test OS.rename' );
.sub main :main
    $P1 = new .OS
    $P1.rename('____some_test_file', '___some_other_file')
    $I0 = stat '___some_other_file', 0
    print $I0
    print "\n"
    $P1.rm('___some_other_file')
.end
CODE
1
OUT
}

# test lstat

my $lstat;

SKIP: {
    skip 'lstat not available on Win 32 yet', 1 if $MSWin32;

    if ($cygwin) {

        # Skip inode number
        my @s = stat('xpto');
        $stat = join( "\n", $s[0], @s[ 2 .. 12 ] ) . "\n";
        pir_output_is( <<'CODE', $stat, "Test OS.lstat" );
.sub main :main
        $P1 = new .OS
        $S1 = "xpto"
        $P2 = $P1."lstat"($S1)

        $I1 = 0
loop:
        $S1 = $P2[$I1]
        print $S1
        print "\n"
        $I1 += 1
        if $I1 == 1 goto inc
        if $I1 == 13 goto done
        goto loop
inc:
        $I1 += 1
        goto loop

done:
        end
.end
CODE
    }
    else {
        $lstat = join( "\n", lstat("xpto") ) . "\n";
        pir_output_is( <<'CODE', $lstat, "Test OS.lstat" );
.sub main :main
        $P1 = new .OS
        $S1 = "xpto"
        $P2 = $P1."lstat"($S1)

        $I1 = 0
loop:
        $S1 = $P2[$I1]
        print $S1
        print "\n"
        $I1 += 1
        if $I1 == 13 goto done
        goto loop
done:
        end
.end
CODE
    }
}

# Test remove on a file
pir_output_is( <<'CODE', <<"OUT", "Test rm call in a file" );
.sub main :main
        $P1 = new .OS

        $S1 = "xpto"
        $P1."rm"($S1)

        print "ok\n"

        end
.end
CODE
ok
OUT

ok( !-f $xpto, "Test that rm removed file" );
rmdir $xpto if -f $xpto;    # this way next test doesn't fail if this one does

# Test symlink
SKIP: {
    skip "Symlinks not available under Windows", 2 if $MSWin32;

    pir_output_is( <<'CODE', <<"OUT", "Test symlink" );
.sub main :main
        $P1 = new .OS

        $S1 = "xpto"
        $S2 = "MANIFEST"
        $P1."symlink"($S2, $S1)

        print "ok\n"

        end
.end
CODE
ok
OUT

    ok( -l "xpto", "symlink was really created" );
    unlink "xpto" if -f "xpto";
}

# Test link
SKIP: {
    skip "Parrot link not implemented for Windows, yet", 2 if $MSWin32;

    pir_output_is( <<'CODE', <<"OUT", "Test link" );
.sub main :main
        $P1 = new .OS

        $S1 = "xpto"
        $S2 = "MANIFEST"
        $P1."link"($S2, $S1)

        print "ok\n"

        end
.end
CODE
ok
OUT

    my $nl = [ stat("MANIFEST") ]->[3];
    ok( $nl > 1, "hard link was really created" );
    unlink "xpto" if -f "xpto";
}

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
