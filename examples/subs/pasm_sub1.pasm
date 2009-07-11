# Copyright (C) 2001-2005, Parrot Foundation.
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

=head1 SEE ALSO

F<docs/ops/core.pod>
F<docs/pdds/pdd03_calling_conventions.pod>

=cut

    # Fetch a subroutine objec
    .const 'Sub' P0 = "_sub"

    # Call the sub in argument, create return continuation
    invokecc P0

    # got here, because the sub invoked the return continuation
    print "Hello from main\n"

    end

# A subroutine
.pcc_sub _sub:
    print "Hello from subroutine\n"

    # Call the return continuation
    returncc

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
