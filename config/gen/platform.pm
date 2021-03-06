# Copyright (C) 2001-2008, Parrot Foundation.
# $Id$

=head1 NAME

config/gen/platform.pm - Platform Files

=head1 DESCRIPTION

Moves the various platform-specific files into place.

=cut

package gen::platform;

use strict;
use warnings;

use base qw(Parrot::Configure::Step);

use Parrot::Configure::Utils ':gen';

sub _init {
    my $self = shift;
    my %data;
    $data{description} = q{Move platform files into place};
    $data{result}      = q{};
    $data{coda} = <<'CODA';
/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
CODA
    return \%data;
}

sub runstep {
    my ( $self, $conf ) = @_;

    my $generated   = $self->_get_generated($conf);

    # headers are merged into platform.h
    $self->_set_headers($conf, $generated);

    # implementation files are merged into platform.c
    $self->_set_implementations($conf, $generated);

    $self->_handle_asm($conf);

    $self->_set_limits($conf);

    return 1;
}

sub _get_generated {
    my ($self, $conf) = @_;
    my $generated = $conf->data->get('TEMP_generated');
    $generated = '' unless defined $generated;
    $conf->debug(" ($generated) \n");
    return $generated;
}

sub _set_headers {
    my $self = shift;
    my ($conf, $generated) = @_;
    my $platform = $conf->data->get('platform');
    my @headers = qw/
        io.h
        math.h
        misc.h
        dl.h
        signal.h
        stat.h
        threads.h
        string.h
        /;

    my $plat_h = q{include/parrot/platform.h};
    $conf->append_configure_log($plat_h);
    open my $PLATFORM_H, ">", $plat_h
        or die "Can't open $plat_h: $!";

    print {$PLATFORM_H} <<"END_HERE";
/* ex: set ro:
** !!!!!!!   DO NOT EDIT THIS FILE   !!!!!!!
**
** This file is generated automatically by config/gen/platform.pm
*/
#ifndef PARROT_PLATFORM_H_GUARD
#define PARROT_PLATFORM_H_GUARD

/*
** platform.h [$platform version]
**
** DO NOT EDIT THIS FILE
**
** Generated by config/gen/platform.pm
*/

END_HERE

    foreach (@headers) {
        my $header_file = "config/gen/platform/generic/$_";
        if ( -e "config/gen/platform/$platform/$_" ) {
            $header_file = "config/gen/platform/$platform/$_";
        }

        if ( -e $header_file ) {
            local $/ = undef;
            $conf->debug("\t$header_file\n");
            open my $IN_H, "<", "$header_file"
                or die "Can't open $header_file: $!";

            # slurp in the header file
            my $in_h = <$IN_H>;

            # remove the (in this case) superfluous coda
            $in_h =~ s{\Q$self->{coda}\E\n*\z}{}xgs;

            print {$PLATFORM_H} <<"END_HERE";
/*
** $header_file:
*/
#line 1 "$header_file"
END_HERE
            print {$PLATFORM_H} $in_h, "\n\n";
            close $IN_H;
        }

        # just fall through if file is missing; means neither this platform nor
        # generic has anything to contribute for this feature.  this might not
        # be desirable if porters don't see the appropriate file in generic/ and
        # shoehorn their function into the wrong file rather than creating the
        # correct one from the above list in their $platform/ dir (e.g. misc.c).
    }

    # finally append generated
    @headers = grep { /\.h$/ } split( m/,/, $generated );
    for my $h (@headers) {
        if ( -e $h ) {
            local $/ = undef;
            $conf->debug("\t$h\n");
            open my $IN_H, "<", $h or die "Can't open $h: $!";
            print {$PLATFORM_H} <<"END_HERE";
/*
** $h
*/
#line 1 "$h"
END_HERE
            print {$PLATFORM_H} <$IN_H>, "\n\n";
            close $IN_H;
        }
        else {
            warn("Header file '$h' listed in TEMP_generated but not found\n");
        }
    }

    # Add the C-coda
    print $PLATFORM_H <<"END_HERE";
#endif /* PARROT_PLATFORM_H_GUARD */

$self->{coda}
END_HERE

    close $PLATFORM_H;
    return 1;
}

