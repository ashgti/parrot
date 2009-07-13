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

(It's NQP. In Perl 6 it should be submethod and invoked automatically)

=cut

method BUILD(*%args) {
    my @files := %args<files>;
    if !+@files {
        die("We need some files!")
    }
    self<files> := @files;

    self<ops>   := <>;

    self;
}

=item C<parse_ops>

Parse all ops files passed to BUILD method. Create self.ops list for parsed ops.

=cut

method parse_ops() {
    for self.files() {
        self.parse_ops_file($_);
        self<ops>.push(self.parse_ops_file($_));
    }
}

method parse_ops_file($file) {
    <>;
}

method ops() {
    self<ops>;
}

method files() {
    self<files>;
}


# Local Variables:
#   mode: perl6
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=perl6:
