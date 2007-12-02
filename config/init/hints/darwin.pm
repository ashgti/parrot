# Copyright (C) 2005, The Perl Foundation.
# $Id$

package init::hints::darwin;

use strict;
use warnings;

sub runstep {
    my ( $self, $conf ) = @_;

    my ( $ccflags, $ldflags, $libs ) = $conf->data->get(qw(ccflags ldflags libs));

    my $OSVers = `uname -r`;
    chomp $OSVers;
    {
        local $^W;
        $OSVers =~ /(\d+)/;
        if ( $1 >= 7 ) {
            $libs =~ s/-ldl//;
        }
    }

    my $lib_dir = $conf->data->get('build_dir') . "/blib/lib";
    $ldflags .= " -L$lib_dir";
    $ccflags .= " -pipe -fno-common -Wno-long-double ";
    $ccflags =~ s/-flat_namespace\s*//;
    $ldflags =~ s/-flat_namespace\s*//;
    $ldflags .= " -flat_namespace ";

    $conf->data->set(
        darwin              => 1,
        ccflags             => $ccflags,
        ldflags             => $ldflags,
        ccwarn              => "-Wno-shadow",
        libs                => $libs,
        share_ext           => '.dylib',
        load_ext            => '.bundle',
        link                => 'c++',
        ld                  => 'c++',
        ld_share_flags      => '-dynamiclib -undefined suppress',
        ld_load_flags       => '-bundle -undefined suppress',
        memalign            => 'some_memalign',
        has_dynamic_linking => 1,

        # RT#43147 when built against a dynamic libparrot installable_parrot records
        # the path to the blib version of the library
        parrot_is_shared       => 0,
        libparrot_shared       => 'libparrot.$(SOVERSION)$(SHARE_EXT)',
        libparrot_shared_alias => 'libparrot$(SHARE_EXT)',
        rpath                  => "-L",
        libparrot_soname       => "-install_name "
            . $lib_dir
            . $conf->data->get('slash')
            . "libparrot"
            . $conf->data->get('share_ext')
    );
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
