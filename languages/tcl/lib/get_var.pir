=head2 _Tcl::get_var

Utility multi-method to return the value of the variable
specified.

If a single string param is passed, then we lookup a scalar.

If two params are passed, then we lookup a TclArray (first param)
the value for a specific key (second param)

Use the call level to determine if we are referring to a 
global variable or a lexical variable - will no doubt
require further refinement later as we support namespaces
other than the default, and multiple interpreters.

=cut

.namespace [ "_Tcl" ]

.sub __get_var @MULTI(STRING)
  .param string argv0

  .local string sigil_argv0
  sigil_argv0 = "$" . argv0

  .local int call_level, return_type
  $P1 = find_global "_Tcl", "call_level"
  call_level = $P1
  return_type = TCL_OK

  .local pmc value, retval, variable
  null variable

get_scalar:
  push_eh catch
    if call_level == 0 goto get_scalar_global
    variable = find_lex call_level, sigil_argv0
    goto resume
get_scalar_global:
    variable = find_global "Tcl", sigil_argv0
resume:
  clear_eh

  isnull variable, no_such_variable 
  $I0 = does variable, "hash"
  unless $I0 goto get_scalar_ok

get_scalar_bad:
  return_type = TCL_ERROR
  $S0 = "can't read \""
  $S0 .= argv0
  $S0 .= "\": variable is an array"
  retval = new String
  retval = $S0
  goto done

get_scalar_ok:
  retval = variable
  goto done

no_such_variable:
  return_type = TCL_ERROR
  $S0 = "can't read \""
  $S0 .= argv0
  $S0 .= "\": no such variable"
  retval = new String
  retval = $S0
  # goto done            

done:
  .return(return_type,retval)

# simple error handler that silently catches the exception
catch:
  goto resume
.end

.sub __get_var @MULTI(STRING,STRING)
  .param string argv0
  .param string argv1

  .local string sigil_argv0
  sigil_argv0 = "$" . argv0

  .local int call_level
  $P1 = find_global "_Tcl", "call_level"
  call_level = $P1

  .local int type, return_type
  return_type = TCL_OK

  .local pmc value, retval, variable

get_indexed:
  push_eh catch
    if call_level == 0 goto get_indexed_global
    variable = find_lex call_level, sigil_argv0
    goto resume
get_indexed_global:
    variable = find_global "Tcl", sigil_argv0
resume:
  clear_eh

  isnull variable, no_such_variable
  $I0 = does variable, "hash"
  unless $I0 goto get_indexed_bad

get_indexed_ok:
  retval = variable[argv1]
  goto done

get_indexed_bad:
  return_type = TCL_ERROR
  $S0 =  "can't read \""
  $S0 .= argv0
  $S0 .= "("
  $S1 = argv1
  $S0 .= $S1
  $S0 .= ")\": variable isn't array"
  retval = new String
  retval = $S0
  goto done

no_such_variable:
  return_type = TCL_ERROR
  $S0 = "can't read \""
  $S0 .= argv0
  $S0 .= "\": no such variable"
  retval = new String
  retval = $S0
  # goto done            

done:
  .return(return_type,retval)

# simple error handler that silently catches the exception
catch:
  goto resume
.end
