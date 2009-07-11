# Copyright (C) 2009, Parrot Foundation.
# $Id$

class Ops::OpLib;

=head1 NAME

C<Ops::OpLib> - library of Parrot Operations.

=head1 DESCRIPTION

Responsible for loading F<src/ops/ops.num> and F<src/ops/ops.skip> files,
parse F<.ops> files, sort them, etc.

Heavily inspired by Perl5 Parrot::Ops2pm.

=head1 SYNOPSIS

  my $oplib := Ops::OpLib.new.BUILD;

=head1 ATTRIBUTES

=over 4

=item C<@.files>

Op files. Mandatory argument of C<BUILD> method.

=cut

=head1 METHODS

=over 4

=item BUILD

Build OpLib.

(It's NQP. In Perl 6 it should be submethod invoked automatically)

=cut

method BUILD(*%args) {
    my $self  := self; # NQP parser isn't full Perl6. So alias self to $self.
    my @files := %args<files>;
    if !+@files {
        die("We need some files!")
    }
    $self<files> := @files;
}


method files() {
    my $self := self;
    $self<files>;
}

# Local Variables:
#   mode: perl6
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=perl6:
