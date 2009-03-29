#!perl
# Copyright (C) 2001-2008, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( t . lib ../lib ../../lib );

use Test::More;
use Parrot::Test tests => 26;

=head1 NAME

t/library/dumper.t - test dumping of data

=head1 SYNOPSIS

    % prove t/library/dumper.t

=head1 DESCRIPTION

Tests data dumping.

=cut

# no. 1
pir_output_is( <<'CODE', <<'OUT', "dumping array of sorted numbers" );

.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc array

    new array, 'ResizablePMCArray'
    push array, 0
    push array, 1
    push array, 2
    push array, 3
    push array, 4
    push array, 5
    push array, 6
    push array, 7
    push array, 8
    push array, 9

    _dumper( array, "array" )
.end
CODE
"array" => ResizablePMCArray (size:10) [
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9
]
OUT

# no. 2

pir_output_is( <<'CODE', <<'OUT', "dumping unsorted numbers" );
.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc array

    new array, 'ResizablePMCArray'
    push array, 6
    push array, 1
    push array, 8
    push array, 3
    push array, 2
    push array, 9
    push array, 7
    push array, 4
    push array, 0
    push array, 5

    _dumper( array, "array" )
.end
CODE
"array" => ResizablePMCArray (size:10) [
    6,
    1,
    8,
    3,
    2,
    9,
    7,
    4,
    0,
    5
]
OUT

# no. 3
pir_output_is( <<'CODE', <<'OUT', "dumping sorted strings" );

.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc array

    new array, 'ResizablePMCArray'
    push array, "alpha"
    push array, "bravo"
    push array, "charlie"
    push array, "delta"
    push array, "echo"
    push array, "foxtrot"
    push array, "golf"
    push array, "hotel"

    _dumper( array, "strings" )
.end
CODE
"strings" => ResizablePMCArray (size:8) [
    "alpha",
    "bravo",
    "charlie",
    "delta",
    "echo",
    "foxtrot",
    "golf",
    "hotel"
]
OUT

# no. 4
pir_output_is( <<'CODE', <<'OUT', "sorting unsorted strings" );

.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc array

    new array, 'ResizablePMCArray'
    push array, "charlie"
    push array, "hotel"
    push array, "alpha"
    push array, "delta"
    push array, "foxtrot"
    push array, "golf"
    push array, "bravo"
    push array, "echo"

    _dumper( array, "strings" )
.end
CODE
"strings" => ResizablePMCArray (size:8) [
    "charlie",
    "hotel",
    "alpha",
    "delta",
    "foxtrot",
    "golf",
    "bravo",
    "echo"
]
OUT

# no. 5
pir_output_is( <<'CODE', <<'OUT', "dumping different types" );

.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc array

    new array, 'ResizablePMCArray'
    push array, 0.1
    push array, "charlie"
    push array, 2
    push array, "hotel"
    push array, 5
    push array, "alpha"
    push array, 0.2
    push array, "delta"
    push array, 4
    push array, "foxtrot"
    push array, 0.5
    push array, 0.4
    push array, 1
    push array, "golf"
    push array, 0.3
    push array, 3
    push array, "bravo"
    push array, 0.0
    push array, 0
    push array, "echo"

    _dumper( array, "array" )
.end
CODE
"array" => ResizablePMCArray (size:20) [
    0.1,
    "charlie",
    2,
    "hotel",
    5,
    "alpha",
    0.2,
    "delta",
    4,
    "foxtrot",
    0.5,
    0.4,
    1,
    "golf",
    0.3,
    3,
    "bravo",
    0,
    0,
    "echo"
]
OUT

# no. 6
pir_output_is( <<'CODE', <<'OUT', "dumping complex data" );

