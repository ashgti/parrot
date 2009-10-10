#! perl

# Copyright (C) 2006-2009, Parrot Foundation.
# $Id$

use strict;
use warnings;

use lib qw/. lib/;
use Parrot::Test tests => 15;
use Test::More;

## test description key
# A  args/params        R  results/returns
# C: src     D: dest
# I  IREG    S  SREG    P  PREG    N  NREG
# m  normal  f  flat
# u  unnamed n  named   r  required  o  optional
# m  normal  s  slurpy  E  end

## the section titles below refer to the row/column designation in the state table

## A1
pcc_ok( { args => "'abc'", params => ".param pmc abc" }, 'A C:Sum D:Pur' );

## A2
pcc_ok( { args => "'abc'",        params => ".param pmc abc :slurpy" }, 'A C:Pum D:Purs' );
pcc_ok( { args => "'abc', 'def'", params => ".param pmc abc :slurpy" }, 'A C:Pum D:Purs' );
pcc_ok( { args => "'123'",        params => ".param pmc abc" },         'A C:Pum D:Pur' );

## A3
pcc_ok( { args => "", params => "" } );

## A5
pcc_like(
    { args => "123", params => ".param pmc x :named('x')" },
    '/positional arg found, named arg expected/',
    'positional found, named expected',
    todo => 'cc processor state missing',
);

## E -- not yet working
#pcc_like({args=>"x=>['123';'456'] :flat", params=>".param pmc x"},
#    '/named arg found, positional param expected/',
#    'E1: named found, required param expected',
#);

#pcc_like({args=>"x=>['123';'456'] :flat", params=>".param pmc x"},
#    '/named arg found, required param expected/',
#    'E2: named found, required slurpy param expected',
#);

## G
pcc_error_like(
    { params => ".param pmc abc" },
    '/too few positional arguments: 0 passed, 1 \(or more\) expected/',
    'G1: argument underflow: required param',
);

pcc_like(
    { params => ".param pmc abc :slurpy" },
    '/argument underflow: required slurpy param expected/',
    'G2: argument underflow: required slurpy param',
    todo => 'failing',
);

pcc_ok( { params => ".param pmc abc :optional" }, 'G3: optional param may be empty', );

pcc_ok( { params => ".param pmc abc :optional :slurpy" },
    'G4: optional slurpy param may be empty' );

pcc_error_like(
    { params => ".param pmc abc :named('x')" },
    '/too few named arguments: no argument for required parameter \'x\'/',
    'G5: argument underflow: named required param',
);

pcc_error_like(
    { params => ".param pmc abc :named('x') :slurpy" },
    '/too few named arguments: no argument for required parameter \'x\'/',
    'G6: argument underflow: named required slurpy param',
);

pcc_ok(
    { params => ".param pmc abc :named('x') :optional" },
    'G7: named optional param may be empty',
);

pcc_ok(
    { params => ".param pmc abc :named('x') :optional :slurpy" },
    'G8: named optional slurpy param may be empty',
);

pcc_ok( {}, 'G9: no args, no params' );

exit;

sub pcc_ok {
    my ( $o, $desc, %todo ) = @_;
    my $test = create(%$o);
    pir_output_is( $test, "ok\n", $desc, %todo )
        or !exists $todo{todo} && diag $test;
}

sub pcc_like {
    my ( $o, $exp, $desc, %todo ) = @_;
    my $test = create(%$o);
    pir_output_like( $test, $exp, $desc, %todo )
        or !exists $todo{todo} && diag $test;
}

sub pcc_error_like {
    my ( $o, $exp, $desc, %todo ) = @_;
    my $test = create(%$o);
    pir_error_output_like( $test, $exp, $desc, %todo )
        or !exists $todo{todo} && diag $test;
}

sub create {
    my %o = @_;
    $o{$_} ||= '' for qw/args params returns results/;

    $o{returns} =
        defined $o{returns}
        ? "($o{returns}) = "
        : '';

    my $t = <<"TEST";
.sub 'main' :main
    $o{returns} 'foo'($o{args})
    say 'ok'
.end

.sub 'foo'
$o{params}
$o{results}
.end
TEST

    #    diag($t);
    return $t;
}

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
