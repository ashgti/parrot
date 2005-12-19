# Copyright: 2005 The Perl Foundation.  All Rights Reserved.
# $Id$

package init::hints::darwin;

use strict;

my($ccflags, $ldflags, $libs)=Parrot::Configure::Data->get(qw(ccflags ldflags libs));

my $OSVers = `uname -r`;
chomp $OSVers;
{
    local $^W;
    if ($OSVers >= 7) {
	$libs =~ s/-ldl//;
    }
}

$ccflags .= " -pipe -fno-common -Wno-long-double ";
$ccflags =~ s/-flat_namespace\s*//;
$ldflags =~ s/-flat_namespace\s*//;
$ldflags .= " -flat_namespace ";

Parrot::Configure::Data->set(
  ccflags => $ccflags,
  ldflags => $ldflags,
  ccwarn => "-Wno-shadow",
  libs => $libs,
  share_ext => '.dylib',
  load_ext => '.bundle',
  link => 'c++',
  ld => 'c++',
  ld_share_flags => '-dynamiclib',
  ld_load_flags => '-bundle -undefined suppress',
  memalign => 'some_memalign',
);

1;
