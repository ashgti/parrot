###
# [uplevel]

# Pretty much a copy of "eval" except for the call_level...
# needs argument checking.

.namespace [ "Tcl" ]

.sub "&uplevel"
  .param pmc argv :slurpy
 
  .local string expr
  .local int argc
  .local int looper
 
  .local pmc compiler,pir_compiler
  compiler = find_global "_Tcl", "compile"
  pir_compiler = find_global "_Tcl", "pir_compiler"

  # save the old call level
  .local pmc old_call_level
  $P2 = find_global "_Tcl", "call_level"
  old_call_level = clone $P2

  .local pmc call_level
  call_level = argv[0]
 
  .local int defaulted 
  (call_level,defaulted) = __get_call_level(call_level)
  if defaulted == 1 goto skip

  $P1 = shift argv # pop the call level argument 
skip:

  expr = ""
  looper = 1
  argc = argv

loop:
  if looper == argc goto loop_done
  $S0 = argv[looper]
  concat expr, $S0
  inc looper
  if looper == argc goto loop_done
  concat expr," "

  goto loop

loop_done:

  # Set the new level 
  store_global "_Tcl", "call_level", call_level

  $P1 = parse(expr,0,0)

  ($I0,$P0) = compiler(0,expr)
  $P1 = pir_compiler($I0,$P0)

  # can't quite tailcall this at the moment due to the hackish call_level
  $P0 = $P1()

  #restore the old level
  store_global "_Tcl", "call_level", old_call_level

done:
  .return($P0) 
.end
