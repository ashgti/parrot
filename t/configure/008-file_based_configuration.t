#! perl
# Copyright (C) 2007, Parrot Foundation.
# $Id$
# 008-file_based_configuration.t

use strict;
use warnings;

BEGIN {
    use FindBin qw($Bin);
    use Cwd qw(cwd realpath);
    our $topdir = realpath($Bin) . "/../..";
    unshift @INC, qq{$topdir/lib};
}
use Test::More tests => 31;
use Carp;
use Parrot::Configure::Options qw| process_options |;

{
    my $configfile = q{examples/config/file/configwithfatalstep};
    my ($args, $steps_list_ref) = _test_good_config_file($configfile);

    ok(! defined $args->{maintainer}, 
        "Configuring from testfoobar: 'maintainer' not defined, as expected");
    is($args->{'verbose-step'}, 'init::hints',
        "Configuring from testfoobar: 'init::hints' is verbose step");
    is($args->{'fatal-step'}, 'init::hints',
        "Configuring from testfoobar: 'init::hints' is fatal step");
    ok($args->{nomanicheck}, 
        "Configuring from testfoobar: will omit check of MANIFEST");
    is($args->{file}, $configfile,
        "Configuring from testfoobar: config file correctly stored");
    ok($args->{debugging}, 
        "Configuring from testfoobar: debugging turned on");
    my %steps_seen = map {$_ => 1} @{ $steps_list_ref };
    ok(exists $steps_seen{'init::manifest'},
        "Configuring from testfoobar: init::manifest is in list even though it will be skipped");
    ok(! exists $steps_seen{'auto::perldoc'},
        "Configuring from testfoobar: auto::perldoc not in list");
}

{
    my $configfile = q{examples/config/file/configcompiler};
    my ($args, $steps_list_ref) = _test_good_config_file($configfile);
    
    my $c_compiler = '/usr/bin/gcc';
    my $cplusplus_compiler = '/usr/bin/g++';
    ok(! defined $args->{maintainer}, 
        "Configuring from yourfoobar: 'maintainer' not defined as expected");
    is($args->{'verbose-step'}, 'init::hints',
        "Configuring from yourfoobar: 'init::hints' is verbose step");
    ok($args->{nomanicheck}, 
        "Configuring from yourfoobar: will omit check of MANIFEST");
    is($args->{file}, $configfile,
        "Configuring from yourfoobar: config file correctly stored");
    ok($args->{debugging}, 
        "Configuring from yourfoobar: debugging turned on");
    is($args->{cc}, $c_compiler,
        "Configuring from yourfoobar: C compiler is $c_compiler");
    is($args->{cxx}, $cplusplus_compiler,
        "Configuring from yourfoobar: C++ compiler is $cplusplus_compiler");
    is($args->{link}, $cplusplus_compiler,
        "Configuring from yourfoobar: linker is $cplusplus_compiler");
    is($args->{ld}, $cplusplus_compiler,
        "Configuring from yourfoobar: shared library loader is $cplusplus_compiler");

    my %steps_seen = map {$_ => 1} @{ $steps_list_ref };

    ok(exists $steps_seen{'init::manifest'},
        "Configuring from yourfoobar: init::manifest is in list even though it will be skipped");
    ok(! exists $steps_seen{'auto::perldoc'},
        "Configuring from yourfoobar: auto::perldoc not in list");
}

{
    my $configfile = q{t/configure/testlib/verbosefoobar};
    my ($args, $steps_list_ref) = _test_good_config_file($configfile);

    ok(! defined $args->{maintainer}, 
        "Configuring from verbosefoobar: 'maintainer' not defined as expected");
    ok($args->{nomanicheck}, 
        "Configuring from verbosefoobar: will omit check of MANIFEST");
    is($args->{file}, $configfile,
        "Configuring from verbosefoobar: config file correctly stored");
    ok($args->{debugging}, 
        "Configuring from verbosefoobar: debugging turned on");
    is($args->{verbose}, 1,
        "Configuring from verbosefoobar: verbose output is on");

    my %steps_seen = map {$_ => 1} @{ $steps_list_ref };

    ok(exists $steps_seen{'init::manifest'},
        "Configuring from verbosefoobar: init::manifest is in list even though it will be skipped");
    ok(! exists $steps_seen{'auto::perldoc'},
        "Configuring from verbosefoobar: auto::perldoc not in list");
}

{
    my $configfile = q{t/configure/testlib/adefectivefoobar};
    my $error = _test_defective_config_file($configfile);
    like($error, qr/Configuration file $configfile did not parse correctly/,
        "Got expected failure message after using defective configuration file");
}

{
    my $configfile = q{t/configure/testlib/bdefectivefoobar};
    my $error = _test_defective_config_file($configfile);
    like($error, qr/Bad variable substitution in $configfile/,
        "Got expected failure message after using defective configuration file");
}

{
    my $configfile = q{t/configure/testlib/cdefectivefoobar};
    my $error = _test_defective_config_file($configfile);
    like($error, qr/Invalid general option foobar in $configfile/,
        "Got expected failure message after using defective configuration file");
}

{
    my $configfile = q{t/configure/testlib/ddefectivefoobar};
    my $error = _test_defective_config_file($configfile);
    like($error, qr/Invalid option "foobar"/,
        "Got expected failure message after using defective configuration file");
}

pass("Completed all tests in $0");

################### SUBROUTINES ###################

sub _test_good_config_file {
    my $configfile = shift;
    local @ARGV = ( qq{--file=$configfile} );
    my ($args, $steps_list_ref) = process_options(
        {
            mode => (defined $ARGV[0]  and $ARGV[0] =~ /^--file=/)
                        ? 'file'
                        : 'configure',
            argv => [@ARGV],
        }
    );
    return ($args, $steps_list_ref);
}

sub _test_defective_config_file {
    my $configfile = shift;
    local @ARGV = ( qq{--file=$configfile} );
    eval {
        my ($args, $steps_list_ref) = process_options(
            {
                mode => (defined $ARGV[0]  and $ARGV[0] =~ /^--file=/)
                            ? 'file'
                            : 'configure',
                argv => [@ARGV],
            }
        );
    };
    my $error = $@;
    $@ = undef;
    return $error;
}

################### DOCUMENTATION ###################

=head1 NAME

008-file_based_configuration.t - test components of Parrot's file-based interface to configuration

=head1 SYNOPSIS

    % prove t/configure/008-file_based_configuration.t

=head1 DESCRIPTION

The files in this directory test functionality used by F<Configure.pl>.

The tests in this file test subroutines exported by
Parrot::Configure::Options as it is used with
C<mode =E<gt> q{file}>.

=head1 AUTHOR

James E Keenan

=head1 SEE ALSO

Parrot::Configure::Options, Parrot::Configure::Options::Conf,
Parrot::Configure::Options::Conf::CLI, F<Configure.pl>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
