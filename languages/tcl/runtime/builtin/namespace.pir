=head2 [namespace]

Parrot has its own namespaces which Tcl is only a part of. So, in the top
level parrot namespace, C<Tcl> refers to the top of the Tcl namespace.

To refer back to something in another parrot namespace, use the special
C<parrot> namespace inside Tcl - this should be an alias back to parrot's
real top level namespace.

=cut

.HLL 'Tcl', 'tcl_group'
.namespace

.sub '&namespace'
   .param pmc argv :slurpy

  .local pmc retval

  $I3 = argv
  unless $I3 goto no_args

  .local string subcommand_name
  subcommand_name = shift argv
  .local pmc subcommand_proc
  null subcommand_proc

  push_eh bad_args
    subcommand_proc = get_root_global ['_tcl';'helpers';'namespace'], subcommand_name
  clear_eh

  .return subcommand_proc(argv)

bad_args:
  $S0 = 'bad option "'
  $S0 .= subcommand_name
  $S0 .= '": must be children, code, current, delete, eval, exists, export, forget, import, inscope, origin, parent, qualifiers, tail, or which'
  .throw ($S0)

no_args:
  .throw ('wrong # args: should be "namespace subcommand ?arg ...?"')

.end

.HLL '_Tcl', ''
.namespace [ 'helpers'; 'namespace' ]

.sub 'current'
  .param pmc argv

  .local int argc
  argc = argv
  if argc goto bad_args

  .local pmc ns, __namespace
  __namespace = get_root_global ['_tcl'], '__namespace'
  ns  = __namespace('')
  $P0 = pop ns
  $S0 = join "::", ns
  $S0 = "::" . $S0
  .return($S0)

bad_args:
  .throw ('wrong # args: should be "namespace current"')
.end

.sub 'delete'
  .param pmc argv

  .local int argc
  argc = elements argv
  # no arg delete does nothing
  if argc == 0 goto return
  
  .local pmc __namespace, ns_root
  __namespace = get_root_global ['_tcl'], '__namespace'
  ns_root = get_root_namespace ['tcl']
  
  $I0 = 0
delete_loop:
  if $I0 == argc goto return
  $S0 = argv[$I0]
  $P0 = __namespace($S0)
  $I1 = 0
  $I2 = elements $P0
  dec $I2
  $P1 = ns_root
loop:
  $S0 = $P0[$I1]
  if $I1 == $I2 goto end
  $P1 = $P1[$S0]
  inc $I1
  goto loop
end:
  delete $P1[$S0]
  inc $I0
  goto delete_loop

return:
  .return('')
.end

.sub "exists"
  .param pmc argv

  .local int argc
  argc = argv
  if argc != 1 goto bad_args

  .local pmc colons, split, name
  colons = get_root_global ['_tcl'], 'colons'
  split  = get_root_global ['parrot'; 'PGE::Util'], 'split'
  name = argv[0]

  $P0 = split(colons, name)
  $I0 = elements $P0
  if $I0 == 0 goto relative

  $S0 = $P0[0]
  if $S0 != "" goto relative
  $P1 = pop $P0
  goto get

relative:

get:
  .local pmc ns
  $I0 = 0
  $I1 = elements $P0
  ns = get_root_namespace ['tcl']
get_loop:
  if $I0 == $I1 goto get_end
  $P1 = $P0[$I0]
  ns  = ns[$P1]
  if null ns goto doesnt_exist
  inc $I0
  goto get_loop
get_end:
  .return(1)
  
doesnt_exist:
  .return(0)

bad_args:
  .throw('wrong # args: should be "namespace exists name"' )
.end

.sub 'qualifiers'
  .param pmc argv
  
  .local int argc
  argc = argv
  if argc != 1 goto bad_args

  .local pmc p6r,match
  p6r = compreg 'PGE::P6Regex'
  match = p6r("(.*)\\:\\:+<-[:]>*$$")

  $S0 = argv[0]
  $P0 = match($S0)
  unless $P0 goto WHOLE
  $P1 = $P0[0]
  $S1 = $P1
  .return ($S1)

WHOLE:
  $S0 = argv[0]
  .return($S0)

  bad_args:
  .throw ('wrong # args: should be "namespace qualifiers string"')

.end

.sub 'tail'
  .param pmc argv
  
  .local int argc
  argc = argv
  if argc != 1 goto bad_args

  .local pmc p6r,match
  p6r= compreg 'PGE::P6Regex'
  match = p6r("\\:\\:+(<-[:]>)$$")

  $S0 = argv[0]
  $P0 = match($S0)
  unless $P0 goto WHOLE

  $P2 = $P0[0]
  
  $S1 = $P2
  .return ($S1)

WHOLE:
  $P0 = argv[0]
  .return($P0)

bad_args:
  .throw ('wrong # args: should be "namespace tail string"')

.end

