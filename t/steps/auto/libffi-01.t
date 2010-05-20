#!perl
# Copyright (C) 2001-2007, Parrot Foundation.
# $Id$
# auto/ctags-01.t

use strict;
use warnings;
use Test::More tests =>  28;
use Carp;
use lib qw( lib t/configure/testlib );
use_ok('config::auto::libffi');
use Parrot::Configure::Options qw( process_options );
use Parrot::Configure::Step::Test;
use Parrot::Configure::Test qw(
    test_step_constructor_and_description
    );

use IO::CaptureOutput qw| capture |;

################### --without-libffi ###################


################### DOCUMENTATION ###################

=head1 NAME

  auto/libffi-01.t - test auto::libffi

=head1 SYNOPSIS

    % prove t/steps/auto/libffi-01.t

=head1 DESCRIPTION

The files in this directory test functionality used by F<Configure.pl>.

The tests in this file test configuration step class auto::libffi

=head1 AUTHOR

John Harrison

=head1 SEE ALSO

config::auto::libff, F<Configure.pl>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:

