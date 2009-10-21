# Copyright (C) 2006-2007, Parrot Foundation.
# $Id$

package init::hints::netbsd;

use strict;
use warnings;

sub runstep {
    my ( $self, $conf ) = @_;

    my $share_ext = $conf->option_or_data('share_ext');
    my $version   = $conf->option_or_data('VERSION');

    my $ccflags = $conf->data->get('ccflags');
    if ( $ccflags !~ /-pthread\b/ ) {
        $ccflags .= ' -pthread';
    }
    $conf->data->set( ccflags => $ccflags );

    my $libs = $conf->data->get('libs');
    if ( $libs !~ /-lpthread\b/ ) {
        $libs .= ' -lpthread';
    }
    $conf->data->set(
    libs => $libs,
    rpath => '-Wl,-R',

    has_dynamic_linking    => 1,
    parrot_is_shared       => 1,
    libparrot_shared       => "libparrot$share_ext.$version",
    libparrot_shared_alias => "libparrot$share_ext",
    libparrot_soname       => "-Wl,-soname=libparrot$share_ext.$version",
    );
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
