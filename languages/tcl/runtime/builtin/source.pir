###
# [source]

.HLL 'Tcl', 'tcl_group'
.namespace

.sub '&source'
  .param pmc argv :slurpy
  
  .local int argc 
  argc = argv
  if argc != 1 goto fail

  .local string chunk, filename, contents
  .local int type
  .local pmc retval, handle, compiler, pir_compiler

  compiler     = get_root_global ['_tcl'], 'compile'
  pir_compiler = get_root_global ['_tcl'], 'pir_compiler'

  $P1 = argv[0] 
  typeof type, $P1
  if type != .ParrotIO goto file
  handle = $P1
  goto loop

file:
  filename = $P1
  handle = open filename, '<'
  $I0 = typeof handle
  if $I0 == .Undef goto badfile
  contents = ''
 
loop:
  read chunk, handle, 1024
  if chunk == '' goto gotfile
  contents = contents . chunk
  goto loop

gotfile:
  ($I0,$P1) = compiler(0,contents)
  $P2       = pir_compiler($I0,$P1)
  .return $P2()

badfile:
  $S0 = "couldn't read file \""
  $S0 .= filename
  $S0 .= '": no such file or directory'
  .throw($S0)

fail:
  .throw('wrong # args: should be "source fileName"')

.end
