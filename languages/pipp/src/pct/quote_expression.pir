.include 'cclass.pasm'

.namespace ['Pipp::Grammar']

.sub 'peek_brackets' :method
    .param string target
    .param int pos

    .local string brackets, start, stop
    brackets = unicode:"<>[](){}\xab\xbb"
    start = substr target, pos, 1
    stop = start
    $I0 = index brackets, start
    if $I0 < 0 goto end
    $I1 = $I0 % 2
    unless $I1 goto bracket_valid
    self.'panic'("Using a closing delimiter for an opener is reserved")
    goto end
  bracket_valid:
    inc $I0
    stop = substr brackets, $I0, 1
  end:
    .return (start, stop)
.end


.sub 'quote_expression' :method
    .param string flags
    .param pmc    options    :slurpy :named

    ##  create a new match object
    .local pmc mob
    .local int pos
    .local string target
    (mob, pos, target) = self.'new'(self)

    ##  get action object
    .local pmc action
    action = options['action']

    ##  set up options based on flags
    .local pmc flagarray, iter
    flagarray = split ' ', flags
    iter = new 'Iterator', flagarray
  iter_loop:
    unless iter goto iter_end
    .local string oname
    oname = shift iter
    oname = substr oname, 1   # remove the leading colon
    options[oname] = 1
    if oname == 'qq' goto opt_qq
    goto iter_loop
  opt_qq:
    options['s'] = 1
    options['a'] = 1
    options['h'] = 1
    options['f'] = 1
    options['c'] = 1
    options['b'] = 1
    options['q'] = 1         #
    goto iter_loop
  iter_end:

    .local string start, stop
    (start, stop) = self.'peek_brackets'(target, pos)

    ##  determine pos, lastpos
    $I0 = length start
    pos += $I0
    .local int stoplen, lastpos
    stoplen = length stop
    lastpos = length target
    lastpos -= stoplen
    options['stop'] = stop

    .local string key
    ##  handle word parsing
  word_start:
    ##  set up escapes based on flags
    .local string escapes
    escapes = ''
    $I0 = options['s']
    unless $I0 goto escape_s_done
    escapes = '$'
  escape_s_done:
    $I0 = options['c']
    unless $I0 goto escape_c_done
    escapes .= '{'
  escape_c_done:
  have_escapes:
    options['escapes'] = escapes

    .local pmc quote_concat
    quote_concat = new 'ResizablePMCArray'

    goto word_plain
  word_loop:
    pos = find_not_cclass .CCLASS_WHITESPACE, target, pos, lastpos
    if pos > lastpos goto fail
    $S0 = substr target, pos, stoplen
    if $S0 == stop goto word_succeed
    if pos >= lastpos goto fail
    goto word_plain
  word_shell:
    $S0 = substr target, pos, 1
    if $S0 == '"' goto word_shell_double
    if $S0 != "'" goto word_plain
  word_shell_single:
    inc pos
    mob.'to'(pos)
    $P0 = mob.'quote_concat'()
    unless $P0 goto fail
    push quote_concat, $P0
    pos = $P0.'to'()
    inc pos
    goto word_loop
  word_shell_double:
    inc pos
    mob.'to'(pos)
    $P0 = mob.'quote_concat'()
    unless $P0 goto fail
    push quote_concat, $P0
    pos = $P0.'to'()
    inc pos
    goto word_loop
  word_plain:
    mob.'to'(pos)
    $P0 = mob.'quote_concat'(options)
    unless $P0 goto fail
    push quote_concat, $P0
    pos = $P0.'to'()
    goto word_loop
  word_succeed:
    key = 'quote_concat'
    mob[key] = quote_concat

  succeed:
    pos += stoplen
    mob.'to'(pos)
    if null action goto succeed_done
    $I0 = can action, 'quote_expression'
    unless $I0 goto succeed_done
    action.'quote_expression'(mob, key)
  succeed_done:
    .return (mob)
  fail:
    mob.'to'(-1)
    .return (mob)
.end


.sub 'quote_concat' :method
    .param pmc options

    ##  create a new match object
    .local pmc mob
    .local int pos
    .local string target
    (mob, pos, target) = self.'new'(self)

    ##  determine pos, lastpos
    .local string stop
    .local int stoplen, lastpos
    stop = options['stop']
    stoplen = length stop
    lastpos = length target
    lastpos -= stoplen

    .local string escapes
    escapes = options['escapes']

    .local pmc quote_term
    quote_term = new 'ResizablePMCArray'

  term_loop:
    mob.'to'(pos)
    $P0 = mob.'quote_term'(options)
    unless $P0 goto fail
    push quote_term, $P0
    pos = $P0.'to'()
    if pos > lastpos goto fail
    $S0 = substr target, pos, stoplen
    if $S0 == stop goto succeed
    goto term_loop
    $I0 = is_cclass .CCLASS_WHITESPACE, target, pos
    unless $I0 goto term_loop
  succeed:
    ##  save the array of captured terms
    mob['quote_term'] = quote_term
    mob.'to'(pos)
    ##  call any related {*} actions
    .local pmc action
    action = options['action']
    if null action goto succeed_done
    $I0 = can action, 'quote_concat'
    unless $I0 goto succeed_done
    action.'quote_concat'(mob)
  succeed_done:
    .return (mob)
  fail:
    mob.'to'(-1)
    .return (mob)
.end


