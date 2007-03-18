=head1 TITLE

app.pir - a tetris application object

B<Note:> The Tetris::App class is implemented as a singleton.

=head1 SYNOPSIS

    find_type $I0, "Tetris::App"
    new app, $I0

    app."run"()
    app."shutdown"()
    end

    ...

    # create a new random C<next block> on board 3
    app = find_global "Tetris::App", "app"
    app."nextBlock"( 3 )

=head1 CLASS INFORMATION

This is the main tetris class. Neither has
it parent classes nor is it subclassed.

=cut


.include "timer.pasm"
.namespace ["Tetris::App"]

.const int tSDL               = 0
.const int tEventHandler      = 1
.const int tDebugFlags        = 2
.const int tTimer             = 3
.const int tTimerDisableCount = 4
.const int tInTimer           = 5
.const int tPlayers           = 6
.const int tPalette           = 7
.const int tBoards            = 8

.sub __onload :load
    find_type $I0, "Tetris::App"
    if $I0 > 1 goto END

    load_bytecode "library/SDL/App.pir"
    load_bytecode "library/SDL/Color.pir"
    load_bytecode "library/SDL/Event.pir"
    load_bytecode "library/SDL/Rect.pir"

    load_bytecode "examples/sdl/tetris/eventhandler.pir"
    load_bytecode "examples/sdl/tetris/board.pir"

    newclass $P0, "Tetris::App"

    addattribute $P0, "SDL"
    addattribute $P0, "EventHandler"
    addattribute $P0, "DebugFlags"
    addattribute $P0, "Timer"
    addattribute $P0, "TimerDisableCount"
    addattribute $P0, "InTimer"
    addattribute $P0, "Players"
    addattribute $P0, "Palette"
    addattribute $P0, "Boards"

    # set the BUILD method name
    $P1 = new .String
    $P1 = 'BUILD'
    setprop $P0, 'BUILD', $P1
END:
.end

=head1 CONSTRUCTOR

=over 4

=item BUILD - called automatically by "new"

Initializes the application.

It performs the SDL initialization and
creates some internal data structures afterwards.

This method throws an exception if an error occurs.

=cut

.sub BUILD method
    # create the app object
    store_global "Tetris::App", "app", self

    classoffset $I0, self, "Tetris::App"
    add $I0, tDebugFlags
    $P0 = new .Hash
    setattribute self, $I0, $P0

    # prepare SDL's constructor arguments
    $P0           = new .Hash
    $P0["height"] = 480
    $P0["width"]  = 640
    $P0["bpp"]    =  32
    $P0["flags"]  =   1

    # create the SDL object
    find_type $I0, "SDL::App"
    $P0 = new $I0, $P0

    # store the SDL object
    classoffset $I0, self, "Tetris::App"
    add $I0, tSDL
    setattribute self, $I0, $P0

    # generate some data structures
    self."genPalette"()

    # create the app timer
    self."initTimer"()

    # init the SDL event handler
    find_type $I0, "Tetris::EventHandler"
    $P0 = new $I0, self
    classoffset $I0, self, "Tetris::App"
    add $I0, tEventHandler
    setattribute self, $I0, $P0

    # create the debug flags hash
    $P0 = new .Hash
    classoffset $I0, self, "Tetris::App"
    add $I0, tDebugFlags
    setattribute self, $I0, $P0

    # start a new single player game
    self."newGame"( 1 )
.end

=back

=head1 METHODS

The Tetris::App class provides the following methods:

=over 4

=item sdl = app."SDL"()

=cut

.sub SDL method
    classoffset $I0, self, "Tetris::App"
    add $I0, tSDL

    getattribute $P0, self, $I0

    .pcc_begin_return
    .return $P0
    .pcc_end_return
.end

=item app."shutdown"()

Shuts SDL down and performs some internal cleanup.

B<Note:> The application object is invalid afterwards, you are not allowed to
call any other of its methods after this one.

This method returns nothing.

=cut

.sub shutdown method
    .local pmc sdl

    # XXX free data structures
    # ...

    # shutdown the SDL system
    $P0 = self."SDL"()
    if_null $P0, END
    $P0."quit"()
END:
.end

=item success = app."run"()

The application's main loop.

Returns if the user requested a shutdown.
An exeption is thrown if an error occurs.

=cut

.sub _app_timer
    find_global $P0, "Tetris::App", "app"
    $P0."timer"()
