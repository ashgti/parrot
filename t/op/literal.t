# Copyright: 2004-2005 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

t/op/literal.t - Testing the PIR and PASM lexer

=head1 SYNOPSIS

	% perl -Ilib t/op/literal.t

=head1 DESCRIPTION

Test lexing of literal numbers.
Taken from from the 2nd aoudad book (page 127).

=head1 TODO

More tests are welcome.

=head1 SEE ALSO

L<http://rt.perl.org/rt3/Ticket/Display.html?id=31197>

=cut

use Parrot::Test tests => 2;

pasm_output_is(<<'CODE', <<'OUTPUT', "integer literals in PASM");
        print 0x2A
        print "\n"
        print 0X2A
        print "\n"
        print 0b101010
        print "\n"
        print 0B101010
        print "\n"
        end
CODE
42
42
42
42
OUTPUT

pir_output_is(<<'CODE', <<'OUTPUT', "integer literals in PIR");
.sub test @MAIN
        print 0x2A
        print "\n"
        print 0X2A
        print "\n"
        print 0b101010
        print "\n"
        print 0B101010
        print "\n"
.end
CODE
42
42
42
42
OUTPUT
