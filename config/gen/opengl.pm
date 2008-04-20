# Copyright (C) 2008, The Perl Foundation.
# $Id$

=head1 NAME

config/gen/opengl.pm - OpenGL binding generated files

=head1 DESCRIPTION

Generates several files used by the OpenGL binding.  These include:

=over 4

=item F<runtime/parrot/include/opengl_defines.pasm>

=back

=cut

package gen::opengl;

use strict;
use warnings;

use base qw(Parrot::Configure::Step);

use Parrot::Configure::Utils ':gen';


my @GLUT_1_CALLBACKS = (
    [ 'Display',          'void' ],
    [ 'Idle',             'void' ],
    [ 'Entry',            'int state' ],
    [ 'Menu State',       'int status' ],
    [ 'Visibility',       'int state' ],
    [ 'Motion',           'int x, int y' ],
    [ 'Passive Motion',   'int x, int y' ],
    [ 'Reshape',          'int width, int height' ],
    [ 'Keyboard',         'unsigned char key, int x, int y' ],
    [ 'Mouse',            'int button, int state, int x, int y' ],

    # NOTE: Hardcoded because of special arguments
    # [ 'Timer',            'int data' ],
);

my @GLUT_2_CALLBACKS = (
    [ 'Button Box',       'int button, int state' ],
    [ 'Dials',            'int dial, int value' ],
    [ 'Spaceball Button', 'int button, int state' ],
    [ 'Tablet Motion',    'int x, int y' ],
    [ 'Spaceball Motion', 'int x, int y, int z' ],
    [ 'Spaceball Rotate', 'int x, int y, int z' ],
    [ 'Special',          'int key, int x, int y' ],
    [ 'Tablet Button',    'int button, int state, int x, int y' ],
);

my @GLUT_3_CALLBACKS = (
    [ 'Overlay Display',  'void' ],
    [ 'Menu Status',      'int status, int x, int y' ],
);

my @GLUT_4_CALLBACKS = (
    [ 'Window Status',    'int state' ],
    [ 'Keyboard Up',      'unsigned char key, int x, int y' ],
    [ 'Special Up',       'int key, int x, int y' ],

    # NOTE: Hardcoded because of special arguments
    # [ 'Joystick',         'int buttons, int xaxis, int yaxis, int zaxis' ],
);

my @MACOSXGLUT_CALLBACKS = (
    # Also works in freeglut
    [ 'WM Close',         'void' ],
);

my @FREEGLUT_CALLBACKS = (
    [ 'Close',            'void' ],
    [ 'Menu Destroy',     'void' ],
    [ 'Mouse Wheel',      'int wheel, int direction, int x, int y' ],
);

my $MACRO_FILE = 'runtime/parrot/include/opengl_defines.pasm';
my $C_FILE     = 'src/glut_callbacks.c';


sub _init {
    my $self = shift;

    return {
        description  => q{Generating OpenGL bindings},
        result       => q{},
    }
}

sub runstep {
    my ( $self, $conf ) = @_;

    unless ( $conf->data->get('has_opengl') ) {
        $self->set_result('skipped');
        return 1;
    }

    my @header_globs = (
        '/usr/include/GL/*.h',
        '/System/Library/Frameworks/OpenGL.framework/Headers/*.h',
        '/System/Library/Frameworks/GLUT.framework/Headers/*.h',
    );

    my @header_files = sort map {glob} @header_globs;

    $self->gen_opengl_defines($conf, \@header_files);
    $self->gen_glut_callbacks($conf);

    return 1;
}

sub gen_opengl_defines {
    my ( $self, $conf, $header_files ) = @_;

    my $verbose = $conf->options->get('verbose');

    my (%defs, @macros);
    my $max_len = 0;

    foreach my $file (@$header_files) {
        open my $header, '<', $file
        or die "Could not open header '$file': $!";

        while (<$header>) {
            my (@F) = split;
            next unless @F > 2 and $F[0] eq '#define';
            next unless $F[1] =~ /^(GL(?:X|(?:UT?))?)_/;

            $max_len = length $F[1] if $max_len < length $F[1];

            my $api = $1;
            if ($F[2] =~ /^GL/) {
                push @macros, [$api, $F[1], $F[2]];
            }
            elsif (   $F[2] =~ /^0x[0-9a-fA-F]+$/
                   || $F[2] =~ /^\d+(?:\.\d*)?(?:e\d+)?$/) {
                $defs{$api}{$F[1]} = $F[2];
            }
            else {
                print "\nUnable to parse '$F[2]'\n" if $verbose;
            }
        }
    }

    foreach my $macro (@macros) {
        my ($api, $define, $value) = @$macro;
        my ($val_api) = $value =~ /^(GL[A-Z]*)_/;

        $defs{$api}{$define} = $defs{$val_api}{$value};

        die "'$define' is defined as '$value', but no '$value' has been defined"
        unless defined $defs{$val_api}{$value};
    }

    open my $macros, '>', $MACRO_FILE
        or die "Could not open macro file '$MACRO_FILE' for write: $!";

    print $macros <<"HEADER";
# DO NOT EDIT THIS FILE.
#
# Any changes made here will be lost.
#
# This file is generated automatically by config/gen/opengl.pm
# using the following files:
#
HEADER

    print $macros "# $_\n" foreach @$header_files;
    print $macros "\n\n";

    foreach my $api (sort keys %defs) {
        my $api_defs = $defs{$api};

        foreach my $define (sort keys %$api_defs) {
        printf $macros ".macro_const %-${max_len}s %s\n",
                       $define, $api_defs->{$define};
        }
    }

    $conf->append_configure_log($MACRO_FILE);

    return 1;
}

sub gen_glut_callbacks {
    my ( $self, $conf ) = @_;

    my $glut_api   = $conf->data->get('has_glut');
    my $glut_brand = $conf->data->get('glut_brand');

    my   @glut_callbacks = @GLUT_1_CALLBACKS;
    push @glut_callbacks,  @GLUT_2_CALLBACKS     if $glut_api   >= 2;
    push @glut_callbacks,  @GLUT_3_CALLBACKS     if $glut_api   >= 3;
    push @glut_callbacks,  @GLUT_4_CALLBACKS     if $glut_api   >= 4;
    push @glut_callbacks,  @FREEGLUT_CALLBACKS   if $glut_brand eq 'freeglut';
    push @glut_callbacks,  @MACOSXGLUT_CALLBACKS if $glut_brand eq 'freeglut'
                                                 or $glut_brand eq 'MacOSX_GLUT';

    my $glut_header = $glut_brand eq 'MacOSX_GLUT' ? 'GLUT/glut.h'   :
                      $glut_brand eq 'OpenGLUT'    ? 'GL/openglut.h' :
                      $glut_brand eq 'freeglut'    ? 'GL/freeglut.h' :
                                                     'GL/glut.h'     ;

    my @callbacks;
    foreach my $raw (@glut_callbacks) {
        my ($friendly, $params) = @$raw;

        my $args   =  $params;
           $args   =~ s/void//;
           $args   =~ s/(^|, )(\w+ )+/$1/g;
           $args   =  ", $args" if $args;
        my $proto  =  $params;
           $proto  =~ s/ \w+(,|$)/$1/g;
        my $sig    =  $proto;
           $sig    =~ s/void//;
           $sig    =~ s/unsigned //;
           $sig    =~ s/(\w)\w+\W*/$1/g;
           $sig    =  "v$sig";

        my $glutcb =  "glutcb${friendly}Func";
           $glutcb =~ s/ //g;
        my $glut   =  $glutcb;
           $glut   =~ s/glutcb/glut/;
        my $thunk  =  'glut_'    . lc($friendly) . '_func';
           $thunk  =~ s/ /_/g;
        my $enum   =  'GLUT_CB_' . uc($friendly);
           $enum   =~ s/ /_/g;

        push @callbacks, {
            friendly  => $friendly,
            params    => $params,
            proto     => $proto,
            args      => $args,
            sig       => $sig,
            glutcb    => $glutcb,
            glut      => $glut,
            thunk     => $thunk,
            enum      => $enum,
        };
    }

    my $enums     = '';
    my $thunks    = '';
    my $reg_funcs = '';
    my $std_cbs   = '';

   foreach (@callbacks) {
        $enums     .= "    $_->{enum},\n";
        $thunks    .= "           void          $_->{thunk}($_->{proto});\n";
        $reg_funcs .= "PARROT_API void          $_->{glutcb}(PMC *, PMC *);\n";
   }

    my $header = <<HEADER;
/*
# DO NOT EDIT THIS FILE.
#
# Any changes made here will be lost.
#
# This file is generated automatically by config/gen/opengl.pm

Copyright (C) 2008, The Perl Foundation.

=head1 NAME

$C_FILE - GLUT Callback Function Handling

=head1 DESCRIPTION

GLUT callbacks are always synchronous and have void return type.  None
of them accept user data parameters, so normal Parrot callback handling
cannot be used.

=head2 Functions

=over 4

=cut

*/

#include <$glut_header>
#include "parrot/parrot.h"


typedef enum {
$enums
    GLUT_CB_TIMER,

#if GLUT_API_VERSION >= 4
    GLUT_CB_JOYSTICK,
#endif

    GLUT_NUM_CALLBACKS
} GLUT_CALLBACKS;

typedef struct GLUT_CB_data {
    PMC *sub;
    PMC *interp_pmc;
} GLUT_CB_data;

GLUT_CB_data callback_data[GLUT_NUM_CALLBACKS];


           Parrot_Interp verify_safe(PMC *, PMC *);
           void          check_notnull_cb(PMC *, PMC *);

           void          glut_timer_func(int);
PARROT_API void          glutcbTimerFunc(PMC *, PMC *, unsigned int, int);

#if GLUT_API_VERSION >= 4
           void          glut_joystick_func(unsigned int, int, int, int);
PARROT_API void          glutcbJoystickFunc(PMC *, PMC *, int);
#endif

$thunks
$reg_funcs

/* Never store a null interp in callback_data */
void
check_notnull_cb(PMC *interp_pmc, PMC *sub)
{
    if (PMC_IS_NULL(interp_pmc) || (Parrot_Interp) PMC_data(interp_pmc) == NULL)
        PANIC((Parrot_Interp) PMC_data(interp_pmc),
              "cannot register callback with null interpreter");
}


/* PANIC before running callback sub if interp or sub are insane;
   return unwrapped Parrot_Interp if everything OK */
Parrot_Interp
verify_safe(PMC *interp_pmc, PMC *sub)
{
    Parrot_Interp interp = (Parrot_Interp) PMC_data(interp_pmc);

    /* XXXX: Verify that interp still exists */

    /* XXXX: Verify that sub exists in interp */

    return PMC_IS_NULL(sub) ? NULL : interp;
}


/*

# glutTimerFunc and glutJoystickFunc must be hardcoded because they have
# special timer-related arguments that do not follow the template of all
# of the other GLUT callbacks

=item C<void glutcbTimerFunc(interp_pmc, sub, milliseconds, data)>

Register a Sub PMC to handle GLUT Timer callbacks.

=cut

*/

void
glut_timer_func(int data)
{
    PMC *sub        = callback_data[GLUT_CB_TIMER].sub;
    PMC *interp_pmc = callback_data[GLUT_CB_TIMER].interp_pmc;

    Parrot_Interp interp = verify_safe(interp_pmc, sub);

    if (interp)
        Parrot_runops_fromc_args_event(interp, sub, "vi", data);
}

PARROT_API
void
glutcbTimerFunc(PMC *interp_pmc, PMC *sub, unsigned int milliseconds, int data)
{
    check_notnull_cb(interp_pmc, sub);

    callback_data[GLUT_CB_TIMER].sub        = sub;
    callback_data[GLUT_CB_TIMER].interp_pmc = interp_pmc;

    if (sub == PMCNULL)
        glutTimerFunc(0, NULL, 0);
    else
        glutTimerFunc(milliseconds, glut_timer_func, data);
}


#if GLUT_API_VERSION >= 4
/*

=item C<void glutcbJoystickFunc(interp_pmc, sub, pollinterval)>

Register a Sub PMC to handle GLUT Joystick callbacks.

=cut

*/

void
glut_joystick_func(unsigned int buttons, int xaxis, int yaxis, int zaxis)
{
    PMC *sub        = callback_data[GLUT_CB_JOYSTICK].sub;
    PMC *interp_pmc = callback_data[GLUT_CB_JOYSTICK].interp_pmc;

    Parrot_Interp interp = verify_safe(interp_pmc, sub);

    if (interp)
        Parrot_runops_fromc_args_event(interp, sub, "viiii", buttons, xaxis, yaxis, zaxis);
}

PARROT_API
void
glutcbJoystickFunc(PMC *interp_pmc, PMC *sub, int pollinterval)
{
    check_notnull_cb(interp_pmc, sub);

    callback_data[GLUT_CB_JOYSTICK].sub        = sub;
    callback_data[GLUT_CB_JOYSTICK].interp_pmc = interp_pmc;

    if (sub == PMCNULL)
        glutJoystickFunc(NULL, 0);
    else
        glutJoystickFunc(glut_joystick_func, pollinterval);
}
#endif

HEADER


    foreach (@callbacks) {
        $std_cbs   .= <<"IMPLEMENTATION"


/*

=item C<void $_->{glutcb}(interp_pmc, sub)>

Register a Sub PMC to handle GLUT $_->{friendly} callbacks.

=cut

*/

void
$_->{thunk}($_->{params})
{
    PMC *sub        = callback_data[$_->{enum}].sub;
    PMC *interp_pmc = callback_data[$_->{enum}].interp_pmc;

    Parrot_Interp interp = verify_safe(interp_pmc, sub);

    if (interp)
        Parrot_runops_fromc_args_event(interp, sub, "$_->{sig}"$_->{args});
}

PARROT_API
void
$_->{glutcb}(PMC *interp_pmc, PMC *sub)
{
    check_notnull_cb(interp_pmc, sub);

    callback_data[$_->{enum}].sub        = sub;
    callback_data[$_->{enum}].interp_pmc = interp_pmc;

    if (sub == PMCNULL)
        $_->{glut}(NULL);
    else
        $_->{glut}($_->{thunk});
}
IMPLEMENTATION
    }


    my $footer = <<FOOTER;

/*

=back

=cut

*/
FOOTER


    ###
    ### ACTUALLY WRITE FILE
    ###

    open my $c_file, '>', $C_FILE
        or die "Could not open '$C_FILE' for write: $!";

    print $c_file $header;
    print $c_file $std_cbs;
    print $c_file $footer;

    $conf->append_configure_log($C_FILE);


    return 1;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
