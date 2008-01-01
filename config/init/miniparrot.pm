# Copyright (C) 2001-2006, The Perl Foundation.
# $Id$

=head1 NAME

config/init/miniparrot.pm - Miniparrot configuration

=head1 DESCRIPTION

Modifies settings to match miniparrot (ANSI C Parrot)'s needs.  This step
primarily overwrites a lot of settings in the Configure database to disable
JIT and match ANSI characteristics.

Centralizing these settings will (hopefully) allow for an eventual move away
from using Configure at all for miniparrot builds.

=cut

package init::miniparrot;

use strict;
use warnings;

use base qw(Parrot::Configure::Step);

sub _init {
    my $self = shift;
    my %data;
    $data{description} = q{Tweaking settings for miniparrot};
    $data{args}        = [ qw( miniparrot ) ];
    $data{result}      = q{};
    return \%data;
}

sub runstep {
    my ( $self, $conf ) = @_;

    unless ( $conf->options->get('miniparrot') ) {
        $self->set_result('skipped');
        return 1;
    }

    $conf->data->set(
        miniparrot         => 1,
        DEVEL              => $conf->data->get('DEVEL') . '-miniparrot',
        TEMP_cg_h          => '',
        TEMP_cg_c          => '',
        TEMP_cg_o          => '',
        TEMP_cg_r          => '',
        cg_flag            => '',
        archname           => 'miniparrot',
        cpuarch            => 'unknown',
        osname             => 'ANSI',
        jitarchname        => 'nojit',
        jitcpuarch         => 'i386',
        jitcpu             => 'I386',
        jitosname          => 'nojit',
        jitcapable         => 0,
        execcapable        => 0,
        cc_hasjit          => '',
        TEMP_jit_o         => '',
        TEMP_exec_h        => '',
        TEMP_exec_o        => '',
        TEMP_atomic_o      => '',
        asmfun_o           => '',
        has___sighandler_t => undef,
        has_sigatomic_t    => undef,
        has_sigaction      => undef,
        has_setitimer      => undef,

        # we can't guarantee anything about pointer alignment under ANSI C89.
        # so we will have to check every byte.
        ptr_alignment => 1
    );

    #Allow ANSI headers only
    foreach (
        qw(assert complex ctype errno locale math setjmp signal stdarg
        stdio stdlib string time)
        )
    {
        $conf->data->set( "i_$_" => 1 );
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
