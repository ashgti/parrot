###
# [for]

.namespace [ "Tcl" ]

.sub "&for"
  .param pmc argv :slurpy

  .local int argc
  argc = argv
  if argc != 4 goto bad_args

  .local string start_code, test_code, next_code, body_code

  start_code = argv[0]
  test_code  = argv[1]
  next_code  = argv[2]
  body_code  = argv[3]

  .local pmc    start_parsed, next_parsed, body_parsed, test_parsed
  .local pmc retval

  .local pmc parse
  .local pmc expression_p, expression_i
  parse = find_global "_Tcl", "parse"
  expression_p = find_global "_Tcl", "__expression_parse"
  expression_i = find_global "_Tcl", "__expression_interpret"

  # Parse the bits that are code.
  start_parsed = parse(start_code)
  next_parsed  = parse(next_code)
  body_parsed  = parse(body_code)

  # first, execute start.
  start_parsed."interpret"()

for_loop:
  # then execute body
  push_eh check_continue
    body_parsed."interpret"()
  clear_eh

continue:
  # then execute next
  next_parsed."interpret"()
  # then check condition
  test_parsed = expression_p(test_code)
  retval = expression_i(test_parsed)
  if retval goto for_loop

  .return ("") 

check_continue:
  .local int return_type
  .get_return_code(P5,return_type)
  if return_type == TCL_CONTINUE goto continue
  if return_type == TCL_BREAK goto done
  .rethrow(P5)

bad_args:
  .throw("wrong # args: should be \"for start test next command\"")

done:
  .return ("")

done_cleansed:
  .return(retval)
.end
