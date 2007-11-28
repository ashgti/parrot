# Copyright (C) 2001-2007, The Perl Foundation.
# $Id$

=head1 NAME

config/auto/gcc.pm - GNU C Compiler

=head1 DESCRIPTION

Determines whether the C compiler is actually C<gcc>.

=cut

package auto::gcc;

use strict;
use warnings;

use base qw(Parrot::Configure::Step::Base);

use Parrot::Configure::Step ':auto';


sub _init {
    my $self = shift;
    my %data;
    $data{description} = q{Determining if your C compiler is actually gcc};
    $data{args}        = [ qw( miniparrot verbose ) ];
    $data{result}      = q{};
    return \%data;
}

sub runstep {
    my ( $self, $conf ) = @_;
    my $gnucref = _probe_for_gcc();
    my $rv = $self->_evaluate_gcc($conf, $gnucref);
    return $rv;
}

sub _probe_for_gcc {
    cc_gen("config/auto/gcc/test_c.in");
    cc_build();
    my %gnuc = eval cc_run() or die "Can't run the test program: $!";
    cc_clean();
    return \%gnuc;
}

sub _evaluate_gcc {
    my ($self, $conf, $gnucref) = @_;

    # Set gccversion to undef.  This will also trigger any hints-file
    # callbacks that depend on knowing whether or not we're using gcc.

    # This key should always exist unless the program couldn't be run,
    # which should have been caught by the 'die' above.
    unless ( exists $gnucref->{__GNUC__} ) {
        $conf->data->set( gccversion => undef );
        return 1;
    }

    my $major = $gnucref->{__GNUC__};
    my $minor = $gnucref->{__GNUC_MINOR__};
    my $intel = $gnucref->{__INTEL_COMPILER};

    my $verbose = $conf->options->get('verbose');
    if ( defined $intel || !defined $major ) {
        print " (no) " if $verbose;
        $self->set_result('no');
        $conf->data->set( gccversion => undef );
        return 1;
    }
    if ( $major =~ tr/0-9//c ) {
        undef $major;    # Don't use it
    }
    if ( defined $minor and $minor =~ tr/0-9//c ) {
        undef $minor;    # Don't use it
    }
    if ( ! defined $major ) {
        print " (no) " if $verbose;
        $self->set_result('no');
        $conf->data->set( gccversion => undef );
        return 1;
    }
    my $gccversion = $major;
    $gccversion .= ".$minor" if defined $minor;
    print " (yep: $gccversion )" if $verbose;
    $self->set_result('yes');

    my $ccwarn = $conf->data->get('ccwarn');

    $conf->data->set( sym_export => '__attribute__ ((visibility("default")))' )
        if $gccversion >= 4.0;

    if ( defined $conf->options->get('miniparrot') ) {
        # make the compiler act as ANSIish as possible, and avoid enabling
        # support for GCC-specific features.
        $conf->data->set(
            ccwarn     => "-ansi -pedantic",
            gccversion => undef
        );
    }
    else {
        $conf->data->set(
            ccwarn              => "$ccwarn",
            gccversion          => $gccversion,
            HAS_aligned_funcptr => 1
        );
        $conf->data->set( HAS_aligned_funcptr => 0 )
            if $^O eq 'hpux';
    }
    return 1;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
