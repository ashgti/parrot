#! perl
# Copyright (C) 2005-2008, Parrot Foundation.
# $Id$

use strict;
use warnings;
use lib qw( . lib ../lib ../../lib );
use Test::More;
use Parrot::Test;
use Parrot::Config;

=head1 NAME

t/dynpmc/gdbmhash.t - test the GDBMHash PMC

=head1 SYNOPSIS

    % prove t/dynpmc/gdbmhash.t

=head1 DESCRIPTION

Tests the C<GDBMHash> PMC.

=cut

if ( $PConfig{has_gdbm} ) {
    plan tests => 13;
}
else {
    plan skip_all => "No gdbm library available";
}

# PIR fragment for setting up a GDBM Hash
my $new_hash_1 = << 'CODE';
.sub test :main
    .local pmc gdbmhash_lib
    gdbmhash_lib = loadlib "gdbmhash"
    .local pmc hash_1
    hash_1 = new "GDBMHash"
CODE

pir_output_is( $new_hash_1 . << 'CODE', << 'OUTPUT', "typeof" );

    .local string type
    type = typeof hash_1
    print type
    print "\n"
.end
CODE
GDBMHash
OUTPUT
unlink('gdbm_hash_1');

pir_output_is( $new_hash_1 . << 'CODE', << 'OUTPUT', "check whether interface is done" );
    .local int bool1
    does bool1, hash_1, "scalar"
    print bool1
    print "\n"
    does bool1, hash_1, "hash"
    print bool1
    print "\n"
    does bool1, hash_1, "no_interface"
    print bool1
    print "\n"
.end
CODE
0
1
0
OUTPUT
unlink('gdbm_hash_1');

pir_output_is( $new_hash_1 . << 'CODE', << 'OUTPUT', "get_integer" );
    .local int hash_size
    hash_size = hash_1
    print "An unitialized GDBMHash has size "
    print hash_size
    print ".\n"

    hash_1 = "gdbm_hash_1"
    hash_size = hash_1
    print "An GDBMHash for a new file has size "
    print hash_size
    print ".\n"

    hash_1["key1"] = "val1"
    hash_size = hash_1
    print "After one assignment GDBMHash has size "
    print hash_size
    print ".\n"

    hash_1["key2"] = "val2"
    hash_size = hash_1
    print "After two assignments GDBMHash has size "
    print hash_size
    print ".\n"

    hash_1["key3"] = "val3"
    hash_1["key4"] = "val4"
    hash_1["key5"] = "val5"
    hash_1["key6"] = "val6"
    hash_1["key7"] = "val7"
    hash_1["key8"] = "val8"
    hash_1["key9"] = "val9"
    hash_1["key10"] = "val10"
    hash_1["key11"] = "val11"
    hash_1["key12"] = "val12"
    hash_1["key13"] = "val13"
    hash_1["key14"] = "val14"
    hash_1["key15"] = "val15"
    hash_size = hash_1
    print "After 15 assignments GDBMHash has size "
    print hash_size
    print ".\n"

    delete hash_1["key7"]
    delete hash_1["key9"]

    hash_size = hash_1
    print "After 15 assignments and 2 deletes GDBMHash has size "
    print hash_size
    print ".\n"


.end
CODE
An unitialized GDBMHash has size 0.
An GDBMHash for a new file has size 0.
After one assignment GDBMHash has size 1.
After two assignments GDBMHash has size 2.
After 15 assignments GDBMHash has size 15.
After 15 assignments and 2 deletes GDBMHash has size 13.
OUTPUT
unlink('gdbm_hash_1');

pir_output_is( $new_hash_1 . << 'CODE', << 'OUTPUT', "get_bool" );
    print "An uninitialized GDBMHash is"
    if hash_1 goto HASH1_IS_1
    print " not"
HASH1_IS_1:
    print ".\n"

    hash_1 = "gdbm_hash_1"
    print "An GDBMHash for a new file is"
    if hash_1 goto HASH1_IS_2
    print " not"
HASH1_IS_2:
    print ".\n"

    hash_1["key1"] = "val1"
    print "After one insert the GDBMHash is"
    if hash_1 goto HASH1_IS_3
    print " not"
