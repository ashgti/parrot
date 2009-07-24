# Copyright (C) 2001-2008, Parrot Foundation.
# $Id$

=head1 NAME

examples/library/ncurses_life.pir - Conway's Game of Life (ncurses version)

=head1 SYNOPSIS

    % ./parrot examples/library/ncurses_life.pir examples/library/acorn.life

=head1 DESCRIPTION

An C<ncurses> version of Conway's Life. F<acorn.life> is a I<life> file.

This version can load F<life.l> files (#P, #A, #R formats only). You get
a lot of .l files by installing C<xlife>.

=head1 COMMANDS

=over 4

=item C<q>, C<Q>

Quit.

=item C<8>, C<2>, C<UP>, C<DOWN>

Move world up or down.

=item C<4>, C<6>, C<LEFT>, C<RIGHT>

Move world left or right

=item C<5>, C<HOME>

Center world.

=item C<g>

Toggle running the game.

=item C<o>

Single step one generation.

=item C<s>

Run slower.

=item C<f>

Run faster.

=back

=cut

.sub _MAIN :main
    .param pmc argv
    # the command line
    load_bytecode 'ncurses.pbc'

    # should autogenerate these
    .globalconst int KEY_DOWN = 258
    .globalconst int KEY_UP   = 259
    .globalconst int KEY_LEFT = 260
    .globalconst int KEY_RIGHT= 261
    .globalconst int KEY_HOME = 262

    # set generation count
    .const int MAX_GEN = 5000

    # kill the space ship flag
    .const int COLLIDE = 1

    .local int GEN_COUNT
    .local num START_TIME
    .local num CUR_TIME
    .local num TIME_DIFF
    .local num GPS
    .local int SUPRESS_PRINT
    .local pmc CURS_SET
    .local pmc ENDWIN
    .local pmc DISPLAY
    .local pmc STDSCR
    .local int size
    .local int stop
    .local int sleep_lim
    .local int x_offs
    .local int y_offs

    # 15 * sizef is real size of world
    .const int sizef = 8

    # delay in usec
    .local int delay
    delay = 20000

    # Note the time
    time START_TIME

    # If true, we don't print
    SUPRESS_PRINT = 0
    stop          = 0        # -1 start with <g>o
    x_offs        = 0
    y_offs        = 0

    ENDWIN   = get_global "ncurses::endwin"
    CURS_SET = get_global "ncurses::curs_set"
    size     = 15 * sizef

    .local string err
    null err

    $I0 = argv
    if $I0 <= 1 goto def_world

    ($S15, err) = _load_file(argv, size)
    length $I0, err

    if $I0, print_err
    goto start_curses

def_world:
    $S15 = _def_world(sizef, size, COLLIDE)

start_curses:
    STDSCR    = _init_curses()
    GEN_COUNT = 0

loop:
    _dump($S15, SUPRESS_PRINT, x_offs, y_offs, size, GEN_COUNT, STDSCR, delay)

    if GEN_COUNT >= MAX_GEN goto getout
    (stop, x_offs, y_offs, delay) = _check_key(stop, x_offs, y_offs, delay)

    if stop != -2 goto not_one
    stop = -1
    goto gen_one

not_one:
    if stop < 0 goto no_gen
    if stop goto getout

gen_one:
    inc GEN_COUNT
    $I31 = GEN_COUNT % 100
    if $I31 goto skip
    printerr "."

skip:
    ($S15, stop) = _generate($S15, size, stop)

no_gen:
    branch loop

getout:
    CURS_SET(1)
    ENDWIN()
    time CUR_TIME
    TIME_DIFF = CUR_TIME - START_TIME

    # sleeping invalidates these data
    say   ""
    print GEN_COUNT
    print " generations in "
    print TIME_DIFF
    print " seconds. "
    GPS = GEN_COUNT / TIME_DIFF

    print GPS
    say " generations/sec"

    interpinfo $I1, 1
    print "A total of "
    print $I1
    say " bytes were allocated"

    interpinfo $I1, 2
    print "A total of "
    print $I1
    say " GC runs were made"

    interpinfo $I1, 3
    print "A total of "
    print $I1
    say " collection runs were made"

    interpinfo $I1, 10
    print "Copying a total of "
    print $I1
    say " bytes"

    interpinfo $I1, 5
    print "There are "
    print $I1
    say " active Buffer structs"

    interpinfo $I1, 7
    print "There are "
    print $I1
    say " total Buffer structs"

    end

print_err:
    printerr "ERROR: "
    printerr err
    printerr "\n"
    exit 1
.end


# S15 has the incoming string, S0 is scratch
.sub _dump
    .param string world
    .param int SUPRESS_PRINT
    .param int x_offs
    .param int y_offs
    .param int size
    .param int GEN_COUNT
    .param pmc STDSCR
    .param int delay

    if SUPRESS_PRINT goto dumpend

    .local pmc WCLEAR
    .local pmc MVWADDSTR
    .local pmc MVWADDCH
    .local pmc WREFRESH

    WCLEAR     = get_global "ncurses::wclear"
    MVWADDSTR  = get_global "ncurses::mvwaddstr"
    MVWADDCH   = get_global "ncurses::mvwaddch"
    WREFRESH   = get_global "ncurses::wrefresh"

    WCLEAR(STDSCR)
    MVWADDSTR(STDSCR, 0, 0, "Generation: ")
    $S0 = GEN_COUNT
    MVWADDSTR(STDSCR, 0, 13, $S0)

    $I0 = size * y_offs
    $I0 = $I0 + x_offs

    .local int CHARACTER_OFFSET
    CHARACTER_OFFSET = $I0

    .local int CHAR_POS
    CHAR_POS = 0

    .local int total
    total = size * size

    .local int cols
    .local int rows
    .local pmc ENV
    ENV  = new "Env"

    $S0  = ENV["COLUMNS"]
    cols = $S0
    say $S0

    if cols, checklines
    cols = 80

 checklines:
    $S0  = ENV["LINES"]
    rows = $S0
    say $S0

    if rows, donelines
    rows = 24

donelines:
    .local int X_COORD
    .local int Y_COORD

printloop:
    # TODO skip unprintable out of screen
    if CHARACTER_OFFSET >= total goto dumpend

    substr $S0, world, CHARACTER_OFFSET, 1
    if $S0 != "*" goto incit

    X_COORD = CHAR_POS % size
    Y_COORD = CHAR_POS / size
    Y_COORD = Y_COORD + 2

    if X_COORD > cols goto incit
    if Y_COORD > rows goto dumpend
    MVWADDCH(STDSCR, Y_COORD, X_COORD, 42) # behold, the lowly star

incit:
    inc CHARACTER_OFFSET
    inc CHAR_POS
    if  CHARACTER_OFFSET < total goto printloop

    WREFRESH(STDSCR)

    if delay < 100 goto dumpend

    # as we gonna sleep here, let's burn some cycles to check if usleep is
    # available
    null $P0
    .local pmc USLEEP
    dlfunc USLEEP, $P0, "usleep", "vi"
    $I0 = defined USLEEP
    if $I0 goto usleep
    sleep 1
    goto dumpend

usleep:
    USLEEP(delay)

dumpend:
    .return()
.end

.sub _init_curses
    .local pmc INITSCR
    .local pmc START_COLOR
    .local pmc INIT_PAIR
    .local pmc COLOR_PAIR
    .local pmc WATTRON
    .local pmc CURS_SET
    .local pmc NODELAY
    .local pmc KEYPAD
    .local pmc STDSCR

    INITSCR     = get_global "ncurses::initscr"
    START_COLOR = get_global "ncurses::start_color"
    INIT_PAIR   = get_global "ncurses::init_pair"
    COLOR_PAIR  = get_global "ncurses::COLOR_PAIR"
    WATTRON     = get_global "ncurses::wattron"
    CURS_SET    = get_global "ncurses::curs_set"
    NODELAY     = get_global "ncurses::nodelay"
    KEYPAD      = get_global "ncurses::keypad"

    STDSCR = INITSCR()

    START_COLOR()

    # Color pair 1, dark green fg, black background
    INIT_PAIR(1, 2, 0)
    $I0 = COLOR_PAIR(1)

    # We pass what's returned from COLOR_PAIR straight on
    WATTRON(STDSCR, $I0)

    CURS_SET(0)            # turn off cursor
    NODELAY(STDSCR, 1)    # set nodelay mode
    KEYPAD(STDSCR, 1)    # set keypad mode

    .return(STDSCR)
.end

# in: world (string)
#     size
# out new world
#     stop
.sub _generate
    .param string world
    .param int size
    .param int stop

#print "World:\n"
#print world
#print "\n"
#sleep 3

    .local int len
    .local int pos
    .local int count
    .local int check    # pos in world
    .local string new_world
    length len, world

    # allocate new world with all space
    repeat new_world, " ", len
    pos = 0

genloop:
    count = 0

NW:
    check = -size
    dec check
    check += len
    check += pos
    check %= len

    # $S0 is always overwritten, so reuse it
    substr $S0, world, check, 1
    if $S0 != "*" goto North
    inc count

North:
    check = -size
    check += len
    check += pos
    check %= len

    substr $S0, world, check, 1
    if $S0 != "*" goto NE
    inc count

NE:
    check = -size
    inc check
    check += len
    check += pos
    check %= len

    substr $S0, world, check, 1
    if $S0 != "*" goto West
    inc count
West:
    check = -1
    check += len
    check += pos
    check %= len

    substr $S0, world, check, 1
    if $S0 != "*" goto East
    inc count

East:
    check = 1
    check += len
    check += pos
    check %= len

    substr $S0, world, check, 1
    if $S0 != "*" goto SW
    inc count

SW:
    check  = size - 1
    check += len
    check += pos
    check %= len

    substr $S0, world, check, 1
    if $S0 != "*" goto South
    inc count

South:
    check = size
    check += len
    check += pos
    check %= len

    substr $S0, world, check, 1
    if $S0 != "*" goto SE
    inc count
SE:
    check = size + 1
    check += len
    check += pos
    check %= len

    substr $S0, world, check, 1
    if $S0 != "*" goto checkl
    inc count

checkl:
    substr $S0, world, pos, 1
    if $S0 == "*" goto check_alive

# If eq 3, put a star in else a space
check_dead:
    if count == 3 goto star
    branch space

check_alive:
    if count < 2 goto space
    if count > 3 goto space

star:
    substr new_world, pos, 1, "*"

space:    # is space already
    inc pos
    if pos < len goto genloop

done:
    if new_world != world goto dif
    sleep 2
    stop = 1

dif:
    .return(new_world, stop)
.end

.sub _def_world
    .param int sizef
    .param int size
    .param int COLLIDE
    set $S17,  "               "
    $I0 = sizef - 1
    unless $I0 goto nosize
    $S16 = ""
    repeat $S16, $S17, $I0

nosize:
    set $S0,  "               "
    set $S1,  "               "
    set $S2,  "               "
    set $S3,  "               "
    set $S4,  "  **           "
    set $S5,  "*    *         "
    set $S6,  "      *        "
    set $S7,  "*     *        "
    set $S8,  " ******        "
    set $S9,  "               "
    set $S10, "               "
    set $S11, "               "
    if COLLIDE goto col

    set $S12, "               "
    set $S13, "               "
    set $S14, "               "
    goto nocol

col:
    set $S12, "             * "
    set $S13, "              *"
    set $S14, "            ***"

nocol:
    .local string world
    world = $S0
    concat world, $S16
    concat world, $S1
    concat world, $S16
    concat world, $S2
    concat world, $S16
    concat world, $S3
    concat world, $S16
    concat world, $S4
    concat world, $S16
    concat world, $S5
    concat world, $S16
    concat world, $S6
    concat world, $S16
    concat world, $S7
    concat world, $S16
    concat world, $S8
    concat world, $S16
    concat world, $S9
    concat world, $S16
    concat world, $S10
    concat world, $S16
    concat world, $S11
    concat world, $S16
    concat world, $S12
    concat world, $S16
    concat world, $S13
    concat world, $S16
    concat world, $S14
    concat world, $S16
    $I1 = size * $I0
    repeat $S16, $S17, $I1
    concat world, $S16
    .return(world)
.end

.sub _load_file
    .param pmc argv
    .param int size

    .local string world
    .local string err
    world = ""

    .local string file
    file = argv[1]
    err  = "File not found " . file

    .local pmc io
    open io, file, 'r'
    $I0 = defined io
    unless $I0 goto nok
    null err
    .local string line
    $I0 = size * size

    repeat world, " ", $I0

    .local int pos
    $I0 = size / 2
    $I1 = $I0 * $I0
    pos = $I0 + $I1

    .local int len
    .local int format
    format = 0

    .const int PICTURE = 1
    .const int ABS     = 2
    .const int REL     = 3
    .local pmc points

loop:
    readline line, io
    length len, line
    unless len goto out
    $S0 = substr line, 0,1
    eq $S0, "#", check_format
    chopn line, 1        # \n
    dec len
    if format != PICTURE goto not_pic
    substr world, pos, len, line
    pos = pos + size
    goto loop

not_pic:
    if format != ABS goto not_abs
    goto do_rel

not_abs:
    if format != REL goto not_rel

do_rel:
    # parse \s(\d+) (\d+)
    # I really want PCRE or better inside Parrot :)
    .local int s
    .local int start
    .local int in_digit
    in_digit = 0
    s = 0
get_d:
    if s >= len goto space
    $S0 = substr line, s, 1
    ord $I0, $S0
    if $I0 == 32   goto space
    if $I0 == 9    goto space
    if $I0 == 43   goto cont_d    # ignore +
    if $I0 == 45   goto dig       # sign, start dig
    if $I0 >= 0x30 goto dig1
    err = "Found junk at " . $S0
    goto out

dig1:
    if $I0 <= 0x39 goto dig
    err = "Found junk at " . $S0
    goto out

dig:
    if in_digit == 1 goto cont_d
    if in_digit == 3 goto cont_d
    start = s
    inc in_digit

cont_d:
    inc s
    goto get_d

space:
    if in_digit == 0 goto cont_d
    if in_digit == 2 goto cont_d
    inc in_digit
    $I1 = s - start
    substr $S1, line, start, $I1
    print "Setting $I2 to "
    say $S1
    $I2 = $S1
    push points, $I2
    if s >= len goto loop
    inc s
    goto get_d

end_d:
    goto loop

not_rel:
    err = "Unhandled file format"
    goto out

check_format:
    $S0 = substr line, 1, 1
    if $S0 == "C" goto loop    # comment
    if $S0 == "#" goto loop    # comment
    if $S0 == "N" goto loop    # name of pattern
    if $S0 == "O" goto loop    # owner
    if $S0 == "U" goto loop    # use format
    unless format goto f1
    err = "Mixed formats found"
    goto out

f1:
    if $S0 != "P" goto not_P    # picture
    format = PICTURE

not_P:
    if $S0 != "A" goto not_A    # absolute
    format = ABS
    points = new 'ResizableIntegerArray'

not_A:
    if $S0 != "R" goto not_R    # relative
    format = REL
    points = new 'ResizableIntegerArray'

not_R:
    goto loop

out:
    close io
    if format == PICTURE goto done
    # we have an array of x,y pairs now
    # find min, max
    .local int min_x
    .local int min_y
    .local int max_x
    .local int max_y
    min_x =  99999
    min_y =  99999
    max_x = -99999
    max_y = -99999

    .local int x
    .local int y
    .local int len
    len = points
    s   = 0

lp:
    x = points[s]
    inc s
    y = points[s]
    inc s
    if x >= min_x goto no_min_x
    min_x = x

no_min_x:
    if x <= max_x goto no_max_x
    max_x = x

no_max_x:
    if y >= min_y goto no_min_y
    min_y = y

no_min_y:
    if y <= max_y goto no_max_y
    max_y = y

no_max_y:
    if s < len goto lp

#    printerr min_x
#    printerr ", "
#    printerr min_y
#    printerr ", "
#    printerr max_x
#    printerr ", "
#    printerr max_y
#    printerr "\n\n"

    # now fill world
    s = 0
lp2:
    x = points[s]
    inc s
    y = points[s]
    inc s
    x = x - min_x
    y = y - min_y
#    printerr x
#    printerr ", "
#    printerr y
#    printerr "\n"
    .local int c
    c = y * size
    c = x + c

    # TODO abs/rel and bounds checking
    #world[c] = "*"
    substr world, c, 1, "*"
    if s < len goto lp2
done:
nok:
    .return(world, err)
.end

.sub _check_key
    .param int stop
    .param int x_offs
    .param int y_offs
    .param int delay

    .local int key
    .local pmc GETCH
    GETCH = get_global "ncurses::getch"
    key   = GETCH()

    if key == KEY_LEFT  goto is_4
    if key == KEY_RIGHT goto is_6
    if key == KEY_UP    goto is_8
    if key == KEY_DOWN  goto is_2
    if key == KEY_HOME  goto is_5
    if key != 113       goto no_q
    stop = 1

no_q:
    if key != 81 goto no_Q
    stop = 1
    goto key_done

no_Q:
    if key != 0x38 goto no_8

is_8:
    y_offs = y_offs + 10
    goto key_done

no_8:
    if key != 0x32 goto no_2

is_2:
    if y_offs < 10 goto key_done
    y_offs = y_offs - 10
    goto key_done

no_2:
    if key != 0x34 goto no_4

is_4:
    x_offs = x_offs + 10
    goto key_done

no_4:
    if key != 0x36 goto no_6

is_6:
    if x_offs < 10 goto key_done
    x_offs = x_offs - 10
    goto key_done

no_6:
    if key != 0x35 goto no_5

is_5:
    x_offs = 0
    y_offs = 0
    goto key_done

no_5:
    if key != 103 goto no_g
    if stop == 0 goto g0
    stop = 0
    goto key_done

g0:
    stop = -1
    goto key_done

no_g:
    if key != 111 goto no_o
    stop = -2
    goto key_done

no_o:
    if key != 115 goto no_s
    stop   = 0
    delay *= 2
    goto key_done

no_s:
    if key != 102 goto no_f
    stop   = 0
    delay /= 2
    if delay goto key_done
    delay = 1
    goto key_done

no_f:

key_done:
    .return(stop, x_offs, y_offs, delay)
.end

=head1 SEE ALSO

F<examples/library/acorn.life>, F<examples/pasm/life.pasm>,
F<runtime/parrot/library/ncurses.pasm>,
F<runtime/parrot/library/ncurses.declarations>.

=head1 NOTE

The normal extension for life files is C<.l> - Parrot treats these
as C<lex> files, however, so we use a more verbose extension

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
