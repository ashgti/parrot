package Parrot::Embed;

use strict;
use warnings;

our $VERSION = '0.02';

use base 'DynaLoader';

Parrot::Embed->bootstrap($VERSION);

1;

__END__

=head1 NAME

Parrot::Embed - use Parrot from Perl 5

=head1 VERSION

Version 0.02

=head1 SYNOPSIS

This module embeds libparrot in Perl 5 programs.  You can load Parrot bytecode,
compile your own code, and call Parrot subroutines and send and receive values
to them.

Do consider using L<Parrot::Interpreter> first however.  See also
L<Parrot::PMC>.

=head1 AUTHOR

chromatic, C<< <chromatic at wgz.org> >>

=head1 BUGS

Please report any bugs or feature requests to the Parrot Porters mailing list.
Someday there may be a CPAN version of this code.  Who knows?

=head1 COPYRIGHT & LICENSE

Copyright (c) 2006 - 2007 The Perl Foundation / chromatic.

This program is free software; you can redistribute it and/or modify it
under the same terms as Parrot itself.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
