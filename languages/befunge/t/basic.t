#! perl
# $Id$

use strict;
use warnings;
use Data::Dumper;

#use Test::More tests => 1;
use Test::More skip_all => 'befunge.pasm does not compile';
use Parrot::Test;

use FindBin;

# execute test.bef
my $language_dir = $FindBin::Bin . '/..';
my $parrot       = "../../parrot";

# flatten filenames (don't use directories)
my $out_f = 'test.out';

# STDERR is written into same output file
my $exit_code = Parrot::Test::run_command(
    "$parrot befunge.pbc test.bef",
    CD     => $language_dir,
    STDOUT => $out_f,
    STDERR => $out_f
);

is( Parrot::Test::slurp_file($out_f), << 'OUT', 'output from test.bef' );
If you can see a 4 here ->4 <- then everything is ok!
OUT

# clean up
unlink($out_f);

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
