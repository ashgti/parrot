#
# list-related utility methods.
#

# This is very similar to _Tcl::__string_index, and perhaps they could be refactored.

.HLL '_Tcl', ''

.sub _list_index
  .param pmc the_list
  .param pmc pmc_position
  
  .local string position
  position = pmc_position

  .local int is_end
  is_end = 1

  .local pmc retval

  .local int index_length, pos
  .local pmc number_result

  if position == "end" goto my_end
  
  $S0 = substr position, 0, 4
  if $S0 == "end-" goto has_end
  index_length = length position
  (retval, pos) = get_number(position,0)
  if pos != index_length goto bad_arg
  $I0 = isa retval, "Integer"
  if $I0 == 0 goto bad_arg
  
  # if the number is greater than the number of elements
  # in the list, we want the end
  $I0 = the_list
  if retval > $I0 goto my_end
  
  is_end = 0
  .return(retval,is_end)

bad_arg:
  $S9  = "bad index \""
  $S9 .= position
  $S9 .= "\": must be integer?[+-]integer? or end?[+-]integer?"
  .throw($S9)

has_end:
  # is this an int? if so, subtract it from -1 to get our parrot-style index.
  index_length = length position
  # is this an int?
  (number_result, pos) = get_number(position,4)
  if pos != index_length goto bad_arg
  $I0 = isa number_result, "Integer"
  if $I0 == 0 goto bad_arg
  # say, 1 if -1
  $I0 = number_result
  # say, 2 if -2
  inc $I0

  # say, length is 6
  .local int index_1
  index_1 = the_list
  # so, if we had end-1, then we'd be at position 4. (end is 5, -1)
  index_1 = index_1 - $I0
  .return (index_1,is_end)

my_end:
  $I0 = the_list
  dec $I0
  .return($I0,is_end)
  
.end
