=head1 NAME

PAST - Parrot abstract syntax tree

=head1 DESCRIPTION

This file brings together the various PAST/POST modules
to build the PAST.pbc module.

=cut

.include 'src/PCT/Node.pir'

.include 'src/PAST/Node.pir'

.include 'src/PAST/Compiler.pir'

.include 'src/POST/Node.pir'

.include 'src/POST/Compiler.pir'

=head1 AUTHOR

Patrick Michaud <pmichaud@pobox.com> is the author and maintainer.
Please send patches and suggestions to the Parrot porters or
Perl 6 compilers mailing lists.

=head1 HISTORY

2007-11-29  Created from older PAST-pm version.

=head1 COPYRIGHT

Copyright (C) 2007-2008, Parrot Foundation.

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
