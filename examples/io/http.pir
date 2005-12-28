# Copyright (C) 2001-2003 The Perl Foundation.  All rights reserved.
# $Id$

=head1 NAME

examples/io/http.pir - HTTP client

=head1 SYNOPSIS

    % ./parrot examples/io/http.pir

=head1 DESCRIPTION

HTTP client, connects to WWW port and grabs a page (L<http://www.ibm.com>). 

You should be running the echo service on your box (port 7). Be sure to
set C<PARROT_NET_DEVEL> to 1 in F<io/io_private.h> and rebuld Parrot or
the network layer won't exist.

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
    #sockaddr address, 80, "129.42.18.99"
    sockaddr address, 80, "www.ibm.com"
    print "Connecting to http://www.ibm.com:80\n"
    connect ret, sock, address 
    print "connect returned "
    print ret 
    print "\n"
    send ret, sock, "GET /us/ HTTP/1.0\nUser-agent: Parrot\n\n" 
    poll ret, sock, 1, 5, 0
MORE:
    recv ret, sock, buf 
    if ret < 0 goto END
    print buf 
    goto MORE 
ERR:
    print "Socket error\n"
    end
END:
    close sock 
    end
.end

=head1 SEE ALSO

F<io/io_private.h>.

=cut
