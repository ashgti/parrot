# Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

Parrot::Docs::Section::Config - Configuration docmentation section

=head1 SYNOPSIS

	use Parrot::Docs::Section::Config;
	my $section = Parrot::Docs::Section::Config->new;

=head1 DESCRIPTION

A documentation section describing Parrot's configuration system.

=head2 Methods

=over

=cut

package Parrot::Docs::Section::Config;

use strict;

use Parrot::Docs::Section;
@Parrot::Docs::Section::Config::ISA = qw(Parrot::Docs::Section);

=item C<new()>

Returns a new section.

=cut

sub new
{
	my $self = shift;
	
	return $self->SUPER::new(
		'Configuration', 'config.html', '',
		$self->item('', 'Configure.pl'),
		$self->group('Initialization', '',
			$self->item('', 'config/init'),
		),
		$self->group('User Dialogues', '',
			$self->item('', 'config/inter'),
		),
		$self->group('System Tests', '',
			$self->item('', 'config/auto'),
		),
		$self->group('File Creation', '',
			$self->item('', 'config/gen'),
		),
	);
}

=back

=cut

1;