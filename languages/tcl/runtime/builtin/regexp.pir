.HLL 'Tcl', 'tcl_group'
.namespace

.sub '&regexp'
  .param pmc argv :slurpy
 
  .local int argc
  argc = argv 
  if argc < 2 goto badargs

  .local pmc options
  options = new .ResizablePMCArray
  push options, 'all'
  push options, 'about'
  push options, 'indices'
  push options, 'inline'
  push options, 'expanded' # RT#40774: use tcl-regexps
  push options, 'line'
  push options, 'linestop'
  push options, 'lineanchor'
  push options, 'nocase'
  push options, 'start'

  .local pmc select_switches, switches
  select_switches  = get_root_global ['_tcl'], 'select_switches'
  switches = select_switches(options, argv, 1, 1)

  .local pmc exp, a_string
  
   exp      = shift argv
   a_string = shift argv

   .local pmc tclARE, rule, match

   # RT#40774: use tcl-regexps
   tclARE = compreg 'PGE::P5Regex'
   rule = tclARE(exp)
   match = rule(a_string)

   # matchVar
   argc = elements argv
   unless argc goto done
   .local string matchStr, matchVar
   .local pmc __set
   __set = get_root_global [ '_tcl' ], '__set'

   matchVar = shift argv

   .local pmc matches

   $I0 = exists switches['indices']
   if $I0 goto matches_ind
   
   matchStr = match 
   __set(matchVar, matchStr)

   matches = match.'get_array'()
   .local string subMatchStr, subMatchVar

subMatches:
   argc = elements argv
   unless argc goto done

   subMatchVar = shift argv
   subMatchStr = ''
   if_null matches, set_it
   $I0 = elements matches
   unless $I0 goto set_it
   $P0 = shift matches
   if_null $P0, set_it
   subMatchStr = $P0
 
set_it:
   __set(subMatchVar,subMatchStr) 

next_submatch:
  goto subMatches  

matches_ind:
  .local pmc matchList
  matchList = new .TclList
  matchList[0] = -1
  matchList[1] = -1
  $I0 = match.'from'()
  $I1 = match.'to'()
  dec $I1
  matchList[0] = $I0 
  matchList[1] = $I1 
  __set(matchVar, matchList) 

  matches = match.'get_array'()

subMatches_ind:
  .local pmc subMatchList 
subMatches_ind_loop:
   argc = elements argv
   unless argc goto done

   subMatchVar = shift argv
   subMatchList = new .TclList
   subMatchList[0] = -1
   subMatchList[1] = -1
   if_null matches, set_it_ind
   $I0 = elements matches
   unless $I0 goto set_it_ind
   $P0 = shift matches
   if_null $P0, set_it_ind
   $I0 = $P0.'from'()
   $I1 = $P0.'to'()
   dec $I1
   subMatchList[0] = $I0
   subMatchList[1] = $I1
 
set_it_ind:
   __set(subMatchVar,subMatchList) 

next_submatch_ind:
  goto subMatches_ind_loop

done:
   .return match.__get_bool()

badargs:
  tcl_error 'wrong # args: should be "regexp ?switches? exp string ?matchVar? ?subMatchVar subMatchVar ...?"'

.end
