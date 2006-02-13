# This is just a stub to allow Punie to execute code. It will be
# replaced.

ROOT: result(.) = {
    .local string output
    output = ".sub _main :main\n"
    $P1 = node.children()
    .local pmc iter
    iter = new Iterator, $P1    # setup iterator for node
    iter = 0
  iter_loop:
    unless iter, iter_end         # while (entries) ...
      shift $P2, iter
      $S3 = tree.get('result', $P2)
      output .= $S3
      goto iter_loop
  iter_end:
    output .= ".end\n"
    .return (output)
}

# Flatten out the ops into a simple sequence.
POST::Ops: result(.) = {
    .local string output
    $P1 = node.children()
    .local pmc iter
    iter = new Iterator, $P1    # setup iterator for node
    iter = 0
  iter_loop:
    unless iter, iter_end         # while (entries) ...
      shift $P2, iter
      $S3 = tree.get('result', $P2)
      output .= $S3
      goto iter_loop
  iter_end:
    .return (output)
}

POST::Op: result(.) = {
    .local string output
    .local int counter
    counter = 0
    .local string opname
    opname = node.op()
    $I1 = opname == 'print'
    if $I1 goto no_lead
      output = "    " . opname
      output .= " "
  no_lead:
    $P1 = node.children()
    .local pmc iter
    iter = new Iterator, $P1    # setup iterator for node
    iter = 0
  iter_loop:
    unless iter goto iter_end         # while (entries) ...
      shift $P2, iter
      inc counter
      $S3 = tree.get('result', $P2)
      if $I1 goto repeat_opname
      if counter <= 1 goto no_comma_out
      output .= ", "
    no_comma_out:
      output .= $S3
      goto iter_loop
    repeat_opname:
      output .= "    "
      output .= opname
      output .= " "
      output .= $S3
      output .= "\n"
      goto iter_loop
  iter_end:
    output .= "\n"
    .return (output)
}

POST::Val: result(.) = {
    $S1 = node.value()
    $S2 = node.valtype()
    if $S2 == 'strq' goto wrap_string_single
    if $S2 == 'strqq' goto wrap_string_double
    # Otherwise, it's not a string value, so return it straight.
    .return ($S1)
  wrap_string_double:
    # Wrap double quoted strings in double quotes
    $S3 = '"' . $S1
    $S3 .= '"'
    .return ($S3)
  wrap_string_single:
    # Wrap single quoted strings in single quotes
    $S3 = "'" . $S1
    $S3 .= "'"
    .return ($S3)
}
