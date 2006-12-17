# Copyright (C) 2004, The Perl Foundation.
# $Id$

=head1 NAME

Parrot::Docs::Section::Ops - Parrot ops documentation section

=head1 SYNOPSIS

        use Parrot::Docs::Section::Ops;

=head1 DESCRIPTION

A documentation section describing the Parrot ops.

=head2 Class Methods

=over

=cut

package Parrot::Docs::Section::Ops;

use strict;
use warnings;

use base qw( Parrot::Docs::Section );

=item C<new()>

Returns a new section.

=cut

sub new {
    my $self = shift;

    return $self->SUPER::new(
        'Ops',
        'ops.html',
        '',
        $self->new_group(
            'Tools',
            '',
            $self->new_item( '', 'tools/build/ops2c.pl' ),
            $self->new_item( '', 'src/ops/ops.num' ),
            $self->new_item( '', 'tools/build/ops2pm.pl' ),
        ),
        $self->new_group( 'Op Libs', '', 'src/ops' ),
    );
}

=back

=cut

1;
