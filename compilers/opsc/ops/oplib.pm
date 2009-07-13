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
    if +@files == 0 {
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
        for @(self.parse_ops_file($_)) {
            self<ops>.push($_);
        }
    }
}

method parse_ops_file($file) {
    my $parser := self._get_compiler();
    my $buffer := slurp($file);
    my $past   := $parser.compile($buffer, :target('past'));
    #_dumper($past);
    #say($file ~ ' ' ~ +@($past<ops>));
    $past<ops>;
}

method _get_compiler() {
    PIR q< 
        %r = compreg 'Ops'
    >;
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
