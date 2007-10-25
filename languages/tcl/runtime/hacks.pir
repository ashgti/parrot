#
# Hacks - these functions exist only to speed up repeated [source]ing of
#         a .tcl file;
# XXX This is currently uninvoked, pending resolution of a GC bug.

.HLL '_Tcl', ''

# given a sub and a filename, freeze the sub and write it out to the file

.sub 'dump_bytecode'
  .param pmc    invokable
  .param string filename

  .local string frozen_code
  .local pmc filehandle
  frozen_code = freeze invokable
  filehandle = open filename, '>'
  print filehandle, frozen_code
  close filehandle
.end


# given a filename, load it, thaw it

.sub 'read_bytecode'
  .param string filename

  .local pmc invokable
  .local string frozen_code

  $P99 = open filename, '<'
  frozen_code = $P99.'slurp' ('')

  invokable = thaw frozen_code
  .return( invokable )
.end

# given a filename, see if a recent enough precompiled version exists.
# in either case, return the precompiled version.

.sub 'check_precompiled'
  .param string filename

  .local string precompiled_filename, file_contents
  .local pmc OS, stat_tcl, stat_prec, code

  .local pmc __script, ns, interp
  interp = getinterp
  ns = interp['namespace';1]

  precompiled_filename = filename . '.pre_c'
  # Does either file exist?
  OS = new 'OS'
  push_eh bad_file
    stat_tcl = OS.'stat'(filename)
  pop_eh

  OS = new 'OS'
  push_eh no_precompiled
    stat_prec = OS.'stat'(precompiled_filename)
  pop_eh

  # Is the precompiled version new enough?
  $I0 = stat_tcl [9] #mtime
  $I1 = stat_prec[9] #mtime
  if $I1 > $I0 goto precompiled_ok

no_precompiled:
  # now read it in...
  $P99 = open filename, '<' 
  file_contents = $P99.'slurp'( '' )

  # and compile it..
  __script = get_root_global ['_tcl'], '__script'
  code = __script ( file_contents, 'ns' => ns, 'bsnl' => 1)

  # and save it out to the precompiled filename.
  dump_bytecode(code,precompiled_filename)
  .return( code )

precompiled_ok:
  .return read_bytecode(precompiled_filename)

bad_file: # not even the original exists..
  tcl_error 'bad file, no donut'
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
