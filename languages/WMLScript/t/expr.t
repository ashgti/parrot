#! perl
# Copyright (C) 2006-2008, The Perl Foundation.
# $Id$

=head1 NAME

t/expr.t - WMLScript expressions

=head1 SYNOPSIS

    % perl t/expr.t

=head1 DESCRIPTION


=cut

use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../../../lib", "$FindBin::Bin";

use Parrot::Test tests => 2;
use Test::More;

language_output_is( 'WMLScript', <<'CODE', <<'OUT', 'assign', cflags => '-On' );
extern function main()
{
    var a = "abc";
    var b = a;
    b = "def";
    Console.println(a);
    Console.println(b);
}
CODE
abc
def
OUT

language_output_is( 'WMLScript', <<'CODE', <<'OUT', 'incr', cflags => '-On' );
extern function main()
{
    var a = 10;
    var b = a;
    b ++;
    Console.println(a);
    Console.println(b);
}
CODE
10
11
OUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:

