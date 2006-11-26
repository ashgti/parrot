#! perl
# Copyright (C) 2005-2006, The Perl Foundation.
# $Id$

=head1 NAME

t/type/userdata.t - Lua Userdata

=head1 SYNOPSIS

    % perl -I../../lib t/type/userdata.t

=head1 DESCRIPTION

Tests C<userdata> type
(implemented in F<languages/lua/type/userdata.pir>).

=cut

use strict;
use warnings;

use Parrot::Test tests => 8;
use Test::More;

pir_output_is( << 'CODE', << 'OUTPUT', 'check inheritance' );
.sub _main
    load_bytecode 'languages/lua/type/userdata.pbc'
    find_type $I0, 'userdata'
    .local pmc pmc1
    pmc1 = new $I0
    .local int bool1
    bool1 = isa pmc1, 'scalar'
    print bool1
    print "\n"
    bool1 = isa pmc1, 'LuaBase'
    print bool1
    print "\n"
    bool1 = isa pmc1, 'base_lua'
    print bool1
    print "\n"
    bool1 = isa pmc1, 'userdata'
    print bool1
    print "\n"
    end
.end
CODE
0
1
1
1
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', 'check interface' );
.sub _main
    load_bytecode 'languages/lua/type/userdata.pbc'
    find_type $I0, 'userdata'
    .local pmc pmc1
    pmc1 = new $I0
    .local int bool1
    bool1 = does pmc1, 'scalar'
    print bool1
    print "\n"
    bool1 = does pmc1, 'no_interface'
    print bool1
    print "\n"
    end
.end
CODE
1
0
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', 'check name' );
.sub _main
    load_bytecode 'languages/lua/type/userdata.pbc'
    find_type $I0, 'userdata'
    .local pmc pmc1
    pmc1 = new $I0
    .local string str1
    str1 = classname pmc1
    print str1
    print "\n"
    str1 = typeof pmc1
    print str1
    print "\n"
    end
.end
CODE
userdata
userdata
OUTPUT

pir_output_like( << 'CODE', << 'OUTPUT', 'check get_string' );
.sub _main
    load_bytecode 'languages/lua/type/userdata.pbc'
    find_type $I0, 'userdata'
    .local pmc val1
    val1 = new .Array
    .local pmc pmc1
    pmc1 = new $I0
    setattribute pmc1, 'data', val1
    print pmc1
    print "\n"
    end
.end
CODE
/userdata: [0-9A-Fa-f]{8}/
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', 'check get_bool' );
.sub _main
    load_bytecode 'languages/lua/type/userdata.pbc'
    find_type $I0, 'userdata'
    .local pmc val1
    val1 = new .Array
    .local pmc pmc1
    pmc1 = new $I0
    setattribute pmc1, 'data', val1
    .local int bool1
    bool1 = istrue pmc1
    print bool1
    print "\n"
.end
CODE
1
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', 'check logical_not' );
.sub _main
    loadlib P1, 'lua_group'
    load_bytecode 'languages/lua/type/userdata.pbc'
    find_type $I0, 'userdata'
    .local pmc val1
    val1 = new .Array
    .local pmc pmc1
    pmc1 = new $I0
    setattribute pmc1, 'data', val1
    find_type $I0, 'LuaBoolean'
    .local pmc pmc2
    pmc2 = new $I0
    pmc2 = not pmc1
    print pmc2
    print "\n"
    .local string str1
    str1 = typeof pmc2
    print str1
    print "\n"
    end
.end
CODE
false
boolean
OUTPUT

pir_output_like( << 'CODE', << 'OUTPUT', 'check tostring' );
.sub _main
    load_bytecode 'languages/lua/type/userdata.pbc'
    .local pmc val1
    val1 = new .Array
    .local pmc pmc1
    pmc1 = new 'userdata'
    setattribute pmc1, 'data', val1
    print pmc1
    print "\n"
    $P0 = pmc1.'tostring'()
    print $P0
    print "\n"
    $S0 = typeof $P0
    print $S0
    print "\n"
.end
CODE
/userdata: [0-9A-Fa-f]{8}\nuserdata: [0-9A-Fa-f]{8}\nstring/
OUTPUT

pir_output_is( << 'CODE', << 'OUTPUT', 'check tonumber' );
.sub _main
    load_bytecode 'languages/lua/type/userdata.pbc'
    .local pmc val1
    val1 = new .Array
    .local pmc pmc1
    pmc1 = new 'userdata'
    setattribute pmc1, 'data', val1
    $P0 = pmc1.'tonumber'()
    print $P0
    print "\n"
    $S0 = typeof $P0
    print $S0
    print "\n"
.end
CODE
nil
nil
OUTPUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:

