=head1 COMMON FUNCTIONS FOR TESTING

=cut

.sub '_parse_buffer'
    .param string buffer
    
    .local pmc compiler
    compiler = compreg 'Ops'
    .tailcall compiler.'parse'(buffer, 'target'=>'parse')
.end


.sub '_slurp'
    .param string file
    .local pmc pio
    pio  = open file
    $S0  = pio.'readall'()
    close pio
    .return ($S0)
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
