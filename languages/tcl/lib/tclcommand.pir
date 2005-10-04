.namespace [ "TclCommand" ]

.HLL "Tcl", "tcl_group"

.include "languages/tcl/lib/returncodes.pir"

=head2 __class_init

Define the attributes required for the class.

=cut

.sub __class_init :load
  $P0 = getclass "TclList"
  $P1 = subclass $P0, "TclCommand"
  addattribute $P1, "name"
.end

=head2 __init

Initialize the attributes for an instance of the class

=cut

.sub __init :method
  #$P0 = new TclString
  #setattribute self, "TclCommand\x00name", $P0
.end


.sub __clone :method
  .local pmc obj
  $I0 = typeof self
  obj = new $I0
  obj = self
  .return(obj)
.end


=head2 interpret

Execute the command.


.sub interpret :method
  .local pmc retval
 
  .local string name
  $P0 = getattribute self, "TclCommand\x00name"
  retval = $P0."interpret"()

  name = retval
  
  .local pmc args
  args = new TclList
  .local int elems, i
  elems = self
  i     = 0
  
  .local pmc cmd
  push_eh no_command
    $S0 = "&" . name
    cmd = find_global "Tcl", $S0
  clear_eh
  
  # we can't delete commands, so we store deleted commands
  # as null PMCs
  if_null cmd, no_command

execute:
  .local pmc word
loop:
  if i == elems goto loop_done
  
  word   = self[i]
  retval = word."interpret"()
  
  push args, retval
  inc i
  goto loop
  
loop_done:
  .return cmd(args :flat)

no_command:
  $P1 = find_global "Tcl", "$tcl_interactive"
  unless $P1 goto no_command_non_interactive

  # XXX Should probably make sure this wasn't redefined on us.
  cmd = find_global "Tcl", "&unknown"
  
  # Add the command into the unknown handler, and fix our bookkeeping
  unshift args, name
  goto execute

no_command_non_interactive:
  $S0 = "invalid command name \""
  $S0 .= name
  $S0 .= "\""
  .throw($S0)
.end

=cut

.sub compile :method
   .param int register_num
 
   inc register_num

   .local string label_num
   label_num = register_num
   .local int inline_result_num
   .local int inline_available
   inline_available=0

   .local pmc compile
   compile = find_global "_Tcl", "compile"

   .local string pir_code
   pir_code = ".include \"languages/tcl/lib/returncodes.pir\"\n"

   .local string retval
   # Generate partial code for each of our arguments
   .local int ii, num_args, result_reg
   num_args = self 
   ii = 0
   .local pmc compiled_args
   compiled_args = new .TclList
arg_loop:
   if ii == num_args goto arg_loop_done
   $P1 = self[ii]
   (result_reg,retval) = compile($P1,register_num)

   push compiled_args, result_reg
   register_num = result_reg + 1
   pir_code .= retval 
   inc ii 
   goto arg_loop
arg_loop_done:
   # Generate code that will invoke our name'd command.
   pir_code .= ".local pmc command\n"

   # XXX Need to trap a missing command
   # Need to actually compile our name, as it might not be constant.

   .local pmc name
   name = getattribute self, "TclCommand\x00name"

   $I0 = typeof name
   $I1 = find_type "TclConst"
   if $I0 != $I1 goto dynamic

   push_eh dynamic
     $S0 = name
     $P1 = find_global "_Tcl::builtins", $S0
   clear_eh
 
   (inline_result_num,retval) = $P1(register_num,self)
   
   register_num = inline_result_num + 1
   # XXX Need check here for global epoch.
   pir_code .= "inlined_command"
   pir_code .= label_num
   pir_code .= ":\n"
   pir_code .= retval
   pir_code .= "$P%i=$P" # placeholder for final result PMC.
   $S0 = inline_result_num
   pir_code .= $S0
   pir_code .= "\n"
   pir_code .= "goto done_command"
   pir_code .= label_num
   pir_code .= "\n"
   inline_available = 1
dynamic:
   pir_code .= "dynamic_command"
   pir_code .= label_num
   pir_code .= ":\n"
   .local int name_register
   (name_register,retval) = compile(name,register_num)
   register_num = name_register

   pir_code .= retval
   $S1 = "$P"
   $S0 = register_num
   $S1 .= $S0

   inc register_num
   $S0 = register_num
   $S2 = "$S"
   $S2 .= $S0

   # Get a string version of the name
   pir_code .= $S2
   pir_code .= "="
   pir_code .= $S1
   pir_code .= "\n"
   
   # Prepend a "&"
   pir_code .= $S2
   pir_code .= " = \"&\" . "
   pir_code .= $S2
   pir_code .= "\n"

   pir_code .= "push_eh bad_command"
   $S0 = register_num
   pir_code .= $S0
   pir_code .= "\n"
   pir_code .= "command = find_global \"Tcl\", "
   pir_code .= $S2
   pir_code .= "\nclear_eh\nif_null command, bad_command"
   pir_code .= $S0
   pir_code .= "\n$P"
   pir_code .= $S0
   pir_code .= " = command("

   ii = 0
elem_loop:
   if ii == num_args goto elem_loop_done   
   $S0 = compiled_args[ii]
   pir_code .= "$P"
   pir_code .= $S0
   inc ii 
   if ii == num_args goto elem_loop_done
   pir_code .= ","
   goto elem_loop 
elem_loop_done:
   pir_code .= ")\ngoto resume"
   $S0 = register_num
   pir_code .= $S0
   pir_code .="\n" 
   pir_code .= "bad_command"
   pir_code .= $S0
   pir_code .= ":\n$S"
   pir_code .= $S0
   pir_code .= "=$P"
   $S1 = name_register
   pir_code .= $S1
   pir_code .= "\n$S"
   pir_code .= $S0
   pir_code .= "=concat \"invalid command name \\\"\" ,"
   pir_code .= "$S"
   pir_code .= $S0
   pir_code .= "\n$S"
   pir_code .= $S0
   pir_code .= ".=\"\\\"\"\n.throw($S"
   pir_code .= $S0
   pir_code .= ")\n"  

   pir_code .= "resume"
   pir_code .= $S0
   pir_code .= ":\n"
   # return the code and the new register_num 
   pir_code .= "done_command"
   pir_code .= label_num
   pir_code .= ":\n"

   unless inline_available goto done

   .local pmc printf_args
   printf_args = new .Array
   printf_args = 1
   printf_args[0] = register_num

   pir_code = sprintf pir_code, printf_args

done:

  .return (register_num,pir_code)
.end
