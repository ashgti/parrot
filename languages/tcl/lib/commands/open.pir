###
# [open]

# XXX These variants of open are not supported.
# open filename access
# open filename access permissions

.namespace [ "Tcl" ]

.sub "&open"
  .local pmc argv 
  argv = foldup

  .local int return_type
  .local pmc retval
  retval = new String

  .local int argc
  argc = argv
  if argc != 1 goto error

  $S1 = argv[0] 
  open $P1, $S1, ">"
  $I0 = typeof $P1
  if $I0 == .Undef goto file_error
  retval = "file"
  $P2 = find_global "_Tcl", "channels"
  # get a new file channel name
  $P3 = find_global "_Tcl", "next_channel_id"
  $S0 = $P3
  retval = retval . $S0
  $P3 = $P3 + 1
  $P2[retval] = $P1
  #print "retval is:"
  #print retval
  #print "\n"
  goto done
 
file_error:
  return_type = TCL_ERROR
  retval = "unable to open specified file"
  goto done
 
error:
  return_type = TCL_ERROR
  retval = "bad call to open"
  goto done

done:
  .return(return_type,retval)
.end
