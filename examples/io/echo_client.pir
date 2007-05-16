# Copyright (C) 2001-2007, The Perl Foundation.
# $Id$

=head1 NAME

examples/io/echo_client.pir - Echo client

=head1 SYNOPSIS

    % ./parrot examples/io/echo_client.pir

=head1 DESCRIPTION

C<echo> client, connects to C<echo> service and echoes what you type.

You should be running the echo service on your box (port 7). Be sure to
set C<PARROT_NET_DEVEL> to 1 in F<io/io_private.h> and rebuld Parrot or
the network layer won't exist.

Shortly we will create an echo server in Parrot and combine these 2 into
a test suite.

=cut

.sub _main
    .local pmc sock
    .local string address
    .local string buf
    .local int ret
    .local int len
    print "Creating socket.\n"
    # create the socket handle 
    socket sock, 2, 1, 0
    unless sock goto ERR
    # Pack a sockaddr_in structure with IP and port
    sockaddr address, 7, "127.0.0.1"
    print "Connecting to port 7 (ECHO)\n"
    connect ret, sock, address 
    print "connect returned "
    print ret 
    print "\n"
COMMAND:
    print "echo>"
    read buf, 100
    chopn buf, 1
    length len, buf 
    if len <= 0 goto END
    concat buf, "\n"
    send ret, sock, buf 
    poll ret, sock, 1, 5, 0
    recv ret, sock, buf 
    if ret < 0 goto END
    print buf 
    branch COMMAND
ERR:
    print "Socket error, you may not have compiled with PARROT_SOCKET_DEV (see io/io_private.h)\n"
    end
END:
    close sock 
    end
.end

=head1 SEE ALSO

F<io/io_private.h>.

=cut


# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