.end

.sub run method
    .local pmc eh
    .local pmc loop

    classoffset $I0, self, "Tetris::App"
    add $I0, tEventHandler
    getattribute eh, self, $I0

    find_type $I0, "SDL::Event"
    loop = new $I0

    self."enableTimer"()
    loop."process_events"( 0.1, eh, self )
    self."disableTimer"()
    print "done\n"
.end

=item app."initTimer"()

=cut

.sub initTimer method
    $P0 = new SArray
    $P1 = find_global "Tetris::App", "_app_timer"
    $P0 = 8
    $P0[0] = .PARROT_TIMER_NSEC
    $P0[1] = 0.1
    $P0[2] = .PARROT_TIMER_HANDLER
    $P0[3] = $P1
    $P0[4] = .PARROT_TIMER_REPEAT
    $P0[5] = -1
    $P0[6] = .PARROT_TIMER_RUNNING
    $P0[7] = 1

    $P1 = new .Timer, $P0

    classoffset $I0, self, "Tetris::App"
    add $I0, tTimer
    setattribute self, $I0, $P1
    sub $I0, tTimer

    $P0 = new .Integer
    $P0 = 1
    add $I0, tTimerDisableCount
    setattribute self, $I0, $P0
    sub $I0, tTimerDisableCount

    $P0 = new .Integer
    $P0 = 0
    add $I0, tInTimer
    setattribute self, $I0, $P0
.end

=item app."setTimerStatus"( status )

=cut

.sub setTimerStatus method
    .param int status

    classoffset $I0, self, "Tetris::App"
    add $I0, tTimer
    getattribute $P0, self, $I0
    set $P0[.PARROT_TIMER_RUNNING], status
.end

=item app."enableTimer"()

=cut

.sub enableTimer method
    classoffset $I0, self, "Tetris::App"
    add $I0, tTimerDisableCount
    getattribute $P0, self, $I0
    dec $P0
    if $P0 != 0 goto END
    self."setTimerStatus"( 1 )
END:
.end

=item app."disableTimer"()

=cut

.sub disableTimer method
    classoffset $I0, self, "Tetris::App"
    add $I0, tTimerDisableCount
    getattribute $P0, self, $I0
    inc $P0
    self."setTimerStatus"( 0 )
.end

=item color = app."color"( number )

Returns the specified color entry from the palette.

=cut

.sub color method
    .param int number
    .local pmc palette
    .local pmc color

    palette = self."palette"()
    color = palette[number]

    .pcc_begin_return
    .return color
    .pcc_end_return
.end

=item palette = app."palette"()

Returns the color palette.

=cut

.sub palette method
    .local pmc palette

    classoffset $I0, self, "Tetris::App"
    add $I0, tPalette
    getattribute palette, self, $I0
    if_null palette, CREATE
    branch RET

NULL:
    print "warning: no color palette found!\n"

RET:
    .pcc_begin_return
    .return palette
    .pcc_end_return
.end

=item palette = app."genPalette"() B<(internal)>

Creates the color palette.

This method returns the created palette.

=cut

.sub genPalette method
    .local pmc palette
    .local pmc hash
    .local pmc color
    .local int i
    .local int r
    .local int g
    .local int b
    .local int l
    .local int color_id

    color_id = find_type "SDL::Color"
    palette = new .ResizablePMCArray
    hash = new .Hash

    set i, 0
GENLOOP:
    band l, i, 8
    band r, i, 4
    band g, i, 2
    band b, i, 1
    shr r, 2
    shr g, 1
    mul r, 127
    mul g, 127
    mul b, 127
    unless l, NOT_BRIGHT
    add r, 64
    add g, 64
    add b, 64
NOT_BRIGHT:

    hash["r"] = r
    hash["g"] = g
    hash["b"] = b
    color = new color_id, hash

    push palette, color
    inc i
    if i < 16 goto GENLOOP

    classoffset $I0, self, "Tetris::App"
    add $I0, tPalette
    setattribute self, $I0, palette

    .pcc_begin_return
    .return palette
    .pcc_end_return
.end

=item board = self."board"( boardID )

Lookup a board using its ID.

=over 4

=item parameter C<boardID>

The ID of the board to return.

=back

Returns the board object, or NULL if
no board with the specified ID exists.

=cut

.sub board method
    .param int boardID
    .local pmc board

    classoffset $I0, self, "Tetris::App"
    add $I0, tBoards
    getattribute board, self, $I0

    $I0 = board
    if boardID < $I0 goto OK
ERR:
    null board
    print "board "
    print boardID
    print " not found!\n"
    branch END

OK:
    board = board[boardID]
    defined $I0, board
    unless $I0 goto ERR

END:
    .pcc_begin_return
    .return board
    .pcc_end_return
.end

=item block = app."currentBlock"( boardID )

Returns the currently falling block of a board.

=over 4

=item parameter C<boardID>

The ID of the board whose C<current block> should be returned.

=back

Returns the block object, or NULL if the board was not found.

=cut

.sub currentBlock method
    .param int boardID
    .local pmc temp

    temp = self."board"( boardID )
    if_null temp, BLOCKISNULL
    temp = temp."currentBlock"()

BLOCKISNULL:
    .pcc_begin_return
    .return temp
    .pcc_end_return
.end

=item success = app."rotate"( boardID, dir )

Rotates the current block of a board.

=over 4

=item parameter C<boardID>

The ID of the board whose block should be rotated.

=item parameter C<dir>

+1 = rotate clockwise

-1 = rotate counterclockwise

=back

Returns 1 if the rotation was possible, 0 otherwise.

=cut

.sub rotate method
    .param int boardID
    .param int dir
    .local pmc block
    .local int ret

    self."disableTimer"()

    ret = 0

    # lookup the block
    block = self."currentBlock"( boardID )
    if_null block, END

    # rotate the block
    ret = block."rotate"( dir )
    if ret == 0 goto END

    # redraw the screen
    self."drawScreen"( 0 )

END:
    self."enableTimer"()

    .pcc_begin_return
    .return ret
    .pcc_end_return
.end

=item success = app."move"( boardID, xval, yval )

Moves the currently falling block of a board.
It does not lock the block onto the board in any case, use
C<_Board::lockBlock()> if you want this.

=over 4

=item parameter C<boardID>

The ID of the board whose block should be moved.

=item parameter C<xval>

Number of units the block should be moved horizontally.

Positiv numbers will move the block rightwards, negative
numbers leftwards.

=item parameter C<xval>

Number of units the block should be moved vertically.

Positiv numbers will move the block downwards, negative
numbers upwards (untested; not recommended).

=back

Returns 1 if the movement was possible, 0 otherwise.

=cut

.sub move method
    .param int boardID
    .param int xval
    .param int yval
    .local int success
    .local pmc block

    # disable the timer
    self."disableTimer"()

    block = self."currentBlock"( boardID )
    if_null block, END
    success = block."move"( xval, yval )

    unless success goto END
    self."drawScreen"( 0 )

END:
    # reenable the timer
    self."enableTimer"()

    .pcc_begin_return
    .return success
    .pcc_end_return
.end

=item block = self."nextBlock"( boardID, id )

Activates the C<next block> on the specified board
and creates a new C<next block>.
The old current block is just overwritten, and not locked
onto the board. Use C<_Board::lockBlock()> for this.

=over 4

=item parameter C<boardID>

The ID of the board where the next block should be activated.

=item parameter C<id> B<(optional)>

The id of the block to create. The id is an index in the
blockdata array. Please refer to <_Tetris::App::blockdata()>
for more information.

=back

This method returns the new falling block.

=cut

.sub nextBlock method
    .param int boardID
    .param int id
    .local pmc temp

    print "nextBlock: ("
    print boardID
    print ") "
    print id
    print "\n"

    # check the number of INT args
    if argcI >= 2 goto SKIP_ID
    # no INT arg => use a random next block
    set id, -1
SKIP_ID:

    temp = self."board"( boardID )
    if_null temp, APP_NEXTBLOCK_END
    temp = temp."nextBlock"(id )

APP_NEXTBLOCK_END:
    .pcc_begin_return
    .return temp
    .pcc_end_return
.end

=item app."fall"( boardID )

Lets the current block of the specified board
fall down fast.

=over 4

=item parameter C<boardID>

The ID of the board where the block should fall down fast.

=back

This method returns 1 if the board was found, 0 otherwise.

=cut

.sub fall method
    .param int boardID
    .local pmc board
    .local int ret

    ret = 0
    board = self."board"( boardID )
    if_null board, APP_FALL_END
    board."fall"()
    ret = 1

APP_FALL_END:
    .pcc_begin_return
    .return ret
    .pcc_end_return
.end

=item falling = app."falling"( boardID )

Checks if the current block of the specified board
is falling down fast.

=over 4

=item parameter C<boardID>

The ID of the board to check.

=back

This method returns 1 if the block is falling down fast, 0 otherwise.

=cut

.sub falling method
    .param int boardID
    .local pmc board
    .local int ret

    ret = 0
    board = self."board"( boardID )
    if_null board, APP_FALLING_END
    ret = board."falling"()

APP_FALLING_END:
    .pcc_begin_return
    .return ret
    .pcc_end_return
.end

=item interval = app."fallInterval"( boardID )

Checks the board's falling speed.

=over 4

=item parameter C<boardID>

The ID of the board to check.

=back

This method returns how many seconds it take for a block
to fall down one unit.

=cut

.sub fallInterval method
    .param int boardID
    .local pmc board
    .local num ret

    ret = 0
    board = self."board"( boardID )
    if_null board, APP_INTERVAL_END
    ret = board."fallInterval"()

APP_INTERVAL_END:
    .pcc_begin_return
    .return ret
    .pcc_end_return
.end

=item nextfall = app."nextFallTime"( boardID )

Checks when the block on the specified board falls down
the next unit.

=over 4

=item parameter C<boardID>

The ID of the board to check.

=back

Returns the time when the block falls down the next time.

=cut

.sub nextFallTime method
    .param int boardID
    .local pmc board
    .local num ret

    board = self."board"( boardID )
    if_null board, APP_NEXTFALL_END
    ret = board."nextFallTime"()

APP_NEXTFALL_END:
    .pcc_begin_return
    .return ret
    .pcc_end_return
.end

=item app."setNextFallTime"( boardID, val )

Sets the time when the block on the specified board
falls down the next unit.

=over 4

=item parameter C<boardID>

The ID of the board to modify.

=item parameter C<val>

The time when the block falls down the next time.

=back

This method returns nothing.

=cut

.sub setNextFallTime method
    .param int boardID
    .param num val
    .local pmc board

    board = self."board"( boardID )
    if_null board, APP_SETFALL_END
    board."setNextFallTime"( val )

APP_SETFALL_END:
    .pcc_begin_return
    .pcc_end_return
.end

=item redrawn = app."timer"()

Has to be called at frequent intervals.

Returns 1 if the screen has been redrawn, 0 otherwise.

=cut

.sub timer method
    .local pmc board
    .local int redraw
    .local int ret
    .local pmc boards
    .local pmc board
    .local int i
    .local pmc inTimer

    redraw = 0

    # check the timer disable count
    # do nothing if the timer is disabled
    classoffset $I0, self, "Tetris::App"
    add $I0, tTimerDisableCount
    getattribute $P0, self, $I0
    if $P0 > 0 goto END

    classoffset $I0, self, "Tetris::App"
    add $I0, tInTimer
    getattribute inTimer, self, $I0

    i = inTimer
    if i goto END

    # XXX: fetch exceptions and reset this flag
    # we are in the timer handler function
    inc inTimer

    # get the boards array
    (boards, i) = self."boards"( self )

LOOP:
    dec i
    if i < 0 goto REDRAW
    board = boards[i]

    # call the board's timer
    ret = board."timer"()

    unless ret goto LOOP
    redraw = 1
    branch LOOP

REDRAW:
    unless redraw goto NOREDRAW
    self."drawScreen"( 0 )
NOREDRAW:

    dec inTimer
END:

    .pcc_begin_return
    .return redraw
    .pcc_end_return
.end

=item app."drawScreen"( full )

Redraws the screen.

=over 4

=item parameter C<full>

0 = update only modified parts (uses a draw cache)

1 = draw everything

=back

This method returns nothing.

=cut

.sub drawScreen method
    .param int full
    .local pmc screen
    .local pmc temp

    self."disableTimer"()

    screen = self."SDL"()
    screen = screen."surface"()

    # draw everything?
    $I0 = self."flag"( "show blocksize" )
    if $I0 goto FULL
    $I0 = self."flag"( "draw full" )
    if $I0 goto FULL
    branch NOT_FULL
FULL:
    full = 1
NOT_FULL:

    #
    # draw the main background
    #

    # to see the draw cache optimisation
    $I0 = self."flag"( "show optimisation" )
    if $I0 goto FORCE
    unless full goto NO_MAINBACKGROUND
