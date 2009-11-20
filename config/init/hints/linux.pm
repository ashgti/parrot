# Copyright (C) 2005-2007, Parrot Foundation.
# $Id$

package init::hints::linux;

use strict;
use warnings;

sub runstep {
    my ( $self, $conf ) = @_;

    my $libs      = $conf->option_or_data('libs');
    my $ccflags   = $conf->option_or_data('ccflags');
    my $cc        = $conf->option_or_data('cc');
    my $linkflags = $conf->option_or_data('linkflags');
    my $share_ext = $conf->option_or_data('share_ext');
    my $version   = $conf->option_or_data('VERSION');
    my $verbose;

    $verbose = $conf->options->get('verbose');
    print "\n" if $verbose;

    # should find g++ in most cases
    my $link = $conf->data->get('link') || 'c++';

    if ( $libs !~ /-lpthread\b/ ) {
        $libs .= ' -lpthread';
    }
    if ( $libs !~ /-lrt\b/ ) {
        $libs .= ' -lrt';
    }
    my $ld_share_flags = $conf->data->get('ld_share_flags');
    my $cc_shared      = $conf->data->get('cc_shared');

    if ( $cc =~ /icc/ ) {

        # Intel C++ compiler has the same name as its C compiler
        $link = $cc;
        $ld_share_flags = ' -shared -g -pipe -fexceptions -fPIC';
        $cc_shared .= ' -fPIC';

        $ccflags = _handle_icc_ccflags($ccflags, $verbose);

    }
    elsif ( $cc =~ /suncc/ ) {
        $link = 'sunCC';
        if ( $ld_share_flags !~ /-KPIC/ ) {
            $ld_share_flags = '-KPIC';
        }
        if ( $cc_shared !~ /-KPIC/ ) {
            $cc_shared = '-KPIC';
        }
    }
    else {
        if ( $ld_share_flags !~ /-fPIC/ ) {
            $ld_share_flags .= ' -fPIC';
        }
        if ( $cc_shared !~ /-fPIC/ ) {
            $cc_shared .= ' -fPIC';
        }

        # --export-dynamic, s. info gcc, ld
        $linkflags .= ' -Wl,-E';
    }

    if ( $ccflags !~ /-D_GNU_SOURCE/ ) {

        # Request visibility of all POSIX symbols
        # _XOPEN_SOURCE=600 doesn't work with glibc 2.1.3
        # _XOPEN_SOURCE=500 gives 2 undefined warns (setenv, unsetenv) on 2.1.3
        $ccflags .= ' -D_GNU_SOURCE';
    }

    $conf->data->set(
        ccflags        => $ccflags,
        libs           => $libs,
        ld_share_flags => $ld_share_flags,
        ld_load_flags  => $ld_share_flags,
        linkflags      => $linkflags,
        link           => $link,
        cc_shared      => $cc_shared,
        rpath          => '-Wl,-rpath=',

        has_dynamic_linking    => 1,
        parrot_is_shared       => 1,
        libparrot_shared       => "libparrot$share_ext.$version",
        libparrot_shared_alias => "libparrot$share_ext",
        libparrot_soname       => "-Wl,-soname=libparrot$share_ext.$version",
    );

     if ( ( split( m/-/, $conf->data->get('archname_provisional'), 2 ) )[0] eq 'ia64' ) {

        $conf->data->set( platform_asm => 1 );
    }
    return;
}

sub _handle_icc_ccflags {
    my ($ccflags, $verbose) = @_;

    # suppress sprintf warnings that don't apply
    $ccflags .= ' -wd269';

    # suppress remarks about floating point comparisons
    $ccflags .= ' -wd1572';

    # suppress remarks about hiding of parameter declarations
    $ccflags .= ' -wd1599';

    # suppress remarks about "argument is incompatible with corresponding
    # format string conversion"
    $ccflags .= ' -wd181';

    # gcc is currently not looking for unused variables, so should icc
    # for the time being (this will reduce the noise somewhat)
    $ccflags .= ' -wd869';

    # ignore "operands are evaluated in unspecified order" warning
    $ccflags .= ' -wd981';

    # ignore "external declaration in primary source file"
    # (only done temporarily to reduce noise)
    $ccflags .= ' -wd1419';

    # ignore "function 'xxx' was declared but never referenced"
    # (only done temporarily to reduce noise)
    $ccflags .= ' -wd117';

    # ignore "conversion from "" to "" may lose significant bits"
    # warnings (only done temporarily to reduce noise)
    $ccflags .= ' -wd810';

    # ignore "function "" was declared but never referenced"
    # warnings (only done temporarily to reduce noise)
    $ccflags .= ' -wd177';

    # ignore warnings springing from problems with computed goto
    # statements.  If someone can find out how to make icc play nicely
    # in these situations, that would be good.
    $ccflags .= ' -wd1296';

    $ccflags .= ' -Wall -Wcheck -w2';

    $ccflags .= ' -Wabi';
    $ccflags .= ' -Wcomment';
    $ccflags .= ' -Wdeprecated';
    $ccflags .= ' -Wmain';
    $ccflags .= ' -Wmissing-prototypes';

    #$ccflags .= ' -Wp64';
    $ccflags .= ' -Wpointer-arith';
    $ccflags .= ' -Wreturn-type';
    $ccflags .= ' -Wstrict-prototypes';

    #$ccflags .= ' -Wtrigraphs';
    $ccflags .= ' -Wuninitialized';
    $ccflags .= ' -Wunknown-pragmas';
    $ccflags .= ' -Wunused-function';
    $ccflags .= ' -Wunused-variable';

    # enable correct floating point behavior
    # which is *not* the default behavior. ahem.
    $ccflags .= ' -we147';

    $verbose and print " ccflags: $ccflags\n";
    return $ccflags;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
