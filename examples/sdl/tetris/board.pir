=head1 TITLE

board.pir - a tetris board class.

=head1 SYNOPSIS

    # get the application singleton
    app = global "Tetris::App"

    # create a new board
    find_type $I0, "Tetris::Board"
    board = new $I0, app

    # let the current block of the board fall down fast
    board."fall"()

=cut

.include "library/dumper.pir"

.namespace ["Tetris::Board"]

.sub __onload :load
    find_type $I0, "Tetris::Board"
    if $I0 > 1 goto END
    load_bytecode "examples/sdl/tetris/boarddata.pir"
    load_bytecode "examples/sdl/tetris/blocks.pir"
    getclass $P0, "Tetris::BoardData"
    subclass $P0, $P0, "Tetris::Board"

    # set the BUILD method name
    $P1 = new .String
    $P1 = 'BUILD'
    setprop $P0, 'BUILD', $P1
END:
.end

=head1 METHODS

A Board object has the folloging methods:

=over 4

=item board."BUILD"( app )

Initializes a new Board object. BUILD is called automatically by C<new>.

=over 4

=item parameter C<app>

The application object this board will belong to.

=back

Returns the created board.

=cut

.sub BUILD method
    .param pmc app
    .local pmc temp
    .local int w
    .local int h
    .local int blocksize
    .local int i

    # blocksize to use
    set blocksize, 20
    
    # field size to use
    set w, 10
    set h, 20

    # create the board data
    self."init"( w, h )
    self."fill"( 0 )

    # setup the cache
    find_type $I0, "Tetris::BoardData"
    new temp, $I0
    temp."init"( w, h )
    temp."fill"( -1 )
    setprop self, "cache", temp

    #
    # setup some properties
    #
    
    # set the application object
    setprop self, "app", app
    # XXX: register the board and save its ID
    i = app."registerBoard"( self )
    
    # set the 'next fall time'
    new temp, .Float
    set temp, 0
    setprop self, "NextFallTime", temp
    
    # set the 'fall interval'
    new temp, .Float
    set temp, 0.5
    setprop self, "FallInterval", temp
    
    # xpos
    new temp, .Integer
    set temp, 10
    setprop self, "xpos", temp

    # ypos
    new temp, .Integer
    set temp, 10
    setprop self, "ypos", temp
    
    # set the blocksize
    new temp, .Integer
    set temp, blocksize
    setprop self, "blocksize", temp
    
    # create the preview block
    self."nextBlock"()

    # make the first block active
    self."nextBlock"()

print "BUILD done\n"
    .pcc_begin_return
    .pcc_end_return
.end

=item block = board."application"()

Returns the application object to which this board belongs to.

=cut

.sub application method
    .local pmc app

    getprop app, "app", self
    
    .pcc_begin_return
    .return app
    .pcc_end_return
.end

=item block = board."getNextBlock"()

Returns the current C<next block> that is shown
in the preview window.

=cut

.sub getNextBlock method
    .local pmc block

    getprop block, "nextblock", self
    
    # return the block
    .pcc_begin_return
    .return block
    .pcc_end_return
.end

=item board."setNextBlock"( block )

Sets the C<next block> that is shown
in the preview window.

This method returns nothing.

=cut

.sub setNextBlock method
    .param pmc block

    setprop self, "nextblock", block
    
    # return the block
    .pcc_begin_return
    .pcc_end_return
.end

=item board."setCurrentBlock"( block )

...

=cut

.sub setCurrentBlock method
    .param pmc block

    setprop self, "block", block
.end

.sub newCurrentBlock method
    .param pmc block
    .local int width
    .local int size
    
    $I0 = typeof block
    if $I0 == .Undef goto END
    
    # assign the board to the block
    block."setBoard"( self )

    #
    # set the block's x position to the center of the board
    # xpos = (self.width - block.size) / 2
    # 
    # get the board's width
    width = self."width"()
    # get the block's size
    size = block."size"()
    # (width - size) / 2
    sub width, size
    div width, 2
    # set the block's x position
    block."setXPosition"( width )

    # set the block as the new currently falling block
    self."setCurrentBlock"( block )
END:
.end

=item block = board."nextBlock"( [id] )

Activates the C<next block> and creates a new C<next block>.
The old current block is just overwritten, and not locked
onto the board.
Use C<_Board::lockBlock()> for this.

=over 4

=item parameter C<id> B<(optional)>

The block id to create.

=back

This method returns the block that is now falling down.

=cut

.sub nextBlock method
    .param int id
    .local pmc block
    .local pmc temp
    
    # check the number of INT args
    if argcI >= 1 goto SKIP_ID
    # no INT arg => use a random next block
    set id, -1
SKIP_ID:

    # get the 'next block' and store it as the current one
    getprop block, "nextblock", self
    
    # create a new 'next block'
    $I0 = self."blockID"( id )
    temp = new $I0, self
    setprop self, "nextblock", temp

    # new currently falling block created, activate it
    self."newCurrentBlock"( block )
    
END:
    # return the block
    .pcc_begin_return
    .return block
    .pcc_end_return
.end

=item board."fall"()

Lets the current block fall down fast.

This method returns nothing.

=cut

.sub fall method
    .local pmc block
    
    getprop block, "block", self
    block."fall"()
    
    .pcc_begin_return
    .pcc_end_return
.end

=item falling = board."falling"()

Returns 1 if the current block is falling down fast, 0 otherwise.

=cut

.sub falling method
    .param pmc self
    .local pmc block
    .local int ret
    
    getprop block, "block", self
    ret = block."falling"()
    
    .pcc_begin_return
    .return ret
    .pcc_end_return
.end

=item block = board."currentBlock"()

Returns the currently falling block.

=cut

.sub currentBlock method
    .local pmc block
    
    getprop block, "block", self

    .pcc_begin_return
    .return block
    .pcc_end_return
.end

=item size = board."blockSize"()

Returns the size of blocks on the board.

B<NOTE:> The size is the width and height of blocks,
measured in pixels.

=cut

.sub blockSize method
    .local pmc temp
    .local int i
        
    getprop temp, "blocksize", self
    set i, temp
    
    .pcc_begin_return
    .return i
    .pcc_end_return
.end

=item offset = board."offset"( x, y )

Calculates the offset of the specified position inside
the boarddata array.

=over 4

=item parameter C<x>

The vertical position on the board.

=item parameter C<x>

The horizontal position on the board.

=back

Returns the calculated offset.

=cut

.sub offset method
    .param int x
    .param int y
    .local int offset
    .local pmc temp
    .local pmc i
    
    offset = self."width"()
    mul offset, y
    add offset, x
    
    .pcc_begin_return
    .return offset
    .pcc_end_return
.end

=item board."tranlateBlockData"( block, data, color )

Translates the position of each occupied block's data entry
into corresponding board data positions and sets these entries
to the specified color value.

=over 4

=item parameter C<block>

The block whose data should be used.

=item parameter C<data>

The board data array to use. Can be a cache, for example.

=item parameter C<color>

The color value to write to the data array when the corresponding
block data entry is not empty.

=back

This method returns nothing.

=cut

.sub translateBlockData method
    .param pmc block
    .param pmc data
    .param int value
    .local int val
    .local int x
    .local int y
    .local int xp
    .local int yp
    .local int s
    .local int offset
    .local int i
        
    s = block."size"()
    
    (xp, yp) = block."position"()

    set i, 0
    set y, -1
TRANS_LOOPy:
    inc y
    if y >= s goto TRANS_LOOPend
    set x, 0
TRANS_LOOPx:
    $I0 = block[i]
    unless $I0 goto TRANS_SKIP
    add xp, x
    add yp, y
    offset = self."offset"( xp, yp )
    sub xp, x
    sub yp, y
    
    # set the value at the calculated offset to the specified color value
    set data[offset], value

TRANS_SKIP:
    inc x
    inc i
    if x >= s goto TRANS_LOOPy
    branch TRANS_LOOPx
    
TRANS_LOOPend:
    .pcc_begin_return
    .pcc_end_return
.end

=item board."lockBlock"( block )

Locks a block onto the board.

=over 4

=item parameter C<block>

The block whose data should be set on the block.

=back

This method returns nothing.

=cut

.sub lockBlock method
    .param pmc block
    .local int value

    $S0 = typeof block
    value = block."id"()
    inc value # make first id (0) blue
    self."translateBlockData"( block, self, value )
    
    .pcc_begin_return
    .pcc_end_return
.end

=item board."removeLine"( line )

Removes the specified line.

This method returns nothing.

=cut

.sub removeLine method
    .param int line
    .local int w
    .local int src
    .local int dst
    .local int temp
    
    w = self."width"()
    
    temp = line
    dst = self."offset"( w, temp )
    dec temp
    src = self."offset"( w, temp )

    if dst > src goto REMOVE
    print "\n\nFATAL ERROR: dst <= src\n'"
    print dst
    print "' <= '"
    print src
    print "\n"
    end

