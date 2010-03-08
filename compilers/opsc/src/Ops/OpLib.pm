# Copyright (C) 2009, Parrot Foundation.
# $Id$

INIT {
    pir::load_bytecode("dumper.pbc");
    pir::load_bytecode("nqp-settings.pbc");
};

class Ops::OpLib is Hash;

=begin NAME

C<Ops::OpLib> - library of Parrot Operations.

=end NAME

=begin DESCRIPTION

Responsible for loading F<src/ops/ops.num> and F<src/ops/ops.skip> files,
parse F<.ops> files, sort them, etc.

Heavily inspired by Perl5 Parrot::Ops2pm.

=end DESCRIPTION

=begin SYNOPSIS

    my $oplib := Ops::OpLib.new(
        :num_file('../../src/ops/ops.num'),
        :skip_file('../../src/ops/ops.skip'),
    ));

=end SYNOPSIS

=begin ATTRIBUTES

=over 4

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

=end ATTRIBUTES

=begin METHODS

=over 4

=item C<new>

Build OpLib.

(It's NQP. In Perl 6 it should be submethod and invoked automatically)

=end METHODS

method new(:$num_file, :$skip_file) {
    self<num_file>   := $num_file  // './src/ops/ops.num';
    self<skip_file>  := $skip_file // './src/ops/ops.skip';

    # Initialize self.
    self<max_op_num> := 0;
    self<optable>    := hash();
    self<skiptable>  := hash();
    self<ops_past>   := list();

    self.load_op_map_files();

    self;
}

=begin METHODS

=item C<load_op_map_files>

Load ops.num and ops.skip files.

=end METHODS

method load_op_map_files() {
    self._load_num_file;
    self._load_skip_file;
}

my method _load_num_file() {
    # slurp isn't very efficient. But extending NQP beyond bare minimum is not in scope.
    my $buf := slurp(self<num_file>);
    grammar NUM {
        rule TOP { <op>+ }

        rule op { $<name>=(\w+) $<number>=(\d+) }
        token ws {
            [
            | \s+
            | '#' \N*
            ]*
        }
    }

    #say("Parsing NUM");
    my $ops := NUM.parse($buf);
    #_dumper($ops);

    my $prev := -1;
    for $ops<op> {
        my $name    := ~$_<name>;
        my $number  := +$_<number>;
        #say(@parts[0] ~ ' => ' ~@parts[1]);
        if (+$number) eq $number {
            if ($prev + 1 != $number) {
                die("hole in ops.num before #$number");
            }
            if self<optable>.exists($name) {
                die("duplicate opcode $name and $number");
            }

            $prev := $number;
            self<optable>{$name} := $number;
            if ( $number > self<max_op_num> ) {
                self<max_op_num> := $number;
            }
        }
    }

    #_dumper(self<optable>);
}

method _load_skip_file() {
    my $buf     := slurp(self<skip_file>);
    grammar SKIP {
        rule TOP { <op>+ }

        rule op { $<name>=(\w+) }
        token ws {
            [
            | \s+
            | '#' \N*
            ]*
        }
    }

    my $lines := SKIP.parse($buf);

    for $lines<op> {
        if self<optable>.exists($_<name>) {
            die("skipped opcode '$_' is also in num_file");
        }
        self<skiptable>{$_<name>} := 1;
    }
}


=begin ACCESSORS

Various methods for accessing internals.

=over 4

=item * C<max_op_num>

=item * C<optable>

=item * C<skiptable>

=end ACCESSORS

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
