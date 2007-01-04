# Copyright (C) 2005, The Perl Foundation.
# $Id$

package init::hints::linux;

use strict;
use warnings;

use Config;

sub runstep {
    my ( $self, $conf ) = @_;

    my $libs      = $conf->data->get('libs');
    my $cflags    = $conf->data->get('ccflags');
    my $cc        = $conf->data->get('cc');
    my $linkflags = $conf->data->get('linkflags');

    # should find g++ in most cases
    my $link = $conf->data->get('link') || 'c++';

    if ( $libs !~ /-lpthread/ ) {
        $libs .= ' -lpthread';
    }
    my $ld_share_flags = $conf->data->get('ld_share_flags');
    my $cc_shared = $conf->data->get( 'cc_shared' );

    if ( $cc =~ /icc/ ) {

        # Intel C++ compiler has the same name as its C compiler
        $link = 'icc';

        # don't allow icc to pretend it's gcc
        $cflags .= ' -no-gcc';

        # suppress sprintf warnings that don't apply
        $cflags .= ' -wd269';
        if ( $ld_share_flags !~ /-fPIC/ ) {
            $ld_share_flags .= ' -fPIC';
        }
        if ($cc_shared !~ /-fPIC/ ) {
            $cc_shared .= ' -fPIC';
        }
    } elsif ( $cc =~ /suncc/ ) {
        $link = 'sunCC';
        if ( $ld_share_flags !~ /-KPIC/ ) {
            $ld_share_flags = '-KPIC';
        }
        if ($cc_shared !~ /-KPIC/ ) {
            $cc_shared = '-KPIC';
        }
    } else {
        if ( $ld_share_flags !~ /-fPIC/ ) {
            $ld_share_flags .= ' -fPIC';
        }
        if ($cc_shared !~ /-fPIC/ ) {
            $cc_shared .= ' -fPIC';
        }
        # --export-dynamic, s. info gcc, ld
        $linkflags .= ' -Wl,-E';
    }

    if ( $cflags !~ /-D_GNU_SOURCE/ ) {

        # Request visibility of all POSIX symbols
        # _XOPEN_SOURCE=600 doesn't work with glibc 2.1.3
        # _XOPEN_SOURCE=500 gives 2 undefined warns (setenv, unsetenv) on 2.1.3
        $cflags .= ' -D_GNU_SOURCE';
    }

    $conf->data->set(
        ccflags        => $cflags,
        libs           => $libs,
        ld_share_flags => $ld_share_flags,
        ld_load_flags  => $ld_share_flags,
        i_lib_pthread  => 1,                 # XXX fake a header entry
        linkflags      => $linkflags,
        link           => $link,
        cc_shared      => $cc_shared,
        rpath          => '-Wl,-rpath=',

        has_dynamic_linking    => 1,
        parrot_is_shared       => 1,
        libparrot_shared       => 'libparrot$(SHARE_EXT).$(SOVERSION)',
        libparrot_shared_alias => 'libparrot$(SHARE_EXT)',
        libparrot_soname       => '-Wl,-soname=libparrot$(SHARE_EXT).$(SOVERSION)',
    );

    if ( ( split( '-', $Config{archname} ) )[0] eq 'ia64' ) {
        $conf->data->set( platform_asm => 1 );
    }
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
