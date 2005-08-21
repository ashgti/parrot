.include "runtime/parrot/library/dumper.imc"

.namespace [ "_Tcl" ]

=head1 _Tcl::__expression_parse

Given a string (or String), return a single stack that contains the work to be
done. This stack can then be passed to C<__expression_interpret> to actually
invoke the items on the stack.

Uses the standard two arg return - if C<TCL_ERROR> is returned, then 
we return our error information back up the chain. If we return C<TCL_OK>,
however, then we're returning the invokable PMC.

=cut

.const int MAX_PRECEDENCE =  9

.sub __expression_parse
  .param string expr
  
  .local pmc retval
  .local int return_type
  return_type = TCL_OK

  .local pmc undef
  undef = new Undef

  .local pmc chunk, chunks, program_stack
  chunks = new TclList
  program_stack = new TclList

  .local int pos
  pos = 0

operand:
  (chunk, pos) = get_operand(expr, pos)
  if_null chunk, no_operand
  push chunks, chunk
  goto operator

no_operand:
  $P0 = new Exception
  $P0["_message"] = "no operand!"
  throw $P0
 
operator:
  (chunk, pos) = get_operator(expr, pos)
  if_null chunk, chunks_done
  push chunks, chunk
  goto operand

  # if we don't match any of the possible cases so far, then we must
  # be a string operand, but for now, die. #XXX

chunks_done:
# convert the chunks into a stack.
 
  # to do this, we scan for our Operators in precedence order.
  # as we find each one, put it on the program_stack with the appropriate
  # args. Leave a "NOOP" placeholder when pulling things. If our target
  # arg is a noop, we can either put it on the stack and ignore it when
  # popping the stack, or not put it on the stack.

  # XXX cheat for now , assume no precedence. means we can just
  # walk through, grabbing ops. (hope nothing is orphaned?)
 
  .local int stack_index 
  .local int input_len

  stack_index = 0

 # we're looping over this once - to handle precedence, I suggest
 # looping multiple times, leaving the NOOPS when we remove something
 # to faciliate processing on further runs. If we try to pull a 
 # left or right arg and see a NO-OP, we know it's safe to skip because
 # walking the stack will convert it to a number by the time we get to it.

  .local pmc our_op 
  input_len  = chunks
  if input_len == 0 goto die_horribly

  # a single value, return now.
  if input_len != 1 goto pre_converter_loop
  # XXX (That's value, not an operator)
  .return(TCL_OK,chunks)

pre_converter_loop:
  .local int precedence_level
  precedence_level = -1 # start with functions
converter_loop:
  if precedence_level > MAX_PRECEDENCE goto converter_done
  if stack_index >= input_len goto precedence_done
  our_op = chunks[stack_index]
  if_null our_op, converter_next
  $I0 = typeof our_op
  if $I0 == .Undef goto converter_next
  $I2 = our_op[0]
  if $I2 == OPERAND goto converter_next
  if $I2 == CHUNK   goto converter_next
  if $I2 == OP   goto is_opfunc

  # Should never be reached (XXX then shouldn't we die horribly?)
  goto converter_next 

is_opfunc:
  #print "is_opfunc\n"
  $I3 = our_op[2] 
  if $I3 != precedence_level goto converter_next

right_arg:
  $I2 = stack_index + 1
  if $I2 >= input_len goto left_arg
  retval = chunks[$I2]
  if_null retval, left_arg
  chunks[$I2] = undef
  inc $I4
  program_stack = unshift retval
  
  # If we're a function, (XXX) assume a single arg (which
  # we've now pulled - so, go to the, skip the left arg.
  if precedence_level == -1 goto shift_op

  # XXX we just deal with binary args at the moment.
left_arg:
  $I2 = stack_index - 1
  if $I2 < 0 goto shift_op
  retval = chunks[$I2]
  if_null retval, shift_op
  chunks[$I2] = undef
  inc $I4
  program_stack = unshift retval

shift_op:
  program_stack = unshift our_op
  chunks[stack_index] = undef

converter_next:
  inc stack_index
  goto converter_loop

precedence_done:
  inc precedence_level
  stack_index = 0
  goto converter_loop

die_horribly:
  return_type = TCL_ERROR 
  program_stack = new String
  program_stack = "An error occurred in EXPR"
  goto converter_done

converter_done:
  .return(return_type,program_stack)

.end

.sub get_operand
  .param string expr
  .param int pos

  .local pmc chunk, retval
  .local int return_type

  .local int start, len
  start = pos
  len   = length expr

  dec pos
eat_space:
  inc pos
  if pos >= len goto fail
  
  $I0 = is_whitespace expr, pos
  if $I0 == 1 goto eat_space
  
  $I0 = is_digit expr, pos
  if $I0 == 1 goto number
  
  $I0 = ord expr, pos
  if $I0 == 91 goto subcommand  # [
  if $I0 == 40 goto subexpr     # (
  if $I0 == 36 goto variable    # $
  if $I0 == 46 goto number      # .
  
  $I0 = is_wordchar expr, pos
  if $I0 == 1 goto function
  
  # unary

fail:
  null chunk
  goto done

subexpr:
  .local int depth
  depth = 1
  start = pos + 1
paren_loop:
  inc pos
  if pos >= len goto premature_end
  $I0 = ord expr, pos
  if $I0 == 41 goto paren_right
  if $I0 == 40 goto paren_left
  if $I0 == 92 goto paren_backslash
  goto paren_loop
paren_right:
  dec depth
  if depth == 0 goto paren_done
  goto paren_loop
paren_left:
  inc depth
  goto paren_loop
paren_backslash:
  inc $I1
  goto paren_loop

paren_done:
  $I0 = pos - start
  inc pos
  $S1 = substr expr, start, $I0
  
  # XXX this is now officially braindead. Fissit.
  (return_type,retval) = __expression_parse($S1)
  if return_type == TCL_ERROR goto die_horribly
  (return_type,retval) = __expression_interpret(retval)
  if return_type == TCL_ERROR goto die_horribly

  chunk = new TclList
  chunk[0] = OPERAND
  chunk[1] = retval
  
  goto done
 
variable:
  (retval, pos) = get_variable(expr, pos)
  
  chunk = new TclList
  chunk[0] = OPERAND
  chunk[1] = retval
  
  goto done

subcommand:
  (retval, pos) = get_subcommand(expr, pos)

  chunk = new TclList
  chunk[0] = OPERAND
  chunk[1] = retval
  
  goto done

function:
  # Does the string of characters here match one of our pre-defined
  # functions? If so, put that function on the stack.
  .local pmc func
  .local int op_length
  (op_length,func) = __expr_get_function(expr,pos)
  if op_length == 0 goto get_operator
  chunk = new TclList
  chunk[0] = OPERAND
  chunk[1] = func
  
  pos += op_length
  goto done

number:
  (op_length,retval) = __expr_get_number(expr,pos)
  chunk = new TclList
  chunk[0] = OPERAND
  chunk[1] = retval
  
  pos += op_length
  # goto done

done:
  .return(chunk, pos)

premature_end:
  $P0 = new Exception
  $S0 = new String
  $S0 = "syntax error in expression \""
  $S0 .= expr
  $S0 .= "\": premature end of expression"
  $P0["_message"] = $S0
  throw $P0
.end

.sub get_operator
  .param string expr
  .param int pos
  
  .local pmc chunk
  null chunk
  
  .local pmc ops, precedences
  # Global list of available ops.
  ops = find_global "_Tcl", "operators"
  # Global list of operator precedence
  precedences = find_global "_Tcl", "precedence"
  
  .local int len
  len = length expr
  dec pos
eat_space:
  inc pos
  if pos >= len goto done
  $I0 = is_whitespace expr, pos
  if $I0 == 1 goto eat_space

  .local int op_len
  .local string test_op

  # cheat - right now there are only 2 and 1 character ops
  # 2 char trump one char.

  $I0 = pos + 1
  if $I0 == len goto one_char

two_char:
  op_len = 2
  test_op = substr expr, pos, op_len
  $P11 = ops[test_op]
  if_null $P11, one_char
  $I1 = typeof $P11
  if $I1 == .Undef goto one_char
  goto op_done

one_char:
  op_len = 1
  test_op = substr expr, pos, op_len
  $P11 = ops[test_op]
  if_null $P11, done
  $I1 = typeof $P11
  if $I1 == .Undef goto done

op_done:
  $I5 = precedences[test_op]
  $I6 = ops[test_op]

  chunk = new TclList
  chunk[0] = OP
  chunk[1] = $I6 # op lookup
  chunk[2] = $I5 # precedence

  pos += op_len

done:  
  .return(chunk, pos)
.end

.sub __expression_interpret
  .param pmc program_stack

  .local pmc result_stack
  result_stack = new TclList
  .local pmc retval
  .local int return_type
  
stack_evaluator:
 # while the prog stack exists:
 .local int size
 size = program_stack
 if size == 0 goto stack_done
 
 .local int type
 .local pmc chunk
 chunk = pop program_stack
 $I10 = typeof chunk
 if $I10 == .Undef goto stack_evaluator
 type = chunk[0]

 # move all non op non funcs to the value stack
 if type == OP goto do_op
 $P0 = chunk[1]
 $P0 = $P0."interpret"()
 chunk[1] = $P0
 push result_stack, chunk
 goto stack_evaluator

