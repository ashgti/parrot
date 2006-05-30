=head1 TITLE

block.pir - a tetris block class

=head1 SYNOPSIS

    # create a new block
    find_type $I0, "Tetris::Block"
    new block, $I0
    block."init"( board )

    # let the block fall down fast
    block."fall"()

=head1 CLASS INFORMATION

This class is derived from the BlockData class and
is not subclassed further.

=cut

.namespace ["Tetris::Block"]

.const int xPos      = 0
.const int xPosDiff  = 1
.const int yPos      = 2
.const int Fall      = 3
.const int Board     = 4

.sub __onload :load
    find_type $I0, "Tetris::Block"
    if $I0 > 1 goto END
    load_bytecode "examples/sdl/tetris/blockdata.pir"
    getclass $P0, "Tetris::BlockData"
    subclass $P0, $P0, "Tetris::Block"

    addattribute $P0, "xpos"
    addattribute $P0, "xposdiff"
    addattribute $P0, "ypos"
    addattribute $P0, "fall"
    addattribute $P0, "board"

    # set the BUILD method name
    $P1 = new .String
    $P1 = 'BUILD'
    setprop $P0, 'BUILD', $P1
END:
.end

=head1 METHODS

=over 4

=item block = new block_class_id, board

Constructs a block object.

=over 4

=item parameter C<board>

The board the new block will belong to.

=back

=cut

.sub BUILD method
    .param pmc board
    .local pmc prop
    .local int id

    classoffset id, self, "Tetris::Block"
    
    #
    # set some properties
    #

    # xpos
    new prop, .Integer
    set prop, 0
    setattribute self, id, prop
    inc id
    
    # xposdiff
    new prop, .Integer
    set prop, 0
    setattribute self, id, prop
    inc id
    
    # ypos
    new prop, .Integer
    set prop, 0
    setattribute self, id, prop
    inc id

    # fall
    new prop, .Integer
    set prop, 0
    setattribute self, id, prop
    inc id
    
    # set the board the block blongs to
    setattribute self, id, board

    # return the block
    .pcc_begin_return
    .return self
    .pcc_end_return
.end

=item block."fall"()

Lets the block fall down fast.

This method returns nothing.

=cut

.sub fall method
    classoffset $I0, self, "Tetris::Block"
    add $I0, Fall
    getattribute $P0, self, $I0
    $P0 = 1
.end

=item falling = block."falling"()

Returns whether the block is falling down fast.

=cut

.sub falling method
    classoffset $I0, self, "Tetris::Block"
    add $I0, Fall
    getattribute $P0, self, $I0
    $I0 = $P0
        
    .pcc_begin_return
    .return $I0
    .pcc_end_return
.end

=item (xpos, ypos) = block."position"()

Returns the x and y position of the block.

=cut

.sub position method
    .local int xpos
    .local int ypos
    .local int vfree
    .local pmc xposP
    .local pmc xposdiffP
    .local pmc yposP
    
    classoffset $I0, self, "Tetris::Block"
    add $I0, xPos
    getattribute xposP, self, $I0
    sub $I0, xPos

    add $I0, xPosDiff
    getattribute xposdiffP, self, $I0
    sub $I0, xPosDiff

    add $I0, yPos
    getattribute yposP, self, $I0
    sub $I0, yPos
    
    # calculate the ypos
    $I0 = self."vfree"()
    ypos = yposP
    sub ypos, $I0

    # calculate the xpos
    xpos = xposP
    $I0 = xposdiffP
    add xpos, $I0

    .pcc_begin_return
    .return xpos
    .return ypos
    .pcc_end_return
.end

=item success = block."rotate"( dir )

Rotates the block.

=over 4

=item parameter C<dir>

+1 = rotate clockwise

-1 = rotate counterclockwise

=back

Returns 1 if the rotation was possible, 0 otherwise.

=cut

.sub rotate method
    .param int val
    .local pmc board
    .local pmc temp
    .local pmc olddata
    .local int size
    .local int i
    .local int j
    .local int ret
    
    size = self."size"()
    ret = self."validPosition"()
    
    unless ret goto END

    # rotate the block
    olddata = self."rotateData"( val )

    # is the position of the rotated block valid?
    ret = self."validPosition"()
    if ret goto END
    print "(1) valid:"
    print i
    print "\n"

    # no => can i move the block?
    i = self."hfree"()
    classoffset $I0, self, "Tetris::Block"
    add $I0, xPosDiff
    getattribute temp, self, $I0
    set temp, i

    # is the position of the rotated block still valid?
    ret = self."validPosition"()
    print "(2) valid:"
    print ret
    print "\n"
    if ret goto END

    # no => restore the old block
    assign self, olddata

