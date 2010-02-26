class Ops::Compiler is HLL::Compiler;
 
INIT {
    Ops::Compiler.language('Ops');
    Ops::Compiler.parsegrammar(Ops::Grammar);
    Ops::Compiler.parseactions(Ops::Actions);
}
