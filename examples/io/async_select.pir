# Copyright (C) 2006-2008, The Perl Foundation.
# $Id$

=head1 NAME

examples/io/async_select.pir - Asynchronous Select Example

=head1 SYNOPSIS

  $ ./parrot examples/io/async_select.pir

=head1 DESCRIPTION

This program is showing async select utilizing the io_thread event handler.
After you started the program, you will see a spinning progress marker,
showing that the program is idle. Type some chars at the console (due
to console buffering, you have to press <enter> too).
These chars will be displayed in bunches of four, as long as more chars are
available.

=cut

.include 'io_thr_msg.pasm'

.sub main :main
    .local pmc handler, pio, data, pio_out
    .local string idles
    .local int i

    pio = getstdin
    $S0 = pop pio   # unbuffer
    data = new 'Integer'
    data = 42
    .const 'Sub' handler = "io_handler"
    pio_out = getstdout
    idles = '|/-\'
    i = 0
spin:
    # XXX: this opcode is long gone
    # the IO event is inactive, after it fired, just reattach always
    # add_io_event pio, handler, data, .IO_THR_MSG_ADD_SELECT_RD
    sleep 0.2
    $S0 = substr idles, i, 1
    inc i
    $I0 = length idles
    i %= $I0
    print $S0
    print "\r"
    pio_out.'flush'()
    goto spin
.end

# io_handler called by the select ready condition from event code
.sub io_handler
    .param pmc pio
    .param pmc data
    $S0 = read pio, 4	# arbitray - could of course read more
    $I0 = length $S0
    unless $I0 goto ex
    print $S0
    print " - "
    print data
    print "\n"
    .return()
ex:
    exit 0
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
