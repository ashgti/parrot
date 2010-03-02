# Copyright (C) 2009, Parrot Foundation.
# $Id$

class Ops::Compiler::Actions is HLL::Actions;

our $CODE;

INIT {
    $CODE := 0;
}

method TOP($/) {
    make $<body>.ast;
}

method body($/) {
    my $past := PAST::Stmts.new(
        :node($/)
    );

    $past<preamble> := PAST::Stmts.new(
        :node($/)
    );
    $past<ops> := PAST::Stmts.new(
        :node($/)
    );

    for $<preamble> {
        $past<preamble>.push($_.ast);
    }

    for $<op> {
        my $ops := $_.ast;
        for @($ops) {
            $_<code> := $CODE++;
            $past<ops>.push($_);
        }
    }

    make $past;
}

method preamble($/) {
    make PAST::Op.new(
        :node($/),
        :pasttype('preamble'),
        ~$<preamble_guts>
    );
}

method op($/) {

    # Handling flags.
    my %flags := pir::new__Ps('Hash');
    for $<op_flag> {
        %flags{~$_<identifier>} := 1;
    }

    my @args := list();
    if ($<op_params>) {
        @args := @($<op_params>[0].ast);
    }

    my @norm_args := normalize_args(@args);
    # We have to clone @norm_args. Otherwise it will be destroyed...
    my @variants  := expand_args(pir::clone__PP(@norm_args));

    my $op := PAST::Block.new(
        :name(~$<op_name>),

        $<op_body>.ast
    );

    $op<flags> := %flags;
    $op<args>  := @args;
    $op<type>  := ~$<op_type>;
    $op<normalized_args> := @norm_args;

    my $past := PAST::Stmts.new(
        :node($/)
    );

    if @variants {
        for @variants {
            my $new_op := pir::clone__PP($op);
            $new_op<args_types> := $_;
            $past.push($new_op);
        }
    }
    else {
        $past.push($op);
    }

    make $past;
}

# Normalize args
# For each arg produce LoL of all available variants
# E.g. "in" will produce "i" and "ic" variants
#
# type one of <i p s n>
# direction one of <i o io>
# is_label one of <0 1>

sub normalize_args(@args) {
    my @result;
    for @args -> $arg {
        my $res := PAST::Var.new(
            :isdecl(1)
        );

        if $arg<type> eq 'LABEL' {
            $res<type>     := 'i';
            $res<is_label> := 1;
        }
        else {
            $res<is_label> := 0;
        }

        if $arg<type> eq 'INTKEY' {
            $res<type> := 'ki';
        }
        else {
            $res<type> := lc(substr($arg<type>, 0, 1));
        }

        my $use := $arg<direction>;

        if $use eq 'in' {
            $res<variant>   := $res<type> ~ "c";
            $res<direction> := 'i';
        }
        elsif $use eq 'invar' {
            $res<direction> := 'i';
        }
        elsif $use eq 'inconst' {
            $res<type>      := $res<type> ~ "c";
            $res<direction> := 'i';
        }
        elsif $use eq 'inout' {
            $res<direction> := 'io';
        }
        else {
            $res<direction> := 'o';
        }

        @result.push($res);
    }
    @result;
}

=begin

=item C<expand_args(@args)>

Given an argument list, returns a list of all the possible argument
combinations.

=end
sub expand_args(@args) {

    return list() unless @args;

    my $arg := @args.shift;

    my @var := list($arg<type>);
    if $arg<variant> {
        @var.push($arg<variant>);
    }

    my @list := expand_args(@args);
    unless +@list {
        return @var;
    }

    my @results;
    for @list -> $l {
        for @var -> $v {
            # NQP can't handle it automagically. So wrap $l into list.
            my @l := pir::does__IPS($l, 'array') ?? $l !! list($l);
            @results.push(list($v, |@l));
        }
    }

    @results;
}


method op_params($/) {
    my $past := PAST::Stmts.new(
        :node($/)
    );

    for $<op_param> {
        $past.push($_.ast);
    }

    make $past;
}

method op_param($/) {
    my $past := PAST::Var.new(
        :node($/),
        :isdecl(1)
    );

    # We have to store 2 "types". Just set 2 properties on Var for now
    $past<direction> := ~$<op_param_direction>;
    $past<type>      := ~$<op_param_type>;

    make $past;
}

method op_body($/) {
    # Single big chunk
    make PAST::Op.new(
        :node($/),
        :pasttype('inline'),
        :inline(~$/)
    );
}

# Local Variables:
#   mode: perl6
#   fill-column: 100
# End:
# vim: expandtab ft=perl6 shiftwidth=4:
