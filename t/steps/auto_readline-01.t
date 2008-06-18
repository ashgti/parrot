#! perl
# Copyright (C) 2007, The Perl Foundation.
# $Id$
# auto_readline-01.t

use strict;
use warnings;
use Test::More tests => 72;
use Carp;
use Cwd;
use File::Spec;
use File::Temp qw( tempdir );
use lib qw( lib );
use_ok('config::init::defaults');
use_ok('config::auto::readline');
use Parrot::Configure;
use Parrot::Configure::Options qw( process_options );
use Parrot::Configure::Test qw( test_step_thru_runstep);
use IO::CaptureOutput qw | capture |;

my $args = process_options(
    {
        argv => [ ],
        mode => q{configure},
    }
);

my $conf = Parrot::Configure->new;

test_step_thru_runstep( $conf, q{init::defaults}, $args );

my $pkg = q{auto::readline};

$conf->add_steps($pkg);
$conf->options->set( %{$args} );

my ( $task, $step_name, $step);
$task        = $conf->steps->[-1];
$step_name   = $task->step;

$step = $step_name->new();
ok( defined $step, "$step_name constructor returned defined value" );
isa_ok( $step, $step_name );
ok( $step->description(), "$step_name has description" );

# Mock values for OS and C-compiler
my ($osname, $cc);
$osname = 'mswin32';
$cc = 'gcc';
ok($step->_add_to_libs( {
    conf            => $conf,
    osname          => $osname,
    cc              => $cc,
    win32_nongcc    => 'readline.lib',
    default         => '-lreadline',
} ),
    "_add_to_libs() returned true value");
like($conf->data->get( 'libs' ), qr/-lreadline/,
    "'libs' modified as expected");

$osname = 'mswin32';
$cc = 'cc';
ok($step->_add_to_libs( {
    conf            => $conf,
    osname          => $osname,
    cc              => $cc,
    win32_nongcc    => 'readline.lib',
    default         => '-lreadline',
} ),
    "_add_to_libs() returned true value");
like($conf->data->get( 'libs' ), qr/readline\.lib/,
    "'libs' modified as expected");

$osname = 'foobar';
$cc = undef;
ok($step->_add_to_libs( {
    conf            => $conf,
    osname          => $osname,
    cc              => $cc,
    win32_nongcc    => 'readline.lib',
    default         => '-lreadline',
} ),
    "_add_to_libs() returned true value");
like($conf->data->get( 'libs' ), qr/-lreadline/,
    "'libs' modified as expected");

$osname = 'foobar';
$cc = undef;
eval {
    $step->_add_to_libs( [
        conf            => $conf,
        osname          => $osname,
        cc              => $cc,
        win32_nongcc    => 'readline.lib',
        default         => '-lreadline',
    ] );
};
like($@, qr/_add_to_libs\(\) takes hashref/,
    "Bad argument to _add_to_libs correctly detected");

my ($flagsbefore, $flagsafter);

$osname = 'foobar';
$flagsbefore = $conf->data->get( 'linkflags' );
ok($step->_handle_darwin_for_fink($conf, $osname, 'readline/readline.h'),
    "handle_darwin_for_fink() returned true value");
$flagsafter = $conf->data->get( 'linkflags' );
is($flagsbefore, $flagsafter, "No change in linkflags, as expected");

my $cwd = cwd();
{
    my $tdir1 = tempdir( CLEANUP => 1 );
    ok(chdir $tdir1, "Able to change to temporary directory");
    ok( (mkdir 'lib'), "Able to make lib directory");
    ok( (mkdir 'include'), "Able to make include directory");
    my $libdir = File::Spec->catdir( $tdir1, 'lib' );
    my $includedir = File::Spec->catdir( $tdir1, 'include' );
    $conf->data->set('fink_lib_dir' => $libdir);
    $conf->data->set('fink_include_dir' => $includedir);
    $osname = 'darwin';
    $flagsbefore = $conf->data->get( 'linkflags' );
    ok($step->_handle_darwin_for_fink($conf, $osname, 'readline/readline.h'),
        "handle_darwin_for_fink() returned true value");
    $flagsafter = $conf->data->get( 'linkflags' );
    is($flagsbefore, $flagsafter,
        "No change in linkflags, as expected, where Fink lib and include directories exist but readline/readline.h does not");

    ok(chdir $cwd, "Able to change back to original directory after testing");
}

{
    my $tdir2 = tempdir( CLEANUP => 1 );
    ok(chdir $tdir2, "Able to change to temporary directory");
    ok( (mkdir 'lib'), "Able to make lib directory");
    ok( (mkdir 'include'), "Able to make include directory");
    ok( (mkdir 'include/readline'), "Able to make include/readline directory");
    my $libdir = File::Spec->catdir( $tdir2, 'lib' );
    my $includedir = File::Spec->catdir( $tdir2, 'include' );
    $conf->data->set('fink_lib_dir' => $libdir);
    $conf->data->set('fink_include_dir' => $includedir);
    my $foo = File::Spec->catfile( $includedir, 'readline', 'readline.h' );
    open my $FH, ">", $foo or croak "Could not open for writing";
    print $FH "Hello world\n";
    close $FH or croak "Could not close after writing";

    $osname = 'darwin';
    $flagsbefore = $conf->data->get( 'linkflags' );
    ok($step->_handle_darwin_for_fink($conf, $osname, 'readline/readline.h'),
        "handle_darwin_for_fink() returned true value");
    $flagsafter = $conf->data->get( 'linkflags' );
    isnt($flagsbefore, $flagsafter, "Change in linkflags, as expected");
    like($conf->data->get( 'linkflags' ), qr/-L\Q$libdir\E/,
        "'linkflags' modified as expected, in case where Fink lib and include dirs exist and readline/readline.h exists");
    $conf->data->set( linkflags => $flagsbefore ); #reset for next test

    ok(chdir $cwd, "Able to change back to original directory after testing");
}

{
    my $tdir3 = tempdir( CLEANUP => 1 );
    ok(chdir $tdir3, "Able to change to temporary directory");
    ok( (mkdir 'lib'), "Able to make lib directory");
    ok( (mkdir 'include'), "Able to make include directory");
    ok( (mkdir 'include/readline'), "Able to make include/readline directory");
    my $libdir = File::Spec->catdir( $tdir3, 'lib' );
    my $includedir = File::Spec->catdir( $tdir3, 'include' );
    $conf->data->set('fink_lib_dir' => $libdir);
    $conf->data->set('fink_include_dir' => undef);

    $osname = 'darwin';
    $flagsbefore = $conf->data->get( 'linkflags' );
    ok($step->_handle_darwin_for_fink($conf, $osname, 'readline/readline.h'),
        "handle_darwin_for_fink() returned true value");
    $flagsafter = $conf->data->get( 'linkflags' );
    is($flagsbefore, $flagsafter,
        "No change in linkflags, as expected, where Fink include directory does not exist");

    ok(chdir $cwd, "Able to change back to original directory after testing");
}

{
    my $tdir4 = tempdir( CLEANUP => 1 );
    ok(chdir $tdir4, "Able to change to temporary directory");
    ok( (mkdir 'lib'), "Able to make lib directory");
    ok( (mkdir 'include'), "Able to make include directory");
    ok( (mkdir 'include/readline'), "Able to make include/readline directory");
    my $libdir = File::Spec->catdir( $tdir4, 'lib' );
    my $includedir = File::Spec->catdir( $tdir4, 'include' );
    $conf->data->set('fink_lib_dir' => undef );
    $conf->data->set('fink_include_dir' => $includedir );

    $osname = 'darwin';
    $flagsbefore = $conf->data->get( 'linkflags' );
    ok($step->_handle_darwin_for_fink($conf, $osname, 'readline/readline.h'),
        "handle_darwin_for_fink() returned true value");
    $flagsafter = $conf->data->get( 'linkflags' );
    is($flagsbefore, $flagsafter,
        "No change in linkflags, as expected, where Fink lib directory does not exist");

    ok(chdir $cwd, "Able to change back to original directory after testing");
}

$osname = 'foobar';
$flagsbefore = $conf->data->get( 'linkflags' );
ok($step->_handle_darwin_for_macports($conf, $osname, 'readline/readline.h'),
    "handle_darwin_for_macports() returned true value");
$flagsafter = $conf->data->get( 'linkflags' );
is($flagsbefore, $flagsafter, "No change in linkflags, as expected");
# Get ready for the next test
$conf->data->set( linkflags => $flagsbefore );

$cwd = cwd();
{
    my $xtdir1 = File::Spec->canonpath( tempdir( CLEANUP => 1 ) );
    ok(chdir $xtdir1, "Able to change to temporary directory");
    ok( (mkdir 'lib'), "Able to make lib directory");
    ok( (mkdir 'include'), "Able to make include directory");
    my $libdir = File::Spec->catdir( $xtdir1, 'lib' );
    my $includedir = File::Spec->catdir( $xtdir1, 'include' );
    $conf->data->set( ports_base_dir => $xtdir1 );
    $conf->data->set( ports_lib_dir => $libdir );
    $conf->data->set( ports_include_dir => $includedir );
    ok( (mkdir 'include/readline'), "Able to make include/readline directory");
    $osname = 'darwin';
    $flagsbefore = $conf->data->get( 'linkflags' );
    ok($step->_handle_darwin_for_macports($conf, $osname, 'readline/readline.h'),
        "handle_darwin_for_macports() returned true value");
    $flagsafter = $conf->data->get( 'linkflags' );
    is($flagsbefore, $flagsafter,
        "No change in linkflags, as expected, where macports lib and include directories exist but readline/readline.h does not");

    chdir $cwd or croak "Unable to change back to original directory";
}