.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc hash1
    .local pmc hash2
    .local pmc hash3
    .local pmc array1
    .local pmc array2

    new hash1, 'Hash'
    new hash2, 'Hash'
    new hash3, 'Hash'
    new array1, 'ResizablePMCArray'
    new array2, 'ResizablePMCArray'

    _dumper( hash1,"hash1" )

    $S0 = "hello"
    $S1 = "world"
    set hash1[$S0], $S1

    _dumper( hash1,"hash1" )

    $S0 = "hello2"
    $S1 = "world2"
    set hash1[$S0], $S1

    _dumper( hash1,"hash1" )

    $S0 = "hash2"
    set hash1[$S0], hash2

    _dumper( hash1,"hash1" )

    $S0 = "hello3"
    $S1 = "world3"
    set hash2[$S0], $S1

    _dumper( hash1,"hash1" )

    $S0 = "name"
    $S1 = "parrot"
    set hash3[$S0], $S1
    $S0 = "is"
    $S1 = "cool"
    set hash3[$S0], $S1

    push array1, "this"
    push array1, "is"
    push array1, "a"
    push array1, "test"
    push array1, hash3

    $S0 = "array1"
    set hash2[$S0], array1

    _dumper( hash1,"hash1" )

.end
CODE
"hash1" => Hash {
}
"hash1" => Hash {
    "hello" => "world"
}
"hash1" => Hash {
    "hello" => "world",
    "hello2" => "world2"
}
"hash1" => Hash {
    "hash2" => Hash {
    },
    "hello" => "world",
    "hello2" => "world2"
}
"hash1" => Hash {
    "hash2" => Hash {
        "hello3" => "world3"
    },
    "hello" => "world",
    "hello2" => "world2"
}
"hash1" => Hash {
    "hash2" => Hash {
        "array1" => ResizablePMCArray (size:5) [
            "this",
            "is",
            "a",
            "test",
            Hash {
                "is" => "cool",
                "name" => "parrot"
            }
        ],
        "hello3" => "world3"
    },
    "hello" => "world",
    "hello2" => "world2"
}
OUT

# no.7
pir_output_is( <<'CODE', <<'OUT', "properties" );

.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc str
    .local pmc array

    new array, 'ResizablePMCArray'
    push array, "test1"
    push array, "test2"

    new str, 'String'
    set str, "value1"
    setprop array, "key1", str

    new str, 'String'
    set str, "value2"
    setprop array, "key2", str

    _dumper( array )

.end
CODE
"VAR1" => ResizablePMCArray (size:2) [
    "test1",
    "test2"
] with-properties: Hash {
    "key1" => "value1",
    "key2" => "value2"
}
OUT

# no. 8
pir_output_is( <<'CODE', <<'OUT', "indent string" );

.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc hash1
    .local pmc hash2
    .local pmc array1
    .local pmc array2
    .local string name
    .local string indent

    new hash1, 'Hash'
    new hash2, 'Hash'
    new array1, 'ResizablePMCArray'
    new array2, 'ResizablePMCArray'

    set hash1["hash2"], hash2
    set hash2["array"], array1
    set hash1["test1"], "test1"
    set hash2["test2"], "test2"
    push array1, 1
    push array1, array2
    push array2, "test"
    setprop hash1, "array2", array2
    name = "hash"
    indent = "|  "
    _dumper( hash1, name, indent )
    _dumper( hash1, name, indent )
    print "name = '"
    print name
    print "'\nindent = '"
    print indent
    print "'\n"
.end
CODE
"hash" => Hash {
|  "hash2" => Hash {
|  |  "array" => ResizablePMCArray (size:2) [
|  |  |  1,
|  |  |  ResizablePMCArray (size:1) [
|  |  |  |  "test"
|  |  |  ]
|  |  ],
|  |  "test2" => "test2"
|  },
|  "test1" => "test1"
} with-properties: Hash {
|  "array2" => \hash["hash2"]["array"][1]
}
"hash" => Hash {
|  "hash2" => Hash {
|  |  "array" => ResizablePMCArray (size:2) [
|  |  |  1,
|  |  |  ResizablePMCArray (size:1) [
|  |  |  |  "test"
|  |  |  ]
|  |  ],
|  |  "test2" => "test2"
|  },
|  "test1" => "test1"
} with-properties: Hash {
|  "array2" => \hash["hash2"]["array"][1]
}
name = 'hash'
indent = '|  '
OUT

