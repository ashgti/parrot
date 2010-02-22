# Copyright (C) 2007-2008, Parrot Foundation.
# $Id$

=head1 NAME

config/auto/warnings.pm - Warning flags detection

=head1 DESCRIPTION

Automagically detect what warning flags, like -Wall, -Wextra,
-Wchar-subscripts, etc., that the compiler can support.

=over 4

=cut

package auto::warnings;

use strict;
use warnings;


use base qw(Parrot::Configure::Step);

use Parrot::Configure::Utils ();
use Parrot::BuildUtil;

# Declare potential warnings for various compilers.
#
# Please keep these sorted by flag name, such that "-Wno-foo" is
# sorted as "-Wfoo", so we can turn off/on as needed.
#
# Note that these warnings may be turned off for individual files
# in the Makefile.

sub _init {
    my $self = shift;

    my $data = {
        description => 'Detect supported compiler warnings',
        result      => '',
    };

    my $gcc_or_gpp = [ qw(
        -falign-functions=16
        -fvisibility=hidden
        -funit-at-a-time
        -maccumulate-outgoing-args
        -W
        -Wall
        -Waggregate-return
        -Wcast-align
        -Wcast-qual
        -Wchar-subscripts
        -Wcomment
        -Wdisabled-optimization
        -Wdiv-by-zero
        -Wendif-labels
        -Wextra
        -Wformat
        -Wformat-extra-args
        -Wformat-nonliteral
        -Wformat-security
        -Wformat-y2k
        -Wimplicit
        -Wimport
        -Winit-self
        -Winline
        -Winvalid-pch
        -Wlogical-op
        -Wmissing-braces
        -Wmissing-field-initializers
        -Wno-missing-format-attribute
        -Wmissing-include-dirs
        -Wpacked
        -Wparentheses
        -Wpointer-arith
        -Wreturn-type
        -Wsequence-point
        -Wno-shadow
        -Wsign-compare
        -Wstrict-aliasing
        -Wstrict-aliasing=2
        -Wswitch
        -Wswitch-default
        -Wtrigraphs
        -Wundef
        -Wunknown-pragmas
        -Wunused
        -Wvariadic-macros
        -Wwrite-strings
    ) ];

    $data->{'gcc'}{'potential'} = $gcc_or_gpp;
    $data->{'g++'}{'potential'} = $gcc_or_gpp;

    push @{$data->{'gcc'}{'potential'}}, [qw(
        -Wbad-function-cast
        -Wc++-compat
        -Wdeclaration-after-statement
        -Werror=declaration-after-statement
        -Wimplicit-function-declaration
        -Wimplicit-int
        -Wmain
        -Wmissing-declarations
        -Wmissing-prototypes
        -Wnested-externs
        -Wnonnull
        -Wold-style-definition
        -Wstrict-prototypes
    ) ],

    my $gcc_or_gpp_cage = [ qw(
        -std=c89
        -Werror-implicit-function-declaration
        -Wformat=2
        -Wlarger-than-4096
        -Wlong-long
        -Wmissing-format-attribute
        -Wdeprecated-declarations
        -Wno-format-extra-args
        -Wno-import
        -Wno-multichar
        -Wno-pointer-sign
        -Wold-style-definition
        -Wunreachable-code
        -Wunused-function
        -Wunused-label
        -Wunused-value
        -Wunused-variable
    ) ];

    $data->{'gcc'}{'cage'} = $gcc_or_gpp;
    $data->{'g++'}{'cage'} = $gcc_or_gpp;

    return $data;
}

sub runstep {
    my ( $self, $conf ) = @_;

    my $verbose = $conf->options->get('verbose');
    print "\n" if $verbose;

    my $compiler = '';
    if ( defined $conf->data->get('gccversion') ) {
        $compiler = $conf->data->get('g++') ? 'g++' : 'gcc';
    }

    if ($compiler eq '') {
        print "We do not (yet) probe for warnings for your compiler\n"
            if $verbose;
        $self->set_result("skipped");
        return;
    }

    my @warnings = @{$self->{$compiler}{potential}};

    # add on some extra warnings if requested
    push @warnings, @{$self->{$compiler}{cage}}
        if $conf->options->get('cage');

    # now try out our warnings
    for my $maybe_warning (@warnings) {
        $self->try_warning( $conf, $maybe_warning, $verbose );
    }

    $self->set_result("set for $compiler");

    return 1;
}

=item C<try_warning>

Try a given warning to see if it is supported by the compiler.  The compiler
is determined by the C<cc> value of the C<Parrot::Configure> object passed
in as the first argument to the method (not counting C<$self>.  The warning
to be checked is passed in as the second argument to the method.

Returns true if the warning flag is recognized by the compiler and undef
otherwise.

=cut

sub try_warning {
    my ( $self, $conf, $warning, $verbose ) = @_;

    my $output_file = 'test.cco';

    $verbose and print "trying attribute '$warning'\n";

    my $cc = $conf->option_or_data('cc');
    $conf->cc_gen('config/auto/warnings/test_c.in');

    my $ccflags  = $conf->data->get('ccflags');
    my $warnings = $conf->data->get('ccwarn'). ' ' . $warning;
    my $tryflags = "$ccflags $warnings";

    my $command_line = Parrot::Configure::Utils::_build_compile_command( $cc, $tryflags );
    $verbose and print "  ", $command_line, "\n";

    # Don't use cc_build, because failure is expected.
    my $exit_code = Parrot::Configure::Utils::_run_command(
        $command_line, $output_file, $output_file
    );
    _set_warning($conf, $warning, $exit_code, $verbose);

    $conf->cc_clean();

    if ($exit_code) {
        unlink $output_file or die "Unable to unlink $output_file: $!";
        return;
    }

    my $output = Parrot::BuildUtil::slurp_file($output_file);
    unlink $output_file or die "Unable to unlink $output_file: $!";
    return _set_ccwarn($conf, $output, $warnings, $verbose);
}

sub _set_warning {
    my ($conf, $warning, $exit_code, $verbose) = @_;
    $verbose and print "  exit code: $exit_code\n";
    $conf->data->set( $warning => !$exit_code || 0 );
}

sub _set_ccwarn {
    my ($conf, $output, $warnings, $verbose) = @_;
    $verbose and print "  output: $output\n";

    if ( $output !~ /error|warning|not supported/i ) {
        $conf->data->set( ccwarn => $warnings );
        $verbose and print "  ccwarn: ", $conf->data->get("ccwarn"), "\n";
        return 1;
    }
    else {
        return 0;
    }
}

=back

=head1 AUTHOR

Paul Cochrane <paultcochrane at gmail dot com>

=cut

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