FORCE:
    .local pmc rect
    .local pmc screen
    .local pmc color

    rect = new .Hash
    rect["width"] = 640
    rect["height"] = 480
    rect["x"] = 0
    rect["y"] = 0
    find_type $I0, "SDL::Rect"
    temp = new $I0, rect
    color = self."color"( 3 )

    screen."fill_rect"( temp, color )
NO_MAINBACKGROUND:
    #
    # draw the boards
    #
    self."drawBoards"( screen, full )

    #
    # update the screen
    #
    # XXX: optimize screen updates
    rect = new .Hash
    rect["width"] = 640
    rect["height"] = 480
    rect["x"] = 0
    rect["y"] = 0
    find_type $I0, "SDL::Rect"
    temp = new $I0, rect
    screen."update_rect"( temp )

    self."enableTimer"()
.end

=item app."drawBoards"( screen, full )

=cut

.sub drawBoards method
    .param pmc screen
    .param int full
    .local pmc boards
    .local pmc board
    .local int i

    (boards, i) = self."boards"()

LOOP:
    dec i
    if i < 0 goto END
    board = boards[i]
    board."draw"( screen, full )
    branch LOOP

END:
    .pcc_begin_return
    .pcc_end_return
.end

=item (boards, count) = app."boards"()

Returns the number of boards registered as well as
the boards array.

=cut

.sub boards method
    .local pmc boards
    .local int count

    classoffset $I0, self, "Tetris::App"
    add $I0, tBoards
    getattribute boards, self, $I0
    count = 0
    if_null boards, END
    count = boards

END:
    .pcc_begin_return
    .return boards
    .return count
    .pcc_end_return
.end

=item app."registerBoard"( board, id ) B<(internal)>

Called by the board constructor to add the board object
to the application's board array. Returns the index where
the board has been added, which is used as the board ID.

=cut

.sub registerBoard method
    .param pmc board
    .local pmc boards
    .local int id

    (boards, id) = self."boards"()
    set boards[id], board

    .pcc_begin_return
    .return id
    .pcc_end_return
.end

=item value = app."flag"( name, value )

Get or sets an integer debug flag.

=over 4

=item parameter C<name>

The flag's name.

=item parameter C<value> B<(optional)>

Set the flag to the specified (integer) value.

=back

Returns the flag's (new) value.

=cut

.sub flag method
    .param string name
    .param int value
    .local pmc flag
    .local int ret

    # get the flags hash
    classoffset $I0, self, "Tetris::App"
    add $I0, tDebugFlags
    getattribute flag, self, $I0

    # check the number of INT args
    if argcI <= 1 goto FLAG_GET
    # set a new value
    set flag[name], value

FLAG_GET:
    set ret, flag[name]

    .pcc_begin_return
    .return ret
    .pcc_end_return
.end

=item app."newGame"( boards )

Starts a new game with the specified number of boards.

=over 4

=item parameter C<boards>

The number of boards to create.

=back

This method returns nothing.

=cut

.sub newGame method
    .param int players
    .local pmc temp
    .local int xpos

    self."disableTimer"()

    set xpos, 10

    # check the number of INT args
    if argcI >= 1 goto SET

    classoffset $I0, self, "Tetris::App"
    add $I0, tPlayers
    getattribute temp, self, $I0
    players = 1
    if_null temp, SET
    players = temp
    branch END_SET

SET:
    # save the number of players
    new temp, .Integer
    set temp, players

    classoffset $I0, self, "Tetris::App"
    add $I0, tPlayers
    setattribute self, $I0, temp
END_SET:

    print "starting a "
    print players
    print " player game...\n"

    # create the boards array
    new temp, .ResizablePMCArray

    classoffset $I0, self, "Tetris::App"
    add $I0, tBoards
    setattribute self, $I0, temp

NEWGAME_NEW_BOARD:
    if players <= 0 goto NEWGAME_END
    find_type $I0, "Tetris::Board"
    print "new board...\n"
    temp = new $I0, self
    print "new board done.\n"
    temp."setPosition"( xpos, 10 )
    add xpos, 320
    dec players
    branch NEWGAME_NEW_BOARD

NEWGAME_END:
    self."drawScreen"( 1 )

    self."enableTimer"()

    .pcc_begin_return
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

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