# no. 9
pir_output_is( <<'CODE', <<'OUT', "back-referencing properties" );

.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc hash

    new hash, 'Hash'

    set hash["hello"], "world"
    setprop hash, "backref", hash
    _dumper( hash )
.end
CODE
"VAR1" => Hash {
    "hello" => "world"
} with-properties: Hash {
    "backref" => \VAR1
}
OUT

# no. 10
pir_output_is( <<'CODE', <<'OUT', "self-referential properties (1)" );

.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc hash
    .local pmc prop

    new hash, 'Hash'

    set hash["hello"], "world"
    setprop hash, "self", hash
    prophash prop, hash
    setprop hash, "self", prop
    _dumper( hash )
.end
CODE
"VAR1" => Hash {
    "hello" => "world"
} with-properties: Hash {
    "self" => \VAR1.properties()
}
OUT

# no. 11
pir_output_is( <<'CODE', <<'OUT', "self-referential properties (2)" );

.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc array
    .local pmc hash1
    .local pmc hash2
    .local pmc prop

    new array, 'ResizablePMCArray'
    new hash1, 'Hash'
    new hash2, 'Hash'

    set hash1["hello1"], "world1"
    set hash2["hello2"], "world2"
    setprop hash1, "das leben", hash2
    prophash prop, hash1
    set prop["das leben"], "ist schoen"
    setprop hash2, "hash1prop", prop
    push array, hash1
    push array, hash2
    push array, prop
    prophash prop, hash2
    push array, prop
    _dumper( array )
.end
CODE
"VAR1" => ResizablePMCArray (size:4) [
    Hash {
        "hello1" => "world1"
    } with-properties: Hash {
        "das leben" => "ist schoen"
    },
    Hash {
        "hello2" => "world2"
    } with-properties: Hash {
        "hash1prop" => \VAR1[0].properties()
    },
    \VAR1[0].properties(),
    \VAR1[1].properties()
]
OUT

# no. 12
pir_output_is( <<'CODE', <<'OUT', "dumping objects" );

.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc temp
    .local pmc array

    newclass temp, "TestClass"

    new array, 'ResizablePMCArray'
    temp = new "TestClass"
    push array, temp
    $P0 = get_class 'TestClass'
    temp = new $P0
    push array, temp

    _dumper( array )
.end

.namespace ["TestClass"]

.sub __dump :method
    .param pmc dumper
    .param string dname
    .local string subindent
    .local string indent
    .local string name

    (subindent, indent) = dumper."newIndent"()
    print "{\n"

    print subindent
    print "this is\n"

    print subindent
    print "_"
    typeof name, self
    print name
    print "::__dump\n"

    print indent
    print "}"

    dumper."deleteIndent"()

    .begin_return
    .end_return
.end
.namespace []
CODE
"VAR1" => ResizablePMCArray (size:2) [
    PMC 'TestClass' {
        this is
        _TestClass::__dump
    },
    PMC 'TestClass' {
        this is
        _TestClass::__dump
    }
]
OUT

# no. 13
pir_output_is( <<'CODE', <<'OUT', "dumping 'null'" );

.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc array
    .local pmc temp

    new array, 'ResizablePMCArray'

    push array, 0
    push array, "0"

    null temp
    push array, temp

    new temp, 'Integer'
    set temp, 0
    push array, temp

    new temp, 'String'
    set temp, "0"
    push array, temp

    _dumper( array, "array" )
.end
CODE
"array" => ResizablePMCArray (size:5) [
    0,
    "0",
    null,
    0,
    "0"
]
OUT

