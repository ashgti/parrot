.namespace [ "_Tcl::builtins" ]

.sub "while"
  .param int register_num
  .param pmc argv

  .local int argc
  argc = argv
  if argc != 2 goto badargs

  .local string pir_code,loop_label,done_label,ex_label,temp_code
  .local int cond_num
  $S0 = register_num
  loop_label = "loop" . $S0
  done_label = "done" . $S0
  ex_label = "excep" . $S0

  .local pmc cond,body,compiler,expr_compiler

  compiler= find_global "_Tcl", "compile"
  expr_compiler= find_global "_Tcl", "__expression_compile"

  cond  = argv[0]
  body  = argv[1]

  pir_code .= loop_label
  pir_code .= ":\n"

  (cond_num,    temp_code) = expr_compiler(register_num, cond)
  register_num = cond_num + 1
  pir_code .= temp_code

  $P1 = new .Array
  $P1 = 2
  $P1[0] = cond_num
  $P1[1] = done_label

  sprintf temp_code, "unless $P%i goto %s\n", $P1

  pir_code .= temp_code

  pir_code .= "push_eh "
  pir_code .= ex_label
  pir_code .= "\n"

  (register_num,temp_code) = compiler(register_num, body)
  pir_code .= temp_code
  inc register_num

  pir_code .= "clear_eh\n"
  pir_code .= "goto "
  pir_code .= loop_label
  pir_code .= "\n"
  
  $S0 = register_num
  $S0 = "$I" . $S0
  pir_code .= ex_label
  pir_code .= ":\n"

  pir_code .= ".catch()\n.get_return_code("
  pir_code .= $S0
  pir_code .= ")\n"

  pir_code .= "if "
  pir_code .= $S0
  pir_code .= " == TCL_CONTINUE goto "
  pir_code .= loop_label
  pir_code .= "\n"

  pir_code .= "if "
  pir_code .= $S0
  pir_code .= " == TCL_BREAK goto "
  pir_code .= done_label
  pir_code .= "\n"

  pir_code .= ".rethrow()\n"
				    

  $S0=<<"END_PIR"
%s:
$P%i=new .TclString
$P%i=\"\"
END_PIR

   $P1 = new .Array
   $P1 = 3
   $P1[0] = done_label
   $P1[1] = register_num
   $P1[2] = register_num

   temp_code = sprintf $S0, $P1
   pir_code .= temp_code

  .return(register_num,pir_code)

badargs:
  pir_code =<<"END_PIR"
.throw('wrong # args: should be \"while test command\"')
END_PIR

  .return(register_num,pir_code)
.end
