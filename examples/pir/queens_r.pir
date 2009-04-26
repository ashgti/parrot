# Copyright (C) 2005-2009, Parrot Foundation.
# $Id$

# recursive queens placement
# prints all 92 solutions in chess notation
# by leo

.sub main :main
    .local pmc queen_row, queen_state
    queen_row = new 'ResizableIntegerArray'
    queen_state = new 'ResizableIntegerArray'
    solve(0, queen_row, queen_state)
.end

.sub solve
    .param int row
    .param pmc queen_row
    .param pmc queen_state
    .local int col, is_free
    col = 0
loop:
    is_free = check_free(col, row, queen_state)
    unless is_free goto not_free
	place_queen(col, row, queen_row, queen_state)
	if row < 7 goto not_fin
	    print_solution(queen_row)
	goto fin
not_fin:
	$I0 = row + 1
	solve($I0, queen_row, queen_state)
fin:
	remove_queen(col, row, queen_row, queen_state)
not_free:
    inc col
    if col < 8 goto loop
.end

# queen_state
# 0..7 columns
# 8..14 first diagonal  := col + row
# 24..  second diagonal := col - row
.sub check_free
    .param int col
    .param int row
    .param pmc queen_state
    $I0 = queen_state[col]
    if $I0 goto ret_0
    $I1 = col + row
    $I1 += 8
    $I0 = queen_state[$I1]
    if $I0 goto ret_0
    $I1 = col - row
    $I1 += 32
    $I0 = queen_state[$I1]
    if $I0 goto ret_0
    .return(1)
ret_0:
    .return(0)
.end

.sub place_queen
    .param int col
    .param int row
    .param pmc queen_row
    .param pmc queen_state
    $I0 = row + 1
    set_queen(col, row, queen_row, queen_state, $I0)
.end

.sub remove_queen
    .param int col
    .param int row
    .param pmc queen_row
    .param pmc queen_state
    set_queen(col, row, queen_row, queen_state, 0)
.end

.sub set_queen
    .param int col
    .param int row
    .param pmc queen_row
    .param pmc queen_state
    .param int val
    queen_row[col] = val
    queen_state[col] = val
    $I1 = col + row
    $I1 += 8
    queen_state[$I1] = val
    $I1 = col - row
    $I1 += 32
    queen_state[$I1] = val
.end

.sub print_solution
    .param pmc queen_row
    .local int c, r
    c = 0
loop:
	$I0 = 65 + c
	$S0 = chr $I0
	print $S0
	r = queen_row[c]
	r += 48
	$S0 = chr r
	print $S0
	print " "
	inc c
    if c < 8 goto loop
    print "\n"
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
