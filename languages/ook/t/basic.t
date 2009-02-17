#! perl
# $Id$

# Copyright (C) 2005-2007, Parrot Foundation.

=head1 NAME

ook/t/basic.t - testing ook

=head1 SYNOPSIS

    % cd languages && perl ook/t/basic.t

=head1 DESCRIPTION

Test hello.ook.

head1 TODO

test executing test.ook

=cut

use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../../../lib";

use Test::More tests => 1;
use Parrot::Test ();
use Parrot::Config qw(%PConfig);
use File::Spec;

# execute hello.ook
my $parrot    = File::Spec->catfile( File::Spec->updir(), File::Spec->updir(), $PConfig{test_prog} );
my $ook       = $parrot . q{ } . File::Spec->catfile( 'ook.pbc' );
my $hello_ook = File::Spec->catfile( 'hello.ook' );

# Test running hello.ook

my $out_fn = File::Spec->catfile( 'hello.out' );
my $cmd    = "$ook $hello_ook";

# STDERR is written into same output file
my $exit_code = Parrot::Test::run_command(
    $cmd,
    STDOUT => $out_fn,
    STDERR => $out_fn,
);

my $out = Parrot::Test::slurp_file($out_fn );
is( $out, << 'OUT', 'output from hello.ook' );
Hello World!
OUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
