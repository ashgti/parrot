=head1 TITLE

boarddata.pir - a tetris board data class

=head1 SYNOPSIS

    # create the board data
    find_type $I0, "BoardData"
    new data, $I0
    data."new"( 10, 20 )

    # fill the board
    data."fill"( 0 )

=head1 CLASS INFORMATION

This is the base class of the Board class.

=cut

.namespace ["Tetris::BoardData"]

.const int bData   = 0
.const int bWidth  = 1
.const int bHeight = 2

.sub __onload :load
    find_type $I0, "Tetris::BoardData"
    if $I0 > 1 goto END
    newclass $P0, "Tetris::BoardData"
    addattribute $P0, "data"
    addattribute $P0, "width"
    addattribute $P0, "height"
END:
.end

=head1 FUNCTIONS

=over 4

=item data = data."init"( width, height )

Initializes the BoardData object.

=over 4

=item parameter C<width>

The width of the board to create.

=item parameter C<height>

The height of the board to create.

=back

Returns the created data object.

=cut

.sub init :method
    .param int w
    .param int h
    .local pmc data
    .local pmc temp
    .local int i
    .local int id
    
    classoffset id, self, "Tetris::BoardData"
    
    # create the data array
    new data, .ResizablePMCArray
    setattribute self, id, data
    
    # calculate the array size
    set i, w
    mul i, h
    # resize the array
    set data, i

    # store the width
    new temp, .Integer
    set temp, w
    inc id
    setattribute self, id, temp

    # store the height
    new temp, .Integer
    set temp, h
    inc id
    setattribute self, id, temp
.end

=back

=head1 METHODS

=over 4

=item data."fill"( val )

Fill the data array with the specified value

=over 4

=item parameter C<val>

The value the board will be filled with.

=back

This method returns nothing.

=cut

.sub fill :method
    .param int val
    .local pmc data
    .local int i

    classoffset $I0, self, "Tetris::BoardData"
    getattribute data, self, $I0
    
    # get data size    
    set i, data
    # fill the data
WHILE:
    dec i
    if i < 0 goto END
    set data[i], val
    branch WHILE
END:
.end

.sub __get_integer :method
    classoffset $I0, self, "Tetris::BoardData"
    getattribute $P0, self, $I0
    $I0 = $P0
    .return ($I0)
.end

.sub __get_integer_keyed :method
    .param pmc key
    
    classoffset $I0, self, "Tetris::BoardData"
    getattribute $P0, self, $I0
    $I0 = key
    $I1 = $P0
    if $I0 < $I1 goto OK
    print "error: out of bounds ("
    print $I0
    print "; "
    print $I1
    print ")!\n"
    sleep 0.1
OK:
    $I0 = $P0[$I0]
    
    .return ($I0)
.end

.sub __set_integer_keyed :method
    .param pmc key
    .param int val

    classoffset $I0, self, "Tetris::BoardData"
    getattribute $P0, self, $I0
    $P0[key] = val
.end

.sub __set_integer_native :method
    .param int val

    classoffset $I0, self, "Tetris::BoardData"
    getattribute $P0, self, $I0
    $P0 = val
.end

.sub __push_integer :method
    .param int val

    classoffset $I0, self, "Tetris::BoardData"
    getattribute $P0, self, $I0
    push $P0, val
.end

=item width = data."width"( self )

Returns the width (number of blocks in one row) of the board.

=cut

.sub width :method
    classoffset $I0, self, "Tetris::BoardData"
    add $I0, bWidth
    getattribute $P0, self, $I0
    $I0 = $P0
    .return ($I0)
.end

=item height = data."height"()

Returns the height (number of blocks in one column) of the board.

=cut

.sub height :method
    classoffset $I0, self, "Tetris::BoardData"
    add $I0, bHeight
    getattribute $P0, self, $I0
    $I0 = $P0
    .return ($I0)
.end

=item (width, height) = data."dimensions"()

Returns the width and height of the board.

=cut

.sub dimensions :method
    .local int w
    .local int h
    .local pmc temp

    classoffset $I0, self, "Tetris::BoardData"
    add $I0, bWidth
    getattribute $P0, self, $I0
    w = $P0
    sub $I0, bWidth

    add $I0, bHeight
    getattribute $P0, self, $I0
    h = $P0
    
    .return (w, h)
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
