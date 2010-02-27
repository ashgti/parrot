# Copyright (C) 2009, Parrot Foundation.
# $Id$

class Ops::Actions is HLL::Actions;

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
    my $past := PAST::Block.new(
        :name(~$<op_name>),
        :node($/),

        $<op_body>.ast
    );

    # Handling flags.
    for $<op_flag> {
        $past<op_flags>{~$_<identifier>} := 1;
    }

    # Handling parameters.
    if $<op_params> {
        $past<parameters> := $<op_params>[0].ast;
    }

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
