#! perl
# Copyright (C) 2009, Parrot Foundation.
# $Id$
# auto/libjit-01.t

use strict;
use warnings;

use Test::More tests => 9;

use lib 'lib';

use Parrot::Configure;
use Parrot::Configure::Options 'process_options';
use Parrot::Configure::Test qw(
    test_step_thru_runstep
    test_step_constructor_and_description
);

use_ok('config::init::defaults');
use_ok('config::auto::libjit');

########## _select_lib() ##########

my ($args, $step_list_ref) = process_options(
    {
	argv => [],
	mode => 'configure',
    }
);

my $conf = Parrot::Configure->new;

test_step_thru_runstep( $conf, 'init::defaults', $args );

my $pkg = 'auto::libjit';

$conf->add_steps($pkg);
$conf->options->set(%$args);
my $stp = test_step_constructor_and_description($conf);

################### DOCUMENTATION ###################

=head1 NAME

auto/libjit-01.t - test auto::libjit

=head1 SYNOPSIS

    % prove t/steps/auto/libjit-01.t

=head1 DESCRIPTION

The files in this directory test functionality used by F<Configure.pl>.

The tests in this file test auto::libjit.

=head1 SEE ALSO

config::auto::libjit, F<Configure.pl>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
