#
# specify a different namespace than "PIR", as "PIR" is a built-in language
#
.namespace [ 'PIR-lang' ]

.include "errors.pasm"

.sub '__onload' :load :init
    load_bytecode 'PGE.pbc'
    load_bytecode 'Parrot/HLLCompiler.pbc'
    load_bytecode 'PAST-pm.pbc'
    load_bytecode 'PGE/Util.pbc'
    load_bytecode 'PGE/Text.pbc'
				    
    load_bytecode 'languages/PIR/lib/ASTGrammar.pbc'

    $P0 = new [ 'HLLCompiler' ]
    $P0.'language'('languages-PIR')
    $P0.'parsegrammar'('PIR::Grammar')
    $P0.'astgrammar'('ASTGrammar')

.end

.sub 'main' :main
    .param pmc args
   
   	# get program name for error reporting		
		.local string prog
		prog = shift args
						
		# Sanity check parameters
    $I0 = args
    unless $I0 >= 1 goto ERR_TOO_FEW_ARGS
		
		# Register the PIR compiler		
    $P0 = compreg 'languages-PIR'
    
    # set up a global variable to keep track of syntax errors
    .local pmc errs
    errs = new .Integer
    set_root_global 'errors', errs

		.local pmc labels
		labels = new .ResizablePMCArray		
		set_root_global 'heredoc', labels
		
		# Process command line options				
		load_bytecode "Getopt/Obj.pir"
		
    .local pmc getopts
    getopts = new "Getopt::Obj"
    getopts."notOptStop"(1)
    push getopts, "output|o=s"
    push getopts, "help|h"    
    .local pmc opts
    opts = getopts."get_options"( args )
    
    # put back the program name    
    unshift args, prog    
        
    # handle help option
    .local string help
    help = opts['help']
    if help goto USAGE    
    
    # handle target option
  	.local string output
    output = opts['output']    
    unless output goto OPTIONS_DONE        
    if output == "PARSE" goto TARGET_PARSE 
    if output == "PAST" goto TARGET_PAST    
    if output == "PIRTIDY" goto TARGET_PIR
    if output == "PARSETREE" goto TARGET_PARSETREE
    goto ERR_UNKNOWN_TARGET
    
  OPTIONS_DONE:  	  	    		
    
  TARGET_PARSE:
    $P1 = $P0.'command_line'(args, 'target' => 'parse')
    goto DONE    
    
  TARGET_PARSETREE:
  	load_bytecode 'PGE/Dumper.pbc'
    load_bytecode 'dumper.pbc'  
    $P1 = $P0.'command_line'(args, 'target' => 'parse')
    goto DONE
    
  TARGET_PAST:
    $P1 = $P0.'command_line'(args, 'target' => 'past')
    goto DONE
    
  TARGET_PIR:   
    $P1 = $P0.'command_line'(args, 'target' => 'PIR')
    goto DONE
    
  ##COMPILE_AND_RUN:   
  ##  $P1 = $P0.'command_line'(args)
 		
  DONE:    
    if errs > 0 goto ERR_MSG
    print "Parse successful!\n"    
    .return($P1)

  ERR_MSG: 		
    if errs == 1 goto ONE_ERROR    
    printerr "There were "           
    printerr errs
    printerr " errors.\n"    
    end

  ONE_ERROR: 		
    printerr "There was 1 error.\n"
    end
    
  USAGE:
 		printerr "Usage: "
    printerr prog
    printerr " [OPTIONS] FILE\n"
    printerr <<"OPTIONS"
 	Options:  	
  	--help            -- print this message
  	--output=TARGET   -- specify target 
  	  possible targets are:
  	     PARSE     -- parse only (default)
  	     PAST      -- print Parrot AST
  	     PIRTIDY   -- print generated PIR code
  	     PARSETREE -- parse and print parse tree 
OPTIONS
    exit 1

	ERR_TOO_FEW_ARGS:
		printerr "Error: too few arguments\n"
		goto USAGE
	ERR_UNKNOWN_TARGET:
		printerr "Error: "
		printerr output
		printerr " is an unknown target\n"		
		exit 1		
.end


.namespace [ 'PIR::Grammar' ]

