.HLL '_Tcl', ''
.namespace [ 'builtins' ]

.sub 'expr'
  .param string retval
  .param pmc    raw_args
  .param pmc    argv

  .local pmc pir
  .local int argc
  argc = elements argv

  if argc == 0 goto bad_args

  $P0 = raw_args[0]
  $I0 = isa $P0, 'TclConst'
  unless $I0 goto not_const
  .local string arg
  $I0 = 1
loop:
  if $I0 == argc goto end
  $P0 = argv[$I0]
  $I1 = isa $P0, 'TclConst'
  unless $I1 goto not_const
  inc $I0
  goto loop
end:
  arg = join ' ', raw_args

  .local pmc __expr
  __expr = get_root_global ['_tcl'], '__expr'

  # make sure errors happen at runtime
  push_eh exception
    ($P0, $S0) = __expr(arg, 'pir_only'=>1)
  clear_eh
  pir = new 'PGE::CodeString'
  pir .= $P0

  pir.emit('  %0 = %1', retval, $S0)

  .return(pir)

not_const:
  null $P0
  .return($P0)

bad_args:
  .return("tcl_error 'wrong # args: should be \"expr arg ?arg ...?\"'\n")

exception:
  .catch()
  .get_message($S0)
  $P0 = new .String
  $P0 = $S0
  $P0.replace('\', '\\')
  $P0.replace('"', '\"')
  $S0 = $P0
  .local string error
  error = 'tcl_error "'
  error .= $S0
  error .= "\"\n"
  .return(error)
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
