# Copyright (C) 2005-2008, Parrot Foundation.
# $Id$

package init::hints::solaris;

use strict;
use warnings;

sub runstep {
    my ( $self, $conf ) = @_;

    my $libs = $conf->data->get('libs');
    if ( $libs !~ /-lpthread\b/ ) {
        $libs .= ' -lpthread';
    }
    if ( $libs !~ /-lrt\b/ ) {
        $libs .= ' -lrt';    # Needed for sched_yield.
    }
    $conf->data->set( libs => $libs );

    ################################################################
    # If we're going to be using ICU (or any other C++-compiled library) we
    # need to use the c++ compiler as a linker.  As soon as the user
    # selects a compiler, we will run the gccversion test.  (If we were to
    # wait till it's normally run, the linker question would have already
    # been asked.)
    my $solaris_link_cb = sub {
        use Carp;
        my ( $key, $cc ) = @_;
        my %gnuc;
        my $link = $conf->data->get('link');
        $conf->cc_gen("config/auto/gcc/test_c.in");

        # Can't call cc_build since we haven't set all the flags yet.
        # This should suffice for this test.
        my $cc_inc = $conf->data->get('cc_inc');
        Parrot::Configure::Utils::_run_command( "$cc -o test_$$ test_$$.c",
            "test_$$.cco", "test_$$.cco" )
            and confess "C compiler failed (see test_$$.cco)";
        %gnuc = eval $conf->cc_run() or die "Can't run the test program: $!";
        if ( defined $gnuc{__GNUC__} ) {
            $link = 'g++';
        }
        else {
            $link =~ s/\bcc\b/CC/;
        }
        unless ($conf->data->get('rpath')) {
            $conf->data->set( 'rpath', '-R' );
        }
        $conf->data->set( link => $link );
        $conf->data->deltrigger( "cc", "solaris_link" );
    };
    $conf->data->settrigger( "cc", "solaris_link", $solaris_link_cb );

    ################################################################
    # cc_shared:  Flags to instruct the compiler to use position-independent
    # code for use in shared libraries.  -KPIC for Sun's compiler, -fPIC for
    # gcc.  We don't know which compiler we're using till after the
    # gccversion test.
    my $solaris_cc_shared_cb = sub {
        my ( $key, $gccversion ) = @_;

        if ($gccversion) {
            $conf->data->set( cc_shared => '-fPIC' );
        }
        else {
            $conf->data->set( cc_shared => '-KPIC' );
        }
        $conf->data->set( 'has_dynamic_linking', '1' );
        $conf->data->set( 'parrot_is_shared', '1' );
        $conf->data->deltrigger( "gccversion", "solaris_cc_shared" );
    };
    $conf->data->settrigger(
        "gccversion", "solaris_cc_shared", $solaris_cc_shared_cb );

    ################################################################
    # Parrot usually aims for IEEE-754 compliance.
    # For Solaris 8/Sun Workshop Pro 4, both
    #    atan2( 0.0, -0.0) and atan2(-0.0, -0.0)
    # return 0, when they should return +PI and -PI respectively.
    # For Sun's compilers, fix this with the -xlibmieee flag.
    # I don't know of an equivalent flag for gcc.
    # (Alternatively, and more generally, perhaps we should run an
    # ieee-conformance test and then call back into a hints-file trigger
    # to set platform-specific flags.)
    #   A. Dougherty  7 March 2005
    # We don't know which compiler we're using till after the gccversion test.
    my $solaris_ieee_cb = sub {
        my ( $key, $gccversion ) = @_;

        if ($gccversion) {

            # Don't know how to do this for gcc.
        }
        else {
            my $linkflags = $conf->data->get('linkflags');
            $conf->data->add( ' ', linkflags => '-xlibmieee' )
                unless $linkflags =~ /-xlibmieee/;
        }
        $conf->data->deltrigger( "gccversion", "solaris_ieee" );
    };
    $conf->data->settrigger( "gccversion", "solaris_ieee", $solaris_ieee_cb );

}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
