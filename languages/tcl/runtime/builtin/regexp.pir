.HLL 'Tcl', 'tcl_group'
.namespace

.sub '&regexp'
  .param pmc argv :slurpy
 
  .local int argc
  argc = argv 
  if argc < 2 goto badargs

  .local pmc exp, a_string
  
   exp      = shift argv
   a_string = shift argv

   .local pmc tclARE, rule, match

   # XXX (#40774):            "tclARE"
   tclARE = compreg 'PGE::P5Regex'

   rule = tclARE(exp)
   match = rule(a_string)

   .return match.__get_bool()

badargs:
  tcl_error 'wrong # args: should be "regexp ?switches? exp string ?matchVar? ?subMatchVar subMatchVar ...?"'

.end
