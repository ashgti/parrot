# $Id$

# ** string mode

#
# string mode.
# befunge stack:
#   before:     ...
#   after:      ... c
# c = ord(current char)
.sub io__push_char
    $P0 = get_global "status"
    $P1 = get_global "stack"

    $I0 = $P0["val"]
    push $P1, $I0

    set_global "stack", $P1
.end


#
# io__put_value()
#
# put a value in the playfield.
# befunge stack:
#   before:     ... i x y
#   after:      ...
# value_at(x,y) = i
#
.sub "io__put_value"
    .local int x, y, v
    y = stack__pop()
    x = stack__pop()
    v = stack__pop()

    $P0 = get_global "playfield"
    $P0[y;x] = v
    set_global"playfield", $P0
.end

=pod

IO_PUSH_CHAR:

# Input integer.
# Befunge stack:
#   before:     ...
#   after:      ... i
# i = readint()
IO_INPUT_INT:
    save S2
    restore S2
    length I10, S2
    gt I10, 0, IO_INPUT_INT_PARSE_INPUT
    getstdin P15
    readline S2, P15
    length I10, S2
IO_INPUT_INT_PARSE_INPUT:
    set I11, 0
    set S10, ""
IO_INPUT_INT_NEXT_CHAR:
    substr S11, S2, I11, 1
    lt S11, "0", IO_INPUT_INT_NAN
    gt S11, "9", IO_INPUT_INT_NAN
    concat S10, S10, S11
    inc I11
    lt I11, I10, IO_INPUT_INT_NEXT_CHAR
    set I10, 0
    set I11, 0
IO_INPUT_INT_NAN:
    substr S2, S2, I11, I10
    push P2, S10
    branch MOVE_PC

# Input character.
# Befunge stack:
#   before:     ...
#   after:      ... c
# c = getchar()
IO_INPUT_CHAR:
    save S2
    restore S2
    length I10, S2
    gt I10, 0, IO_INPUT_CHAR_SUBSTR
    getstdin P15
    readline S2, P15
IO_INPUT_CHAR_SUBSTR:
    substr S10, S2, 0, 1
    length I10, S2
    substr S2, S2, 1, I10
    ord I10, S10
    push P2, I10
    save S2
    restore S2
    branch MOVE_PC

# Output integer.
# Befunge stack:
#   before:     ... i
#   after:      ...
# writeint(i)
IO_OUTPUT_INT:
    set I10, P2
    unless I10, IO_OUTPUT_INT_POP_1
    pop I10, P2
IO_OUTPUT_INT_POP_1:
    print I10
    print " "
    branch MOVE_PC

# Output character.
# Befunge stack:
#   before:     ... i
#   after:      ...
# writechar( chr(i) )
IO_OUTPUT_CHAR:
    set I10, P2
    unless I10, IO_OUTPUT_CHAR_POP_1
    pop I10, P2
IO_OUTPUT_CHAR_POP_1:
    chr S10, I10
    print S10
    branch MOVE_PC

# Get a value from playfield.
# Befunge stack:
#   before:     ... x y
#   after:      ... i
# i = value_at(x,y)
IO_GET_VALUE:
    set I11, P2
    unless I11, IO_GET_VALUE_POP_1
    pop I11, P2
IO_GET_VALUE_POP_1:
    set I10, P2
    unless I10, IO_GET_VALUE_POP_2
    pop I10, P2
IO_GET_VALUE_POP_2:
    set I12, P1[I11;I10]
    push P2, I12
    branch MOVE_PC


=cut

########################################################################
# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:

