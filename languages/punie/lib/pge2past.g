ROOT: result(.) = {
    # Ask the child node for its result
    .local pmc child
    $I0 = defined node["PunieGrammar::lineseq"]
    unless $I0 goto err_no_tree
    $P0 = node["PunieGrammar::lineseq"]
    child = tree.get('result', $P0, 'PunieGrammar::lineseq')

    .return (child)

  err_no_tree:
    print "The top-level node doesn't contain an 'lineseq' match.\n"
    end
}

PunieGrammar::block: result(.) = {
    # Ask the child node for its result
    .local pmc child
    $I0 = defined node["PunieGrammar::lineseq"]
    unless $I0 goto err_no_tree
    $P0 = node["PunieGrammar::lineseq"]
    child = tree.get('result', $P0, 'PunieGrammar::lineseq')

    .return (child)

  err_no_tree:
    print "The block node doesn't contain an 'lineseq' match.\n"
    end
}

PunieGrammar::lineseq: result(.) = {
    .local pmc newchildren
    newchildren = new PerlArray
    # Ask the child node for its result
    .local pmc child
    $I0 = defined node["PunieGrammar::line"]
    unless $I0 goto err_no_tree
    $P0 = node["PunieGrammar::line"]

    .local pmc iter
    iter = new Iterator, $P0    # setup iterator for node
    iter = 0
  iter_loop:
    unless iter, iter_end         # while (entries) ...
      shift $P2, iter
      $P3 = tree.get('result', $P2, 'PunieGrammar::line')
      push newchildren, $P3
      goto iter_loop
  iter_end:

    # get the source string and position offset from start of source
    # code for this match node
    $S1 = node 
    $I1 = node.from()
    $P4 = new 'PAST::Stmts'
    $P4.set_node($S1,$I1,newchildren)
    .return ($P4)
  err_no_tree:
    print "The 'lineseq' node doesn't contain a 'line' match.\n"
    end
}

PunieGrammar::line: result(.) = {
    .local pmc newchildren
    newchildren = new PerlArray

    .local pmc iter
    iter = new Iterator, node    # setup iterator for node
    iter = 0
  iter_loop:
    unless iter, iter_end         # while (entries) ...
      shift $S1, iter           # get the key of the iterator
      $P2 = iter[$S1]
      $P3 = tree.get('result', $P2, $S1)
      if null $P3 goto iter_loop
      push newchildren, $P3
      goto iter_loop
  iter_end:

    # get the source string and position offset from start of source
    # code for this match node
    $S2 = node 
    $I1 = node.from()
    .local pmc result
    result = new 'PAST::Stmt'
    result.set_node($S2,$I1,newchildren)

    .return (result)
}

PunieGrammar::expr: result(.) = {
    .local pmc result
    .local pmc children
    children = new PerlArray
    result = new 'PAST::Exp'
    $P1 = node.get_hash()
    $P0 = new Iterator, $P1    # setup iterator for node
    set $P0, 0 # reset iterator, begin at start
  iter_loop:
    unless $P0, iter_end         # while (entries) ...
      shift $S2, $P0             # get key for next entry
      $P2 = $P0[$S2]      # get entry at current key
      $P3 = tree.get('result', $P2, $S2)
      push children, $P3
      goto iter_loop
  iter_end:

    # get the source string and position offset from start of source
    # code for this match node
    $S2 = node 
    $I3 = node.from()
    result.set_node($S2,$I3,children)
    .return (result)
}

PunieGrammar::gprint: result(.) = {
    .local pmc result
    .local pmc children
    children = new PerlArray
    result = new 'PAST::Op'
    $P1 = node.get_hash()
    $P0 = new Iterator, $P1    # setup iterator for node
    set $P0, 0 # reset iterator, begin at start
  iter_loop:
    unless $P0, iter_end         # while (entries) ...
      shift $S2, $P0             # get key for next entry
      $P2 = $P0[$S2]      # get entry at current key
      $P3 = tree.get('result', $P2, $S2)
      push children, $P3
      goto iter_loop
  iter_end:

    # get the source string and position offset from start of source
    # code for this match node
    $S2 = node 
    $S3 = node[0]
    $I3 = node.from()
    result.set_node($S2,$I3,$S3,children)
    .return (result)
}

PunieGrammar::cond: result(.) = {
    .local pmc result
    .local pmc children
    children = new PerlArray
    result = new 'PAST::Op'
    $P1 = node.get_hash()
    .local pmc iter
    iter = new Iterator, $P1    # setup iterator for node
    set iter, 0 # reset iterator, begin at start
  iter_loop:
    unless iter, iter_end         # while (entries) ...
      shift $S2, iter             # get key for next entry
      $P2 = iter[$S2]      # get entry at current key
      $P3 = tree.get('result', $P2, $S2)
      push children, $P3
      goto iter_loop
  iter_end:

    # get the source string and position offset from start of source
    # code for this match node
    $S2 = node 
    $S3 = node[0]
    $I3 = node.from()
    result.set_node($S2,$I3,$S3,children)
    .return (result)
}
PunieGrammar::label: result(.) = {
    .return ()
}

