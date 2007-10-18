# Copyright (C) 2001-2003, The Perl Foundation.
# $Id$

=head1 NAME

config/inter/ops.pm - Ops Files

=head1 DESCRIPTION

Asks the user to select which ops files to include.

=cut

package inter::ops;

use strict;
use warnings;

use base qw(Parrot::Configure::Step::Base);

use Parrot::Configure::Step ':inter';


sub _init {
    my $self = shift;
    my %data;
    $data{description} = q{Determining what opcode files should be compiled in};
    $data{args}        = [ qw( ask ops ) ];
    $data{result}      = q{};
    return \%data;
}

sub runstep {
    my ( $self, $conf ) = @_;

    my @ops = (
        sort {
            if ( $a =~ /core\.ops/ ) { return -1 }
            if ( $b =~ /core\.ops/ ) { return 1 }
            return ( $a cmp $b )
            }
            grep { !/vtable\.ops/ } glob "src/ops/*.ops"
    );

    my $ops = join ' ', grep { !/obscure\.ops/ } @ops;

    $ops = $conf->options->get('ops') if defined $conf->options->get('ops');

    # ops selection disabled - until we can build and load
    # opcode subset libs
    if ( 0 && $conf->options->get('ask') ) {
        print <<"END";


The following opcode files are available:
  @ops
END
        {
            $ops = prompt( 'Which opcode files would you like?', $ops );

            if ( $ops !~ m{\bcore\.ops} ) {
                print "core.ops must be the first selection.\n";
                redo;
            }
        }
    }

    $conf->data->set( ops => $ops );

    return 1;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