REMOVE:
    temp = self[src]
    self[dst] = temp
    dec src
    dec dst
    if src >= 0 goto REMOVE
    # erase the top line
ERASE:
    if dst == 0 goto REMOVELINE_END
    self[dst] = 0
    dec dst
    branch ERASE
REMOVELINE_END:
    .pcc_begin_return
    .pcc_end_return
.end

=item lines = board."removeFullLines"()

Removes all full lines.

This method returns the number of lines removed.

=cut

.sub removeFullLines method
    .local pmc temp
    .local int w
    .local int h
    .local int line
    .local int pos
    .local int val
    .local int hits
    .local int offset
    .local int offset2
    
    (w, h) = self."dimensions"()
    
    set hits, 0
    set line, -1
RFL_NEXTLINE:
    inc line
    if line >= h goto RFL_LOOPend
    set pos, 0
RFL_LINE:
    offset = self."offset"( pos, line )
    val = self[offset]

    unless val goto RFL_NEXTLINE
    inc pos
    if pos >= w goto RFL_HIT
    branch RFL_LINE
RFL_HIT:
    self."removeLine"( line )
    inc hits
    branch RFL_NEXTLINE
RFL_LOOPend:
    # XXX: remove debug code
    unless hits goto RFL_NOHITS
    print "hits: "
    print hits
    print "\n"
    if hits <= 4 goto RFL_NOHITS
    print "ERROR!\n"
    _dumper( "board", self )
    sleep 10
    print "Continuing...\n"
RFL_NOHITS:
    .pcc_begin_return
    .return hits
    .pcc_end_return
.end

=item board."draw"( surface, full )

Draws the board onto the specified surface.

=over 4

=item parameter C<surface>

the surface to draw to

=item parameter C<full>

0 = update only modified parts (uses a draw cache)

1 = draw everything

=back

This method returns nothing.

=cut

.sub draw method
    .param pmc surface
    .param int full
    .local pmc temp
    .local pmc palette
    .local pmc cache
    .local int blocksize
    .local int xpos
    .local int ypos
    .local pmc color
    .local pmc color2
    .local int x
    .local int y
    .local int w
    .local int h
    .local int i
    .local int xp
    .local int yp
    .local pmc rect
    
    rect = new .Hash
    
    # get the app's palette
    $P0 = self."application"()
    palette = $P0."palette"()
    
    (xpos, ypos) = self."position"()
    (w, h) = self."dimensions"()
    blocksize = self."blockSize"()

    # get the field cache
    getprop cache, "cache", self

    # clear the field cache
    unless full goto NO_CLEAR_CACHE
    set i, cache
    set cache, 0
    # set an invalid/other nextblock cache
    # to force a redraw
    setprop self, "nextblockcache", cache
CLEAR_CACHE:
    push cache, -1
    dec i
    if i > 0 goto CLEAR_CACHE
NO_CLEAR_CACHE:

    set xp, w
    set yp, h
    mul xp, blocksize
    mul yp, blocksize
    add xp, 1
    add yp, 1

    unless full goto NO_FIELDBACKGROUND
    
    rect["x"] = xpos
    rect["y"] = ypos
    rect["width"] = xp
    rect["height"] = yp
    find_type $I0, "SDL::Rect"
    temp = new $I0, rect
    color = palette[15]
    surface."fill_rect"( temp, color )
NO_FIELDBACKGROUND:

    inc xpos
    inc ypos

    #
    # draw the board
    #

    set y, -1
    set i, 0
LOOPy:
    inc y
    set x, 0
    if y >= h goto LOOPend


LOOPx:
    set xp, x
    set yp, y
    mul xp, blocksize
    mul yp, blocksize
    add xp, xpos
    add yp, ypos

    $I0 = blocksize
    dec $I0
    rect["x"] = xp
    rect["y"] = yp
    rect["width"] = $I0
    rect["height"] = $I0
    find_type $I0, "SDL::Rect"
    temp = new $I0, rect

    $I0 = self[i]
    $I1 = cache[i]
    # does the cached color match?
    if $I0 == $I1 goto NO_DRAW
    cache[i] = $I0
    $I1 = palette
    if $I0 >= $I1 goto LOOPend
    color = palette[$I0]
    
    surface."fill_rect"( temp, color )

NO_DRAW:
    inc i
    inc x
    if x >= w goto LOOPy
    branch LOOPx
