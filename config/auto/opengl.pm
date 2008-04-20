# Copyright (C) 2008, The Perl Foundation.
# $Id $

=head1 NAME

config/auto/opengl.pm - Probe for OpenGL, GLU, and GLUT libraries

=head1 DESCRIPTION

Determines whether the platform supports OpenGL, GLU and GLUT.  The optimal
result at this time is to find OpenGL 2.1, GLU 1.3, and GLUT API version 4.

=cut

package auto::opengl;

use strict;
use warnings;
use File::Spec;

use base qw(Parrot::Configure::Step);

use Parrot::Configure::Utils ':auto';

sub _init {
    my $self = shift;
    my %data;
    $data{description} = q{Determining if your platform supports OpenGL};
    $data{result}      = q{};
    return \%data;
}

sub runstep {
    my ( $self, $conf ) = @_;

    my ( $verbose, $without ) = $conf->options->get(
        qw|
            verbose
            without-opengl
        |
    );

    if ($without) {
        $conf->data->set( has_opengl => 0 );
        $self->set_result('no');
        return 1;
    }

    my $cc        = $conf->data->get('cc');
    my $libs      = $conf->data->get('libs');
    my $linkflags = $conf->data->get('linkflags');
    my $ccflags   = $conf->data->get('ccflags');

    my $osname = $conf->data->get_p5('OSNAME');

    _handle_mswin32($conf, $osname, $cc);

    # On OS X check the presence of the OpenGL headers in the standard
    # Fink/macports locations.
    # Mindlessly morphed from readline ... may need to be fixed
    $self->_handle_darwin_for_fink    ($conf, $osname, 'GL/glut.h');
    $self->_handle_darwin_for_macports($conf, $osname, 'GL/glut.h');

    $conf->cc_gen('config/auto/opengl/opengl.in');
    my $has_glut = 0;
    eval { $conf->cc_build() };
    if ( !$@ ) {
        my $test = $conf->cc_run();
        $has_glut = _handle_glut($conf, $self->_evaluate_cc_run($test, $verbose));
    }
    unless ($has_glut) {
        # The Parrot::Configure settings might have changed while class ran
        $self->_recheck_settings($conf, $libs, $ccflags, $linkflags, $verbose);
    }

    return 1;
}

sub _handle_mswin32 {
    my ($conf, $osname, $cc) = @_;
    # Mindlessly morphed from readline ... may need to be fixed
    if ( $osname =~ /mswin32/i ) {
        if ( $cc =~ /^gcc/i ) {
            $conf->data->add( ' ', libs => '-lglut32 -lglu32 -lopengl32' );
        }
        else {
            $conf->data->add( ' ', libs => 'glut.lib glu.lib gl.lib' );
        }
    }
    elsif ( $osname =~ /darwin/i ) {
        $conf->data->add( ' ', libs => '-framework OpenGL -framework GLUT' );
    }
    else {
        $conf->data->add( ' ', libs => '-lglut -lGLU -lGL' );
    }
    return 1;
}

sub _evaluate_cc_run {
    my ($self, $test, $verbose) = @_;
    my ($glut_api_version, $glut_brand) = split ' ', $test;

    print " (yes, $glut_brand API version $glut_api_version) " if $verbose;
    $self->set_result("yes, $glut_brand $glut_api_version");

    return ($glut_api_version, $glut_brand);
}

sub _handle_glut {
    my ($conf, $glut_api_version, $glut_brand) = @_;

    $conf->data->set(
        # Completely cargo culted
        opengl     => 'define',
        has_opengl => 1,
        HAS_OPENGL => 1,

        glut       => 'define',
        glut_brand => $glut_brand,
        has_glut   => $glut_api_version,
        HAS_GLUT   => $glut_api_version,
    );

    return 1;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