$cwd = cwd();
{
    my $xtdir2 = File::Spec->canonpath( tempdir( CLEANUP => 1 ) );
    ok(chdir $xtdir2, "Able to change to temporary directory");
    ok( (mkdir 'lib'), "Able to make lib directory");
    ok( (mkdir 'include'), "Able to make include directory");
    my $libdir = File::Spec->catdir( $xtdir2, 'lib' );
    my $includedir = File::Spec->catdir( $xtdir2, 'include' );
    $conf->data->set( ports_base_dir => $xtdir2 );
    $conf->data->set( ports_lib_dir => $libdir );
    $conf->data->set( ports_include_dir => $includedir );
    ok( (mkdir 'include/readline'), "Able to make include/readline directory");
    my $foo = File::Spec->catfile( $includedir, 'readline', 'readline.h' );
    open my $FH, ">", $foo or croak "Could not open for writing";
    print $FH "Hello world\n";
    close $FH or croak "Could not close after writing";

    $osname = 'darwin';
    $flagsbefore = $conf->data->get( 'linkflags' );
    ok($step->_handle_darwin_for_macports($conf, $osname, 'readline/readline.h'),
        "handle_darwin_for_macports() returned true value");
    $flagsafter = $conf->data->get( 'linkflags' );
    isnt($flagsbefore, $flagsafter, "Change in linkflags, as expected");
    like($conf->data->get( 'linkflags' ), qr/-L\Q$libdir\E/,
        "'linkflags' modified as expected, in case where macports lib and include dirs exist and readline/readline.h exists");
    $conf->data->set( linkflags => $flagsbefore );

    chdir $cwd or croak "Unable to change back to original directory";
}


$cwd = cwd();
{
    my $xtdir3 = File::Spec->canonpath( tempdir( CLEANUP => 1 ) );
    ok(chdir $xtdir3, "Able to change to temporary directory");
    ok( (mkdir 'lib'), "Able to make lib directory");
    ok( (mkdir 'include'), "Able to make include directory");
    my $libdir = File::Spec->catdir( $xtdir3, 'lib' );
    my $includedir = File::Spec->catdir( $xtdir3, 'include' );
    $conf->data->set( ports_base_dir => $xtdir3 );
    $conf->data->set( ports_lib_dir => $libdir );
    $conf->data->set( ports_include_dir => undef );

    $osname = 'darwin';
    $flagsbefore = $conf->data->get( 'linkflags' );
    ok($step->_handle_darwin_for_macports($conf, $osname, 'readline/readline.h'),
        "handle_darwin_for_macports() returned true value");
    $flagsafter = $conf->data->get( 'linkflags' );
    is($flagsbefore, $flagsafter,
        "No change in linkflags, as expected, where Macports include directory does not exist");

    chdir $cwd or croak "Unable to change back to original directory";
}

$cwd = cwd();
{
    my $xtdir4 = File::Spec->canonpath( tempdir( CLEANUP => 1 ) );
    ok(chdir $xtdir4, "Able to change to temporary directory");
    ok( (mkdir 'lib'), "Able to make lib directory");
    ok( (mkdir 'include'), "Able to make include directory");
    my $libdir = File::Spec->catdir( $xtdir4, 'lib' );
    my $includedir = File::Spec->catdir( $xtdir4, 'include' );
    $conf->data->set( ports_base_dir => $xtdir4 );
    $conf->data->set( ports_lib_dir => undef );
    $conf->data->set( ports_include_dir => $includedir );

    $osname = 'darwin';
    $flagsbefore = $conf->data->get( 'linkflags' );
    ok($step->_handle_darwin_for_macports($conf, $osname, 'readline/readline.h'),
        "handle_darwin_for_macports() returned true value");
    $flagsafter = $conf->data->get( 'linkflags' );
    is($flagsbefore, $flagsafter,
        "No change in linkflags, as expected, where Macports lib directory does not exist");

    chdir $cwd or croak "Unable to change back to original directory";
}

pass("Completed all tests in $0");

################### DOCUMENTATION ###################

=head1 NAME

auto_readline-01.t - test config::auto::readline

=head1 SYNOPSIS

    % prove t/steps/auto_readline-01.t

=head1 DESCRIPTION

The files in this directory test functionality used by F<Configure.pl>.

The tests in this file test subroutines exported by config::auto::readline.

=head1 AUTHOR

James E Keenan

=head1 SEE ALSO

config::auto::readline, F<Configure.pl>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