END:
    .pcc_begin_return
    .return ret
    .pcc_end_return
.end

=item board = block."board"()

Returns the board associated with this block.

=cut

.sub board method
    # get the board
    classoffset $I0, self, "Tetris::Block"
    add $I0, Board
    getattribute $P0, self, $I0
    
    .pcc_begin_return
    .return $P0
    .pcc_end_return
.end

=item block."setBoard"( board )

Set the board associated with this block.

=cut

.sub setBoard method
    .param pmc board
    
    # get the board
    classoffset $I0, self, "Tetris::Block"
    add $I0, Board
    setattribute self, $I0, board
.end

=item valid = block."validPosition"()

Checks if the block's position is valid.

It is valid, if the corresponding positions
of each entry != 0 inside data array
on the board is within the board's boundary,
and if the board is free at these positions.

Returns 1 if position is valid, 0 otherwise.

=cut

.sub validPosition method
    .local pmc board
    .local pmc temp
    .local int xpos
    .local int ypos
    .local int width
    .local int height
    .local int size
    .local int x
    .local int y
    .local int i
    .local int xp
    .local int yp

    # get the board
    board = self."board"()

    # get x/y position
    (xpos, ypos) = self."position"()

    # get board width
    (width, height) = board."dimensions"()

    # get block size
    size = self."size"()
    set y, -1
yLOOP:
    inc y
    set x, 0
    if y >= size goto END
xLOOP:
    set i, y
    mul i, size
    add i, x

    set i, self[i]
    unless i goto SKIP

    set xp, xpos
    add xp, x
    if xp < 0 goto ERROR
    if xp >= width goto ERROR

    set yp, ypos
    add yp, y
    if yp < 0 goto ERROR
    if yp >= height goto ERROR
    
    i = board."offset"( xp, yp )
    i = board[i]
    if i goto ERROR
    
SKIP:
    inc x
    if x >= size goto yLOOP
    branch xLOOP

END:

    .pcc_begin_return
    .return 1
    .pcc_end_return

ERROR:
    .pcc_begin_return
    .return 0
    .pcc_end_return
.end

=item success = block."move"( xval, yval )

Moves the block. It does not lock the block onto the
board in any case, use C<Board::lockBlock()> if you
want this.

This method returns 1 on success, 0 otherwise.

=cut

.sub move method
    .param int xval
    .param int yval
    .local pmc xpos
    .local pmc ypos
    .local int xold
    .local int yold
    .local int valid

    classoffset $I0, self, "Tetris::Block"
    add $I0, xPos
    getattribute xpos, self, $I0
    sub $I0, xPos

    add $I0, yPos
    getattribute ypos, self, $I0
    
    set xold, xpos
    set yold, ypos
    add xpos, xval
    add ypos, yval

    valid = self."validPosition"()
    if valid goto BLOCK_MOVE_DONE
    set xpos, xold
    set ypos, yold
BLOCK_MOVE_DONE:
    .pcc_begin_return
    .return valid
    .pcc_end_return
.end

=item block."draw"( surface, xshift, yshift, extent )

XXX continue

=over 4

=item parameter C<surface>

The surface to draw to.

=item parameter C<xshift>

XXX continue

=item parameter C<yshift>

XXX continue

=item parameter C<extent>

The size of the block's subblocks, in pixels.

=back

This method returns nothing.

=cut

