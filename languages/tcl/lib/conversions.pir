.namespace [ "_Tcl" ]

=head2 _Tcl::__list

Given a PMC, get a list from it. If the PMC is a TclLst,
this is as simple as returning the list. If it's a string,
use __stringToList.

=cut

.sub __list
  .param pmc value

  $I0 = typeof value
  if $I0 == .TclList goto done

  $S0 = value
  .return __stringToList($S0)

done:
  .return(value)
.end

=head2 _Tcl::__number

Given a PMC, get a number from it.

=cut

.sub __number
  .param pmc value

  $I0 = typeof value
  if $I0 == .TclInt goto done
  if $I0 == .TclFloat goto done
  
  $S0 = value
  $I0 = length $S0

  .local int multiplier
  multiplier = 1

  $S1 = substr $S0, 0, 1
  if $S1 == "+" goto positive
  # If the first character is -, assume a negative number.
  unless $S1 == "-" goto get_value
  multiplier = -1
positive:
  $S0 = substr $S0, 1
  # we check for the length below
  dec $I0
get_value:
  (value, $I1) = get_number($S0, 0)
  if null value goto NaN
  if $I0 != $I1 goto NaN
  value *= multiplier

done:
  .return(value)

NaN:
  .throw("Not a number!")
.end
