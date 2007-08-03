# Copyright (C) 2005-2007, The Perl Foundation.
# $Id$

=head1 NAME

config/auto/msvc.pm - Microsoft Visual C++ Compiler

=head1 DESCRIPTION

Determines whether the C compiler is actually C<Visual C++>.

=cut

package auto::msvc;

use strict;
use warnings;
use vars qw($description @args);

use base qw(Parrot::Configure::Step::Base);

use Parrot::Configure::Step ':auto';

$description = 'Determining if your C compiler is actually Visual C++';

@args = qw(verbose);

sub runstep {
    my ( $self, $conf ) = ( shift, shift );

    my $verbose = $conf->options->get('verbose');

    cc_gen("config/auto/msvc/test_c.in");
    cc_build();
    my %msvc = eval cc_run() or die "Can't run the test program: $!";
    cc_clean();

    # Set msvcversion to undef.  This will also trigger any hints-file
    # callbacks that depend on knowing whether or not we're using Visual C++.

    # This key should always exist unless the program couldn't be run,
    # which should have been caught by the 'die' above.
    # Therefore, test if it's defined to see if MSVC's installed.
    # return 'no' if it's not.
    unless ( defined $msvc{_MSC_VER} ) {
        $self->set_result('no');
        $conf->data->set( msvcversion => undef );
        return $self;
    }

    my $major = int( $msvc{_MSC_VER} / 100 );
    my $minor = $msvc{_MSC_VER} % 100;
    unless ( defined $major && defined $minor ) {
        print " (no) " if $verbose;
        $self->set_result('no');
        $conf->data->set( msvcversion => undef );
        return $self;
    }

    my $msvcversion = "$major.$minor";
    print " (yep: $msvcversion )" if $verbose;
    $self->set_result('yes');

    $conf->data->set( msvcversion => $msvcversion );

    # Add Visual C++ specifics here
    if ( $msvcversion >= 14.00 ) {

        # Version 14 (aka Visual C++ 2005) warns about unsafe, deprecated
        # functions with the following message.
        #
        # This function or variable may be unsafe. Consider using xxx_s instead.
        # To disable deprecation, use _CRT_SECURE_NO_DEPRECATE. See online help
        # for details.
        $conf->data->add( " ", "ccflags", "-D_CRT_SECURE_NO_DEPRECATE" );
    }

    return $self;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
