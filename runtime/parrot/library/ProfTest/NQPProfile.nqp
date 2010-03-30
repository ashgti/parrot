
# Copyright (C) 2010, Parrot Foundation.
# $Id$

class ProfTest::NQPProfile is ProfTest::PIRProfile;

method new($nqp_code, $canonical? = 1) {
    my $nqp_compiler := pir::compreg__ps("NQP-rx");
    my $pir_code     := $nqp_compiler.compile($nqp_code, :target('pir'));
    ProfTest::PIRProfile.new($pir_code, $canonical);
}