do_op:
  # right now, we assume binary ops. Later, each op will define the
  # number of and type of ops it takes, and we will respect it.

  .local int op
  op = chunk[1]

  # XXX assume all operands take two args.
  .local pmc r_arg
  .local pmc l_arg
  .local pmc op_result
  op_result = new TclInt
  l_arg = pop result_stack
  l_arg = l_arg[1]

  r_arg = pop result_stack
  r_arg = r_arg[1]

  # Is there a more efficient way to do this dispatch?
  if op == OPERATOR_MUL goto op_mul
  if op == OPERATOR_DIV goto op_div
  if op == OPERATOR_MOD goto op_mod
  if op == OPERATOR_PLUS goto op_plus
  if op == OPERATOR_MINUS goto op_minus
  if op == OPERATOR_SHL goto op_shl
  if op == OPERATOR_SHR goto op_shr
  if op == OPERATOR_LT goto op_lt
  if op == OPERATOR_GT goto op_gt
  if op == OPERATOR_LTE goto op_lte
  if op == OPERATOR_GTE goto op_gte
  if op == OPERATOR_EQUAL goto op_equal
  if op == OPERATOR_UNEQUAL goto op_unequal
  if op == OPERATOR_BITAND goto op_bitand
  if op == OPERATOR_BITXOR goto op_bitxor
  if op == OPERATOR_BITOR goto op_bitor
  if op == OPERATOR_NE goto op_ne
  if op == OPERATOR_EQ goto op_eq

  #error_S = "invalid function lookup returned"
  goto die_horribly

op_mul:
  op_result = mul l_arg, r_arg
  goto done_op 
op_div:
  op_result = div l_arg, r_arg
  goto done_op 
op_mod:
  op_result = mod l_arg, r_arg
  goto done_op 
op_plus:
  op_result = l_arg + r_arg
  goto done_op 
op_minus:
  op_result = l_arg - r_arg
  goto done_op 
op_shl:
  op_result = shl l_arg, r_arg
  goto done_op 
op_shr:
  op_result = shr l_arg, r_arg
  goto done_op 
op_lt:
  op_result = 1
  if l_arg < r_arg goto done_op
  op_result = 0
  goto done_op 
op_gt:
  op_result = 1
  if l_arg > r_arg goto done_op
  op_result = 0
  goto done_op 
op_lte:
  op_result = 1
  if l_arg <= r_arg goto done_op
  op_result = 0
  goto done_op 
op_gte:
  op_result = 1
  if l_arg >= r_arg goto done_op
  op_result = 0
  goto done_op 
op_equal:
  op_result = 1
  if l_arg == r_arg goto done_op
  op_result = 0
  goto done_op 
op_unequal:
  op_result = 1
  if l_arg != r_arg goto done_op
  op_result = 0
  goto done_op 
op_bitand:
  op_result = band l_arg, r_arg
  goto done_op 
op_bitxor:
  op_result = bxor l_arg, r_arg
  goto done_op 
op_bitor:
  op_result = bor l_arg, r_arg
  goto done_op
op_ne:
  op_result = 1
  $S0 = l_arg
  $S1 = r_arg
  if $S0 != $S1 goto done_op
  op_result = 0
  goto done_op
op_eq:
  op_result = 1
  $S0 = l_arg
  $S1 = r_arg
  if $S0 == $S1 goto done_op
  op_result = 0
  # goto done_op

done_op:
  $P5 = new FixedPMCArray
  $P5 = 2
  $P5[0] = OPERAND
  $P5[1] = op_result
  push result_stack, $P5

  # Ignoring exceptions for now.
  goto stack_evaluator

stack_done:
  $I0 = result_stack
  if $I0 == 0 goto die_horribly
  retval = pop result_stack
  goto evaluation_done

die_horribly:
  return_type = TCL_ERROR 
  retval = new String
  retval = "An error occurred in EXPR"
  goto evaluation_return

evaluation_done:
  return_type = TCL_OK 
  retval = retval[1]

evaluation_return:
  .return(return_type,retval)
.end

# given a string, starting at position, return the length
# of the number starting at that position. return '0' if 
# no number was found.

.sub __expr_get_number
  .param string expr
  .param int pos

  .local int len
  len = length expr
  .local int char, start
  .local pmc value
  null value

  start = pos
  if pos >= len goto failure

