# Copyright (C) 2001-2005, Parrot Foundation.
# This program is free software. It is subject to the same
# license as Parrot.
#
# $Id$

=head1 NAME

examples/subs/jsr_ret.pasm - Jumping to subroutine

=head1 SYNOPSIS

    % ./parrot examples/subs/jsr_ret.pasm

=head1 DESCRIPTION

A program to demonstrate the use of the C<jump> and of the C<jsr> opcode.

=head1 SEE ALSO

F<docs/ops/core.pod>
F<docs/pdds/pdd03_calling_conventions.pod>

=cut

MAIN:       print "Example of the jump op.\n"
            print "\n"
            print "Jumping to subroutine SUB_1.\n"
            set_addr I1, SUB_1
            jump I1

RET:        print "Returned from subroutine SUB_1.\n"
            print "Jumping to subroutine SUB_2.\n"
            set_addr I3, SUB_2
            jsr I3
            print "Returned from subroutine SUB_2.\n"
            end

SUB_1:      print "Entered subroutine SUB_1.\n"
            set_addr I2, RET
            print "Returning from subroutine SUB_1.\n"
            jump I2

SUB_2:      print "Entered subroutine SUB_2.\n"
            print "Returning from subroutine SUB_2.\n"
            ret

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
