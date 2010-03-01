# nqp
class Ops::Compiler is HLL::Compiler;

INIT {
    Ops::Compiler.language('Ops');
    Ops::Compiler.parsegrammar(Ops::Compiler::Grammar);
    Ops::Compiler.parseactions(Ops::Compiler::Actions);
}


# vim: ft=perl6 expandtab shiftwidth=4:
