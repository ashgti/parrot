# Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

Parrot::Docs::Section::Info - Project info docmentation section

=head1 SYNOPSIS

	use Parrot::Docs::Section::Info;
	
	my $section = Parrot::Docs::Section::Info->new;

=head1 DESCRIPTION

A documentation section describing Parrot's project info.

=head2 Methods

=over

=cut

package Parrot::Docs::Section::Info;

use strict;

use Parrot::Docs::Section;
@Parrot::Docs::Section::Info::ISA = qw(Parrot::Docs::Section);

=item C<new()>

Returns a new section.

=cut

sub new
{
	my $self = shift;
	
	return $self->SUPER::new(
		'Project Information', 
		'info.html', 
		'A quick look at the conventional all-caps project information files which contain assorted bits and pieces of information about the distribution. There\'s not much to detain us here except for the README file which is of course just asking to be read.',
		$self->new_group('Documentation', '',
			$self->new_item('General information about licences, prerequisites, and building instructions.', 'README'),
			$self->new_item('This file lists what\'s new in each version.', 'NEWS'),
			$self->new_item('This is a list of thing that are known to be broken or at least provisionally implemented. You\'d be best to check in CVS to see how up-to-date it is.', 'KNOWN_ISSUES'),
			$self->new_item('A list of things that need doing in Parrot. Best to check CVS to see how up-to-date this is.', 'TODO'),
			$self->new_item('This is a description the steps someone should follow when they have to prepare a new version for release. We\'re a long way from being in that position.', 'RELEASE_INSTRUCTIONS'),
		),
		$self->new_group('Legal', '',
			$self->new_item('The licences under which Parrot is distributed.', 'LICENSES/Artistic', 'LICENSES/gpl.txt'),
		),
		$self->new_group('Status', '',
			$self->new_item('Contains the version number of the distribution.', 'VERSION'),
			$self->new_item('The presence of this file indicates that the distribution is a development version.', 'DEVELOPING'),
		),
		$self->new_group('People', '',
			$self->new_item('A list of some of the Parrot developers with CVS commit access. Best to check CVS to see how up-to-date this is.', 'RESPONSIBLE_PARTIES'),
			$self->new_item('A list of some of people who have contributed to Parrot.', 'CREDITS'),
		),
		$self->new_group('Contents', 'See the ExtUtils::Manifest documentation for more information about MANIFEST files.',
			$self->new_item('Lists all the files in the distribution, with their metadata.', 'MANIFEST'),
			$self->new_item('Lists all the generated files, i.e. the files that will be added to the distribution after configuration and installing. There\'s a note on this in CVS.', 'MANIFEST.generated'),
			$self->new_item('Specifies the files to leave out of MANIFEST.', 'MANIFEST.SKIP'),
		),
		$self->new_group('History', '',
			$self->new_item('This should provide a timeline of changes made to the project - just depends on how often it is updated.', 'ChangeLog'),
			$self->new_item('Documents incompatible changes to Parrot that invalidate existing bytecode files. This is the new experimental alternative to the fingerprint produced by build_tools/fingerprint_c.pl.', 'PBC_COMPAT'),
		),
	);
}

=back

=cut

1;