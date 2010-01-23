# Copyright (C) 2001-2003, Parrot Foundation.
# $Id$

=head1 NAME

config/init/hints.pm - Platform Hints

=head1 DESCRIPTION

Loads the platform and local hints files, modifying the defaults set up in
F<config/init/default.pm>.

=cut

package init::hints;

use strict;
use warnings;

use File::Spec::Functions qw/catfile/;
use base qw(Parrot::Configure::Step);

sub _init {
    my $self = shift;
    my %data;
    $data{description} = q{Load platform and local hints files};
    $data{result}      = q{};
    return \%data;
}

sub runstep {
    my ( $self, $conf ) = @_;

    my $verbose = $conf->options->get('verbose');
    print "\n[ " if $verbose;

    my $hints_used = 0;

    my $osname = lc( $conf->data->get('OSNAME_provisional') );
    $osname = 'linux' if ($osname eq 'gnukfreebsd');
    my $hints_file = catfile('config', 'init', 'hints', "$osname.pm");
    if ( -f $hints_file ) {
        my $hints_pkg = "init::hints::" . $osname;

        print "$hints_pkg " if $verbose;

        eval "use $hints_pkg";
        die $@ if $@;

        # Call the runstep method if it exists.
        # Otherwise the step must have done its work when it was loaded.
        $hints_pkg->runstep( $conf, @_ ) if $hints_pkg->can('runstep');
        $hints_used++;

        $hints_pkg = "init::hints::local";
        print "$hints_pkg " if $verbose;
        eval "use $hints_pkg";

        unless ($@) {
            $hints_pkg->runstep( $conf, @_ ) if $hints_pkg->can('runstep');
            $hints_used++;
        }
    }
    else {
        print "No $hints_file found.  " if $verbose;
    }

    if ( $hints_used == 0 and $verbose ) {
        print "(no hints) ";
    }

    print "]" if $verbose;

    return 1;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
