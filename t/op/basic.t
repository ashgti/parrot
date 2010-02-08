#!perl
# Copyright (C) 2001-2007, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test tests => 23;

=head1 NAME

t/op/basic.t - Basic Ops

=head1 SYNOPSIS

        % prove t/op/basic.t

=head1 DESCRIPTION

Tests basic string and branching operations.

=cut

# It would be very embarrassing if these didnt work...
pasm_output_is( <<'CODE', '', "noop, end" );
        noop
        end
CODE

pasm_output_is( <<'CODE', '1', "print 1" );
        print   1
        end
CODE

pasm_output_is( <<'CODE', 'Parrot flies', "print string" );
        print 'Parrot flies'
        end
CODE

pasm_output_is( <<'CODE', 'Parrot flies', "print double-quoted string" );
       print "Parrot flies"
       end
CODE

pasm_output_is( <<'CODE', "Parrot\tflies", "print double-quoted string, tabs" );
       print "Parrot\tflies"
       end
CODE

pasm_output_is( <<'CODE', q('Parrot' flies), "print double-quoted string, nested single" );
       print "'Parrot' flies"
       end
CODE

pasm_output_is( <<'CODE', q("Parrot" flies), "print single-quoted string, nested double" );
       print '"Parrot" flies'
       end
CODE

pasm_output_is( <<'CODE', q(Parrot flies), "print string with embedded hex escape" );
       print "Parrot\x20flies"
       end
CODE

pasm_output_is( <<'CODE', q(Parrot flies), "escaped non-special" );
       print "Parrot fl\ies"
       end
CODE

pasm_output_is( <<'CODE', <<OUTPUT, "print string with embedded newline" );
       print "Parrot flies\n"
       end
CODE
Parrot flies
OUTPUT

pasm_output_is( <<'CODE', '42', "branch_ic" );
        set     I4, 42
        branch  HERE
        set     I4, 1234
HERE:
        print   I4
        end
CODE

pasm_output_is( <<'CODE', '42', "branch_ic (backward)" );
        set     I4, 42
        branch  one
two:    branch  three
        set     I4, 1234
        add     I4, I4, I4
one:
        branch  two
three:
        print   I4
        end
CODE

pasm_output_is( <<'CODE', <<'OUTPUT', "local_branch_c_i" );
        print    "start\n"

        new P0, 'ResizableIntegerArray'

        local_branch P0, LAB1

        print    "done\n"
        end

LAB1:   print    "lab 1\n"
        local_return P0
CODE
start
lab 1
done
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "set_addr" );
       set_addr I1, FOO
       jump I1
       print "Jump failed\n"
       end

FOO:   print "Jump succeeded\n"
       end
CODE
Jump succeeded
OUTPUT

pasm_output_is( <<'CODE', <<'OUTPUT', "multiple labels" );
     if 0,FOO
     if 1,BAR
     print "not "
FOO:
BAR:
     print "ok 1\n"
     end
CODE
ok 1
OUTPUT

pasm_output_is( <<'CODE', 32, "Predeclared opcodes" );
     set_i_ic I0,32
     print I0
     end
CODE

pir_output_is( <<'CODE', <<'OUTPUT', "pir syntax with marker - is" );

.sub _main
     .const string OK = "ok\n"
     print OK
     end
.end

CODE
ok
OUTPUT

pir_output_isnt( <<'CODE', <<'OUTPUT', "pir syntax with marker - isnt" );

.sub _main
     .const string OK = "ok\n"
     print OK
     end
.end

CODE
parrot
OUTPUT

pir_output_like( <<'CODE', <<'OUTPUT', "pir syntax with marker - like" );

.sub _main
     .const string OK = "ok\n"
     print OK
     end
.end

CODE
/^\w\w\s+$/
OUTPUT

pir_output_is( <<'CODE', <<'OUTPUT', "pir syntax with function - is" );
.sub _main
     .const string OK = "ok\n"
     print OK
     end
.end

CODE
ok
OUTPUT

pir_output_isnt( <<'CODE', <<'OUTPUT', "pir syntax with function - isnt" );
.sub _main
     .const string OK = "ok\n"
     print OK
     end
.end

CODE
nada niete
OUTPUT

pir_output_like( <<'CODE', <<'OUTPUT', "pir syntax with function - like" );
.sub _main
     .const string OK = "ok 1\n"
     print OK
     end
.end

CODE
/^\w{2}\s\d\s+$/
OUTPUT

my $CODE = '
.sub _main
    print "ok\n"
    end
.end

#.namespace';    # no \n at end of file

pir_error_output_like( $CODE, <<'OUTPUT', "end of line handling" );
/unexpected/
OUTPUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