sub _set_limits {
    my ($self, $conf) = @_;
    my $platform = $conf->data->get('platform');

    my $limits = "config/gen/platform/generic/platform_limits.h";
    if ( -e "config/gen/platform/$platform/platform_limits.h" ) {
        $limits = "config/gen/platform/$platform/platform_limits.h";
    }
    copy_if_diff( $limits, "include/parrot/platform_limits.h" );

    return;
}

sub _set_implementations {
    my $self = shift;
    my ($conf, $generated) = @_;
    my $platform = $conf->data->get('platform');
    my @impls = qw/
        time.c
        env.c
        dl.c
        stat.c
        math.c
        signal.c
        itimer.c
        memexec.c
        exec.c
        misc.c
        hires_timer.c
        /;

    my $plat_c = q{src/platform.c};
    $conf->append_configure_log($plat_c);
    open my $PLATFORM_C, ">", $plat_c
        or die "Can't open $plat_c: $!";

    print {$PLATFORM_C} <<"END_HERE";
/*
** platform.c [$platform version]
**
** DO NOT EDIT THIS FILE
**
** Generated by config/gen/platform.pm
*/

/* HEADERIZER HFILE: none */
/* HEADERIZER STOP */
END_HERE

    # We need to put things from begin.c before the parrot.h include.
    $self->_handle_begin_c($platform, $PLATFORM_C);

    # Copy the rest.
    print {$PLATFORM_C} <<'END_HERE';
#include "parrot/parrot.h"

END_HERE

    for my $im (@impls) {
        my $impl_file = "config/gen/platform/generic/$im";
        if ( -e "config/gen/platform/$platform/$im" ) {
            $impl_file = "config/gen/platform/$platform/$im";
        }

        if ( -e $impl_file ) {
            local $/ = undef;
            $conf->debug("\t$impl_file\n");
            open my $IN_C, "<", $impl_file or die "Can't open $impl_file: $!";

            # slurp in the C file
            my $in_c = <$IN_C>;

            # remove the (in this case) superfluous coda
            $in_c =~ s{\Q$self->{coda}\E\n*\z}{}xgs;

            print {$PLATFORM_C} <<"END_HERE";
/*
** $impl_file:
*/
#line 1 "$impl_file"
END_HERE
            print {$PLATFORM_C} $in_c, "\n\n";
            close $IN_C;
        }
    }

    # append generated c files
    @impls = grep { /\.c$/ } split( m/,/, $generated );
    for my $im (@impls) {
        if ( -e $im ) {
            local $/ = undef;
            $conf->debug("\t$im\n");
            open my $IN_C, "<", $im or die "Can't open $im: $!";
            print {$PLATFORM_C} <<"END_HERE";
/*
** $im:
*/
#line 1 "$im"
END_HERE
            print {$PLATFORM_C} <$IN_C>, "\n\n";
            close $IN_C;
        }
    }

    # append the C code coda to the generated file
    print {$PLATFORM_C} <<"END_HERE";

$self->{coda}
END_HERE

    close $PLATFORM_C;
    return 1;
}

sub _handle_begin_c {
    my $self = shift;
    my ($platform, $PLATFORM_C) = @_;
    if ( -e "config/gen/platform/$platform/begin.c" ) {
        local $/ = undef;
        open my $IN_C, "<", "config/gen/platform/$platform/begin.c" or die "Can't open begin.c: $!";

        # slurp in the C file
        my $in_c = <$IN_C>;

        # remove the (in this case) superfluous coda
        $in_c =~ s{\Q$self->{coda}\E\n*\z}{}xgs;

        print {$PLATFORM_C} <<"END_HERE";
/*
** begin.c
*/
#line 1 "config/gen/platform/$platform/begin.c"
END_HERE
        print {$PLATFORM_C} $in_c, "\n\n";
        close $IN_C;
    }

    return;
}

sub _handle_asm {
    my ($self, $conf) = @_;
    my $platform = $conf->data->get('platform');
    if ( $conf->data->get('platform_asm') ) {
        my $asm_file = "config/gen/platform/$platform/asm.s";
        if ( -e $asm_file ) {
            copy_if_diff( $asm_file, "src/platform_asm.s" );
        }
    }

    return;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
