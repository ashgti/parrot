# Copyright: 2001-2004 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

config/auto/gmp.pl - Test for GNU MP (GMP) Math library

=head1 DESCRIPTION

Determines whether the platform supports GMP.

=cut

package Configure::Step;

use strict;
use vars qw($description @args);
use Parrot::Configure::Step ':auto';

$description="Determining if your platform supports GMP...";

@args=qw(verbose);

sub runstep {
    my ($verbose) = @_;

    my $libs = Configure::Data->get('libs');
    my $linkflags = Configure::Data->get('linkflags');
    my $ccflags = Configure::Data->get('ccflags');
    Configure::Data->add(' ', 'libs', '-lgmp');

    my $archname = $Config{archname};
    my ($cpuarch, $osname) = split('-', $archname);
    if(!defined $osname) {
        ($osname, $cpuarch) = ($cpuarch, "");
    }

    # On OS X check the presence of the gmp header in the standard
    # Fink location. TODO: Need a more generalized way for finding
    # where Fink lives.
    if($osname =~ /darwin/) {
        if( -f "/sw/include/gmp.h") {
            Configure::Data->add(' ', 'linkflags', '-L/sw/lib');
            Configure::Data->add(' ', 'ldflags', '-L/sw/lib');
            Configure::Data->add(' ', 'ccflags', '-I/sw/include');
        }
    }

    cc_gen('config/auto/gmp/gmp.in');
    eval { cc_build(); };
    my $has_gmp = 0;
    if (! $@) {
	my $test = cc_run();
	if ($test eq "499999500000 0\n") {
            $has_gmp = 1;
	    print " (yes) " if $verbose;
            $Configure::Step::result = 'yes';

	    Configure::Data->set(
		gmp     => 'define',
		HAS_GMP => $has_gmp,
	    );
	}
    }
    unless ( $has_gmp ) {
        # The Config::Data settings might have changed for the test
        Configure::Data->set('libs', $libs);
        Configure::Data->set('ccflags', $ccflags);
        Configure::Data->set('linkflags', $linkflags);
        print " (no) " if $verbose;
        $Configure::Step::result = 'no';
    }
}

