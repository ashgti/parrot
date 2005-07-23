=head2 [namespace]

Parrot has its own namespaces which Tcl is only a part of. So, in the top
level parrot namespace, C<Tcl> refers to the top of the Tcl namespace.

To refer back to something in another parrot namespace, use the special
C<parrot> namespace inside Tcl - this should be an alias back to parrot's
real top level namespace.

=cut

.namespace [ "Tcl" ]

.sub "namespace"
   .local pmc argv 
  argv = foldup

  unless argv goto failure
  .local pmc subcommand
  subcommand = shift argv

  if subcommand == "children"   goto children
  if subcommand == "code"       goto code 
  if subcommand == "current"    goto current
  if subcommand == "delete"     goto delete
  if subcommand == "ensemble"   goto ensemble
  if subcommand == "eval"       goto eval
  if subcommand == "exists"     goto exists
  if subcommand == "export"     goto export
  if subcommand == "forget"     goto forget
  if subcommand == "import"     goto import
  if subcommand == "inscope"    goto inscope
  if subcommand == "origin"     goto origin
  if subcommand == "parent"     goto parent
  if subcommand == "qualifiers" goto qualifiers
  if subcommand == "tail"       goto tail
  if subcommand == "which"      goto which

children:
code:
  goto failure

current:
  $P1 = new TclString
  $P1 = "::"
  .return(TCL_OK,$P1)

delete:
  if argv goto failure #XXX todo
 
  # No arg delete does nothing.
  $P1 = new String
  $P1 = ""
  .return(TCL_OK,$P1)


ensemble:
eval:
  goto failure

exists:
  # canonicalize namespace.
  $P1 = new TclInt
  $P1 = 0
  .return(TCL_OK,$P1)

export:
forget:
import:
inscope:
origin:
parent:
qualifiers:
tail:
which:
failure:
  # XXX lame error handling
  .return(TCL_ERROR,"bad call to namespace")
.end

.namespace [ "_Tcl" ]

.sub "_canon_namespace"
 ## this sub should convert the variable name to its canonical repr.
.end
