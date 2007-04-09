=head1 NAME

c99 -- A parser for c99 

=head1 SYNOPSIS

  $ ./parrot languages/c99/c99.pir code.c

=head1 DESCRIPTION

  c99 is a PGE parser running on Parrot.

=cut

.HLL 'c99', 'c99_group'
.include 'errors.pasm'
.include 'library/dumper.pir'

.sub _main :main
    .param pmc args
    
    errorson .PARROT_ERRORS_PARAM_COUNT_FLAG
    
    load_bytecode 'PGE.pbc'
    load_bytecode 'dumper.pbc'
    load_bytecode 'PGE/Dumper.pbc'
    load_bytecode 'PGE/Text.pbc'
    load_bytecode 'Getopt/Obj.pbc'
    
    print "Parrot C99 Compiler\n"

    load_bytecode 'languages/c99/src/c99_PGE.pbc'
    load_bytecode 'languages/c99/src/CPP_PGE2AST.pbc'
    load_bytecode 'languages/c99/src/CPP_PASTNodes.pbc'
    load_bytecode 'languages/c99/src/CPP_AST2Constants.pbc'
    #load_bytecode 'languages/c99/src/CPP_POST.pbc'
    #load_bytecode 'languages/c99/src/CPP_PGE2AST.pbc'
    #load_bytecode 'languages/c99/src/CPP_AST2OST.pbc'
    #load_bytecode 'languages/c99/src/CPP_OST2PIR.pbc'

    .local pmc _dumper
    .local pmc getopts
    .local pmc opts
    .local string arg0
    _dumper = get_root_global [ 'parrot' ], '_dumper'
    arg0 = shift args
    getopts = new 'Getopt::Obj'
    getopts.'notOptStop'(1)
    push getopts, 'target=s'
    push getopts, 'constants'
    opts = getopts.'get_options'(args)

    $S0 = opts['help']
    if $S0 goto usage
    .local int constants
    constants = opts['constants']


    .local int stopafter
    .local string dump
    .local string target
    .local int dump_src_early
    .local int dump_pge
    .local int dump_ast
    .local int dump_constants
    
    .local pmc c99_compiler
    .local string filename
    .local string source
    
    stopafter = 10
    dump_src_early = 1
    dump_pge = 1
    dump_ast = 1
    dump_constants = 1

   c99_compiler = compreg 'C99'
    
    $I0 = elements args
    if $I0 > 0 goto file_arg

  stdin_arg:
    .local pmc stdin
    filename = "STDIN"
    stdin = getstdin
    push stdin, 'utf8'
    # enable interactive readline if possible
    $I0 = stdin.'set_readline_interactive'(1)

  stdin_read_loop:
    .local string stmt
    stmt = stdin.'readline'('c99> ')
    unless stmt goto end
    #bsr c99_eval
    goto stdin_read_loop

  file_arg:
    filename = args[1]
    source = slurp_file(filename)
    goto compile_it

  usage:
    print "usage: c99.pbc [-o output] [file]\n"
    end

  compile_it:
    .local pmc parse_tree
    .local pmc start_rule
    
    unless dump_src_early goto after_src_dump_early
    print "\n\nSource dump:\n"
    print source
  after_src_dump_early:

    start_rule = get_root_global [ 'parrot'; 'C99::Grammar'], 'translation_unit'
    start_rule = get_root_global [ 'parrot'; 'C99::Grammar'], 'prereprocessing_file'
    parse_tree = start_rule(source, 'grammar'=> 'C99::Grammar')

    # Verify the parse was successful
    $I0 = parse_tree.__get_bool()
    unless $I0 goto err_parse_fail

    unless dump_pge goto after_pge_dump
    print "Parse succeeded: C99 PGE parse tree dump:\n"
    _dumper(parse_tree, "C99 PGE Parse Tree Dump")
  after_pge_dump:
    eq stopafter, 1, end
    
    # "Traverse" the parse tree
    .local pmc grammar
    .local pmc astbuilder
    .local pmc ast
    grammar = new 'C99::CPP::ASTGrammar'
    astbuilder = grammar.apply(parse_tree)
    ast = astbuilder.get('result')
    $I0 = defined ast
    unless $I0 goto err_no_ast 

    unless dump_ast goto after_ast_dump
    print "\n\nAST tree dump:\n"
    _dumper(ast)
    #ast.'dump'()
  after_ast_dump:
    eq stopafter, 2, end
    
  unless constants goto after_constants_dump
    # "Traverse" the parse tree
    .local pmc grammar
    .local pmc constantsbuilder
    .local pmc constants_out
    grammar = new 'C99::CPP::Constants::ConstantsGrammar'
    constantsbuilder = grammar.apply(ast)
    constants_out = constantsbuilder.get('root')
    $I0 = defined constants_out
    unless $I0 goto err_no_constants

    unless dump_constants goto after_constants_dump
    print "\n\nConstants dump:\n"
    print constants_out
    print "\n"
  after_constants_dump:
    eq stopafter, 2, end

    exit 0


  err_parse_fail:
    print "Parse failed on "
    print filename
    print "\n"
    exit -1

  err_no_ast:
    print "Unable to construct AST.\n"
    exit -2

  err_no_constants:
    print "Unable to construct Constants.\n"
    exit -2

  err_no_ost:
    print "Unable to construct OST.\n"
    exit -3

  err_no_pir:
    print "Unable to construct PIR.\n"
    exit -4

  end:
    exit 0
.end

.sub pir_exec
    .param pmc code
    .local pmc pir_compiler
    .local pmc pir_compiled
    pir_compiler = compreg "PIR"
    pir_compiled = pir_compiler(code)
    pir_compiled()
.end

.sub slurp_file
    .param string filename
    .local pmc filehandle
    filehandle = open filename, "<"
    unless filehandle goto err_no_file
    $S1 = read filehandle, 65535
    close filehandle
    .return ($S1)

  err_no_file:
    print "Unable to open file "
    print filename
    print "\n"
    end
.end

.sub spew_file
    .param string filename
    .param string contents
    .local pmc filehandle
    filehandle = open filename, ">"
    unless filehandle goto err_no_file
    print filehandle, contents
    close filehandle
    .return ($S1)

  err_no_file:
    print "Unable to open file "
    print filename
    print "\n"
    end
.end

=head1 LICENSE

Copyright (c) 2005 The Perl Foundation

This is free software; you may redistribute it and/or modify
it under the same terms as Parrot.

=head1 AUTHOR

Kevin Tew <kevintew@tewk.com>

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
