###
# [upvar]

.namespace [ "Tcl" ]

.sub "&upvar"
  .param pmc argv :slurpy

  .local pmc call_level,current_call_level
  call_level = argv[0]
  current_call_level = find_global "_Tcl", "call_level"

  .local int defaulted
  (call_level,defaulted) = __get_call_level(call_level)
  if defaulted == 1 goto skip
  $P1 = shift argv

skip:
  # XXX Need error handling.
  
  # for each othervar/myvar pair, created a mapping from

  .local int counter,argc
  argc = argv
  counter = 0
loop:
  if counter >= argc goto done

  $I0 = call_level
  $I1 = current_call_level
 
  $S0 = argv[counter]
  inc counter
  $S1 = argv[counter]
 
  push_eh catch 
    $P1 = find_lex $S0
    store_lex $S1, $P1
  clear_eh
resume:

  inc counter
  goto loop
 
done:
  .return("")

catch:
  goto resume

.end
