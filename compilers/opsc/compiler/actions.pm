# Copyright (C) 2009, Parrot Foundation.
# $Id$

class Ops::Grammar::Actions;

method TOP($/) {
    my $past := PAST::Op.new(
        :node($/)
    );

    $past<header> := $<header>.ast;
    $past<ops>    := $<ops>.ast;
    make $past;
}

method header($/) {
    # Single big chunk
    make PAST::Op.new(
        :node($/),
        :pasttype('inline'),
        :inline(
            substr($/.orig, 0, $/.from)
        )
    );
}

method ops($/) {
    # Brr.. We need something more tasty here.
    my $past := PAST::Stmts.new(
        :node($/)
    );

    for $<op> {
        $past.push($_.ast);
    }

    make $past;
}

method op($/) {
    my $past := PAST::Block.new(
        :name(~$<op_name>),
        :node($/),

        $<op_body>.ast
    );

    # Handling flags.
    for $<op_flag> {
        $past<flags>{~$_<identifier>} := 1;
    }

    # Handling parameters.
    #$past<parameters> := $<op_params>.ast;

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
# vim: expandtab shiftwidth=4:
