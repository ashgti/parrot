# Copyright (C) 2001-2005 The Perl Foundation.  All rights reserved.
# $Id$

=head1 NAME

examples/subs/pasm_sub1.pasm - Parrot calling conventions

=head1 SYNOPSIS

    % ./parrot examples/subs/pasm_sub1.pasm

=head1 DESCRIPTION

Sample subroutines in Parrot assembler.

Shows you how to create a C<.Sub> and C<invokecc> it.

= TODO

Examples for parameter passing.

=cut

    # Create a subroutine
    newsub P0, .Sub, _sub

    # Call the sub in argument, create and invoke return continuation 
    invokecc P0 

    # got here, because the sub invoked the return continuation
    print "Hello from main\n"

    end

# A subroutine
_sub:
    print "Hello from subroutine\n"

    # Call the return continuation
    returncc

