# Copyright (C) 2009, Parrot Foundation.
# $Id$

class Ops::Op;

=head1 NAME

C<Ops::Op> - Parrot Operation

=head1 DESCRIPTION

Extends PAST::Block with various helper methods.

=head2 Methods

=over 4

=item C<set_body>

Set the body of this Op and do some processing on the body to determine which
jump flags need to be set.

=cut

method set_body($body) {
    my %jumps;
    my @jumps;

    #figure out which control flow flags need to be set for this op
    if (match( " 'goto' \s+ 'ADDRESS' ", $body)) {
        %jumps{'PARROT_JUMP_ADDRESS'} := 1;
    }

    if (match( " 'goto' \s+ 'OFFSET' ", $body) ||
        self.name eq 'runinterp' ) {
        %jumps{'PARROT_JUMP_RELATIVE'} := 1;
    }

    if (match( " 'goto' \s+ 'POP' ", $body)) {
        %jumps{'PARROT_JUMP_POP'} := 1;
    }

    if (match( " 'expr' \s+ 'NEXT' ", $body) ||
        self.name eq 'runinterp' ) {
        %jumps{'PARROT_JUMP_ENEXT'} := 1;
    }

    if (match( " 'restart' \s+ 'OFFSET' ", $body)) {
        %jumps{'PARROT_JUMP_RELATIVE'} := 1;
        %jumps{'PARROT_JUMP_RESTART'}  := 1;
    }
    elsif (match( " 'restart' \s+ 'OFFSET' ", $body)) {
        %jumps{'PARROT_JUMP_RESTART'} := 1;
        %jumps{'PARROT_JUMP_ENEXT'}   := 1;
    }
    elsif (self.name eq 'branch_cs' || self.name eq 'returncc' ) {
        %jumps{'PARROT_JUMP_RESTART'} := 1;
    }
    elsif (match( " 'restart' \s+ 'ADDRESS' ", $body)) {
        %jumps{'PARROT_JUMP_RESTART'} := 1;
        %jumps{'PARROT_JUMP_ENEXT'}   := 0;
    }

    #XXX: need to handle PARROT_JUMP_GNEXT
    
    for %jumps {
        if %jumps{$_} {
            @jumps.push($_);
        }
    }
    
    if +@jumps == 0 {
        self<jump_flags> := '0';
    }
    else {
        self<jump_flags> := join('|', @jumps);
    }
}