LOOPend:

    #
    # draw the current block
    #
    temp = self."currentBlock"()
    temp."draw"( surface, xpos, ypos, blocksize )
    # clear the cache at the position of the current block
    self."translateBlockData"( temp, cache, -1 )
    
    #
    # draw the next block
    #
    getprop cache, "nextblockcache", self
    getprop temp, "nextblock", self
    eq_addr cache, temp, SKIP_NEXTBLOCK
    setprop self, "nextblockcache", temp
    set xp, w
    add xp, 1
    mul xp, blocksize
    add xp, xpos
    set yp, ypos
    set w, 4
    set h, 3
    mul w, blocksize
    mul h, blocksize
    dec xp
    dec yp
    add w, 2
    add h, 2
    rect["x"] = xp
    rect["y"] = yp
    rect["width"] = w
    rect["height"] = h
    find_type $I0, "SDL::Rect"
    temp = new $I0, rect
    color = palette[15]
    surface."fill_rect"( temp, color )
    inc xp
    inc yp
    sub w, 2
    sub h, 2
    rect["x"] = xp
    rect["y"] = yp
    rect["width"] = w
    rect["height"] = h
    find_type $I0, "SDL::Rect"
    temp = new $I0, rect
    color = palette[0]
    surface."fill_rect"( temp, color )
    getprop temp, "nextblock", self
    temp."draw"( surface, xp, yp, blocksize )
SKIP_NEXTBLOCK:
    .pcc_begin_return
    .pcc_end_return
.end

=item redraw = board."timer"()

Has to be called at frequent intervals.

Returns 1 if a redraw is necessay, 0 otherwise.

=cut

.sub timer method
    .local pmc block
    .local int redraw

    redraw = 0
    block = self."currentBlock"()
    can $I0, block, "timer"
    unless $I0 goto END
    redraw = block."timer"()
END:
    .pcc_begin_return
    .return redraw
    .pcc_end_return
.end

=item interval = board."fallInterval"()

Returns the number of seconds it takes for a block to fall down
one unit.

=cut

.sub fallInterval method
    .local pmc temp
    .local num ret

    getprop temp, "FallInterval", self
    set ret, temp

    .pcc_begin_return
    .return ret
    .pcc_end_return
.end

=item nexttime = board."nextFallTime"()

Returns the time when the current block will fall down
the next unit.

=cut

.sub nextFallTime method
    .local pmc temp
    .local num ret

    getprop temp, "NextFallTime", self
    set ret, temp

    .pcc_begin_return
    .return ret
    .pcc_end_return
.end

=item board."setNextFallTime"( val )

Sets the time when the current block will fall down
the next time.

This method returns nothing.

=cut

.sub setNextFallTime method
    .param num val
    .local pmc temp

    getprop temp, "NextFallTime", self
    set temp, val

    .pcc_begin_return
    .pcc_end_return
.end

=item (xpos, ypos) = board."position"()

Returns the x and y position of the board's
left top corner.

=cut

.sub position method
    .local int xpos
    .local int ypos
    .local pmc temp

    getprop temp, "xpos", self
    set xpos, temp
    getprop temp, "ypos", self
    set ypos, temp

    .pcc_begin_return
    .return xpos
    .return ypos
    .pcc_end_return
.end

=item board."setPosition"( xpos, ypos )

This method returns nothing.

=cut

.sub setPosition method
    .param int xpos
    .param int ypos
    .local pmc temp

    getprop temp, "xpos", self
    set temp, xpos
    getprop temp, "ypos", self
    set temp, ypos

    .pcc_begin_return
    .pcc_end_return
.end

=item board."blockID"()

TDB

=cut

.sub blockID method
    .param int id
    .local pmc blocks
    .local int ret
    
    blocks = find_global "Tetris::Block", "blocks"
    
    if id != -1 goto OK

    # max id = number of blocks - 1
    $I0 = blocks

    # get a random block id
    $P0 = new .Random
    $N0 = $P0
    $N0 = $N0 * $I0
    id = $N0
    
OK:
    ret = blocks[id]

    .pcc_begin_return
    .return ret
    .pcc_end_return
.end

=back

=head1 AUTHOR

Jens Rieks E<lt>parrot at jensbeimsurfen dot deE<gt> is the author
and maintainer.
Please send patches and suggestions to the Perl 6 Internals mailing list.

=head1 COPYRIGHT

Copyright (C) 2004, The Perl Foundation.

=cut