.sub draw method
    .param pmc surface
    .param int xshift
    .param int yshift
    .param int extent
    .local pmc temp
    .local pmc temp2
    .local int xshift
    .local int yshift
    .local pmc color
    .local int xp
    .local int yp
    .local int xe
    .local int ye
    .local int xs
    .local int i
    .local int size
    .local pmc app
    .local pmc rect

    rect = new .Hash
    
    # get the application
    app = self."application"()
    
    # get the block's width/height
    size = self."size"()
    # get the block's id
    $I0 = self."id"()
    # use it as an color index; convert it to a bright color
    add $I0, 9 # +1 = make first block id blue; +8 = make it bright
    # lookup the color value
    color = app."color"( $I0 )
    $I0 = typeof color
    if $I0 == .Undef goto END

    # get the x/y position in board coordinates
    (xp, yp) = self."position"()

    # calculate the absolute x/y end pos
    set xe, xp
    set ye, yp
    add xe, size
    add ye, size
    mul xe, extent
    mul ye, extent
    add xe, xshift
    add ye, yshift

    # calculate the absolute start pos
    mul xp, extent
    mul yp, extent
    add xp, xshift
    add yp, yshift
    
    i = app."flag"( "show blocksize" )
    unless i goto NO_BLOCKSIZE
    set i, size
    mul i, extent
    
    rect["x"] = xp
    rect["y"] = yp
    rect["width"] = i
    rect["height"] = i
    find_type $I0, "SDL::Rect"
    temp = new $I0, rect
    
    surface."fill_rect"( temp, color )
    sub i, 2
    inc xp
    inc yp

    rect["x"] = xp
    rect["y"] = yp
    rect["width"] = i
    rect["height"] = i
    find_type $I0, "SDL::Rect"
    temp = new $I0, rect

    # lookup the color value
    $P0 = app."color"( 0 )
    surface."fill_rect"( temp, $P0 )
    dec xp
    dec yp
NO_BLOCKSIZE:

    # set the x start pos
    set xs, xp

    sub yp, extent
    set i, 0
LOOPy:
    add yp, extent
    set xp, xs
    if yp >= ye goto END
LOOPx:

    $I0 = self[i]
    if $I0 == 0 goto SKIP
    
    dec extent
    rect["x"] = xp
    rect["y"] = yp
    rect["width"] = extent
    rect["height"] = extent
    find_type $I0, "SDL::Rect"
    temp = new $I0, rect
    inc extent
    surface."fill_rect"( temp, color )
SKIP:

    inc i
    add xp, extent
    if xp >= xe goto LOOPy
    branch LOOPx
END:

    .pcc_begin_return
    .pcc_end_return
.end

=item redraw = block."timer"()

Has to be called at frequent intervals.

Returns 1 if the screen has to be redrawn.

=cut

.sub timer method
    .local pmc board
    .local pmc app
    .local num curtime
    .local num diff
    .local pmc temp
    .local int fall
    .local int redraw

    redraw = 0
    app = self."application"()
    board = self."board"()
    
    fall = self."falling"()
    if fall goto FALL

    diff = board."nextFallTime"()
    time curtime
    sub diff, curtime
    if diff >= 0.0 goto NOFALL

FALL:
    redraw = 1
    # next fall time
    diff = board."fallInterval"()
    add curtime, diff
    board."setNextFallTime"( curtime )
    
    # let the block fall down one unit
    self."moveDown"()
    
    # is it not falling fast?
    unless fall goto NOFALL
    
    # let the block fall down fast
AGAIN:
    app."drawScreen"( 0 )
    fall = self."moveDown"()
    if fall goto AGAIN
    redraw = fall

NOFALL:
    .pcc_begin_return
    .return redraw
    .pcc_end_return
.end

=item success = block."moveDown"()

Moves the block one unit downwards.

This method returns 1 on success, 0 if the block
has been locked onto the board, in which case
the C<next block> has been prepared.

=cut

.sub moveDown method
    .local pmc board
    .local int okay

    okay = self."move"( 0, +1 )
    if okay goto BLOCK_MOVEDOWN_OKAY
    
    # get the board
    board = self."board"()
    # lock the block
    board."lockBlock"( self )
    # XXX: returns the number of lines removed, needed for scoring
    board."removeFullLines"()
    # activate the next block
    board."nextBlock"()

BLOCK_MOVEDOWN_OKAY:
    .pcc_begin_return
    .return okay
    .pcc_end_return
.end

=item app = block."application"()

Returns the application object assosicated with the block's board.

=cut

.sub application method
    .local pmc temp

    temp = self."board"()
    temp = temp."application"()

    .pcc_begin_return
    .return temp
    .pcc_end_return
.end

.sub setXPosition method
    .param int pos
    
    getprop $P0, "xpos", self
    $P0 = pos
.end

=back

=head1 AUTHOR

Jens Rieks E<lt>parrot at jensbeimsurfen dot deE<gt> is the author
and maintainer.
Please send patches and suggestions to the Perl 6 Internals mailing list.

=head1 COPYRIGHT

Copyright (C) 2004, The Perl Foundation.

=cut
