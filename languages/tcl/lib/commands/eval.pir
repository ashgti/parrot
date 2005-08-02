###
# [eval]

#
# eval arg [... arg arg]

.namespace [ "Tcl" ]

.sub "&eval"
  .local pmc argv
  argv = foldup  
 
  .local string expr
  .local int argc
  argc = argv
  unless argc goto no_args
 
  .local int looper
 
  .local pmc parse
  .local pmc interpret 
  parse = find_global "_Tcl", "parse"
  interpret = find_global "_Tcl", "__interpret"

  expr = ""
  looper = 0

loop:
  if looper == argc goto loop_done
  $S0 = argv[looper]
  concat expr, $S0
  inc looper
  if looper == argc goto loop_done
  concat expr," "

  goto loop

loop_done:
  $P1 = parse(expr)
  register $P1

  .return interpret($P1) 

no_args:
  $P1 = new String
  $P1 = "wrong # args: should be \"eval arg ?arg ...?\""
  .return(TCL_ERROR, $P1)

.end
