=head1 NAME

SDL::LCD - A LCD object

=head1 SYNOPSIS

    # create an LCD
    $I0 = find_type 'SDL::LCD'
    lcd = new $I0

    # set the LCD position
    lcd."xpos"( 10 )
    lcd."ypos"( 10 )

    # set the LCD content
    lcd = "-01:23.4"

    # draw the LCD
    lcd."draw"( screen )

=head1 DESCRIPTION

Displays a liquid crystal display (LCD) with a variable
number of digits. Supported are 0-9, a-f, ".", ":", "-" and blanks.

Each digit is 10 pixels width, plus 2 pixels space between the digits.
The height of each digit is 21 pixels.

=head1 METHODS

An SDL::LCD object has the following methods:

=over 4

=cut

.namespace ["SDL::LCD"]

.sub __onload :load
    $I0 = find_type 'SDL::LCD'
    if $I0 > 1 goto END
    
    load_bytecode "library/SDL/Image.pir"
    load_bytecode "library/SDL/Rect.pir"

    $P0 = new PerlString
    $P0 = "runtime/parrot/library/SDL/LCD.png"
    $I0 = find_type "SDL::Image"
    $P0 = new $I0, $P0
    store_global "SDL::LCD", "digits", $P0
    
    $P0 = newclass 'SDL::LCD'
    addattribute $P0, "value"
    addattribute $P0, "numdigits"
    addattribute $P0, "xpos"
    addattribute $P0, "ypos"
END:
.end

.sub __init method
    $I0 = classoffset self, "SDL::LCD"

    $P0 = new PerlString
    setattribute self, $I0, $P0

    inc $I0
    $P0 = new PerlInt
    $P0 = -1
    setattribute self, $I0, $P0

    inc $I0
    $P0 = new PerlInt
    $P0 = 0
    setattribute self, $I0, $P0

    inc $I0
    $P0 = new PerlInt
    $P0 = 0
    setattribute self, $I0, $P0
.end

=item _digits( count )

Adjusts the minimum number of digits to display.

=cut

.sub _digits method
    .param int val

    $I0 = classoffset self, "SDL::LCD"
    inc $I0
    $P0 = getattribute self, $I0
    $P0 = val
.end

=item __set_integer_native( val )

Sets the LCD content to display.

=cut

.sub __set_integer_native method
    .param int val

    $I0 = classoffset self, "SDL::LCD"
    $P0 = getattribute self, $I0
    $P0 = val
.end

=item __set_string_native( val )

Sets the LCD content to display.

=cut

.sub __set_string_native method
    .param string val

    $I0 = classoffset self, "SDL::LCD"
    $P0 = getattribute self, $I0
    $P0 = val
.end

=item draw( screen )

Draws the LCD onto the specified screen.

=cut

.sub draw method
    .param pmc screen
    .local string val
    .local int i
    .local int len
    .local pmc rect
    .local pmc drect
    .local pmc digits
    .local int xpos
    .local int ypos
    
    $I0 = classoffset self, "SDL::LCD"
    $P0 = getattribute self, $I0
    val = $P0
    
    inc $I0
    $P0 = getattribute self, $I0
    len = $P0

    inc $I0
    $P0 = getattribute self, $I0
    xpos = $P0

    inc $I0
    $P0 = getattribute self, $I0
    ypos = $P0

    if len != -1 goto NO_AUTOLEN
    len = length val
    branch LEN_OK
NO_AUTOLEN:

    val = clone val
    $I0 = length val
    $I0 = len - $I0
    if $I0 <= 0 goto LEN_OK
    $S0 = '0'
    repeat $S0, $S0, $I0
    concat $S0, val
    val = $S0
LEN_OK:

    rect = new PerlHash
    rect["width"] = 10
    rect["height"] = 21
    $I0 = find_type "SDL::Rect"
    rect["x"] = 0
    rect["y"] = 0
    drect = new $I0, rect
    rect["x"] = xpos
    rect["y"] = ypos
    rect = new $I0, rect
    
    digits = find_global "SDL::LCD", "digits"
    
    i = 0
LOOP:
    if i >= len goto END
    $S0 = substr val, i, 1
    
    $I0 = $S0
    if $I0 != 0 goto OK
    
    $I1 = ord $S0

    $I0 = 0
    if $I1 == 48 goto OK
    
    $I0 = 10
    if $I1 == 65 goto OK
    if $I1 == 97 goto OK

    $I0 = 11
    if $I1 == 66 goto OK
    if $I1 == 98 goto OK

    $I0 = 12
    if $I1 == 67 goto OK
    if $I1 == 99 goto OK

    $I0 = 13
    if $I1 == 68 goto OK
    if $I1 == 100 goto OK

    $I0 = 14
    if $I1 == 69 goto OK
    if $I1 == 101 goto OK

    $I0 = 15
    if $I1 == 70 goto OK
    if $I1 == 102 goto OK

    $I0 = 16
    if $I1 == 46 goto OK

    $I0 = 17
    if $I1 == 58 goto OK

    $I0 = 18
    if $I1 == 32 goto OK

    $I0 = 19
OK:
    $I0 *= 10
    drect.'x'( $I0 )
    screen.'blit'( digits, drect, rect )

    $I0 = rect.'x'()
    add $I0, 12
    rect.'x'( $I0 )
    
    inc i
    branch LOOP
END:
    
    $I0 = len * 12
    $I0 -= 2
    rect.'x'( xpos )
    rect.'y'( ypos )
    rect.'width'( $I0 )
    rect.'height'( 21 )
    screen.'update_rect'( rect )
.end

=item xpos( val )

Sets the x position of the LCD.

=cut

.sub xpos method
    .param int val
    
    $I0 = classoffset self, "SDL::LCD"
    add $I0, 2
    $P0 = getattribute self, $I0
    $P0 = val
.end

=item ypos( val )

Sets the y position of the LCD.

=cut

.sub ypos method
    .param int val
    
    $I0 = classoffset self, "SDL::LCD"
    add $I0, 3
    $P0 = getattribute self, $I0
    $P0 = val
.end

=back

=head1 AUTHOR

Jens Rieks E<lt>parrot at jensbeimsurfen dot deE<gt> is the author
and maintainer.
Please send patches and suggestions to the Perl 6 Internals mailing list.

=head1 COPYRIGHT

Copyright (c) 2004, The Perl Foundation.

=cut
