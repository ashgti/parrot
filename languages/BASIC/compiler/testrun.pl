#! perl
$parpath = '../../../';

unlink "_test.pbc";

# XXX - This file should be generated at config time, so as to get
# the appropriate path and executable name.
$a = system("$parpath/parrot TARG_test.pir");

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
