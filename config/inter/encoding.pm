# Copyright (C) 2001-2003, The Perl Foundation.
# $Id$

=head1 NAME

config/inter/encoding.pm - encoding files

=head1 DESCRIPTION

Asks the user to select which encoding files to include.

=cut

package inter::encoding;

use strict;
use warnings;

use base qw(Parrot::Configure::Step::Base);

use File::Basename qw/basename/;

use Parrot::Configure::Step ':inter';


sub _init {
    my $self = shift;
    my %data;
    $data{description} = q{Determining what encoding files should be compiled in};
    $data{args}        = [ qw( ask encoding ) ];
    $data{result}      = q{};
    return \%data;
}

sub runstep {
    my ( $self, $conf ) = @_;

    my @encoding = (
        sort
            map { basename($_) } glob "./src/encodings/*.c"
    );

    my $encoding_list = $conf->options->get('encoding')
        || join( ' ', grep { defined $_ } @encoding );

    if ( $conf->options->get('ask') ) {
        print <<"END";


The following encodings are available:
  @encoding
END
        {
            $encoding_list = prompt( 'Which encodings would you like?', $encoding_list );
        }
    }

    # names of class files for src/pmc/Makefile
    ( my $TEMP_encoding_o = $encoding_list ) =~ s/\.c/\$(O)/g;

    my $TEMP_encoding_build = <<"E_NOTE";

# the following part of the Makefile was built by 'config/inter/encoding.pm'

E_NOTE

    foreach my $encoding ( split( /\s+/, $encoding_list ) ) {
        $encoding =~ s/\.c$//;
        $TEMP_encoding_build .= <<END
src/encodings/$encoding\$(O): src/encodings/$encoding.h src/encodings/$encoding.c \$(NONGEN_HEADERS)


END
    }

    # build list of libraries for link line in Makefile
    my $slash = $conf->data->get('slash');
    $TEMP_encoding_o =~ s/^| / src${slash}encodings${slash}/g;

    $conf->data->set(
        encoding            => $encoding_list,
        TEMP_encoding_o     => $TEMP_encoding_o,
        TEMP_encoding_build => $TEMP_encoding_build,
    );

    return 1;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
