.include "languages/tcl/src/returncodes.pir"
.include "languages/tcl/src/macros.pir"

.HLL 'parrot', ''
.namespace [ 'TclFunc' ]

.cloneable()

# functions
 .const int FUNCTION_ABS    = 53
 .const int FUNCTION_ACOS   = 54
 .const int FUNCTION_ASIN   = 55
 .const int FUNCTION_ATAN   = 56
 .const int FUNCTION_COS    = 57
 .const int FUNCTION_COSH   = 58
 .const int FUNCTION_EXP    = 59
 .const int FUNCTION_LOG    = 60
 .const int FUNCTION_LOG10  = 61
 .const int FUNCTION_ROUND  = 62
 .const int FUNCTION_SIN    = 63
 .const int FUNCTION_SINH   = 64
 .const int FUNCTION_SQRT   = 65
 .const int FUNCTION_TAN    = 66
 .const int FUNCTION_TANH   = 67
 .const int FUNCTION_RAND   = 68

=head2 __class_init

Define the attributes required for the class.

=cut

.sub __class_init :load
  $P0 = newclass "TclFunc"
  addattribute $P0, "argument"
  addattribute $P0, "name"
.end

=head2 __init

Initialize the attributes for an instance of the class

=cut

.sub __init :method
  $P0 = new 'TclString'
  setattribute self, "TclFunc\x00name", $P0
  $P0 = new 'TclString'
  setattribute self, "TclFunc\x00argument", $P0
.end

.sub compile :method
  .param int register_num

  .local pmc retval
  .local string pir_code
  .local pmc funcs,  __number, compile
  .get_from_HLL(funcs,'_tcl','functions')
  .get_from_HLL(__number,'_tcl','__number')
  .get_from_HLL(compile,'_tcl','compile_dispatch')

  # eventually, we'll need to deal with more than one arg.

  .local pmc arg_code, arg_reg, name
  $P1  = getattribute self, "TclFunc\x00argument"
  (arg_reg,pir_code)  = compile(register_num, $P1)

  inc register_num

  # XXX We shouldn't store the name. we should store the opcode, avoid
  # the lookup cost. (at least for builtins)

  name = getattribute self, "TclFunc\x00name"
  $I0 = funcs[name]

  .local string opcode_name
 
  if $I0 == FUNCTION_ABS goto func_abs
  if $I0 == FUNCTION_ACOS goto func_acos
  if $I0 == FUNCTION_ASIN goto func_asin
  if $I0 == FUNCTION_ATAN goto func_atan
  if $I0 == FUNCTION_COS goto func_cos
  if $I0 == FUNCTION_COSH goto func_cosh
  if $I0 == FUNCTION_EXP goto func_exp
  if $I0 == FUNCTION_LOG goto func_log
  if $I0 == FUNCTION_LOG10 goto func_log10
  if $I0 == FUNCTION_SIN goto func_sin
  if $I0 == FUNCTION_SINH goto func_sinh
  if $I0 == FUNCTION_SQRT goto func_sqrt
  if $I0 == FUNCTION_TAN goto func_tan
  if $I0 == FUNCTION_TANH goto func_tanh
  
func_abs:
  .sprintf2($S0,"$P%i = abs $P%i\n",register_num,arg_reg)

  pir_code .= $S0
  goto done_all

func_acos:
  opcode_name = "acos"
  goto done

func_asin:
  opcode_name = "asin"
  goto done

func_atan:
  opcode_name = "atan"
  goto done

func_cos:
  opcode_name = "cos"
  goto done

func_cosh:
  opcode_name = "cosh"
  goto done

func_exp:
  opcode_name = "exp"
  goto done

func_log:
  opcode_name = "ln"
  goto done

func_log10:
  opcode_name = "log10"
  goto done

func_sin:
  opcode_name = "sin"
  goto done

func_sinh:
  opcode_name = "sinh"
  goto done

func_sqrt:
  opcode_name = "sqrt"
  goto done

func_tan:
  opcode_name = "tan"
  goto done

func_tanh:
  opcode_name = "tanh"


done:
  .local string format
  format = <<"END_PIR"
# src/class/tclfunc.pir :: compile
$N%i=$P%i
$N%i=%s $N%i
$P%i=new .TclFloat
$P%i=$N%i
END_PIR

  .sprintf8($S0,format, register_num, arg_reg, register_num, opcode_name, register_num, register_num, register_num, register_num)

  pir_code .= $S0

done_all:

  .return(register_num,pir_code)

.end