# no. 14
pir_output_is( << 'CODE', << 'OUT', "dumping strings" );

.sub _test :main
    load_bytecode "dumper.pbc"
    .local pmc array
    array = new 'ResizablePMCArray'

    .local pmc pmc_string, pmc_perl_string
    .local string string_1

    pmc_string = new 'String'
    pmc_string = "This is a String PMC"
    push array, pmc_string

    pmc_perl_string = new 'String'
    pmc_perl_string = "This is a String PMC"
    push array, pmc_perl_string

    string_1 = "This is a String"
    push array, string_1

    _dumper( array, "array of various strings" )
.end
CODE
"array of various strings" => ResizablePMCArray (size:3) [
    "This is a String PMC",
    "This is a String PMC",
    "This is a String"
]
OUT

# no. 15
pir_output_is( <<'CODE', <<'OUT', "dumping complex data in Hash" );

.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc hash1
    .local pmc hash2
    .local pmc hash3
    .local pmc array1

    new hash1, 'Hash'
    new hash2, 'Hash'
    new hash3, 'Hash'
    new array1, 'ResizablePMCArray'

    _dumper( hash1,"hash1" )

    $S0 = "hello"
    $S1 = "world"
    set hash1[$S0], $S1

    _dumper( hash1,"hash1" )

    $S0 = "hello2"
    $S1 = "world2"
    set hash1[$S0], $S1

    _dumper( hash1,"hash1" )

    $S0 = "hash2"
    set hash1[$S0], hash2

    _dumper( hash1,"hash1" )

    $S0 = "hello3"
    $S1 = "world3"
    set hash2[$S0], $S1

    _dumper( hash1,"hash1" )

    $S0 = "name"
    $S1 = "parrot"
    set hash3[$S0], $S1
    $S0 = "is"
    $S1 = "cool"
    set hash3[$S0], $S1

    array1 = 5
    array1[0] = "this"
    array1[1] = "is"
    array1[2] = "a"
    array1[3] = "test"
    array1[4] = hash3

    $S0 = "array1"
    set hash2[$S0], array1

    _dumper( hash1,"hash1" )

.end
CODE
"hash1" => Hash {
}
"hash1" => Hash {
    "hello" => "world"
}
"hash1" => Hash {
    "hello" => "world",
    "hello2" => "world2"
}
"hash1" => Hash {
    "hash2" => Hash {
    },
    "hello" => "world",
    "hello2" => "world2"
}
"hash1" => Hash {
    "hash2" => Hash {
        "hello3" => "world3"
    },
    "hello" => "world",
    "hello2" => "world2"
}
"hash1" => Hash {
    "hash2" => Hash {
        "array1" => ResizablePMCArray (size:5) [
            "this",
            "is",
            "a",
            "test",
            Hash {
                "is" => "cool",
                "name" => "parrot"
            }
        ],
        "hello3" => "world3"
    },
    "hello" => "world",
    "hello2" => "world2"
}
OUT

# no. 16
pir_output_is( <<'CODE', <<'OUTPUT', "dumping Integer PMC" );

.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc int1

    new int1, 'Integer'
    int1 = 12345
    _dumper( int1, "Int:" )
.end
CODE
"Int:" => 12345
OUTPUT

# no. 17
pir_output_is( <<'CODE', <<'OUTPUT', "dumping Float PMC" );

.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc float1

    new float1, 'Float'
    float1 = 12345.678
    _dumper( float1, "Float:" )
.end
CODE
"Float:" => 12345.678
OUTPUT

# no. 18
pir_output_is( <<'CODE', <<'OUTPUT', "dumping ResizablePMCArray PMC" );
.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc array

    new array, 'ResizablePMCArray'
    push array, 12345
    push array, "hello"
    _dumper( array, "array:" )
.end
CODE
"array:" => ResizablePMCArray (size:2) [
    12345,
    "hello"
]
OUTPUT

