# Copyright (C) 2001-2007, The Perl Foundation.
# $Id$

=head1 NAME

examples/pir/hello-dwim.pir - Parrot's confusing opcodes made easy

=head1 SYNOPSIS

    % ./parrot examples/pir/hello-dwim.pir

=head1 DESCRIPTION

The first time you run this program

    .sub _main
        .include "DWIM.pir"
        print "The answer is\n"
        add $I0, 20, 23
        dec $I0
        print $I0
        print "\nsay's Parrot!\n"
        end
    .end

the module replaces all the unsightly opcodes from your source file with
the new DWIM comment: C<# DWIM> and runs normally.

The code continues to work exactly as it did before, but now it looks
like this:

    .sub _main
       .include "DWIM.pir"
      # DWIM  "The answer is\n"
      # DWIM  $I0, 20, 23
      # DWIM  $I0
      # DWIM  $I0
      # DWIM  "\nsay's Parrot!\n"
      # DWIM
    .end

=cut

.sub _main
    .include "DWIM.pir"
    print "The answer is\n"
    add $I0, 20, 23
    dec $I0
    print $I0
    print "\nsay's Parrot!\n"
    end
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
