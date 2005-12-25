.namespace [ "_Tcl::builtins" ]

.sub "for"
  .param int register_num
  .param pmc argv

  .local int argc
  argc = argv
  if argc != 4 goto badargs

  .local string pir_code,loop_label,temp_code
  .local string ex_label, step_label, done_label
  .local int cond_num, body_num
  $S0 = register_num
  loop_label = "loop"  . $S0
  ex_label   = "excep" . $S0
  step_label = "step"  . $S0
  done_label = "done"  . $S0

  .local pmc start,cond,step,body
  .local pmc compiler,compile_dispatch,expr_compiler

  compiler         = find_global "_Tcl", "compile"
  compile_dispatch = find_global "_Tcl", "compile_dispatch"
  expr_compiler    = find_global "_Tcl", "__expression_compile"

  start = argv[0]
  cond  = argv[1]
  step  = argv[2]
  body  = argv[3]

  (register_num,temp_code) = compiler(register_num,start)
  pir_code .= "#PRE LOOP\n"
  pir_code .= temp_code
  inc register_num
  (register_num,temp_code) = compile_dispatch(register_num, body)
  pir_code .= temp_code
  pir_code .= ".local pmc compiler, pir_compiler, pir\n"
  pir_code .= "compiler     = find_global '_Tcl', 'compile'\n"
  pir_code .= "pir_compiler = find_global '_Tcl', 'pir_compiler'\n"
  pir_code .= "($I0, pir) = compiler(0, $P"
  $S0 = register_num
  pir_code .= $S0
  pir_code .= ")\n"
  inc register_num
  pir_code .= "$P"
  $S0 = register_num
  pir_code .= $S0
  pir_code .= " = pir_compiler($I0, pir)\n"
  body_num = register_num
  inc register_num
  
  pir_code .= loop_label
  pir_code .= ":\n"
  pir_code .= "#BODY LOOP\n"

  pir_code .= "push_eh "
  pir_code .= ex_label
  pir_code .= "\n"

  pir_code .= "$P"
  $S0 = body_num
  pir_code .= $S0
  pir_code .= "()\n"

  pir_code .= "clear_eh\n"

  inc register_num
  (register_num,temp_code) = compiler(register_num, step)

  pir_code .= step_label
  pir_code .= ":\n"
  
  pir_code .= "#STEP LOOP\n"
  pir_code .= temp_code
  inc register_num
  (cond_num,    temp_code) = expr_compiler(register_num, cond)
  pir_code .= "#COND LOOP\n"
  register_num = cond_num + 1
  pir_code .= temp_code
  
  $S0=<<"END_PIR"
if $P%i goto %s
$P%i=new .TclString
$P%i=\"\"
END_PIR

  $P1 = new .Array
  $P1 = 4
  $P1[0] = cond_num
  $P1[1] = loop_label
  $P1[2] = register_num
  $P1[3] = register_num

  temp_code = sprintf $S0, $P1
  pir_code .= temp_code

  pir_code .= "goto "
  pir_code .= done_label
  pir_code .= "\n"

  inc register_num
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
  pir_code .= step_label
  pir_code .= "\n"

  pir_code .= "if "
  pir_code .= $S0
  pir_code .= " == TCL_BREAK goto "
  pir_code .= done_label
  pir_code .= "\n"

  pir_code .= ".rethrow()\n"

  pir_code .= done_label
  pir_code .= ":\n"

  .return(register_num,pir_code)

badargs:
  pir_code =<<"END_PIR"
.throw(\"wrong # args: should be \\\"for start test next command\\\"\")
END_PIR

  .return(register_num,pir_code)
.end
