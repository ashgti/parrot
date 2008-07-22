#!perl
# Copyright (C) 2008, The Perl Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );

use Test::More;
use Parrot::Test tests => 1;

pir_output_is( <<'CODE', <<'OUT', ".param :slurpy (using PMC)" );

.HLL 'misc', ''
.HLL_map 'ResizablePMCArray', 'ResizableStringArray'

.sub main :main
  elm('a','b','c')
.end

.sub elm
  .param pmc args :slurpy

  $I1 = elements args
  say $I1

  $S0 = typeof args
  say $S0
.end
CODE
3
ResizableStringArray
OUT
