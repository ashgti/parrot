#! perl -w
# Copyright: 2001-2004 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

t/op/string_cs.t - String Charset Tests

=head1 SYNOPSIS

	% perl -Ilib t/op/string_cs.t

=head1 DESCRIPTION

Tests charset support.

=cut

use Parrot::Test tests => 20;
use Test::More;

output_is( <<'CODE', <<OUTPUT, "basic syntax" );
    set S0, ascii:"ok 1\n"
    print S0
    set S0, binary:"ok 2\n"
    print S0
    set S0, iso-8859-1:"ok 3\n"
    print S0
    end
CODE
ok 1
ok 2
ok 3
OUTPUT

output_is( <<'CODE', <<OUTPUT, "charset name" );
    set S0, ascii:"ok 1\n"
    charset I0, S0
    charsetname S1, I0
    print S1
    print "\n"
    end
CODE
ascii
OUTPUT

output_is( <<'CODE', <<OUTPUT, "find_charset" );
    find_charset I0, "iso-8859-1"
    print "ok 1\n"
    find_charset I0, "ascii"
    print "ok 2\n"
    find_charset I0, "binary"
    print "ok 3\n"
    end
CODE
ok 1
ok 2
ok 3
OUTPUT

output_like( <<'CODE', <<OUTPUT, "find_charset - not existing" );
    find_charset I0, "no_such"
    end
CODE
/charset 'no_such' not found/
OUTPUT

output_is( <<'CODE', <<OUTPUT, "downcase" );
    set S0, "AEIOU_���\n"
    downcase S1, S0
    print S1
    end
CODE
aeiou_���
OUTPUT

output_is( <<'CODE', <<OUTPUT, "upcase" );
    set S0, "aeiou_����\n"
    upcase S1, S0
    print S1
    end
CODE
AEIOU_����
OUTPUT

output_is( <<'CODE', <<OUTPUT, "titlecase" );
    set S0, "zAEIOU_���\n"
    titlecase S1, S0
    print S1
    end
CODE
Zaeiou_���
OUTPUT

output_is( <<'CODE', <<OUTPUT, "is_whitespace");
    set S0, "a\t\n \xa0"
    is_whitespace I0, S0, 0
    is_whitespace I1, S0, 1
    is_whitespace I2, S0, 2
    is_whitespace I3, S0, 3
    set I4, 4
    is_whitespace I4, S0, I4
    print I0
    print I1
    print I2
    print I3
    print I4
    print "\n"
    set S0, ascii:"a\t\n "
    is_whitespace I0, S0, 0
    is_whitespace I1, S0, 1
    is_whitespace I2, S0, 2
    is_whitespace I3, S0, 3
    print I0
    print I1
    print I2
    print I3
    print "\n"
    end
CODE
01111
0111
OUTPUT

output_is( <<'CODE', <<OUTPUT, "is_wordchar");
    set S0, "az019-,._"
    length I1, S0
    set I2, 0
lp:
    is_wordchar I0, S0, I2
    print I0
    inc I2
    lt I2, I1, lp
    print "\n"
    end
CODE
111110001
OUTPUT

output_is( <<'CODE', <<OUTPUT, "is_digit");
    set S0, "az019-,._"
    length I1, S0
    set I2, 0
lp:
    is_digit I0, S0, I2
    print I0
    inc I2
    lt I2, I1, lp
    print "\n"
    end
CODE
001110000
OUTPUT

output_is( <<'CODE', <<OUTPUT, "is_punctuation");
    set S0, "az019-,._"
    length I1, S0
    set I2, 0
lp:
    is_punctuation I0, S0, I2
    print I0
    inc I2
    lt I2, I1, lp
    print "\n"
    end
CODE
000001110
OUTPUT

output_is( <<'CODE', <<OUTPUT, "is_newline");
    set S0, "a\n"
    is_newline I0, S0, 0
    print I0
    is_newline I0, S0, 1
    print I0
    print "\n"
    end
CODE
01
OUTPUT

output_is( <<'CODE', <<OUTPUT, "find_wordchar");
    set S0, "_ ab 09"
    set I0, 0
lp:
    find_wordchar I0, S0, I0
    print I0
    print " "
    eq I0, -1, done
    inc I0
    branch lp
done:
    print "ok\n"
    end
CODE
0 2 3 5 6 -1 ok
OUTPUT

output_is( <<'CODE', <<OUTPUT, "find_digit");
    set S0, "_ ab 09"
    set I0, 0
lp:
    find_digit I0, S0, I0
    print I0
    print " "
    eq I0, -1, done
    inc I0
    branch lp
done:
    print "ok\n"
    end
CODE
5 6 -1 ok
OUTPUT
output_is( <<'CODE', <<OUTPUT, "find_punctuation");
    set S0, "_ .b ,9"
    set I0, 0
lp:
    find_punctuation I0, S0, I0
    print I0
    print " "
    eq I0, -1, done
    inc I0
    branch lp
done:
    print "ok\n"
    end
CODE
2 5 -1 ok
OUTPUT

output_is( <<'CODE', <<OUTPUT, "find_word_boundary");
    set S0, "_ab 09z"
    set I0, 0
lp:
    find_word_boundary I0, S0, I0
    print I0
    print " "
    eq I0, -1, done
    inc I0
    branch lp
done:
    print "ok\n"
    end
CODE
0 2 3 6 -1 ok
OUTPUT

output_is( <<'CODE', <<OUTPUT, "trans_charset_s_s_i");
    set S0, "abc"
    find_charset I0, "ascii"
    trans_charset S1, S0, I0
    print S1
    print "\n"
    charset I0, S1
    charsetname S2, I0
    print S2
    print "\n"
    end
CODE
abc
ascii
OUTPUT

output_is( <<'CODE', <<OUTPUT, "trans_charset_s_i");
    set S1, "abc"
    find_charset I0, "ascii"
    trans_charset S1, I0
    print S1
    print "\n"
    charset I0, S1
    charsetname S2, I0
    print S2
    print "\n"
    end
CODE
abc
ascii
OUTPUT


output_like( <<'CODE', <<OUTPUT, "trans_charset_s_i - lossy");
    set S1, "abc�"
    find_charset I0, "ascii"
    trans_charset S1, I0
    print "never\n"
    end
CODE
/lossy conversion to ascii/
OUTPUT

output_is( <<'CODE', <<OUTPUT, "trans_charset_s_i - same");
    set S1, ascii:"abc"
    find_charset I0, "ascii"
    trans_charset S1, I0
    print S1
    print "\n"
    charset I0, S1
    charsetname S2, I0
    print S2
    print "\n"
    end
CODE
abc
ascii
OUTPUT

