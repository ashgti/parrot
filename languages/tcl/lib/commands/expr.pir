###
# [expr]

#
# expr arg [... arg arg]

.namespace [ "Tcl" ]

.sub "&expr"
  .param pmc argv :slurpy
 
  .local string expr
  .local int argc
  .local int looper

  .local pmc retval
  .local pmc expression_p
  .local pmc expression_i
  expression_p = find_global "_Tcl", "__expression_parse"

  expr = ""
  looper = 0
  argc = argv 
  unless argc goto no_args

loop:
  if looper == argc goto loop_done
  $S0 = argv[looper]
  concat expr, $S0
  inc looper
  if looper == argc goto loop_done
  concat expr," "

  goto loop

loop_done:
  $P1 = expression_p(expr)
  .return $P1()

no_args:
  .throw("wrong # args: should be \"expr arg ?arg ...?\"")
.end