PunieGrammar::cexpr: result(.) = {
    .local pmc result
    $I0 = defined node["PunieGrammar::oexpr"]
    unless $I0 goto err_no_oexpr
    $P1 = node["PunieGrammar::oexpr"]
    .local pmc children
    children = new PerlArray
    $P0 = new Iterator, $P1    # setup iterator for node
    set $P0, 0 # reset iterator, begin at start
  iter_loop:
    unless $P0, iter_end         # while (entries) ...
      shift $P2, $P0             # get next entry
      $P3 = tree.get('result', $P2, 'PunieGrammar::oexpr')
      push children, $P3
      goto iter_loop
  iter_end:

    # If there's only one child node, it's a single element, not a list,
    # so just return the single element. Otherwise, we have a comma
    # separated list, so build a comma op node.
    $I0 = elements children
    unless $I0 > 1 goto no_comma
    result = new 'PAST::Op'
    # get the source string and position offset from start of source
    # code for this match node
    $S2 = node 
    $I3 = node.from()
    result.set_node($S2,$I3,'O_COMMA',children)
    .return (result)
  no_comma:
    result = shift children # there's only one result
    .return (result)
  err_no_oexpr:
    print "The cexpr node doesn't contain a 'oexpr' match.\n"
    end
}

PunieGrammar::oexpr: result(.) = {
    .local pmc newchildren
    newchildren = new PerlArray

    .local pmc iter
    $P1 = node.get_hash()
    iter = new Iterator, $P1    # setup iterator for node
    iter = 0
  iter_loop:
    unless iter, iter_end         # while (entries) ...
      shift $S1, iter           # get the key of the iterator
      $P2 = iter[$S1]
      $P3 = tree.get('result', $P2, $S1)
      if null $P3 goto iter_loop
      push newchildren, $P3
      goto iter_loop
  iter_end:

    # get the source string and position offset from start of source
    # code for this match node
    $S2 = node 
    $I1 = node.from()
    .local pmc result
    result = new 'PAST::Exp'
    result.set_node($S2,$I1,newchildren)

    .return (result)
}


PunieGrammar::number: result(.) = {
    .local pmc result
    result = new 'PAST::Val'

    # get the source string and position offset from start of source
    # code for this match node
    $S2 = node 
    $I3 = node.from()

    result.set_node($S2,$I3,$S2)
    result.valtype('num')
    .return (result)
}

PunieGrammar::integer: result(.) = {
    .local pmc result
    result = new 'PAST::Val'

    # get the source string and position offset from start of source
    # code for this match node
    $S2 = node 
    $I3 = node.from()

    result.set_node($S2,$I3,$S2)
    result.valtype('int')
    .return (result)
}

PunieGrammar::stringdouble: result(.) = {
    .local pmc result
    result = new 'PAST::Val'

    # get the source string and position offset from start of source
    # code for this match node
    $S2 = node 
    $I3 = node.from()

    .local string value
    # Check if this is a string match
    $I0 = defined node["PGE::Text::bracketed"]
    if $I0 goto bracketed_value
    value = node
    goto no_bracketed_value
  bracketed_value:
    $P1 = node["PGE::Text::bracketed"]
    $P2 = $P1[0]
    value = $P2
  no_bracketed_value:

    result.set_node($S2,$I3,value)
    result.valtype('strqq')
    .return (result)
}

PunieGrammar::stringsingle: result(.) = {
    .local pmc result
    result = new 'PAST::Val'

    # get the source string and position offset from start of source
    # code for this match node
    $S2 = node 
    $I3 = node.from()

    .local string value
    # Check if this is a string match
    $I0 = defined node["PGE::Text::bracketed"]
    if $I0 goto bracketed_value
    value = node
    goto no_bracketed_value
  bracketed_value:
    $P1 = node["PGE::Text::bracketed"]
    $P2 = $P1[0]
    value = $P2
  no_bracketed_value:

    result.set_node($S2,$I3,value)
    result.valtype('strq')
    .return (result)
}

# The following rules are for the results of the operator precedence
# parser. These operate very differently than the standard grammar
# rules, because they give the node type in a "type" hash key inside the
# node, instead of storing the node as the value of a hash key that
# is their type.

expr: result(.) = {
    .local string type
    type = node["type"]
    unless node goto error_no_node
    if type == 'term:' goto transform_term
      # else
      $P1 = tree.get('op', node, 'expr')
      .return($P1)
    transform_term:
      $P1 = tree.get('term', node, 'expr')
      .return($P1)
    error_no_node:
      print "error: no node\n"
      end
}

expr: op(.) = {
    .local string type
    type = node["type"]
    .local pmc newchildren
    newchildren = new PerlArray
    $P1 = node.get_array()
    .local pmc iter
    iter = new Iterator, $P1    # setup iterator for node
    set iter, 0 # reset iterator, begin at start
  iter_loop:
    unless iter, iter_end         # while (entries) ...
      shift $P2, iter            # get entry
      $P3 = tree.get('result', $P2, 'expr')
      if null $P3 goto iter_loop
      push newchildren, $P3
      goto iter_loop
  iter_end:

    # get the source string and position offset from start of source
    # code for this match node
    $S2 = node 
    $I3 = node.from()
    .local pmc result
    result = new 'PAST::Op'
    result.set_node($S2,$I3,type,newchildren)
    .return (result)
}

expr: term(.) = {
    .local pmc result
    .local pmc children
    children = new PerlArray
    $P1 = node.get_hash()
    $P0 = new Iterator, $P1    # setup iterator for node
    set $P0, 0 # reset iterator, begin at start
  iter_loop:
    unless $P0, iter_end         # while (entries) ...
      shift $S2, $P0             # get key for next entry
      # skip 'type' keys added by the operator precedence parser
      if $S2 == 'type' goto iter_loop 
      $P2 = $P0[$S2]      # get entry at current key
      $P3 = tree.get('result', $P2, $S2)
      push children, $P3
      goto iter_loop
  iter_end:

    $I0 = elements children
    unless $I0 == 1 goto err_too_many
    result = children[0]
    .return (result)

  err_too_many:
    print "error: Currently, 'term' nodes should have only one child.\n"
    end
}
