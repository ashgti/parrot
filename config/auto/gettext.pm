# Copyright (C) 2008, The Perl Foundation.
# $Id$

=head1 NAME

config/auto/gettext.pm - Test for GNU native language support (gettext) library

=head1 DESCRIPTION

Determines whether the platform supports gettext. This is needed for parrot
internationalization.

From L<http://www.gnu.org/software/gettext/>:  "[T]he GNU `gettext' utilities
are a set of tools that provides a framework to help other GNU packages
produce multi-lingual messages."

=cut

package auto::gettext;

use strict;
use warnings;

use base qw(Parrot::Configure::Step);

use Parrot::Configure::Utils ':auto';


sub _init {
    my $self = shift;
    my %data;
    $data{description} = q{Seeing if your configuration includes gettext};
    $data{result}      = q{};
    return \%data;
}

sub runstep {
    my ( $self, $conf ) = @_;

    my ( $verbose, $without ) = $conf->options->get(
        qw|
            verbose
            without-gettext
        |
    );

    if ($without) {
        $conf->data->set( has_gettext => 0 );
        $self->set_result('no');
        return 1;
    }

    my $cc        = $conf->data->get('cc');
    my $libs      = $conf->data->get('libs');
    my $linkflags = $conf->data->get('linkflags');
    my $ccflags   = $conf->data->get('ccflags');

    my $osname = $conf->data->get_p5('OSNAME');

    # On OS X check the presence of the gettext header in the standard
    # Fink location.
    $self->_handle_darwin_for_fink($conf, $osname, 'libintl.h');

    $conf->cc_gen('config/auto/gettext/gettext.in');
    if ( $osname =~ /mswin32/i ) {
        if ( $cc =~ /^gcc/i ) {
            eval { $conf->cc_build( '', '-llibintl' ); };
        }
        else {
            eval { $conf->cc_build( '', 'intl.lib' ); };
        }
    }
    else {
        eval { $conf->cc_build( '', '-lintl' ); };
    }
    my $has_gettext = 0;
    if ( !$@ ) {
        my $test = $conf->cc_run();
        $has_gettext = $self->_evaluate_cc_run($test, $has_gettext, $verbose);
    }
    if ($has_gettext) {
        $conf->data->set( ccflags => "$ccflags -DHAS_GETTEXT" );
        $ccflags = $conf->data->get("ccflags");
        $verbose and print "\n  ccflags: $ccflags\n";
    }
    else {
        # The Parrot::Configure settings might have changed while class ran
        $self->_recheck_settings($conf, $libs, $ccflags, $linkflags, $verbose);
    }
    $conf->data->set( has_gettext => $has_gettext );

    return 1;
}

sub _evaluate_cc_run {
    my $self = shift;
    my ($test, $has_gettext, $verbose) = @_;
    if ( $test eq "Hello, world!\n" ) {
        $has_gettext = 1;
        print " (yes) " if $verbose;
        $self->set_result('yes');
    }
    return $has_gettext;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
