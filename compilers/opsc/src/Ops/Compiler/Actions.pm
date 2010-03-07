# Copyright (C) 2009, Parrot Foundation.
# $Id$

class Ops::Compiler::Actions is HLL::Actions;

our $CODE;

INIT {
    pir::load_bytecode("nqp-settings.pbc");
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
        $past<preamble>.push($_<preamble_guts>);
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
    my %flags := pir::new__Ps('OrderedHash');
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

        $<op_body>.ast
    );

    $op.jump($<op_body>.ast<jump>);
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
    # Single big chunk
    my $op := PAST::Op.new(
        :node($/),
        :pasttype('inline'),
    );
    $op<inline> := munch_body($op, ~$/);
    make $op;
}

sub munch_body($op, $body) {
    #
    # Macro substitutions:
    #
    # We convert the following notations:
    #
    #   .ops file          Op body  Meaning       Comment
    #   -----------------  -------  ------------  ----------------------------------
    #   goto OFFSET(X)     {{+=X}}  PC' = PC + X  Used for branches
    #   goto NEXT()        {{+=S}}  PC' = PC + S  Where S is op size
    #   goto ADDRESS(X)    {{=X}}   PC' = X       Used for absolute jumps
    #   expr OFFSET(X)     {{^+X}}  PC + X        Relative address
    #   expr NEXT()        {{^+S}}  PC + S        Where S is op size
    #   expr ADDRESS(X)    {{^X}}   X             Absolute address
    #   restart OFFSET(X)  {{=0,+=X}}   PC' = 0   Restarts at PC + X
    #   restart NEXT()     {{=0,+=S}}   PC' = 0   Restarts at PC + S
    #
    #   $X                 {{@X}}   Argument X    $0 is opcode, $1 is first arg
    #
    # For ease of parsing, if the argument to one of the above
    # notations in a .ops file contains parentheses, then double the
    # enclosing parentheses and add a space around the argument,
    # like so:
    #
    #    goto OFFSET(( (void*)interp->happy_place ))
    #
    # Later transformations turn the Op body notations into C code, based
    # on the mode of operation (function calls, switch statements, gotos
    # with labels, etc.).
    #
    
    $op<jump> := '0';

    if ($body ~~ / [ goto | restart ] \s+ OFFSET / ) {
        $op<jump> := 'PARROT_JUMP_RELATIVE';
    }

    #'goto ADDRESS((foo))' -> '{{=foo}}'
    $body := subst($body,
                /goto \s+ ADDRESS '((' $<addr>=[.*?] '))'/,
                -> $m { '{{=' ~ $m<addr> ~ '}}' }
            );

    #'expr ADDRESS((foo))' -> '{{^foo}}'
    $body := subst($body,
                /expr \s+ ADDRESS '((' $<addr>=[.*?] '))'/,
                -> $m { '{{^' ~ $m<addr> ~ '}}' }
            );


    #'goto ADDRESS(foo)' -> '{{=foo}}'
    $body := subst($body,
                /goto \s+ ADDRESS '(' $<addr>=[.*?] ')'/,
                -> $m { '{{=' ~ $m<addr> ~ '}}' }
            );

    #'expr ADDRESS(foo)' -> '{{^=foo}}'
    $body := subst($body,
                /expr \s+ ADDRESS '(' $<addr>=[.*?] ')'/,
                -> $m { '{{^' ~ $m<addr> ~ '}}' }
            );

    #'goto OFFSET((foo))' -> '{{+=foo}}'
    $body := subst($body,
                /goto \s+ OFFSET '((' $<addr>=[.*?] '))'/,
                -> $m { '{{+=' ~ $m<addr> ~ '}}' }
            );


    #'goto OFFSET(foo)' -> '{{+=foo}}'
    $body := subst($body,
                /goto \s+ OFFSET '(' $<addr>=[.*?] ')'/,
                -> $m { '{{+=' ~ $m<addr> ~ '}}' }
            );

    #'expr OFFSET((foo))' -> '{{^=foo}}'
    $body := subst($body,
                /expr \s+ OFFSET '((' $<addr>=[.*?] '))'/,
                -> $m { '{{^=' ~ $m<addr> ~ '}}' }
            );


    #'expr OFFSET(foo)' -> '{{+=foo}}'
    $body := subst($body,
                /expr \s+ OFFSET '(' $<addr>=[.*?] ')'/,
                -> $m { '{{^=' ~ $m<addr> ~ '}}' }
            );

    #'expr NEXT()' -> '{{^+OP_SIZE}}'
    $body := subst($body, /expr \s+ NEXT '(' ')'/, '{{^+OP_SIZE}}');
    #'goto NEXT()' -> '{{+=OP_SIZE}}'
    $body := subst($body, /goto \s+ NEXT '(' ')'/, '{{+=OP_SIZE}}');


    #'restart OFFSET(foo)' -> '{{=0,+=foo}}'
    $body := subst($body,
                /restart \s+ OFFSET '(' $<addr>=[.*?] ')'/,
                -> $m { '{{=0,+=' ~ $m<addr> ~ '}}' }
            );

    #'restart NEXT()' -> '{{=0,+=OP_SIZE}}'
    $body := subst($body,
                /restart \s+ NEXT '(' ')'/,
                '{{=0,+=OP_SIZE}}'
            );

    #'restart ADDRESS(foo)' -> '{{=foo}}'
    $body := subst($body,
                /restart \s+ ADDRESS '(' $<addr>=[.*?] ')'/,
                -> $m { '{{=' ~ $m<addr> ~ '}}' }
            );

    #'$1' -> '{{@1}}'        
    $body := subst($body,
                /'$' $<arg_num>=[\d+]/,
                -> $m { '{{@' ~ $m<arg_num> ~ '}}' }
            );


=begin COMMENT

    my $file_escaped = $file;
    $file_escaped =~ s|(\\)|$1$1|g;    # escape backslashes
    $op->body( $nolines ? $body : qq{#line $line "$file_escaped"\n$body} );

    # Constants here are defined in include/parrot/op.h
    or_flag( \$jumps, "PARROT_JUMP_RELATIVE" ) if $branch;

=end COMMENT

    $body;

}

# Local Variables:
#   mode: perl6
#   fill-column: 100
# End:
# vim: expandtab ft=perl6 shiftwidth=4:
