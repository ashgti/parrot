package Parrot::Configure::RunSteps;

use strict;
use vars qw(@steps);

# EDIT HERE TO ADD NEW TESTS
@steps=qw(
	init/manifest.pl
	init/data.pl
	init/miniparrot.pl
	init/hints.pl
        init/optimize.pl
	inter/progs.pl
	inter/types.pl
	inter/ops.pl
	inter/exp.pl
	inter/pmc.pl
	auto/ccflags.pl
	auto/alignptrs.pl
	auto/headers.pl
	auto/sizes.pl
	auto/byteorder.pl
	auto/pack.pl
	auto/format.pl
	auto/gcc.pl
	auto/isreg.pl
	auto/jit.pl
	auto/funcptr.pl
	auto/cgoto.pl
	auto/gc.pl
	auto/memalign.pl
	gen/config_h.pl
	gen/feature_h.pl
	gen/config_pm.pl
	gen/makefiles.pl
	gen/myconfig.pl
	gen/platform.pl
	gen/libparrot_def.pl
        gen/core_pmcs.pl
	gen/parrot_include.pl
);

sub runsteps {
	shift;
	my %args=@_;

	local $SIG{__WARN__} = sub {
		warn $_[0] unless $_[0] =~ /^Subroutine runstep redefined at config/
	};

	for(@steps) {
                #print "$_\n";
		die "No config/$_" unless -e "config/$_";
		require "config/$_";
		print "\n$Configure::Step::description";

        $Configure::Step::result='done';

		{
			local $_;
			Configure::Step::runstep(@args{@Configure::Step::args});
		}

		print "$Configure::Step::result." unless m{^inter/} && $args{ask};
	}
}

1;
