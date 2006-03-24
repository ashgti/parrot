
=head1 TITLE

PGE::P6Grammar.pir - compile a Perl 6 grammar

=head1 DESCRIPTION

This module defines the "PGE::P6Grammar" compiler, which can be
used to compile complete sets of rules (grammars).  Input to the 
grammar compiler is a string containing statements of the form

    grammar <name> ;
    rule <ident> { <p6rule> } ;
    # comment

A C<grammar> statement indicates where to load any subsequent rules.
Unlike Perl 6, there can be multiple C<grammar> statements in the
string.  

Here's a code snippet to load a set of rules using the grammar compiler:

    load_bytecode "PGE.pbc"
    load_bytecode "PGE/P6Grammar.pir"

    $S0 = <<'END'
        grammar PGE::Rule;
        rule ident { [ _ | <?alpha> ] \w* }
        rule name { <ident> [ \:\: <ident> ]* }
    END
    $P0 = compreg "PGE::P6Grammar"
    (code, onload) = $P0($S0)
    # rules <PGE::Rule::ident> and <PGE::Rule::name> have now been loaded

The return value from the compiler is a string containing the
PIR generated from the rules in the grammar, and code for generating
any classes defined by the grammar.  This code can be used
to generate precompiled .pbc files for other packages.

=head2 Functions

=over 4

=item C<__onload()>

Generates the grammar parse rule needed to parse grammar strings.

=cut

.namespace [ "PGE" ]

.sub "__onload" :load
    $S0 = ':w ( (\#)\N* | (grammar) (\w+[\:\:\w+]*) ;? | (rule) (\w+) \{<p6rule>\} )*'
    $P0 = compreg "PGE::P6Rule"
    $P0($S0, "PGE", "_grammar_rule")
    $P0 = find_global "PGE", "compile_rules"
    compreg "PGE::P6Grammar", $P0
    .return ()
.end

=item C<compile_rules(STR src)>

Compiles and loads a sequence of rule statements from the string
given by C<src>.  

=cut


.sub "compile_rules"
    .param string str
    .local pmc p6rule
    .local pmc parser
    .local pmc match, slist, stmt
    .local pmc code
    .local string namespace
    .local int namecount
    .local pmc namehash
    .local pmc onload
    .local pmc emit

    code = new .String
    p6rule = compreg "PGE::P6Rule"
    parser = find_global "PGE", "_grammar_rule"
    namespace = "PGE::Rule"
    namecount = 0
    namehash = new .Hash
    namehash[namespace] = 1
    onload = new .String
    onload = ".sub __onload :anon :load\n"
    emit = find_global "PGE::Exp", "emit"

    match = parser(str)
    unless match goto end
    slist = match[0]
  loop:
    unless slist goto end
    stmt = shift slist
    $S1 = stmt[0]
    if $S1 == "rule" goto compile_rule
    if $S1 == "grammar" goto grammar
    goto loop
  grammar:
    namespace = stmt[1]
    $I0 = namehash[namespace]
    if $I0 > 0 goto loop
    namehash[namespace] = 1
    inc namecount
    emit(onload, "    $I0 = find_type \"%s\"", namespace)
    emit(onload, "    if $I0 != 0 goto onload_%s", namecount)
    emit(onload, "    load_bytecode \"PGE.pbc\"")
    emit(onload, "    $P0 = getclass \"PGE::Rule\"")
    emit(onload, "    $P0 = subclass $P0, \"%s\"", namespace)
    emit(onload, "  onload_%s:", namecount)
    goto loop
  compile_rule:
    $S1 = stmt[1]
    $S2 = stmt['p6rule']
    ($P3, $P4, $P5) = p6rule($S2, namespace, $S1)
    emit(code, "\n# <%s::%s>", namespace, $S1)
    code .= $P4
    code .= "\n"
    goto loop
  end:
    emit(onload, ".end\n")
    if namecount > 0 goto end_1
    onload = ""
  end_1:
    .return (code, onload)
.end

