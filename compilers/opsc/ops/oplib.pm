# Copyright (C) 2009, Parrot Foundation.
# $Id$

INIT {
    Q:PIR<
        load_bytecode "dumper.pbc"
    >
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

    my $oplib := Ops::OpLib.new.BUILD(
        :files(@files),
        :num_file('../../src/ops/ops.num'),
        :skip_file('../../src/ops/ops.skip'),
    ));

=end SYNOPSIS

=begin ATTRIBUTES

=over 4

=item * C<@.files>

Op files. Mandatory argument of C<BUILD> method.

=item * C<@.ops_past>

List of parsed ops. Currently as C<PAST::Block>, but we probably will use
more specific class inherited from C<PAST::Block> to provide some helper
methods.

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

=item C<BUILD>

Build OpLib.

(It's NQP. In Perl 6 it should be submethod and invoked automatically)

=end METHODS

method BUILD(:@files, :$num_file, :$skip_file) {
    # Process arguments
    if +@files == 0 {
        die("We need some files!")
    }
    self<files>      := @files;
    self<num_file>   := $num_file  || './src/ops/ops.num';
    self<skip_file>  := $skip_file || './src/ops/ops.skip';

    # Initialize self.
    self<max_op_num> := 0;
    self<optable>    := hash();
    self<skiptable>  := hash();
    self<ops_past>   := <>;

    self.load_op_map_files();

    self;
}

=begin METHODS

=item C<parse_ops>

Parse all ops files passed to BUILD method. Create self.ops list for parsed
ops.  This function is used primarily for testing.  When invoked directly, the
HLLCompiler sets the past directly.

=end METHODS

method parse_ops() {
    for self.files() {
        for @(self.parse_ops_file($_)) {
            self<ops_past>.push($_);
        }
    }
}

=begin METHODS

=item C<parse_ops_file>

Parse single ops file. Returns list of parsed ops.

=end METHODS

method parse_ops_file($file) {
    my $parser := self._get_compiler();
    my $buffer := slurp($file);
    my $past   := $parser.compile($buffer, :target('past'));
    #_dumper($past);
    #say($file ~ ' ' ~ +@($past<ops>));
    $past<ops>;
}

=begin METHODS

=item C<set_ops_past>

Assign an already-constructed past tree to self.

=end METHODS

method set_ops_past($past) {
    self<ops_past> := $past;
}

=begin METHODS

=item C<load_op_map_files>

Load ops.num and ops.skip files.

=end METHODS

method load_op_map_files() {
    self._load_num_file();
    self._load_skip_file();
}

=begin METHODS

=item C<build_ops>

Take C<ops_past> and do any runcore-agnostic processing on it.  This means such
things as expanding opcodes with C<in> params into multiple functions,
determining jump flags, etc.

=end METHODS

method build_ops() {
    
    #ops.num is guaranteed not to have any holes, but the ordering also comes
    #from all the .ops files catted together.  It should be sufficient to go
    #through the past, expanding ops to opfuncs as necessary and simply
    #verifying that they end up with the right number.  A special case is
    #experimental.ops, which don't appear in ops.num.

    my $op_num := 0;

    for self<ops_past><ops>.iterator {
        my $cur_op := $_;
        say("found an op: "~ ~$_<name>);
        #my $jump_flags := self.get_jump_flags($cur_op);
        #figure out all the constant data: flags, name, etc
        #build an array of args
        #$args := [ [] ];
        #for eq:
        #     [ [ {'dir'=>'in', 'type'=>'ic', 'label'=>0}, {'dir'=>'in', 'type'=>'i', 'label'=>0} ] , ...



        #check for any skiplisted long opnames before adding them to ops

    }

    #'JUMP' => '0',
    #'NAME' => 'end',
    #'FLAGS' => {
    #    '' => undef,
    #    'flow' => undef,
    #    'check_event' => undef,
    #    'base_core' => undef
    #},
    #'ARGDIRS' => [],
    #'TYPE' => 'inline',
    #'CODE' => 0,
    #'ARGS' => [],
    #'BODY' => '#line 53 "src/ops/core.ops"
    #{{=0}};
    #',
    #'LABELS' => []


}

=begin METHODS

=item C<get_jump_flags>

Process the body of this op to figure out which jump flags need to be set.

=end METHODS

method get_jump_flags($op) {
    my %jumps;
    my @jumps;

    #figure out which control flow flags need to be set for this op
    if (match( " 'goto' \s+ 'ADDRESS' ", ~$op)) {
        %jumps{'PARROT_JUMP_ADDRESS'} := 1;
    }

    if (match( " 'goto' \s+ 'OFFSET' ", ~$op) ||
        $op.name eq 'runinterp' ) {
        %jumps{'PARROT_JUMP_RELATIVE'} := 1;
    }

    if (match( " 'goto' \s+ 'POP' ", ~$op)) {
        %jumps{'PARROT_JUMP_POP'} := 1;
    }

    if (match( " 'expr' \s+ 'NEXT' ", ~$op) ||
        $op.name eq 'runinterp' ) {
        %jumps{'PARROT_JUMP_ENEXT'} := 1;
    }

    if (match( " 'restart' \s+ 'OFFSET' ", ~$op)) {
        %jumps{'PARROT_JUMP_RELATIVE'} := 1;
        %jumps{'PARROT_JUMP_RESTART'}  := 1;
    }
    elsif (match( " 'restart' \s+ 'OFFSET' ", ~$op)) {
        %jumps{'PARROT_JUMP_RESTART'} := 1;
        %jumps{'PARROT_JUMP_ENEXT'}   := 1;
    }
    elsif ($op.name eq 'branch_cs' || $op.name eq 'returncc' ) {
        %jumps{'PARROT_JUMP_RESTART'} := 1;
    }
    elsif (match( " 'restart' \s+ 'ADDRESS' ", ~~$op)) {
        %jumps{'PARROT_JUMP_RESTART'} := 1;
        %jumps{'PARROT_JUMP_ENEXT'}   := 0;
    }

    #XXX: need to handle PARROT_JUMP_GNEXT

    for %jumps {
        if %jumps{$_} {
            @jumps.push($_);
        }
    }

    if +@jumps == 0 {
        $op<jump_flags> := '0';
    }
    else {
        $op<jump_flags> := join('|', @jumps);
    }
    say(~$op<jump_flags>);
}



method _load_num_file() {
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
        if ( exists( self<optable>, $_) ) {
            die("skipped opcode '$_' is also in num_file");
        }
        self<skiptable>{$_} := 1;
    }
}


method _get_compiler() {
    Q:PIR { 
        %r = compreg 'Ops'
    };
}

=begin ACCESSORS

Various methods for accessing internals.

=over 4

=item * C<ops_past>

=item * C<files>

=item * C<max_op_num>

=item * C<optable>

=item * C<skiptable>

=end ACCESSORS

method ops_past() {
    self<ops_past>;
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
