#! perl
# Copyright (C) 2001-2007, The Perl Foundation.
# $Id$

use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../..";
use lib "$FindBin::Bin/../../lib";
use Getopt::Long;
use Parrot::Pmc2c::Pmc2cMain;

my ( %action, %options, @pmc_include_paths );

GetOptions(
    #pmc include paths
    "include=s" => \@pmc_include_paths,
    #program actions
    "vtable"    => \$action{default},
    "dump"      => \$action{dump},
    "c|gen-c"   => \$action{gen_c},
    "tree"      => \$action{tree},
    #command line options
    "no-lines"  => \$options{nolines},
    "debug+"    => \$options{debug},
    "verbose+"  => \$options{verbose},
    "library=s" => \$options{library},
    "testing"   => \$options{testing},
) or exit 1;

if ( 0 == grep { $action{$_} } keys %action ) {
    die "No action specified!\n";
}

my @args = @ARGV;

my $self = Parrot::Pmc2c::Pmc2cMain->new(
    {
        include => \@pmc_include_paths,
        opt     => \%options,
        args    => \@args,
    }
);

if ( $action{default} ) {
    $self->dump_vtable("$FindBin::Bin/../../vtable.tbl");
    exit;
}

if ( $action{dump} ) {
    $self->dump_pmc();
    exit;
}

if ( $action{tree} ) {
    $self->print_tree();
    exit;
}

if ( $action{gen_c} ) {
    $self->gen_c();
    exit;
}

__END__

=head1 NAME

tools/build/pmc2c.pl - PMC definition to C compiler

=head1 SYNOPSIS

=head2 Options used in Parrot F<Makefile>

Create F<src/pmc/foo.dump>:

    % perl tools/build/pmc2c.pl --dump src/pmc/foo.pmc ...

Create F<vtable.dump>:

    % perl tools/build/pmc2c.pl --vtable

Create F<src/pmc/foo.c> and C<pmc_foo.h> from F<src/pmc/foo.dump>:

    % perl tools/build/pmc2c.pl -c src/pmc/foo.pmc ...

=head2 Other Options

Print a class tree for the specified PMCs:

    % perl tools/build/pmc2c.pl --tree src/pmc/*.pmc

Create fooX.c and pmc_fooX.h from fooX.dump files, also create libfoo.c
containing the initialization function for all fooX PMCs.

    % perl tools/build/pmc2c.pl --library libfoo -c \
           src/pmc/foo1.pmc src/pmc/foo2.pmc ...

=head1 DESCRIPTION

The job of the PMC compiler is to take .pmc files and create C files
that can be compiled for use with the Parrot interpreter.

=head1 COMMAND-LINE OPTIONS

=over 4

=item C<--debug>

Increase debug level

=item C<--verbose>

Increase verbose level

=item C<--no-lines>

Omit source line info

=item C<--include=/path/to/pmc>

Specify include path where to find PMCs.

=item C<--library=libname>

Specify the library name. This will create F<E<lt>libnameE<gt>.c> and
F<pmc_E<lt>libnameE<gt>.h>. The initialization function will be named
after libname and will initialize all PMCs in the library.

=back

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