.sub 'quote_term' :method
    .param pmc options

    .local pmc action
    action = options['action']

    .local pmc mob
    .local int pos
    .local string target
    (mob, pos, target) = self.'new'(self)

    .local string leadchar, escapes
    escapes = options['escapes']
    leadchar = substr target, pos, 1
    $I0 = index escapes, leadchar
    if $I0 < 0 goto term_literal
    if leadchar == '$' goto term_scalar
    if leadchar == '{' goto term_closure
  term_literal:
    mob.'to'(pos)
    #_dumper(pos)
    $P0 = mob.'quote_literal'(options)
    unless $P0 goto fail
    pos = $P0.'to'()
    mob['quote_literal'] = $P0
    .local string key
    key = 'literal'
    goto succeed

  term_scalar:
    mob.'to'(pos)
    $P0 = mob.'var'('action'=>action)
    unless $P0 goto term_literal
    pos = $P0.'to'()
    key = 'var'
    mob[key] = $P0
    goto succeed

  term_closure:
    mob.'to'(pos)
    $P0 = mob.'curly_interpolation'('action'=>action)
    unless $P0 goto term_literal
    pos = $P0.'to'()
    key = 'curly_interpolation'
    mob[key] = $P0
    goto succeed

  succeed:
    mob.'to'(pos)
    if null action goto succeed_done
    $I0 = can action, 'quote_term'
    unless $I0 goto succeed_done
    action.'quote_term'(mob, key)
  succeed_done:
    .return (mob)

  fail:
    mob.'to'(-1)
    .return (mob)
.end


.sub 'quote_literal' :method
    .param pmc options

    #_dumper( options )

    .local pmc mob
    .local int pos
    .local string target
    (mob, pos, target) = self.'new'(self)

    .local string stop, stop1
    .local int stoplen, lastpos
    stop = options['stop']
    stop1 = substr stop, 0, 1
    stoplen = length stop
    lastpos = length target
    lastpos -= stoplen

    .local string escapes
    .local int optq, optb
    escapes = options['escapes']
    optq = options['q']
    optb = options['b']

    .local string literal
    literal = ''

  scan_loop:
    if pos > lastpos goto fail
    $S0 = substr target, pos, stoplen
    if $S0 == stop goto succeed
    goto scan_loop_1
    $I0 = is_cclass .CCLASS_WHITESPACE, target, pos
    if $I0 goto succeed
  scan_loop_1:
    if pos >= lastpos goto fail

  scan_char:
    .local string litchar
    litchar = substr target, pos, 1
    ##  if we've reached an escape char, we're done
    $I0 = index escapes, litchar
    if $I0 >= 0 goto succeed
    ##  if this isn't an interpolation, add the char
    unless optq goto add_litchar
    if litchar != "\\" goto add_litchar
    ##  okay, we have a backslash, let's process it
    .local string backchar
    $I0 = pos + 1
    backchar = substr target, $I0, 1
    ##  handle :q options, \\ and \+stop
    if backchar == "\\" goto add_backchar
    if backchar == stop1 goto add_backchar
    unless optb goto add_litchar
    ##  handle :b options
    $I0 = index "0abefnrtxdo123456789", backchar
    if $I0 < 0 goto add_backchar
    if $I0 >= 11 goto fail_backchar_digit
    if $I0 >= 8 goto scan_xdo
    litchar = substr "\0\a\b\e\f\n\r\t", $I0, 1
    if $I0 >= 1 goto add_litchar2
    ##  peek ahead for octal digits after \0
    $I0 = pos + 2
    $S0 = substr target, $I0, 1
    $I0 = index "01234567", $S0
    if $I0 >= 0 goto fail_backchar_digit
  add_litchar2:
    pos += 2
    literal .= litchar
    goto scan_loop
  add_backchar:
    literal .= backchar
    pos += 2
    goto scan_loop
  add_litchar:
    literal .= litchar
    inc pos
    goto scan_loop

  scan_xdo:
    ##  handle \x, \d, and \o escapes.  start by converting
    ##  the backchar into 8, 10, or 16 (yes, it's a hack
    ##  but it works).  Then loop through the characters
    ##  that follow to compute the decimal value of codepoints,
    ##  and add the codepoints to our literal.
    .local int base, decnum, isbracketed
    base = index '        o d     x', backchar
    decnum = 0
    pos += 2
    $S0 = substr target, pos, 1
    isbracketed = iseq $S0, '['
    pos += isbracketed
  scan_xdo_char_loop:
    $S0 = substr target, pos, 1
    $I0 = index '0123456789abcdef0123456789ABCDEF', $S0
    if $I0 < 0 goto scan_xdo_char_end
    $I0 %= 16
    if $I0 >= base goto scan_xdo_char_end
    decnum *= base
    decnum += $I0
    inc pos
    goto scan_xdo_char_loop
  scan_xdo_char_end:
    $S1 = chr decnum
    concat literal, $S1
    unless isbracketed goto scan_xdo_end
    if $S0 == ']' goto scan_xdo_end
    if $S0 != ',' goto fail
    inc pos
    decnum = 0
    goto scan_xdo_char_loop
  scan_xdo_end:
    pos += isbracketed
    goto scan_loop

  succeed:
    mob.'result_object'(literal)
    mob.'to'(pos)
    .return (mob)
  fail_backchar_digit:
    self.panic('\123 form deprecated, use \o123 instead')
  fail:
    mob.'to'(-1)
    .return (mob)
.end


# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
