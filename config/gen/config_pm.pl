package Configure::Step;

use strict;
use vars qw($description @args);
use Parrot::Configure::Step;
use Data::Dumper;

$description="Writing Parrot::Config module...";

@args=();

sub runstep {
  open(IN, "config/gen/config_pm/Config_pm.in") or die "Can't open Config_pm.in: $!";
  open(OUT, ">lib/Parrot/Config.pm") or die "Can't open lib/Parrot/Config.pm: $!";

  while(<IN>) {
    s/<<HERE>>/Configure::Data->dump()/e;
    print OUT;
  }
 
  close IN  or die "Can't close Config_pm.in: $!";
  close OUT or die "Can't close Config.pm: $!";
}

1;