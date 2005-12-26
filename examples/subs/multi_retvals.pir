# Copyright (C) 2001-2003 The Perl Foundation.  All rights reserved.
# $Id$

=head1 NAME

examples/subs/multi_retvals.pir - Subroutine example

=head1 SYNOPSIS

    % ./parrot examples/subs/multi_retvals.pir

=head1 DESCRIPTION

Call a sub with multiple return values.

=cut

.sub _main :main
  ($I0, $I1, $I99) = _foo(7, 8, "nine")
  print "return: "
  print $I0
  print " "
  print $I1
  print " "
  print $I99
  print "\n"
.end

.sub _foo
  .return( 10, 11, 12 )
.end