HASH1_IS_3:
    print ".\n"

.end
CODE
An uninitialized GDBMHash is not.
An GDBMHash for a new file is not.
After one insert the GDBMHash is.
OUTPUT
unlink('gdbm_hash_1');

# The value is a STRING, with umlaut
# The key used for insertion is a STRING.
pir_output_is( $new_hash_1 . << 'CODE', << 'OUTPUT', "modify an entry" );
    hash_1 = "gdbm_hash_1"

    .local pmc    val_pmc

    hash_1["Schluessel"] = "Wert urspruenglich\n"
    val_pmc = hash_1["Schluessel"]
    print val_pmc
    hash_1["Schluessel"] = "Wert geaendert\n"
    val_pmc = hash_1["Schluessel"]
    print val_pmc
    hash_1["Schluessel"] = "Wert nocheinmal geaendert\n"
    val_pmc = hash_1["Schluessel"]
    print val_pmc
    hash_1["Schluessel"] = "Wert urspruenglich\n"
    val_pmc = hash_1["Schluessel"]
    print val_pmc
.end
CODE
Wert urspruenglich
Wert geaendert
Wert nocheinmal geaendert
Wert urspruenglich
OUTPUT
unlink('gdbm_hash_1');

# The value is a STRING.
# The key used for insertion is a STRING.
pir_output_is( $new_hash_1 . << 'CODE', << 'OUTPUT', "exists_keyed" );
    hash_1 = "gdbm_hash_1"

    hash_1["Schluessel"] = "Wert\n"

    .local int exist_flag
    exist_flag = exists hash_1["a"]
    print exist_flag
    print "\n"
    exist_flag = exists hash_1["Schluessel"]
    print exist_flag
    print "\n"

    .local pmc key_out
    key_out = new 'String'
    key_out = "b"
    exist_flag = exists hash_1[key_out]
    print exist_flag
    print "\n"
    key_out = "Schluessel"
    exist_flag = exists hash_1[key_out]
    print exist_flag
    print "\n"
.end
CODE
0
1
0
1
OUTPUT
unlink('gdbm_hash_1');

# The value is a STRING.
# The key used for insertion is a STRING.
pir_output_is( $new_hash_1 . << 'CODE', << 'OUTPUT', "set STRING with STRING key" );
    hash_1 = "gdbm_hash_1"

    hash_1["Schluessel"] = "Wert\n"

    .local pmc    val_pmc
    .local string val_string

    val_string = hash_1["Schluessel"]
    print val_string
    val_pmc = hash_1["Schluessel"]
    print val_pmc
    .local pmc key_out
    key_out = new 'String'
    key_out = "Schluessel"
    val_string = hash_1[key_out]
    print val_string
    val_pmc = hash_1[key_out]
    print val_pmc
.end
CODE
Wert
Wert
Wert
Wert
OUTPUT
unlink('gdbm_hash_1');

# The value is a STRING.
# The key used for insertion is a PMC.
pir_output_is( $new_hash_1 . << 'CODE', << 'OUTPUT', "set STRING with a PMC key" );
    hash_1 = "gdbm_hash_1"

    .local pmc key_pmc
    key_pmc = new 'String'
    key_pmc = "Schluessel"
    hash_1[key_pmc] = "Wert\n"

    .local pmc    val_pmc
    .local string val_string

    val_string = hash_1["Schluessel"]
    print val_string
    val_pmc = hash_1["Schluessel"]
    print val_pmc
    .local pmc key2
    key2 = new 'String'
    key2 = "Schluessel"
    val_string = hash_1[key2]
    print val_string
    val_pmc = hash_1[key2]
    print val_pmc
.end
CODE
Wert
Wert
Wert
Wert
OUTPUT
unlink('gdbm_hash_1');

# The value is a PMC.
# The key used for insertion is a STRING.
pir_output_is( $new_hash_1 . << 'CODE', << 'OUTPUT', "set PMC with STRING key" );
    hash_1 = "gdbm_hash_1"

    .local pmc val
    val = new 'String'
    val = "Wert\n"
    hash_1["Schluessel"] = val

    .local pmc    val_pmc
    .local string val_string

    val_string = hash_1["Schluessel"]
    print val_string
    val_pmc = hash_1["Schluessel"]
    print val_pmc
    .local pmc key_out
    key_out = new 'String'
    key_out = "Schluessel"
    val_string = hash_1[key_out]
    print val_string
    val_pmc = hash_1[key_out]
    print val_pmc
.end
CODE
Wert
Wert
Wert
Wert
OUTPUT
unlink('gdbm_hash_1');

# The value is a PMC.
# The key used for insertion is a PMC.
pir_output_is( $new_hash_1 . << 'CODE', << 'OUTPUT', "set PMC with a PMC key" );
    hash_1 = "gdbm_hash_1"

    .local pmc val
    val = new 'String'
    val = "Wert\n"
    .local pmc key_pmc
    key_pmc = new 'String'
    key_pmc = "Schluessel"
    hash_1[key_pmc] = val

    .local pmc    val_pmc
    .local string val_string

    val_string = hash_1["Schluessel"]
    print val_string
    val_pmc = hash_1["Schluessel"]
    print val_pmc
    .local pmc key2
    key2 = new 'String'
    key2 = "Schluessel"
    val_string = hash_1[key2]
    print val_string
    val_pmc = hash_1[key2]
    print val_pmc
.end
CODE
Wert
Wert
Wert
Wert
OUTPUT
unlink('gdbm_hash_1');

# The value is an INTVAL
# The key used for insertion is a STRING.
pir_output_is( $new_hash_1 . << 'CODE', << 'OUTPUT', "set INTVAL with STRING key" );
    hash_1 = "gdbm_hash_1"

    hash_1["Schluessel"] = -11012005

    .local pmc    val_pmc
    .local string val_string

    val_string = hash_1["Schluessel"]
    print val_string
    print "\n"
    val_pmc = hash_1["Schluessel"]
    print val_pmc
    print "\n"
    .local pmc key_out
    key_out = new 'String'
    key_out = "Schluessel"
    val_string = hash_1[key_out]
    print val_string
    print "\n"
    val_pmc = hash_1[key_out]
    print val_pmc
    print "\n"
.end
CODE
-11012005
-11012005
-11012005
-11012005
OUTPUT
unlink('gdbm_hash_1');

# The value is a FLOATVAL.
# The key used for insertion is a PMC.
pir_output_is( $new_hash_1 . << 'CODE', << 'OUTPUT', "set FLOATVAL with a PMC key" );
    hash_1 = "gdbm_hash_1"

    .local pmc key_pmc
    key_pmc = new 'String'
    key_pmc = "Schluessel"
    hash_1[key_pmc] = -1101.2005

    .local pmc    val_pmc
    .local string val_string

    val_string = hash_1["Schluessel"]
    print val_string
    print "\n"
    val_pmc = hash_1["Schluessel"]
    print val_pmc
    print "\n"
    .local pmc key2
    key2 = new 'String'
    key2 = "Schluessel"
    val_string = hash_1[key2]
    print val_string
    print "\n"
    val_pmc = hash_1[key2]
    print val_pmc
    print "\n"
.end
CODE
-1101.2005
-1101.2005
-1101.2005
-1101.2005
OUTPUT
unlink('gdbm_hash_1');

pir_output_is( $new_hash_1 . << 'CODE', << 'OUTPUT', "delete_keyed" );
    hash_1 = "gdbm_hash_1"

    .local int exist_flag
    .local int hash_size

    exist_flag = exists hash_1["a"]
    print '"a" exists: '
    print exist_flag
    print "\n"

    hash_1["a"] = "A"
    exist_flag = exists hash_1["a"]
    print '"a" exists: '
    print exist_flag
    print "\n"

    delete hash_1["a"]
    exist_flag = exists hash_1["a"]
    print '"a" exists: '
    print exist_flag
    print "\n"
.end
CODE
"a" exists: 0
"a" exists: 1
"a" exists: 0
OUTPUT
unlink('gdbm_hash_1');

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
