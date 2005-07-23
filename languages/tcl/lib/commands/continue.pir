###
# [continue]

.namespace [ "Tcl" ]

.sub "continue"

  .local int return_type

  .local pmc retval
  retval = new String

  if I3 == 0 goto noargs

  return_type = TCL_ERROR
  retval = "wrong # args: should be \"continue\"\n"
  goto done

noargs:
  return_type = TCL_CONTINUE
  retval = "" 

 # The sole purpose of this command (and break, and continue) is to 
 # generate a specific return code. Normally, when a proc finishes TCL_OK
 # control passes to the next item in the queue.

done:
  .return(return_type,retval)
.end
