###
# [while]

.namespace [ "Tcl" ]

.sub "&while"
  .param pmc cond_p
  .param pmc body_p

  if argcP != 2 goto bad_args

  .local pmc retval, parsed_code
  .local int return_type

  .local pmc parser
  .local pmc interpret
  .local pmc expression_p
  .local pmc expression_i

  parser = find_global "_Tcl", "parser"
  interpret = find_global "_Tcl", "__interpret"
  expression_p = find_global "_Tcl", "__expression_parse"
  expression_i = find_global "_Tcl", "__expression_interpret"

while_loop:
  $S0 = body_p
  parsed_code = parser."parse"($S0,0,0)
  register parsed_code
  (return_type,retval) = expression_p(cond_p)
  if return_type == TCL_ERROR goto done_done
  (return_type,retval) = expression_i(retval)
  if return_type == TCL_ERROR goto done_done
  unless retval goto done
  (return_type,retval) = interpret(parsed_code)
  if return_type == TCL_BREAK goto done
  if return_type == TCL_RETURN goto done

  goto while_loop

done:
  if return_type == TCL_RETURN goto done_cleaned
  if return_type == TCL_ERROR  goto done_cleaned
  retval = ""
  return_type = TCL_OK
  goto done_done

done_cleaned:
  retval = ""

done_done:
  # while always returns "", regardless of the code it may have executed
  # XXX - (unless there's an error)

  .return(return_type,retval)

bad_args:
  $P1 = new String
  $P1 = "wrong # args: should be \"while test command\""
  .return (TCL_ERROR, $P1)

.end
