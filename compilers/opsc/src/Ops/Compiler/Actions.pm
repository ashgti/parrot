# Copyright (C) 2009, Parrot Foundation.
# $Id$

class Ops::Compiler::Actions is HLL::Actions;

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
        $past<ops>.push($_.ast);
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
        @args := $<op_params>[0].ast;
    }

    my $past := Ops::Op.new(
        :code(-1),
        :name(~$<op_name>),
        :type(~$<op_type>),

        :flags(%flags),
        :args(@args),

        $<op_body>.ast
    );

    make $past;
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
