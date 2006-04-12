=head1 TITLE

blocks.pir - tetris block classes

=cut

.namespace ["Tetris::Blocks"]

.sub __onload :load
    find_type $I0, "Tetris::Block::1"
    if $I0 > 1 goto END

    load_bytecode "examples/sdl/tetris/block.pir"

    getclass $P1, "Tetris::Block"

    $P2 = new .String
    $P2 = "__init"

    subclass $P0, $P1, "Tetris::Block::0"
    setprop $P0, "BUILD", $P2

    subclass $P0, $P1, "Tetris::Block::1"
    setprop $P0, "BUILD", $P2
    
    subclass $P0, $P1, "Tetris::Block::2"
    setprop $P0, "BUILD", $P2
    
    subclass $P0, $P1, "Tetris::Block::3"
    setprop $P0, "BUILD", $P2
    
    subclass $P0, $P1, "Tetris::Block::4"
    setprop $P0, "BUILD", $P2
    
    subclass $P0, $P1, "Tetris::Block::5"
    setprop $P0, "BUILD", $P2
    
    subclass $P0, $P1, "Tetris::Block::6"
    setprop $P0, "BUILD", $P2
    
    .local pmc blocks
    .local pmc block
    
    blocks = new IntList
    
    find_type $I0, "Tetris::Block::0"
    push blocks, $I0
    find_type $I0, "Tetris::Block::1"
    push blocks, $I0
    find_type $I0, "Tetris::Block::2"
    push blocks, $I0
    find_type $I0, "Tetris::Block::3"
    push blocks, $I0
    find_type $I0, "Tetris::Block::4"
    push blocks, $I0
    find_type $I0, "Tetris::Block::5"
    push blocks, $I0
    find_type $I0, "Tetris::Block::6"
    push blocks, $I0
    
    store_global "Tetris::Block", "blocks", blocks
END:
.end

.namespace ["Tetris::Block::0"]

# ##
# ##
.sub __init method
    .local pmc block

    block = new IntList
    push block, 1
    push block, 1
    push block, 1
    push block, 1
    assign self, block
.end

.sub id method
    .pcc_begin_return
    .return 0
    .pcc_end_return
.end

.namespace ["Tetris::Block::1"]

# ###
# #..
# ...
.sub __init method
    .local pmc block

    block = new IntList
    push block, 1
    push block, 1
    push block, 1
    push block, 1
    push block, 0
    push block, 0
    push block, 0
    push block, 0
    push block, 0
    assign self, block
.end

.sub id method
    .pcc_begin_return
    .return 1
    .pcc_end_return
.end

.namespace ["Tetris::Block::2"]

# ###
# ..#
# ...
.sub __init method
    .local pmc block
    
    block = new IntList
    push block, 1
    push block, 1
    push block, 1
    push block, 0
    push block, 0
    push block, 1
    push block, 0
    push block, 0
    push block, 0
    assign self, block
.end

.sub id method
    .pcc_begin_return
    .return 2
    .pcc_end_return
.end

.namespace ["Tetris::Block::3"]

# ##.
# .##
# ...
.sub __init method
    .local pmc block
    
    block = new IntList
    push block, 1
    push block, 1
    push block, 0
    push block, 0
    push block, 1
    push block, 1
    push block, 0
    push block, 0
    push block, 0
    assign self, block
.end

.sub id method
    .pcc_begin_return
    .return 3
    .pcc_end_return
.end

.namespace ["Tetris::Block::4"]

# .##
# ##.
# ...
.sub __init method
    .local pmc block
    
    block = new IntList
    push block, 0
    push block, 1
    push block, 1
    push block, 1
    push block, 1
    push block, 0
    push block, 0
    push block, 0
    push block, 0
    assign self, block
.end

.sub id method
    .pcc_begin_return
    .return 4
    .pcc_end_return
.end

.namespace ["Tetris::Block::5"]

# ###
# .#.
# ...
.sub __init method
    .local pmc block
    
    block = new IntList
    push block, 1
    push block, 1
    push block, 1
    push block, 0
    push block, 1
    push block, 0
    push block, 0
    push block, 0
    push block, 0
    assign self, block
.end

.sub id method
    .pcc_begin_return
    .return 5
    .pcc_end_return
.end

.namespace ["Tetris::Block::6"]

# ####
# ....
# ....
# ....
.sub __init method
    .local pmc block
    
    block = new IntList
    push block, 1
    push block, 1
    push block, 1
    push block, 1
    push block, 0
    push block, 0
    push block, 0
    push block, 0
    push block, 0
    push block, 0
    push block, 0
    push block, 0
    push block, 0
    push block, 0
    push block, 0
    push block, 0
    assign self, block
.end

.sub id method
    .pcc_begin_return
    .return 6
    .pcc_end_return
.end

=head1 AUTHOR

Jens Rieks E<lt>parrot at jensbeimsurfen dot deE<gt> is the author
and maintainer.
Please send patches and suggestions to the Perl 6 Internals mailing list.

=head1 COPYRIGHT

Copyright (c) 2004, the Perl Foundation.

=cut
