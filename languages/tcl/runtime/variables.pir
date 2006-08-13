.HLL '_Tcl', ''
.namespace

=head2 _Tcl::__read

Read a variable from its name. It may be a scalar or an
array.

Use the call level to determine if we are referring to a 
global variable or a lexical variable - will no doubt
require further refinement later as we support namespaces
other than the default, and multiple interpreters.

=cut

.sub __read
  .param string name

  .local pmc variable

  # is this an array?
  # ends with )
  .local int char
  char = ord name, -1
  if char != 41 goto scalar
  # contains a (
  char = index name, '('
  if char == -1 goto scalar

array:
  .local string var
  var = substr name, 0, char
  
  .local string key
  .local int len
  len = length name
  len -= char
  len -= 2
  inc char
  key = substr name, char, len
 
  variable = __find_var(var)
  if_null variable, no_such_variable
  
  $I0 = does variable, 'hash'
  unless $I0 goto cant_read_not_array

  variable = variable[key]
  if_null variable, bad_index 
  .return(variable)

bad_index:
  $S0 = "can't read \""
  $S0 .= name
  $S0 .= '": no such element in array'
  .throw($S0)

cant_read_not_array:
  $S0 =  "can't read \""
  $S0 .= name
  $S0 .= "\": variable isn't array"
  .throw($S0)

scalar:
  variable = __find_var(name)
  if_null variable, no_such_variable
  
  $I0 = does variable, 'hash'
  if $I0 goto cant_read_array
  .return(variable)

cant_read_array:
  $S0 = "can't read \""
  $S0 .= name
  $S0 .= '": variable is array'
  .throw($S0)

no_such_variable:
  $S0 = "can't read \""
  $S0 .= name
  $S0 .= '": no such variable'
  .throw($S0)
.end

=head2 _Tcl::__make

Read a variable from its name. If it doesn't exist, create it. It may be a
scalar or an array.

Use the call level to determine if we are referring to a 
global variable or a lexical variable - will no doubt
require further refinement later as we support namespaces
other than the default, and multiple interpreters.

=cut

.sub __make
  .param string name

  .local pmc variable

  # is this an array?
  # ends with )
  .local int char
  char = ord name, -1
  if char != 41 goto scalar
  # contains a (
  char = index name, '('
  if char == -1 goto scalar

array:
  .local string var
  var = substr name, 0, char
  
  .local string key
  .local int len
  len = length name
  len -= char
  len -= 2
  inc char
  key = substr name, char, len
 
  variable = __find_var(var)
  unless null variable goto check_is_hash

  variable = new .TclArray
  __store_var(var, variable)
  
check_is_hash:
  $I0 = does variable, 'hash'
  unless $I0 goto cant_read_not_array

  $P0 = variable[key]
  if null $P0 goto create_elem 
  .return($P0)

create_elem:
  $P0 = new .Undef
  variable[key] = $P0
  .return($P0)

cant_read_not_array:
  $S0 =  "can't read \""
  $S0 .= name
  $S0 .= "\": variable isn't array"
  .throw($S0)

scalar:
  variable = __find_var(name)
  if null variable goto make_variable  
  .return(variable)

make_variable:
    variable = new .Undef
    __store_var(name, variable)
    .return(variable)
.end

=head2 _Tcl::__set

Set a variable by its name. It may be a scalar or an array.

Use the call level to determine if we are referring to a 
global variable or a lexical variable - will no doubt
require further refinement later as we support namespaces
other than the default, and multiple interpreters.

=cut

.sub __set
  .param string name
  .param pmc value

  .local pmc variable

  # Some cases in the code allow a NULL pmc to show up here.
  # This defensively converts them to an empty string.
  unless_null value, got_value
  value = new TclString
  value = ''

 got_value:
  # is this an array?
  # ends with )
  .local int char
  char = ord name, -1
  if char != 41 goto find_scalar
  # contains a (
  char = index name, '('
  if char == -1 goto find_scalar

find_array:
  .local string var
  var = substr name, 0, char
  
  .local string key
  .local int len
  len = length name
  len -= char
  len -= 2
  inc char
  key = substr name, char, len
  
  .local pmc array
  null array
  array = __find_var(var)
  if_null array, create_array

  $I0 = does array, 'hash'
  unless $I0 goto cant_set_not_array
  goto set_array

create_array:
  array = new .TclArray
  __store_var(var, array)

set_array:
  variable = array[key]
  if null variable goto set_new_elem
  assign variable, value
  .return(variable)

set_new_elem:
  array[key] = value
  .return(value)

cant_set_not_array:
  $S0 =  "can't set \""
  $S0 .= name
  $S0 .= "\": variable isn't array"
  .throw($S0)

find_scalar:
  .local pmc scalar
  null scalar
  scalar = __find_var(name)
  if_null scalar, create_scalar
  assign scalar, value
  goto return_scalar
  
create_scalar:
  __store_var(name, value)

return_scalar:
  $S0 = typeof value
  .return(value)
.end

=head2 _Tcl::__find_var

Utility function used by __read and __set.

Gets the actual variable from memory and returns it.

=cut

.sub __find_var
  .param string name

  name = '$' . name
  
  .local pmc value

  $S0 = substr name, 1, 2
  if $S0 == '::'     goto coloned
  
  .local int call_level
  $P1 = find_global 'call_level'
  call_level = $P1
  if call_level == 0 goto global_var

  .local pmc call_chain, lexpad, variable
  call_chain = get_root_global ['_tcl'], 'call_chain'
  push_eh notfound
    lexpad     = call_chain[-1]
    value      = lexpad[name]
  clear_eh
  if null value goto found
  $I0 = isa value, 'Undef'
  if $I0 goto notfound
  goto found

coloned:
    substr name, 1, 2, ''

global_var:
  push_eh notfound
    value = get_root_global ['tcl'], name
  clear_eh
  if null value goto found
  $I0 = isa value, 'Undef'
  if $I0 goto notfound
  goto found

notfound:
  null value
found:
  .return(value)
.end

=head2 _Tcl::__store_var

Utility function used by __read and __set.

Sets the actual variable from memory.

=cut

.sub __store_var
  .param string name
  .param pmc    value

  name = '$' . name

  $S0 = substr name, 1, 2
  if $S0 == '::'     goto coloned

  .local int call_level
  $P1 = find_global 'call_level'
  call_level = $P1
  if call_level == 0 goto global_var
lexical_var:
  .local pmc call_chain, lexpad
  call_chain   = get_root_global ['_tcl'], 'call_chain'
  lexpad       = call_chain[-1]
  lexpad[name] = value
  .return()

coloned:
  substr name, 1, 2, ''
global_var:
  set_root_global ['tcl'], name, value

  .return()
.end
