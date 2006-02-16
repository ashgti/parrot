grammar PunieGrammar;

rule prog    { ^<PunieGrammar::lineseq>$ }

rule block { \s* \{ <PunieGrammar::lineseq> \} \s* }

rule lineseq { \s*<PunieGrammar::line>*\s* }

rule line    { 
    [ <PunieGrammar::label> <PunieGrammar::cond> 
    | <PunieGrammar::label> <PunieGrammar::expr>;
    ] \s* 
}

rule label { \s* [<PunieGrammar::word>\:]? \s* }

rule word { \w[\w|\d]* }

rule cond {
    (if|unless) \s* \( \s* <PunieGrammar::expr> \s* \) <PunieGrammar::block>
} 

rule expr    { <PunieGrammar::gprint> | <PunieGrammar::cexpr> }

rule gprint  { (print) \s* <PunieGrammar::expr> }

rule cexpr {
    <PunieGrammar::oexpr> \s* [, \s* <PunieGrammar::oexpr>]*
}

rule term {
    <PunieGrammar::number>
    | <PunieGrammar::integer>
    | <PunieGrammar::stringdouble>
    | <PunieGrammar::stringsingle>
    | do <PunieGrammar::block>
}
rule integer { \d+ }
rule number { \d+\.\d+ }
rule stringdouble { <PGE::Text::bracketed: "> }
rule stringsingle { <PGE::Text::bracketed: '> }


