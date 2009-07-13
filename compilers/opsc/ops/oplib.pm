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

=item * C<@.files>

Op files. Mandatory argument of C<BUILD> method.

=item * C<$.max_op_num>

Scalar holding number of highest non-experimental op.  Example:

    'max_op_num' => 1246,

=item * C<%.optable>

Hash holding mapping of opcode names ops to their numbers.
Example:

  'optable' => {
    'pow_p_p_i' => 650,
    'say_s' => 463,
    'lsr_p_p_i' => 207,
    'lt_s_sc_ic' => 289,
    # ...
    'debug_init' => 429,
    'iseq_i_nc_n' => 397,
    'eq_addr_sc_s_ic' => 254
  },

Per F<src/ops/ops.num>, this mapping exists so that we can nail down
the op numbers for the core opcodes in a particular version of the
bytecode and provide backward-compatibility for bytecode.

=item * C<%.skiptable>

Reference to a 'seen-hash' of skipped opcodes.

  'skiptable' => {
    'bor_i_ic_ic' => 1,
    'xor_i_ic_ic' => 1,
    'tanh_n_nc' => 1,
    # ...
  },

As F<src/ops/ops.skip> states, these are "... opcodes that could be listed in
F<[src/ops/]ops.num> but aren't ever to be generated or implemented because
they are useless and/or silly."

=back

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

    self<max_op_num> := 0;
    self<num_file>   := %args<num_file> || 'src/ops/ops.num';
    self<skip_file>  := %args<skip_file> || 'src/ops/ops.skip';

    my %optable;
    self<optable>    := %optable;
    my %skiptable;
    self<skiptable>  := %skiptable;
    self<ops>        := <>;

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

=item C<parse_ops_file>

Parse single ops file. Returns list of parsed ops.

=cut

method parse_ops_file($file) {
    my $parser := self._get_compiler();
    my $buffer := slurp($file);
    my $past   := $parser.compile($buffer, :target('past'));
    #_dumper($past);
    #say($file ~ ' ' ~ +@($past<ops>));
    $past<ops>;
}


=item C<load_op_map_files>

Load ops.num and ops.skip files.

=cut

method load_op_map_files() {
    self._load_num_file();
    self._load_skip_file();
}

method _load_num_file() {
##    open $op, '<', $num_file
##        or die "Can't open $num_file: $!";

    # slurp isn't very efficient. But extending NQP beyond bare minimum is not in scope.
    my $buf := slurp(self<num_file>);
    my @ops := split("\n", $buf);

    my $prev := -1;
##    while (<$op>) {
    for @ops {
##        chomp;
##        s/#.*$//;
##        s/\s*$//;
##        s/^\s*//;
##        next unless $_;
        my @parts   := split( /\s+/, $_ );
        my $name    := @parts[0];
        my $number  := @parts[1];
        #say(@parts[0] ~ ' => ' ~@parts[1]);
        if ((+$number) eq $number) {
            if ($prev + 1 != $number) {
                die("hole in ops.num before #$number");
            }
            if ( exists(self<optable>, $name) ) {
                die("duplicate opcode $name and $number");
            }

            $prev := $number;
            self<optable>{$name} := $number;
            if ( $number > self<max_op_num> ) {
                self<max_op_num> := $number;
            }
        }
    }
}

method _load_skip_file() {
##    open $op, '<', $skip_file
##        or die "Can't open $skip_file: $!";
    # slurp isn't very efficient. But extending NQP beyond bare minimum is not in scope.
    my $buf     := slurp(self<skip_file>);
    my @lines   := split("\n", $buf);

##    while (<$op>) {
    for @lines {
        # XXX Bit ugly. But I don't know how to invoke ~~ in NQP...
        if /^^ \S+ $$/($_) {
            if ( exists( self<optable>, $_) ) {
                die("skipped opcode '$_' is also in num_file");
            }
            self<skiptable>{$_} := 1;
        }
    }
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

method max_op_num() {
    self<max_op_num>;
}

method optable() {
    self<optable>;
}

method skiptable() {
    self<skiptable>;
}


# Local Variables:
#   mode: perl6
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=perl6:
