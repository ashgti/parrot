# Copyright (C) 2004-2006, Parrot Foundation.
# $Id$

=head1 NAME

Parrot::Docs::Section::PMCs - PMCs documentation section

=head1 SYNOPSIS

        use Parrot::Docs::Section::PMCs;

=head1 DESCRIPTION

A documentation section describing all the PMCs.

=head2 Class Methods

=over

=cut

package Parrot::Docs::Section::PMCs;

use strict;
use warnings;

use Parrot::Distribution;

use base qw( Parrot::Docs::Section );

=item C<new()>

Returns a new section.

=cut

sub new {
    my $self = shift;
    my $dist = Parrot::Distribution->new;
    my $dir  = $dist->existing_directory_with_name('src/pmc');

    my @concrete_items = ();
    my @abstract_items = ();

    foreach my $file ( $dir->files_with_suffix('pmc') ) {
        my $code = $file->read;

        if ( $code =~ /^pmclass\s+[A-Z]/smo ) {
            push( @concrete_items, $self->new_item( '', $dist->relative_path($file) ) );
        }
        elsif ( $code =~ /^pmclass\s+[a-z]/smo ) {
            push( @abstract_items, $self->new_item( '', $dist->relative_path($file) ) );
        }
    }

    return $self->SUPER::new(
        'PMCs',
        'pmc.html',
        'PMCs are Parrot\'s internal "classes". There are currently '
            . scalar(@abstract_items)
            . ' abstract PMCs and '
            . scalar(@concrete_items)
            . ' concrete PMCs.',
        $self->new_group(
            'Tools',
            'PMC-related tools.',
            $self->new_item( '', 'tools/dev/gen_class.pl' ),
        ),
        $self->new_group( 'Abstract PMCs', 'These PMCs are not instantiated.', @abstract_items ),
        $self->new_group( 'Concrete PMCs', 'These PMCs are instantiated.',     @concrete_items )
    );
}

=back

=cut

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
