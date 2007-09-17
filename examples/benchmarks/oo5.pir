# $Id$

.sub bench :main
    .local pmc cl
    cl = newclass "Foo"
    addattribute cl, ".i"
    addattribute cl, ".j"

    .local int typ
    .local int i
    i = 1
    typ = find_type "Foo"
    .local pmc o
    o = new  typ
loop:
    $P4 = o."i"()
    .local pmc x
    x = new 'Integer'
    assign x, $P4

    $P5 = o."j"()
    .local pmc y
    y = new 'Integer'
    assign y, $P5
    inc i
    if i <= 500000 goto loop

    $P2 = o."i"()
    print $P2
    print "\n"
    end
.end

.namespace ["Foo"]
.sub __init :method
    .local int ofs
    ofs = classoffset self, "Foo"
    new $P10, 'Integer'
    set $P10, 10
    setattribute self, ofs, $P10
    inc ofs
    new $P10, 'Integer'
    set $P10, 20
    setattribute self, ofs, $P10
.end

.pcc_sub i :method
    .local int ofs
    ofs = classoffset self, "Foo"
    .local pmc r
    r = getattribute self, ofs
    .pcc_begin_return
    .return r
    .pcc_end_return
.end

.pcc_sub j :method
    .local int ofs
    ofs = classoffset self, "Foo"
    inc ofs
    .local pmc r
    r = getattribute self, ofs
    .pcc_begin_return
    .return r
    .pcc_end_return
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
