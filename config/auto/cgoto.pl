#! perl -w
# Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

config/auto/cgoto.pl - Computed C<goto>

=head1 DESCRIPTION

Determines whether the compiler supports computed C<goto>.

=cut

package Configure::Step;

use strict;
use vars qw($description @args);
use Parrot::Configure::Step ':auto';

$description="Determining if your compiler supports computed goto...";

@args=qw(cgoto miniparrot);

sub runstep {
  my $test;
  my ($cgoto, $miniparrot) = @_;

  $cgoto = 0 if (defined $miniparrot);

  if (defined $cgoto) {
    $test = $cgoto;
  }
  else {
    cc_gen('config/auto/cgoto/test_c.in');
    $test=eval { cc_build(); 1; } || 0;
    cc_clean();
  }

  if ($test) {
    Configure::Data->set(
      cg_h          => '$(INC)/oplib/core_ops_cg.h $(INC)/oplib/core_ops_cgp.h',
      cg_c          => <<'EOF',
$(OPS)/core_ops_cg$(O): $(GENERAL_H_FILES) $(OPS)/core_ops_cg.c
$(OPS)/core_ops_cgp$(O): $(GENERAL_H_FILES) $(OPS)/core_ops_cgp.c

$(OPS)/core_ops_cg.c $(INC)/oplib/core_ops_cg.h: $(OPS_FILES) $(BUILD_TOOL)/ops2c.pl lib/Parrot/OpsFile.pm lib/Parrot/Op.pm lib/Parrot/OpTrans/CGoto.pm
	$(PERL) $(BUILD_TOOL)/ops2c.pl CGoto --core
$(OPS)/core_ops_cgp.c $(INC)/oplib/core_ops_cgp.h: $(OPS_FILES) $(BUILD_TOOL)/ops2c.pl lib/Parrot/OpsFile.pm lib/Parrot/Op.pm lib/Parrot/OpTrans/CGP.pm
	$(PERL) $(BUILD_TOOL)/ops2c.pl CGP --core
EOF
      cg_o          => '$(OPS)/core_ops_cg$(O) $(OPS)/core_ops_cgp$(O)',
      cg_r          => '$(RM_F) $(INC)/oplib/core_ops_cg.h $(OPS)/core_ops_cg.c \
                                $(INC)/oplib/core_ops_cgp.h $(OPS)/core_ops_cgp.c',
      cg_flag       => '-DHAVE_COMPUTED_GOTO'
    );
  }
  else {
    Configure::Data->set(
      cg_h    => '',
      cg_c    => '',
      cg_o    => '',
      cg_r    => '',
      cg_flag => ''
    );
  }
}

1;
