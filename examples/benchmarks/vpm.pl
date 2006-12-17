# perl program by Thomas Klausner posted on Vienna-PM list
#
use strict;
use warnings;

my $big;
my $string = "just another perl hacker";

for ( 0 .. 99999 ) {
    $big++;
    my @str = split( //, $string );
    my $f = shift(@str);
    push( @str, $f );
    $string = join( '', @str );
}

print "$big;
$string
";

# ende

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
