# Copyright (C) 2009-2010, Parrot Foundation.
# $Id$

class Ops::Compiler::Actions is HLL::Actions;

our $CODE;
our $OPLIB;

INIT {
    pir::load_bytecode("nqp-settings.pbc");
    $CODE := 0;
}

method TOP($/) {
    make $<body>.ast;
}

method body($/) {
    $OPLIB := Ops::OpLib.new() if !defined($OPLIB);

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
        $past<preamble>.push($_<preamble_guts>);
    }

    for $<op> {
        my $ops := $_.ast;
        my $skiptable := $OPLIB.skiptable;
        my $optable   := $OPLIB.optable;
        for @($ops) -> $op {
            my $full_name := $op.full_name;
            if ! $skiptable.exists($full_name) {
                if $optable.exists($full_name) {
                    $op<code> := $optable{$full_name};
                }
                else {
                    say('# experimental. Not in ops.num ' ~ $full_name);
                    $op<code> := $CODE++;
                }

                $past<ops>.push($op);
            }
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
    my %flags := hash();
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

    my $op := Ops::Op.new(
        :name(~$<op_name>),
    );

    # Flatten PAST::Stmts into Op.
    for @($<op_body>.ast) {
        $op.push($_);
    }

    for $<op_body>.ast<jump> {
        $op.add_jump($_);
    }
    if ~$<op_name> eq 'runinterp' {
        $op.add_jump('PARROT_JUMP_RELATIVE');
    }
    $op<flags> := %flags;
    $op<args>  := @args;
    $op<type>  := ~$<op_type>;
    $op<normalized_args> := @norm_args;

    if !%flags<flow> {
        my $goto_next := PAST::Op.new(
            :pasttype('call'),
            :name('goto_offset')
        );
        $goto_next<is_next> := 1;

        my $nl := "\n";
        $op.push(PAST::Op.new(
                :pasttype<inline>,
                :inline($nl)
            ));
        $op.push($goto_next);
        $op.push(PAST::Op.new(
                :pasttype<inline>,
                :inline<;>
            ));
    }

    my $past := PAST::Stmts.new(
        :node($/)
    );

    if @variants {
        for @variants {
            my $new_op := pir::clone__PP($op);
            $new_op<arg_types> := $_;
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
        elsif $arg<type> ne 'LABEL' {
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
    my $past := PAST::Stmts.new(
        :node($/),
    );
    $past<jump> := list();
    my $prev_words := '';
    for $<body_word> {
        if $prev_words && $_<word> {
            $prev_words := $prev_words ~ ~$_<word>;
        }
        elsif $_<word> {
            $prev_words := ~$_<word>;
        }
        else {
            $past.push(PAST::Op.new(
                :pasttype('inline'),
                :inline($prev_words),
            ));
            $prev_words := '';

            if $_<macro_param> {
                $past.push($_<macro_param>.ast);
            }
            elsif $_<op_macro> {
                $past.push($_<op_macro>.ast);
                for $_<op_macro>.ast<jump> {
                    $past<jump>.push($_);
                }
            }
        }
    }
    if $prev_words {
        $past.push(PAST::Op.new(
            :pasttype('inline'),
            :inline($prev_words)
        ));
    }
    make $past;
}

method macro_param($/) {
    make PAST::Var.new(
        :name(~$<num>),
        :node($/),
    );
}

method body_word($/) {
    #say('# body_word');
    my $past;
    if $<word> {
        $past := PAST::Op.new(
            :pasttype('inline'),
            :inline(~$<word>)
        );
    }
    elsif $<macro_param> {
        $past := $<macro_param>.ast;
    }
    elsif $<op_macro> {
        $past := $<op_macro>.ast;
    }
    else {
        die('horribly');
    }
    #_dumper($past);
    make $past;
}

method op_macro($/) {
    #say('# op_macro');
    # Generate set of calls to Trans:
    # goto NEXT()         -> goto_offset(opsize())
    # goto OFFSET($addr)  -> goto_offset($addr)
    # goto ADDRESS($addr) -> goto_address($addr)
    # expr NEXT()         -> expr_offset(opsize())
    # expr OFFSET($addr)  -> expr_offset($addr)
    # expr ADDRERR($addr) -> expr_address($addr)
    # restart NEXT()      -> restart_offset(opsize()); goto_offset(opsize())
    # restart OFFSET()    -> restart_offset($addr); goto_offset($addr)
    # restart ADDRESS()   -> restart_address($addr); goto_offset($addr)

    my $is_next    := ~$<macro_destination> eq 'NEXT';
    my $macro_name := ~$<macro_type> ~ '_' ~ lc($is_next ?? 'offset' !! ~$<macro_destination>);

    my $past := PAST::Op.new(
        :pasttype('call'),
        :name($macro_name),
    );

    $past<jump> := list();

    if ~$<macro_type> ne 'expr' && ~$<macro_destination> eq 'OFFSET' {
        $past<jump>.push('PARROT_JUMP_RELATIVE');
    }

    #_dumper($<body_word>);
    if $<body_word> {
        for $<body_word> {
            #say(' word ' ~ $_);
            my $bit := $_.ast;
            $past.push($_.ast) if defined($bit);
        }
    }
    $past<is_next> := $is_next;
    make $past;
}

method macro_sanity_checks($/) {
    #can't have NEXT with non-empty param
    #must have param with OFFSET or ADDRESS
    #can't have restart ADDRESS

}

# Local Variables:
#   mode: perl6
#   fill-column: 100
# End:
# vim: expandtab ft=perl6 shiftwidth=4:
