# Copyright (C) 2001-2004, The Perl Foundation.
# $Id$

=head1 NAME

config/auto/gmp.pm - Test for GNU MP (GMP) Math library

=head1 DESCRIPTION

Determines whether the platform supports GMP.

From L<http://gmplib.org/>:  "GMP is a free library for arbitrary precision
arithmetic, operating on signed integers, rational numbers, and floating point
numbers. There is no practical limit to the precision except the ones implied
by the available memory in the machine GMP runs on. ..."

"The main target applications for GMP are cryptography applications and
research, Internet security applications, algebra systems, computational
algebra research, etc."

=cut

package auto::gmp;

use strict;
use warnings;

use base qw(Parrot::Configure::Step);

use Parrot::Configure::Utils ':auto';

sub _init {
    my $self = shift;
    my %data;
    $data{description} = q{Determining if your platform supports GMP};
    $data{result}      = q{};
    $data{cc_run_expected} =
"6864797660130609714981900799081393217269435300143305409394463459185543183397656052122559640661454554977296311391480858037121987999716643812574028291115057151 0\n";

    return \%data;
}

sub runstep {
    my ( $self, $conf ) = @_;

    my ( $verbose, $without ) = $conf->options->get(
        qw|
            verbose
            without-gmp
        |
    );

    if ($without) {
        $conf->data->set( has_gmp => 0 );
        $self->set_result('no');
        return 1;
    }

    my $cc        = $conf->data->get('cc');
    my $libs      = $conf->data->get('libs');
    my $linkflags = $conf->data->get('linkflags');
    my $ccflags   = $conf->data->get('ccflags');

    my $osname    = $conf->data->get_p5('OSNAME');

    _handle_mswin32($conf, $osname, $cc);

    # On OS X check the presence of the gmp header in the standard
    # Fink location.
    $self->_handle_darwin_for_fink($conf, $osname, 'gmp.h');
    $self->_handle_darwin_for_macports($conf, $osname, 'gmp.h');

    $conf->cc_gen('config/auto/gmp/gmp.in');
    eval { $conf->cc_build(); };
    my $has_gmp = 0;
    if ( !$@ ) {
        my $test = $conf->cc_run();
        $has_gmp = $self->_evaluate_cc_run( $conf, $test, $has_gmp, $verbose );
    }
    unless ($has_gmp) {
        # The Parrot::Configure settings might have changed while class ran
        $self->_recheck_settings($conf, $libs, $ccflags, $linkflags, $verbose);
    }

    return 1;
}

sub _handle_mswin32 {
    my ($conf, $osname, $cc) = @_;
    if ( $osname =~ /mswin32/i ) {
        if ( $cc =~ /^gcc/i ) {
            $conf->data->add( ' ', libs => '-lgmp' );
        }
        else {
            $conf->data->add( ' ', libs => 'gmp.lib' );
        }
    }
    else {
        $conf->data->add( ' ', libs => '-lgmp' );
    }
    return 1;
}

sub _evaluate_cc_run {
    my ($self, $conf, $test, $has_gmp, $verbose) = @_;
    if ( $test eq $self->{cc_run_expected} ) {
        $has_gmp = 1;
        print " (yes) " if $verbose;
        $self->set_result('yes');

        $conf->data->set(
            gmp     => 'define',
            HAS_GMP => $has_gmp,
        );
    }
    return $has_gmp;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