integer:
  if pos >= len goto integer_done
  char = ord expr, pos
  if char > 57 goto integer_done # > "9"
  if char < 48 goto integer_done # < "0"
  inc pos
  goto integer 
integer_done:
  if char == 46 goto floating
  pos -= start
  if pos == 0 goto done # failure
  
  $S0 = substr expr, start, pos
  $I0 = $S0
  value = new TclInt
  value = $I0
  goto done

floating:
  inc pos
float_loop:
  if pos >= len goto float_done
  char = ord expr, pos
  if char > 57 goto float_done # > "9"
  if char < 48 goto float_done # < "0"
  inc pos
  goto float_loop
float_done:
  pos -= start
  
  $S0 = substr expr, start, pos
  $N0 = $S0
  value = new TclFloat
  value = $N0
  # goto done

done:
  .return(pos,value)
.end

.sub __expr_get_function
  .param string expr
  .param int start

  .local int len 
  .local pmc func,operand

  .local int start_paren_pos
  .local int expr_length
  expr_length = length expr

  # if we are starting with the text of a defined function,
  # and it's followed by a (), 
 
  index start_paren_pos, expr, "(", start
  if start_paren_pos == -1 goto fail

  .local int depth
  depth = 1
  $I0 = start_paren_pos
loop:
  inc $I0
  if $I0 >= expr_length goto fail
  $I1 = ord expr, $I0
  if $I1 == 40 goto left
  if $I1 == 41 goto right
  if $I1 == 92 goto backslash
  goto loop
left:
  inc depth
  goto loop
right:
  dec depth
  if depth == 0 goto loop_done
  goto loop
backslash:
  inc $I0
  goto loop

loop_done:
  $I1 = $I0 - start_paren_pos
  dec $I1
 
  # so, we know that the function name must be before the first (
  .local int len
  len = start_paren_pos - start

  $S0 = substr expr, start, len
  $P1 = find_global "_Tcl", "functions"
  
  func = $P1[$S0]
  if_null func, fail
  
  $I0 = find_type "TclFunc"
  func = new $I0
  $P0 = new String
  $P0 = $S0
  setattribute func, "TclFunc\x00name", $P0

  # and the operand is what's between the ()'s - get the result
  # of /that/ expression and return it.
  # XXX - If there are commas in the op, then split the operand
  #   and parse each one as an operand. needed for:
  #   atan2,pow,fmod,hypot

  inc start_paren_pos
  .local int len_operand
  len_operand = $I1

  $S1 = substr expr, start_paren_pos, len_operand
  ($I9,operand) = __expression_parse($S1) 
  if $I9 == TCL_ERROR goto fail
  
  setattribute func, "TclFunc\x00argument", operand

  len = start_paren_pos + len_operand
  inc len
  len -= start
  goto done

fail:
  len = 0

done:
  .return(len,func)
.end

=head2 _Tcl::__get_call_level

Given a pmc containing the tcl-style call level, return an int-like pmc
indicating the parrot-style level, and an integer with a boolean 0/1 -
was this a valid tcl-style level, or did we get this value as a default?

=cut

.sub __get_call_level
  .param pmc tcl_level
  .local pmc parrot_level, defaulted, orig_level
  defaulted = new Integer
  defaulted = 0

  .local pmc current_call_level
  current_call_level = find_global "_Tcl", "call_level"
  orig_level = current_call_level
 
  .local int num_length

get_absolute:
  # Is this an absolute? 
  $S0 = tcl_level
  $S1 = substr $S0, 0, 1
  if $S1 != "#" goto get_integer
  $S0 = tcl_level
  (num_length,parrot_level) = __expr_get_number($S0,1)
  $I0 = isa parrot_level, "Integer"
  if $I0 == 0 goto default
  $S0 = tcl_level
  $I0 = length $S0

  dec $I0
  if $I0 != num_length goto default
  goto bounds_check
 
get_integer:
  # Is this an integer? 
  $S0 = tcl_level
  (num_length,parrot_level) = __expr_get_number($S0,0)
  $I0 = isa parrot_level, "Integer"
  if $I0 == 0 goto default
  $S0 = tcl_level
  $I0 = length $S0
  if $I0 != num_length goto default
  parrot_level = orig_level - parrot_level
  goto bounds_check
 
default:
  defaulted = 1
  parrot_level = orig_level - 1
  # fallthrough.

bounds_check:
  # Are we < 0 ? 
  if parrot_level >= 0 goto bounds_pos
  parrot_level = 0
  goto done

bounds_pos:
  # Are we > the current max level?
  if parrot_level <= orig_level goto done
  parrot_level = orig_level

done: 
  $I1 = defaulted 
  .return(parrot_level,$I1)
.end 
