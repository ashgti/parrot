#! perl
# Copyright (C) 2005-2007, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 5;
use Parrot::Config;

=head1 NAME

t/examples/subs.t - Test examples in F<examples/subs>

=head1 SYNOPSIS

    % prove t/examples/subs.t

=head1 DESCRIPTION

Test the examples in F<examples/subs>.

=head1 SEE ALSO

F<t/examples/japh.t>
F<t/examples/pasm.t>
F<t/examples/pir.t>

=cut

# Set up expected output for examples
my %expected = (
    'coroutine.pasm' => << 'END_EXPECTED',
Calling 1st co-routine
Entry
Resumed
Done
Calling 2nd co-routine
Entry
Resumed
Done
END_EXPECTED

    'pasm_sub1.pasm' => << 'END_EXPECTED',
Hello from subroutine
Hello from main
END_EXPECTED

    'single_retval.pir' => << 'END_EXPECTED',
7 8 nine 10
return: 10
7 8 nine 10
return: 10
END_EXPECTED

    'multi_retvals.pir' => << 'END_EXPECTED',
return: 10 11 12
END_EXPECTED

    'no_retval.pir' => << 'END_EXPECTED',
7 8 nine
END_EXPECTED
);

while ( my ( $example, $expected ) = each %expected ) {
    example_output_is( "examples/subs/$example", $expected );
}

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