.sub 'eval'
  .param pmc argv
  
  .local int argc
  argc = elements argv
  if argc < 2 goto bad_args
  
  .local pmc ns, __namespace
  __namespace = get_root_global ['_tcl'], '__namespace'
  
  ns = shift argv
  ns = __namespace(ns, 1)

  .local string namespace
  namespace = ""
  $I0 = elements ns
  if $I0 == 0 goto global_ns
  
  namespace = join "'; '", ns
  namespace = "['" . namespace
  namespace .= "']"

global_ns:
  .local pmc compile, code
  compile = get_root_global ['_tcl'], 'compile'
  code = new 'PGE::CodeString'
  $S0 = join " ", argv
  ($I0, $S0) = compile(1, $S0)
  $I0 = code.unique()
  code.emit(<<'END_PIR', namespace, $S0, $I0)
.HLL 'tcl', 'tcl_group'
.namespace %0
# src/compiler.pir :: pir_compiler (2)
.pragma n_operators 1
.sub compiled_tcl_sub_%2 
  .include "languages/tcl/src/returncodes.pir"
  .local pmc epoch, colons, split
  epoch  = get_root_global ['_tcl'], 'epoch'
  colons = get_root_global ['_tcl'], 'colons'
  split  = get_root_global ['parrot'; 'PGE::Util'], 'split'
  %1
  .return()
.end
END_PIR
  
  .local pmc pir_compiler
  pir_compiler = compreg "PIR"
  $P0 = pir_compiler(code)
  $P0()
  
  .return()

bad_args:
  .throw('wrong # args: should be "namespace eval name arg ?arg...?"')  
.end

.sub 'export'
.end

.sub 'children'
  .param pmc argv

  .local int has_pattern
  has_pattern = 0

  .local int argc
  argc = elements argv
  if argc > 2  goto bad_args
  if argc != 2 goto iterate

  .local pmc glob, pattern
  glob        = compreg 'PGE::Glob'
  pattern     = argv[1]
  pattern     = glob(pattern)
  has_pattern = 1

iterate:
  .local pmc list
  list = new .TclList
  
  .local pmc __namespace, ns
  ns = get_root_global 'tcl'
  if argc == 0 goto namespace_end
  __namespace = get_root_global ['_tcl'], '__namespace'
  
  .local pmc ns_name
  $S0 = argv[0]
  $S0 .= ":: "
  ns_name  = __namespace($S0)
  $S1      = pop ns_name

  $I0 = 0
  $I1 = elements ns_name
namespace_loop:
  if $I0 == $I1 goto namespace_end
  $S0 = ns_name[$I0]
  ns  = ns[$S0]
  inc $I0
  goto namespace_loop
namespace_end:
  
  if null ns goto unknown_namespace
  .local pmc iter
  iter = new .Iterator, ns

loop:
  unless iter goto end
  $S0 = shift iter
  $P0 = ns[$S0]
  $I0 = isa $P0, "NameSpace"
  unless $I0 goto loop
  $P0 = $P0.'name'()
  $S0 = shift $P0 # get rid of 'tcl'
  $S0 = join "::", $P0
  $S0 = "::" . $S0
  $P0 = new .TclString
  $P0 = $S0
  unless has_pattern goto is_namespace
  $P1 = pattern($P0)
  unless $P1 goto loop
is_namespace:
  push list, $P0
  goto loop
end:

  $P0 = find_name 'lc_cmp'
  list.sort($P0)
  .return(list)

bad_args:
  .throw('wrong # args: should be "namespace children ?name? ?pattern?"')

unknown_namespace:
  $S0 = argv[0]
  $S0 = 'unknown namespace "' . $S0
  $S0 = $S0 . '" in namespace children command'
  .throw($S0)
.end

.sub 'lc_cmp'
    .param string a
    .param string b

    a = downcase a
    b = downcase b

    $I0 = cmp a, b
    .return($I0)
.end

.sub 'code'
.end

.sub 'export'
.end

.sub 'forget'
.end

.sub 'import'
.end

.sub 'inscope'
.end

.sub 'origin'
.end

.sub 'parent'
  .param pmc argv

  .local int argc
  argc = elements argv

  .local string name
  name = ""
  
  if argc > 1  goto bad_args
  if argc == 0 goto get_parent

  name = argv[0]
  
get_parent:
  .local pmc ns, __namespace
  __namespace = get_root_global ['_tcl'], '__namespace'
  ns  = __namespace(name)
  $S0 = pop ns
  if $S0 != "" goto no_pop
  # for when someone calls [namespace current] from ::
  push_eh current_in_root
    $S0 = pop ns
  clear_eh
no_pop:
  $S0 = join "::", ns
  $S0 = "::" . $S0
  .return($S0)

current_in_root:
  .return('')

bad_args:
  .throw('wrong # args: should be "namespace parent ?name?"')
.end

.sub 'which'
.end