.sub _load :load :init
    load_bytecode 'PGE.pbc'
    load_bytecode 'PGE/Text.pbc'

    $P0 = getclass 'PGE::Grammar'
    $P1 = subclass $P0, 'PIR::Grammar'

    # The parser is split up into several files
    # for faster edit-compile-test cycles.
    # Compiling the file pasm_grammar takes ages
    # due to the large number of instructions.
    load_bytecode 'languages/PIR/lib/pir_grammar_gen.pbc'
    load_bytecode 'languages/PIR/lib/pasm_grammar_gen.pbc'
    load_bytecode 'languages/PIR/lib/pasm_args_gen.pbc'
    load_bytecode 'languages/PIR/lib/pasm_instr_gen.pbc'
    load_bytecode 'languages/PIR/lib/pasm_io_gen.pbc'
    load_bytecode 'languages/PIR/lib/pasm_pmc_gen.pbc'
    load_bytecode 'languages/PIR/lib/pasm_core_gen.pbc'
.end




.sub process_heredocs
		.param pmc mob
		
	
		.local pmc labels
		labels = get_root_global "heredoc"
		.local pmc id
		.local int count, i
		
		count = labels
		i = 0
	
	loop:	
		if i >= count goto endloop
		id = shift labels		
		i += 1
		
		printerr id
		#mob = heredoc_label(mob)
		#heredoc_label()
								
		goto loop
	endloop:

.end



.sub store_heredoc_label
	.param pmc mob
	.param pmc heredocid
	
	printerr heredocid
	printerr "\n"
		
	.local pmc labels
	labels = get_root_global "heredoc"
	push labels, heredocid
	
#	.local pmc iter, obj
#	iter = new .Iterator, labels
#	printerr "\n===============\n"
#loop:
#	unless iter goto endloop
#	obj = shift iter
#	printerr "Obj: "
#	printerr obj
#	printerr "\n"
#	goto loop
#endloop:
#	printerr "\n===============\n"
#	.return (mob, adverbs :flat :named)	
.end

#
# Clear all heredoc labels
#
.sub clear_heredocs
		.local pmc labels
		.local int count, i
		labels = get_root_global 'heredoc'
		count = labels
		i = 0
	loop:
		if i >= count goto endloop
		delete labels[i]
		i += 1
		goto loop
	endloop:	
		#printerr "=============\n"
.end






.sub warning
	  .param pmc self
	  .param string message
		
	  if null self goto NO_SELF
	  if null message goto NO_MSG
	
	  printerr "Warning: "
	  $P0 = get_hll_global ['PGE::Util'], 'warn'
	  if null $P0 goto NO_WARN
	  self.$P0(message)
	  .return()
	NO_WARN:
	  printerr "Cannot find method 'warn'\n"
	  .return()
	NO_MSG:
	  printerr "Warning: 'no message specified for warning()\n"
	  .return()
	NO_SELF:
	  printerr "No 'self' in warning()\n"
	  
	  .return()	  
.end

.sub syntax_error
  	.param pmc self
  	.param string message
  	
  	$P0 = get_hll_global ['PGE::Util'], 'line_number'
  	if null $P0 goto NO_LINE_NR_METHOD
  	$I0 = self.$P0()
  	
  	# line_number() starts counting at line 0, so increment:
  	inc $I0
  	
  	printerr "Syntax error (line "
  	printerr $I0
  	printerr "): "
  	printerr message
  	printerr "\n\n"
  	
  	# increment parse errors
  	.local pmc errs
  	errs = get_root_global 'errors'
  	inc errs
  	
  	.return()
  	
	NO_LINE_NR_METHOD:
	  printerr "can't find PGE::Util::line_number"
	  exit 1
.end



=head2 Custom parse methods

The rules C<target> and C<label> need to be redefined when parsing 
macros. These wrapper rules invoke the appropiate rules in pir.pg
depending on the context.

=cut

.sub target
		.param pmc mob
		.param pmc adverbs :slurpy :named
		$P0 = get_global 'macro_context'
		if null $P0 goto do_normal
		if $P0 > 0 goto do_macro
	do_normal:				
		.return normal_target(mob, adverbs :flat :named)
	do_macro:			
		.return macro_target(mob, adverbs :flat :named)
.end


.sub label
		.param pmc mob
		.param pmc adverbs :slurpy :named
		$P0 = get_global 'macro_context'
		if null $P0 goto do_normal
		if $P0 > 0 goto do_macro
	do_normal:				
		.return normal_label(mob, adverbs :flat :named)
	do_macro:			
		.return macro_label_decl(mob, adverbs :flat :named)
.end

.sub init_macro_rules        	
		.local pmc macro_context
		macro_context = new .Integer
		macro_context = 1
		set_global 'macro_context', macro_context
.end

.sub close_macro_rules	
		$P0 = get_global 'macro_context'
		$P0 = 0
.end







=head1 LICENSE

Copyright (C) 2007, The Perl Foundation.

This is free software; you may redistribute it and/or modify
it under the same terms as Parrot.

=head1 AUTHOR

Klaas-Jan Stol <parrotcode@gmail.com>

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
