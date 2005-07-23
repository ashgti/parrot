=head1 [proc]

Create a PIR sub on the fly for this user defined proc.

=cut

.namespace [ "Tcl" ]

.sub "&proc"
  .param pmc name_p
  .param pmc args_p
  .param pmc body_p

  if I3 != 3 goto error

  .local int return_type
  .local pmc retval
  return_type = TCL_OK

  .local pmc parser
  parser = find_global "_Tcl", "parser"

  .local pmc __list
  __list = find_global "_Tcl", "__list"

  .local string name, args

  name = name_p
  (return_type, retval) = __list(args_p)
  if return_type == TCL_ERROR goto done
  args_p = retval
  args = args_p

got_args:

  # Save the parsed body.
  .local pmc parsed_body
  $S0 = body_p
  parsed_body = parser."parse"($S0,0,0)
  register parsed_body

  # XXX these need to go away - for now, we'll just escape
  # the code portion and put it, escaped, into the proc 
  # definition. The arg list will be used to generate the proc's
  # indvidual argument handling code.
 
  # Now, shove the parsed routine into the global hash...
  $P0 = find_global "_Tcl", "proc_parsed"
  $P0[name] = parsed_body

  # Save the code for the proc for [info body]
  $P1 = find_global "_Tcl", "proc_body"
  $P1[name] = body_p

  # Save the args for the proc for [info body]
  # XXX When dealing with defaults, this will have to be updated.
  $P1 = find_global "_Tcl", "proc_args"
  $P1[name] = args_p

  .local pmc escaper
  escaper = find_global "Data::Escape", "String"
  .local string esc_name
  esc_name = escaper(name)

  .local string proc_body
  proc_body  = ".namespace [\"Tcl\"]\n.sub \"&"
  proc_body .= name
  proc_body .= "\"\n.param pmc args\nargs = foldup\nnew_pad 1\n"
  proc_body .= ".local pmc call_level\ncall_level = find_global \"_Tcl\", \"call_level\"\ninc call_level\n"
  .local int arg_count 
  arg_count = args_p
  .local int ii
  ii = 0
arg_loop:
  if ii == arg_count goto arg_loop_done
  $S1 = ii
  proc_body .= "$P1 = args["
  proc_body .= $S1
  proc_body .= "]\nstore_lex -1,\"$"
  # XXX This should be Escape'd
  $S1 = args_p[ii]
  proc_body .= $S1
  proc_body .= "\", $P1\n"
  ii = ii + 1
  goto arg_loop
arg_loop_done:

  proc_body .= ".local pmc interpret\ninterpret = find_global \"_Tcl\", \"__interpret\"\n.local pmc proc_body\n$P0 = find_global \"_Tcl\", \"proc_parsed\"\nproc_body=$P0[\""
  proc_body .= esc_name
  proc_body .= "\"]\nif I3 == "
  $S1 = arg_count
  proc_body .= $S1
  proc_body .= " goto args_ok\nbad_args:\n.return(1,\"wrong # args: should be \\\""
  proc_body .= name
  proc_body .= " "
  proc_body .= args
  proc_body .= "\\\"\")\nargs_ok:\n"

  # XXX Is the pop_pad necessary, or would it be  handled as a side
  #  effect of the .return?
 
  # a TCL_RETURN (2) from a sub body should be transformed into a TCL_OK (0)
  # to stop propagation outward.  XXX Should use the real constants here

  proc_body .= "($I0,$P0) = interpret(proc_body)\nif $I0 != 2 goto done\n$I0 = 0\ndone:\npop_pad\ndec call_level\n.return($I0,$P0)\n.end\n"

  #print "PROC_BODY=\n"
  #print proc_body
  #print "\n--\n"
  
  .local pmc pir_compiler
  pir_compiler = compreg "PIR"
  $P0 = compile pir_compiler, proc_body 

  # XXX because of the current implementation of the PIR compiler, we must save a reference
  # to our newly compiled function or run the risk of having it garbage collected
  $P1 = find_global "_Tcl", "proc_exec"
  $P1[name] = $P0
  
  retval = new String
  retval = ""
  goto done
 
error:
  return_type = TCL_ERROR
  retval = new String
  retval = "wrong # args: should be \"proc name args body\"\n"

done:
  .return(return_type,retval)
.end
