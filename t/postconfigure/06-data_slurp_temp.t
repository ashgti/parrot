#! perl
# Copyright (C) 2007, The Perl Foundation.
# $Id$
# 06-data_slurp_temp.t

use strict;
use warnings;

use Test::More tests => 32;
use Carp;
use lib qw( . lib ../lib ../../lib );
use Parrot::Configure;
use Parrot::Configure::Options qw( process_options );
use_ok('Parrot::Configure::Step::List', qw|
    get_steps_list
| );
use Parrot::IO::Capture::Mini;

$| = 1;
is($|, 1, "output autoflush is set");

my $args = process_options( {
    argv    => [ q{--step=gen::makefiles}, q{--target=Makefile} ],
    mode    => q{reconfigure},
} );
ok(defined $args, "process_options returned successfully");
my %args = %$args;

my $conf = Parrot::Configure->new;
ok(defined $conf, "Parrot::Configure->new() returned okay");
isa_ok($conf, "Parrot::Configure");

my $newconf = Parrot::Configure->new;
ok(defined $newconf, "Parrot::Configure->new() returned okay");
isa_ok($newconf, "Parrot::Configure");
is($conf, $newconf, "Parrot::Configure object is a singleton");

# Since these tests peek into the Parrot::Configure object, they will break if
# the structure of that object changes.  We retain them for now to delineate
# our progress in testing the object.
foreach my $k (qw| steps options data |) {
    ok(defined $conf->$k, "Parrot::Configure object has $k key");
}
is(ref($conf->steps), q{ARRAY},
    "Parrot::Configure object 'steps' key is array reference");
is(scalar @{$conf->steps}, 0,
    "Parrot::Configure object 'steps' key holds empty array reference");
foreach my $k (qw| options data |) {
    isa_ok($conf->$k, "Parrot::Configure::Data");
}

can_ok("Parrot::Configure", qw| data |);
can_ok("Parrot::Configure", qw| options |);
can_ok("Parrot::Configure", qw| steps |);
can_ok("Parrot::Configure", qw| add_step |);
can_ok("Parrot::Configure", qw| add_steps |);
can_ok("Parrot::Configure", qw| run_single_step |);
can_ok("Parrot::Configure", qw| runsteps |);
can_ok("Parrot::Configure", qw| _run_this_step |);

$conf->add_step($args->{step});
my @confsteps = @{$conf->steps};
isnt(scalar @confsteps, 0,
    "Parrot::Configure object 'steps' key holds non-empty array reference");
my $nontaskcount = 0;
foreach my $k (@confsteps) {
    $nontaskcount++ unless $k->isa("Parrot::Configure::Task");
}
is($nontaskcount, 0, "Each step is a Parrot::Configure::Task object");

$conf->options->set(%{$args});
is($conf->options->{c}->{step}, 'gen::makefiles',
    "command-line option '--step=gen::makefiles' has been stored in object");
is($conf->options->{c}->{target}, 'Makefile',
    "command-line option '--target=Makefiles' has been stored in object");
is($conf->options->{c}->{debugging}, 1,
    "command-line option '--debugging' has been stored in object");

my $res  = eval "no strict; use Parrot::Config::Generated; \\%PConfig";
SKIP: {
    my $reason = <<REASON;
If you have already completed configuration,
you can call Parrot::Configure::Data::slurp().
You appear not to have completed configuration;
hence, two tests are skipped.
REASON

    skip $reason, 2 unless defined $res;

    eval { $conf->data()->slurp(); };
    ok( (defined $@) && (! $@), "Parrot::Configure::slurp() succeeded");

    eval { $conf->data()->slurp_temp(); };
    ok( (defined $@) && (! $@), "Parrot::Configure::slurp_temp() succeeded");

    my $tie_out = tie *STDOUT, "Parrot::IO::Capture::Mini"
        or croak "Unable to tie";
    my $ret = $conf->run_single_step( $args->{step} );
    my @more_lines = $tie_out->READLINE;
    ok( (defined $@) && (! $@),
        "Parrot::Configure::run_single_step() succeeded");
}

pass("Completed all tests in $0");

################### DOCUMENTATION ###################

=head1 NAME

06-data_slurp_temp.t - test Parrot::Configure::Data::slurp() once configuration has been completed

=head1 SYNOPSIS

    % prove t/postconfigure/06-data_slurp_temp.t

=head1 DESCRIPTION

The files in this directory test functionality used by F<Configure.pl>.
Certain of the modules C<use>d by F<Configure.pl> have functionality which is
only meaningful I<after> F<Configure.pl> has actually been run and
Parrot::Config::Generated has been created.  So certain tests need to be run
when your Parrot filesystem is in a "pre-F<make>, post-F<Configure.pl>" state.

The tests in this file mimic the functionality of F<tools/dev/reconfigure.pl>
and test C<Parrot::Configure::Data::slurp()>.  What is 'slurped' here is an
already created C<%Parrot::Config::PConfig>.

=head1 AUTHOR

James E Keenan

=head1 SEE ALSO

Parrot::Configure, F<Configure.pl>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
