###
# [unknown]

.namespace [ "Tcl" ]

.sub "&unknown"
  .local pmc argv 
  argv = foldup

  $S0 = join " ", argv
  $I1 = spawnw $S0

  $P1 = new String
  $P1 = ""
  .return(TCL_OK,$P1)
.end
