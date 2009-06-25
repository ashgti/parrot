#! parrot
# Copyright (C) 2009, Parrot Foundation.
# $Id$

=head1 NAME

t/pmc/pipehandle.t - test the PipeHandle PMC


=head1 SYNOPSIS

    % prove t/pmc/pipehandle.t

=head1 DESCRIPTION

Tests the PipeHandle and PMC. Most of the pipe testing is in pipe.t

=cut

.sub main :main
.include 'test_more.pir'
    plan(1)
    test_new()
.end

.sub test_new
  $P0 = new ['PipeHandle']
  $S0 = typeof $P0
  is($S0, "PipeHandle", "typeof(PipeHandle) == 'PipeHandle'")
.end

