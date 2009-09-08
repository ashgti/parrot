#!parrot
# Copyright (C) 2006-2008, Parrot Foundation.
# $Id$

=head1 NAME

t/pmc/parrotobject.t - test the Object PMC

=head1 SYNOPSIS

    % prove t/pmc/parrotobject.t

=head1 DESCRIPTION

Tests the Object PMC.

=cut

.sub main
    .include 'test_more.pir'
    plan(20)

    test_new_object_exception()
    test_vtable_override()
    test_vtable_override2()
    test_bad_vtable_name()
    # causes memory errors
    # test_bad_vtable_name_tt992()

    # attempt to access code outside of current code segment
    # test_vtable_init(_tt993)
.end

.sub test_new_object_exception
    throws_like(<<'CODE',':s Object must be created by a class','Object must be created by a class')
    .sub main
        new $P0, ['Object']
    .end
CODE
.end

.sub test_vtable_override
   $P0 = newclass [ "Test1" ]
   $P1 = new [ "Test1" ]
   $I1 = $P1[11]
   is($I1,42,'vtable override')
.end

.sub test_vtable_override2
    $P0 = newclass [ "Test2" ]
    $P1 = new [ "Test2" ]
    $S1 = $P1[11]
    is($S1,'monkey','vtable override with :vtable("...") syntax')
.end

.sub test_bad_vtable_name_tt992
    throws_like(<<'CODE',':s not_in_the_vtable. is not a v\-table method','bad vtable name')
.namespace [ "Test" ]
.sub monkey :method :vtable("not_in_the_vtable")
    .param int key
    .return("monkey")
.end
CODE
.end

.sub test_bad_vtable_name
    throws_like(<<'CODE',':s not_in_the_vtable. is not a v\-table method','bad vtable name')
.sub monkey :method :vtable("not_in_the_vtable")
    .param int key
    .return("monkey")
.end
CODE
.end

.sub test_vtable_init_tt993
    throws_like(<<'CODE','init','vtable init happens at the correct time')
    .sub main :main
        $P0 = newclass [ "Test" ]
        $P1 = new [ "Test" ]
        die '42'
    .end

    .namespace [ "Test" ]

    .sub init :method :vtable
        die 'init'
    .end
CODE
.end

.namespace [ "Test1" ]

.sub get_integer_keyed_int :method :vtable
   .param int key
   .return(42)
.end

.namespace [ "Test2" ]

.sub monkey :method :vtable("get_string_keyed_int")
    .param int key
    .return("monkey")
.end
