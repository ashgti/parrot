#!perl

use strict;
use warnings;
use lib qw( lib ../lib ../../lib dotnet dotnet/t );

use DotNetTesting;

use Test::More tests => 2;

## Testing class for this file.t';
#
die unless compile_cs( "t.dll", <<'CSHARP');
namespace Testing
{
    public class Test
    {
        public static int x;

        static Test()
        {
            x = 35;
        }

	    public int get_x()
    	{
	    	return x;
    	}
    }
}
CSHARP

## Attempt to translate.
ok( translate( "t.dll", "t.pbc" ), 'translate' );

## Tests.
is( run_pir(<<'PIR'), <<'OUTPUT', 'get_x' );
.sub main
	.local pmc obj
	load_bytecode "t.pbc"
	obj = new [ "Testing" ; "Test" ]
	$I0 = obj.get_x()
	print $I0
	print "\n"
.end
PIR
35
OUTPUT

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
