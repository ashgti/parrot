#! perl
# Copyright (C) 2006-2007, The Perl Foundation.
# $Id$

=head1 NAME

t/pmc/string.t - WmlsString

=head1 SYNOPSIS

    % perl -I../../lib t/pmc/string.t

=head1 DESCRIPTION

Tests C<WmlsString> PMC
(implemented in F<languages/WMLScript/pmc/wmlsstring.pmc>).

=cut

use strict;
use warnings;
use Parrot::Test tests => 12;
use Test::More;

pir_output_is( << 'CODE', << 'OUTPUT', 'check inheritance' );
.sub _main
    loadlib P1, "wmls_group"
    .local pmc pmc1
    pmc1 = new "WmlsString"
    .local int bool1
    bool1 = isa pmc1, "String"
    print bool1
    print "\n"
    bool1 = isa pmc1, "WmlsString"
    print bool1
    print "\n"
    end
.end
CODE
1
1
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', 'check interface' );
.sub _main
    loadlib P1, "wmls_group"
    .local pmc pmc1
    pmc1 = new "WmlsString"
    .local int bool1
    bool1 = does pmc1, "scalar"
    print bool1
    print "\n"
    bool1 = does pmc1, "string"
    print bool1
    print "\n"
    bool1 = does pmc1, "no_interface"
    print bool1
    print "\n"
    end
.end
CODE
1
1
0
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', 'check name' );
.sub _main
    loadlib P1, "wmls_group"
    .local pmc pmc1
    pmc1 = new "WmlsString"
    .local string str1
    str1 = typeof pmc1
    print str1
    print "\n"
    end
.end
CODE
WmlsString
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', 'check clone' );
.sub _main
    loadlib P1, "wmls_group"
    .local pmc pmc1
    pmc1 = new "WmlsString"
    pmc1 = "str"
    .local pmc pmc2
    pmc2 = clone pmc1
    pmc1 = "STR"
    .local string str1
    str1 = typeof pmc2
    print str1
    print "\n"
    .local string str2
    str2 = pmc2
    print str2
    print "\n"
    str1 = pmc1
    print str1
    print "\n"
    end
.end
CODE
WmlsString
str
STR
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', 'check get_bool' );
.sub _main
    loadlib P1, "wmls_group"
    .local pmc pmc1
    pmc1 = new "WmlsString"
    pmc1 = "str"
    .local int bool1
    bool1 = istrue pmc1
    print bool1
    print "\n"
    pmc1 = ""
    bool1 = istrue pmc1
    print bool1
    print "\n"
    end
.end
CODE
1
0
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', 'check embedded zero' );
.sub _main
    loadlib P1, "wmls_group"
    .local pmc pmc1
    pmc1 = new "WmlsString"
    pmc1 = "embe\0_dd\0_ed\0"
    .local int len1
    len1 = elements pmc1
    print len1
    print "\n"
.end
CODE
13
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', 'check HLL' );
.HLL "WMLScript"
.loadlib "wmls_group"
.sub _main
    .local pmc pmc1
    pmc1 = new "WmlsString"
    pmc1 = "simple string"
    print pmc1
    print "\n"
    .local int bool1
    bool1 = isa pmc1, "WmlsString"
    print bool1
    print "\n"
    end
.end
CODE
simple string
1
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', 'check HLL & .const' );
.HLL "WMLScript"
.loadlib "wmls_group"
.sub _main
    .const "WmlsString" cst1 = "simple string"
    print cst1
    print "\n"
    .local int bool1
    bool1 = isa cst1, "WmlsString"
    print bool1
    print "\n"
.end
CODE
simple string
1
OUTPUT

    pir_output_is( << 'CODE', << 'OUTPUT', '.const & empty string' );
.HLL "WMLScript"
.loadlib "wmls_group"
.sub _main
    .const "WmlsString" cst1 = ""
    print cst1
    print "\n"
    .local int bool1
    bool1 = isa cst1, "WmlsString"
    print bool1
    print "\n"
.end
CODE

1
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', 'check istrue' );
.loadlib "wmls_ops"
.HLL "WMLScript"
.loadlib "wmls_group"
.sub _main
    .const "WmlsString" cst1 = "simple string"
    print cst1
    print "\n"
    $P0 = istrue cst1
    print $P0
    print "\n"
    $S0 = typeof $P0
    print $S0
    print "\n"
.end
CODE
simple string
true
WmlsBoolean
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', 'check typeof' );
.loadlib "wmls_ops"
.HLL "WMLScript"
.loadlib "wmls_group"
.sub _main
    .const "WmlsString" cst1 = "simple string"
    print cst1
    print "\n"
    $P0 = typeof cst1
    print $P0
    print "\n"
    $S0 = typeof $P0
    print $S0
    print "\n"
.end
CODE
simple string
2
WmlsInteger
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', 'check defined' );
.loadlib "wmls_ops"
.HLL "WMLScript"
.loadlib "wmls_group"
.sub _main
    .const "WmlsString" cst1 = "simple string"
    print cst1
    print "\n"
    $P0 = defined cst1
    print $P0
    print "\n"
    $S0 = typeof $P0
    print $S0
    print "\n"
.end
CODE
simple string
true
WmlsBoolean
OUTPUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:

