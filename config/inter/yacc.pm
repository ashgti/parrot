# Copyright (C) 2001-2007, Parrot Foundation.
# $Id$

=head1 NAME

config/auto/yacc.pm - parser generator

=head1 DESCRIPTION

Determines whether C<yacc> is installed and if it's actually C<bison> of at
least the user-specified minimum version, defaulting to 2.1.

=cut

package inter::yacc;

use strict;
use warnings;


use base qw(Parrot::Configure::Step);

use Parrot::Configure::Utils qw( :inter capture_output check_progs );

sub _init {
    my $self = shift;
    my %data;
    $data{description} = q{Is yacc installed};
    $data{result}      = q{};
    return \%data;
}

my @yacc_defaults =
    defined( $ENV{TEST_YACC} )
    ? $ENV{TEST_YACC}
    : ( 'bison -v -y', 'yacc', 'byacc' );

my $default_required = '2.1';

sub runstep {
    my ( $self, $conf ) = @_;
    my $util    = 'yacc';
    my $prompt  = "Do you have a parser generator, like bison or yacc?";

    my $verbose = $conf->options->get('verbose');

    # undef means we don't have bison... default to not having bison
    $conf->data->set( bison_version => undef );

    unless ( $conf->options->get('maintainer') ) {
        $conf->data->set( $util => 'echo' );
        $self->set_result('skipped');
        return 1;
    }

    # precedence of sources for the program:
    # default -> probe -> environment -> option -> ask
    my $prog = $conf->options->get($util);
    unless ($prog) {
        $prog = $ENV{ uc($util) };
    }

    # never override the user.  If a non-existent program is specified then
    # the user is responsible for the consequences.
    if ( defined $prog ) {
        $conf->data->set( $util => $prog );
        $self->set_result('user defined');
        return 1;
    }
    else {

        $prog = check_progs( [@yacc_defaults], $verbose );
        if ( !$prog ) {
            $self->set_result('no yacc program was found');
            return;
        }
        else {

            if ( $conf->options->get('ask') ) {
                $prog = prompt( $prompt, $prog ? $prog : $conf->data->get($util) );
            }
            my ( $stdout, $stderr, $ret ) = capture_output( $prog, '--version' );

            # don't override the user even if the program they provided
            # appears to be broken
            if ( $ret == -1 and !$conf->options->get('ask') ) {

                # fall back to default
                $self->set_result('yacc program does not exist or does not understand --version');
                return;
            }
            elsif ( $stdout =~ /Bison .*? (\d+) \. (\d+) (\w)?/x ) {

                # if '--version' returns a string assume that this is bison.
                # if this is bison pretending to be yacc
                # '--version' doesn't work
                # someday we might need to check $3 also.
                my ( $prog_major, $prog_minor, $prog_patch ) = ( $1, $2, $3 );
                my $prog_version = "$1.$2";
                $prog_version .= $3 if $3;

                # is there a version requirement?
                my $req = $conf->options->get('bison_required');
                unless ( defined $req ) {
                    $req = $default_required;
                }
                if ($req) {
                    my ( $rmajor, $rminor ) = ( $req =~ / ^ (\d+) \. (\d+) (\w)? $ /x );
                    if ( !defined $rmajor ) {
                        $self->set_result("could not understand bison version requirement");
                        return;
                    }
                    elsif (
                        $prog_major < $rmajor
                        or (    $prog_major == $rmajor
                            and $prog_minor < $rminor )
                        )
                    {
                        $self->set_result( "found bison version $prog_version"
                                . " but at least $rmajor.$rminor is required" );
                        return;
                    }
                    else {
                        1;    # lack an explicit 'else' here
                    }
                }
                $conf->data->set( bison_version => $prog_version );
                $self->set_result("bison $prog_version");
                $conf->data->set( $util => $prog );
                return 1;
            }
            else {
                $self->set_result('yacc program does not exist or does not understand --version');
                return;
            }
        }
    }
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
