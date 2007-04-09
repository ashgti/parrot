#
# [lset]
#

.HLL 'Tcl', 'tcl_group'
.namespace

.sub '&lset'
  .param pmc argv :slurpy

  .local int argc
  argc = elements argv
  if argc < 2 goto wrong_args

  .local string name, value
  name  = argv[0]
  value = pop argv
  dec argc

  .local pmc __read, __list, __set
  __read = get_root_global ['_tcl'], '__read'
  __list = get_root_global ['_tcl'], '__list'
  __set  = get_root_global ['_tcl'], '__set'

  .local pmc retval, list
  list = __read(name)
  list = __list(list)
  retval = list

  # we removed the value, so this would be one now
  if argc == 1 goto replace

lset:
  .local pmc __index
  __index = get_root_global ['_tcl'], '__index'

  unless argc == 2 goto iterate
  $P0 = argv[1]
  $P0 = __list($P0)
  $I0 = elements $P0
  if $I0 == 0 goto replace

iterate:
  .local pmc indices, prev
  .local int outer_i
  outer_i = 0
outer_loop:
  inc outer_i
  if outer_i == argc goto done
  indices = argv[outer_i]
  indices = __list(indices)

  $I0 = 0
  $I1 = elements indices
loop:
  if $I0 == $I1 goto outer_loop

  $P0 = indices[$I0]
  $I2 = __index($P0, list)
  if $I2 < 0 goto out_of_range
  $I3 = elements list
  if $I2 >= $I3 goto out_of_range

  prev = list
  list = list[$I2]
  list = __list(list)
  prev[$I2] = list

  inc $I0
  goto loop

done:
  prev[$I2] = value
  __set(name, retval)
  .return(retval)

out_of_range:
  tcl_error 'list index out of range'

wrong_args:
  tcl_error 'wrong # args: should be "lset listVar index ?index...? value"'

replace:
  .return __set(name, value)
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
