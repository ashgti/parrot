# Copyright: 2001-2005 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

config/gen/icu.pm - ICU

=head1 DESCRIPTION

Configures ICU and add appropriate targets to the Makefile.

=cut

package Configure::Step;

use strict;
use vars qw($description $result @args);

use base qw(Parrot::Configure::Step::Base);

use Cwd qw(cwd);
use Parrot::Configure::Step qw(capture_output);

$description = "Determining whether ICU is installed";

@args = qw(verbose icushared icuheaders icu-config without-icu);

sub runstep {
    my $self = shift;
  my ($verbose, $icushared, $icuheaders, $icuconfig, $without) = @_;

  my @icu_headers = qw(ucnv.h utypes.h uchar.h);
  my $autodetect =    !defined($icushared)
                   && !defined($icuheaders);

  $result = undef;
  unless ($without) {
    if (!$autodetect) {
      print "specified a icu config parameter,\nICU autodetection disabled.\n" if $verbose;
    } elsif (!defined $icuconfig || !$icuconfig) {
      my (undef, undef, $ret) = capture_output("icu-config", "--exists");

      if (($ret == -1) || (($ret >> 8) != 0)) {
        undef $icuconfig;
        $autodetect = 0;
        $without = 1;
      } else {
        $icuconfig = "icu-config";
        print "icu-config found... good!\n" if $verbose;
      }
    }
  
    if (!$without && $autodetect && $icuconfig && $icuconfig ne "none") {
      my $slash = Parrot::Configure::Data->get('slash');
  
      # icu-config script to use
      $icuconfig = "icu-config" if $icuconfig eq "1";
  
      # ldflags
      $icushared = capture_output("$icuconfig --ldflags");
      if (defined $icushared) {
        chomp $icushared;
        $icushared =~ s/-licui18n\w*//;     # "-licui18n32" too
        $without = 1 if length $icushared == 0;
      }

      # location of header files
      $icuheaders = capture_output("$icuconfig --prefix");
      if (defined $icuheaders) {
        chomp $icuheaders;
        $without = 1 unless -d $icuheaders;
        $icuheaders .= "${slash}include";
        $without = 1 unless -d $icuheaders;
      }

      if ($without) {
        $result = "failed";
      }
    }
  }
    
  if ($verbose) {
    print "icuconfig: $icuconfig\n" if defined $icuconfig;
    print "icushared='$icushared'\n" if defined $icushared;
    print "headers='$icuheaders'\n" if defined $icuheaders;
  }

  if ($without) {
    Parrot::Configure::Data->set(
      has_icu     => 0,
      icu_shared  => '',  # used for generating src/dynclasses/Makefile
    );
    $result = "no" unless defined $Configure::Step::result;
    return;
  }

  my $ok = 1;
  
  unless (defined $icushared) {
    warn "error: icushared not defined\n";
    $ok = 0;
  }

  unless (defined $icuheaders and -d $icuheaders) {
    warn "error: icuheaders not defined or invalid\n";
    $ok = 0;
  } else {
    $icuheaders =~ s![\\/]$!!;
    foreach my $header ( @icu_headers ) {
      $header = "$icuheaders/unicode/$header";
      unless (-e $header) {
        $ok = 0;
        warn "error: ICU header '$header' not found\n";
      }
    }
  }


  die <<"HELP" unless $ok; # this text is also in Configure.PL!
Something is wrong with your ICU installation!
   
   If you do not have a full ICU installation:

   --without-icu        Build parrot without ICU support
   --icu-config=(file)  Location of icu-config
   --icuheaders=(path)  Location of ICU headers without /unicode
   --icushared=(flags)  Full linker command to create shared libraries
HELP
#'
  
  Parrot::Configure::Data->set(
    has_icu     => 1,
    icu_shared  => $icushared,
  );

  # Add -I $Icuheaders if necessary
  my $header = "unicode/ucnv.h";
  Parrot::Configure::Data->set(testheaders =>"#include <$header>\n");
  Parrot::Configure::Data->set(testheader => "$header");
  cc_gen('config/auto/headers/test_c.in');

  Parrot::Configure::Data->set(testheaders => undef);  # Clean up.
  Parrot::Configure::Data->set(testheader => undef);
  eval { cc_build(); };
  if (!$@ && cc_run() =~ /^$header OK/) {
    # Ok, we don't need anything more.
    print "Your compiler found the icu headers... good!\n" if $verbose;
  }
  else {
    print "Adding -I $icuheaders to ccflags for icu headers.\n" if $verbose;
    Parrot::Configure::Data->add(' ', ccflags => "-I $icuheaders");
  }
  cc_clean();

  $result = "yes";

}

1;
