# Copyright (C) 2001-2004, Parrot Foundation.
# $Id$

=head1 NAME

config/auto/boehm.pm - Test for Boehm GC availability.

=head1 DESCRIPTION

Determines whether the platform supports Boehm GC.

From L<http://www.hpl.hp.com/personal/Hans_Boehm/gc/>: "The Boehm-Demers-Weiser
conservative garbage collector can be used as a garbage collecting replacement
for C malloc or C++ new. It allows you to allocate memory basically as you
normally would, without explicitly deallocating memory that is no longer
useful. The collector automatically recycles memory when it determines that it
can no longer be otherwise accessed."

=cut

package auto::boehm;

use strict;
use warnings;

use base qw(Parrot::Configure::Step);

use Parrot::Configure::Utils ':auto';

sub _init {
    my $self = shift;
    my %data;
    $data{description} = q{Does your platform support Boehm GC};
    $data{result}      = q{};

    return \%data;
}

# This is actually stub for detecting Boehm GC. Requires testing on Win32 and
# other platforms.
sub runstep {
    my ( $self, $conf ) = @_;

    my ( $verbose, $without ) = $conf->options->get(
        qw|
            verbose
            without-boehm-gc
        |
    );

    if ($without) {
        $conf->data->set( has_boehm_gc => 0 );
        $self->set_result('no');
        return 1;
    }

    my $osname = $conf->data->get('osname');

    my $extra_libs = $self->_select_lib( {
        conf            => $conf,
        osname          => $osname,
        cc              => $conf->data->get('cc'),
        win32_nongcc    => 'gc.lib',
        default         => '-lgc',
    } );

    my $header = "gc.h";
    $conf->data->set( TEMP_testheaders => "#include <$header>\n" );
    $conf->data->set( TEMP_testheader  => "$header" );
    $conf->cc_gen('config/auto/headers/test_c.in');

    # Clean up.
    $conf->data->set( TEMP_testheaders => undef );
    $conf->data->set( TEMP_testheader  => undef );

    eval { $conf->cc_build( '-I/usr/include/gc', '-lgc' ); };

    my $has_boehm = ( ! $@ && $conf->cc_run() =~ /^$header OK/ );
    if ($has_boehm) {
        $conf->data->add( ' ', libs => $extra_libs );
        $conf->data->add( ' ', ccflags => '-I/usr/include/gc' );
    }
    $self->set_result($has_boehm ? 'yes' : 'no');
    $conf->data->set( HAS_BOEHM_GC => $has_boehm);

    return 1;
}


1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
