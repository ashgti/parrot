#! perl
# Copyright (C) 2009, Parrot Foundation
# $Id$

=head1 NAME

t/tools/pbc_disassemble.t - test the Parrot Bytecode (PBC) Disassembler

=head1 SYNOPSIS

    % prove t/tools/pbc_disassemble.t

=head1 DESCRIPTION

Tests the C<pbc_disassemble> tool by providing it with a number of source
files, and running through it with various commands.

We never actually check the I<full> output of pbc_disassemble.  We simply check
several smaller components to avoid a test file that is far too unwieldy.


=head1 REQUIREMENTS

This test script requires you to build pbc_disassemble, by typing
"make parrot_utils" (using a suitable make tool for your platform).
If this requirement has not been met, all tests will be skipped.

=cut

use strict;
use warnings;
use lib qw(lib);

use Test::More;
use IO::File ();
use Parrot::Config;
use File::Spec;
use Parrot::Test;

my $path;

BEGIN {
    $path = File::Spec->catfile( ".", "pbc_disassemble" );
    my $exefile = $path . $PConfig{exe};
    unless ( -f $exefile ) {
        plan skip_all => "pbc_disassemble hasn't been built. Run make parrot_utils";
        exit(0);
    }
    plan tests => 4;
}

disassemble_output_like( <<PIR, "pir", qr/PMC_CONST.*set_n_nc.*print_n/ms, 'pbc_disassemble numeric ops');
.sub main :main
    \$N3 = 3.14159
    print \$N3
    print "\\n"
.end
PIR

disassemble_output_like( <<PIR, "pir", qr/PMC_CONST.*set_i_ic.*print_i/ms, 'pbc_disassemble integer ops');
.sub main :main
    \$I0 = 1982
    print \$I0
    print "\\n"
.end
PIR

disassemble_output_like( <<PIR, "pir", qr/PMC_CONST.*new_p_sc.*"ResizablePMCArray".*set_p_kic_ic\s*P.*set_i_p_kic\s*I.*/ms, 'pbc_disassemble pmc ops');
.sub main :main
    \$P0    = new 'ResizablePMCArray'
    \$P0[0] = 42
    \$I0 = \$P0[0]
.end
PIR

disassemble_output_like( <<PIR, "pir", qr/PMC_CONST.*set_s_sc\s*S.*print_s\s*S.*print_sc/ms, 'pbc_disassemble string ops');
.sub main :main
    \$S0 = "Wheels within wheels"
    print \$S0
    print "\\n"
.end
PIR

=head1 HELPER SUBROUTINES

=head2 disassemble_output_like

    disassemble_output_like(<<PASM, "pasm", "some output", "running $file");

Takes 3-4 arguments: a file to run,
the filename-extension of the file (probably "pir" or "pasm"),
an arrayref or single regex string to match within pbc_disassemble's output,
and the optional test diagnostic.

=cut

sub disassemble_output_like {
    pbc_postprocess_output_like($path, @_ );
}

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