# no. 19
pir_output_is( <<'CODE', <<'OUTPUT', "dumping ResizableStringArray PMC" );
.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc array

    new array, 'ResizableStringArray'
    push array, "hello"
    push array, "world"
    _dumper( array, "array:" )
.end
CODE
"array:" => ResizableStringArray (size:2) [
    "hello",
    "world"
]
OUTPUT

# no. 20
pir_output_is( <<'CODE', <<'OUTPUT', "dumping ResizableIntegerArray PMC" );
.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc array

    new array, 'ResizableIntegerArray'
    push array, 12345
    push array, 67890
    _dumper( array, "array:" )
.end
CODE
"array:" => ResizableIntegerArray (size:2) [
    12345,
    67890
]
OUTPUT

# no. 21
pir_output_is( <<'CODE', <<'OUTPUT', "dumping ResizableFloatArray PMC" );
.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc array

    new array, 'ResizableFloatArray'
    push array, 123.45
    push array, 67.89
    _dumper( array, "array:" )
.end
CODE
"array:" => ResizableFloatArray (size:2) [
    123.45,
    67.89
]
OUTPUT

# no. 22
pir_output_is( <<'CODE', <<'OUTPUT', "dumping FixedPMCArray PMC" );
.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc array

    new array, 'FixedPMCArray'
    array = 2
    array[0] = 12345
    array[1] = "hello"
    _dumper( array, "array:" )
.end
CODE
"array:" => FixedPMCArray (size:2) [
    12345,
    "hello"
]
OUTPUT

# no. 23
pir_output_is( <<'CODE', <<'OUTPUT', "dumping FixedStringArray PMC" );
.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc array

    new array, 'FixedStringArray'
    array = 2
    array[0] = "hello"
    array[1] = "world"
    _dumper( array, "array:" )
.end
CODE
"array:" => FixedStringArray (size:2) [
    "hello",
    "world"
]
OUTPUT

# no. 24
pir_output_is( <<'CODE', <<'OUTPUT', "dumping FixedIntegerArray PMC" );
.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc array

    new array, 'FixedIntegerArray'
    array = 2
    array[0] = 12345
    array[1] = 67890
    _dumper( array, "array:" )
.end
CODE
"array:" => FixedIntegerArray (size:2) [
    12345,
    67890
]
OUTPUT

# no. 25
pir_output_is( <<'CODE', <<'OUTPUT', "dumping FixedFloatArray PMC" );
.sub test :main
    load_bytecode "dumper.pbc"
    .local pmc array

    new array, 'FixedFloatArray'
    array = 2
    array[0] = 123.45
    array[1] = 67.89
    _dumper( array, "array:" )
.end
CODE
"array:" => FixedFloatArray (size:2) [
    123.45,
    67.89
]
OUTPUT

# no. 26 - Deleted --leo

# no. 27
pir_output_is( <<'CODE', <<'OUTPUT', "custom dumper" );
.sub main :main
    load_bytecode "dumper.pbc"
    .local pmc o, cl
    cl = subclass 'ResizablePMCArray', 'bar'
    o = new cl
    _dumper(o)
.end

.namespace ["bar"]
.sub init :vtable :method
    .local pmc ar
    ar = getattribute self, ['ResizablePMCArray'], 'proxy'
    push ar, 1
    push ar, 2
.end

.sub __dump :method
    .param pmc dumper
    .param string label
    print " __value => {\n"
    .local pmc ar
    ar = getattribute self, ['ResizablePMCArray'], 'proxy'
    dumper.'dump'('attr', ar)
    print "\n}"
.end
.namespace []

CODE
"VAR1" => PMC 'bar'  __value => {
ResizablePMCArray (size:2) [
    1,
    2
]
}
OUTPUT

# pir_output_is(<<'CODE', <<'OUTPUT', "dumping IntegerArray PMC");
# pir_output_is(<<'CODE', <<'OUTPUT', "dumping FloatValArray PMC");

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
