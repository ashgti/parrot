# Copyright (C) 2001-2008, Parrot Foundation.
# $Id$
package Parrot::Configure::Step::List;
use strict;
use warnings;
use base qw( Exporter );
our @EXPORT_OK = qw( get_steps_list );

# EDIT HERE TO ADD NEW TESTS
my @steps = qw(
    init::manifest
    init::defaults
    init::install
    init::hints
    init::headers
    inter::progs
    inter::make
    inter::lex
    inter::yacc
    auto::gcc
    auto::glibc
    auto::backtrace
    auto::msvc
    auto::attributes
    auto::warnings
    init::optimize
    inter::shlibs
    inter::libparrot
    inter::charset
    inter::encoding
    inter::types
    auto::ops
    auto::pmc
    auto::alignptrs
    auto::headers
    auto::sizes
    auto::byteorder
    auto::va_ptr
    auto::format
    auto::isreg
    auto::arch
    auto::jit
    auto::frames
    auto::cpu
    auto::cgoto
    auto::inline
    auto::gc
    auto::memalign
    auto::signal
    auto::socklen_t
    auto::neg_0
    auto::env
    auto::thread
    auto::gmp
    auto::readline
    auto::gdbm
    auto::pcre
    auto::opengl
    auto::crypto
    auto::gettext
    auto::snprintf
    auto::perldoc
    auto::pod2man
    auto::ctags
    auto::revision
    auto::icu
    gen::config_h
    gen::core_pmcs
    gen::crypto
    gen::parrot_include
    gen::opengl
    gen::call_list
    gen::makefiles
    gen::platform
    gen::config_pm
);

sub get_steps_list { return @steps; }

1;

#################### DOCUMENTATION ####################

=head1 NAME

Parrot::Configure::Step::List - Get sequence of configuration steps

=head1 SYNOPSIS

    use Parrot::Configure::Step::List qw( get_steps_list );

    @steps = get_steps_list();

=head1 DESCRIPTION

Parrot::Configure::Step::List exports on demand a single subroutine,
C<get_steps_list()>.  This subroutine returns a list of Parrot's configuration
steps in the order in which they are to be executed.  To change the order in
which the steps are executed, edit C<@steps> inside this module.

=head1 SUBROUTINE

=head2 C<get_steps_list()>

=over 4

=item * Purpose

Provide Parrot configuration steps in their order of execution.

=item * Arguments

None.

=item * Return Value

List holding strings representing the configuration steps.

=item * Comment

=back

=head1 NOTES

The functionality in this package was transferred from F<Configure.pl> by Jim
Keenan.

=head1 SEE ALSO

F<Configure.pl>.

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
