# Copyright (C) 2005-2008, Parrot Foundation.
# $Id$

package init::hints::dec_osf;

use strict;
use warnings;

sub runstep {
    my ( $self, $conf ) = @_;

    # Tru64
    my $ccflags = $conf->data->get('ccflags');
    if ( $ccflags !~ /-pthread\b/ ) {
        $ccflags .= ' -pthread';
    }
    if ( $ccflags !~ /-D_REENTRANT/ ) {
        $ccflags .= ' -D_REENTRANT';
    }
    if ( $ccflags !~ /-D_XOPEN_SOURCE=/ ) {
        # Request all POSIX visible (not automatic for cxx, as it is for cc)
        $ccflags .= ' -D_XOPEN_SOURCE=500';
    }
    $conf->data->set( ccflags => $ccflags );

    my $libs = $conf->data->get('libs');
    if ( $libs !~ /-lpthread\b/ ) {
        $libs .= ' -lpthread';
    }
    $conf->data->set( libs => $libs );

    for my $ldflags (qw(ld_load_flags ld_share_flags)) {
        my $f = $conf->data->get($ldflags);
        if ( $f =~ s/ -s / / ) {
            $conf->data->set( $ldflags => $f );
        }
    }

    my $linkflags = $conf->data->get('linkflags');
    if ( $linkflags !~ /-expect_unresolved\b/ ) {
        $linkflags = "-expect_unresolved '*' -O4 -msym -std $linkflags";
        $conf->data->set( linkflags => $linkflags );
    }

    unless ( $conf->data->get("gccversion") ) {
        $conf->data->set( link => "cxx" );
    }

    # Perl 5 hasn't been compiled with this visible.
    $conf->data->set( has_socklen_t => 1 );
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
