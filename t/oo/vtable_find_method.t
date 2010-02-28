#! parrot
# Copyright (C) 2007-2009, Parrot Foundation.
# $Id: vtableoverride.t 36833 2009-02-17 20:09:26Z allison $

=head1 NAME

t/oo/vtable_find_method.t - test various aspects of the find_method vtable

=head1 SYNOPSIS

    % prove t/oo/vtable_find_method.t

=head1 DESCRIPTION

Tests the behavior of the find_method vtable, especially when overriden and
subclassed.

=cut

.sub main :main
    .include 'test_more.pir'
    plan(2)

    override_fails()
.end

.sub override_fails
    $P0 = newclass "Test"
    $P1 = subclass $P0, "MyTest"
    $P2 = new $P1
    $P3 = find_method $P2, "foo"
    $I0 = isnull $P3
    is($I0, 0, "can find foo in MyTest")
    $P4 = find_method $P2, "bar"
    $I0 = isnull $P4
    is($I0, 0, "can find bar in MyTest from parent")
.end

.namespace ["Test"]

.sub "bar" :method
    .return("bar")
.end

.namespace ["MyTest"]

.sub 'find_method' :vtable
    .param string name
    if name == "foo" goto have_foo
    $P0 = null
    .return($P0)
  have_foo:
    .const 'Sub' foo = 'meth_foo'
    .return(foo)
.end

.sub 'meth_foo' :method
    .return("foo")
.end
