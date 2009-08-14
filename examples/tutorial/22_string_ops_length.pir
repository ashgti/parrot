# Copyright (C) 2007-2009, Parrot Foundation.
# $Id$

=head1 String Operations (continued)

To find the length of a string in PIR, use the length opcode. It works on
any variable containing a basic Parrot string, but not the String PMC.
C<length> returns an integer value, and 0 means an empty string.

=cut


.sub main :main

    $S0 = "Hello"
    $I0 = length $S0
    say $I0

    .local string text
    text = "longer string"
    $I1 = length text
    say $I1

.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:

