#! perl
# Copyright (C) 2007, Parrot Foundation.
# $Id$
# inter/lex-02.t

use strict;
use warnings;
use Test::More tests =>   7;
use Carp;
use lib qw( lib t/configure/testlib );
$ENV{TEST_LEX} = 'foobar';
use_ok('config::inter::lex');
use Parrot::Configure::Options qw( process_options );
use Parrot::Configure::Step::Test;
use Parrot::Configure::Test qw(
    test_step_constructor_and_description
);

########## ask; maintainer; $ENV{TEST_LEX} ##########

my ($args, $step_list_ref) = process_options(
    {
        argv => [ q{--ask}, q{--maintainer} ],
        mode => q{configure},
    }
);

my $conf = Parrot::Configure::Step::Test->new;
$conf->include_config_results( $args );

my $pkg = q{inter::lex};

$conf->add_steps($pkg);
$conf->options->set( %{$args} );
my $step = test_step_constructor_and_description($conf);

my $ret = $step->runstep($conf);
is( $ret, undef, "runstep() returned undefined value" );
my $result_expected = q{no lex program was found};
is( $step->result(), $result_expected, "Result was $result_expected" );

pass("Completed all tests in $0");

################### DOCUMENTATION ###################

=head1 NAME

inter/lex-02.t - test inter::lex

=head1 SYNOPSIS

    % prove t/steps/inter/lex-02.t

=head1 DESCRIPTION

The files in this directory test functionality used by F<Configure.pl>.

The tests in this file test inter::lex.

=head1 AUTHOR

James E Keenan

=head1 SEE ALSO

config::inter::lex, F<Configure.pl>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
