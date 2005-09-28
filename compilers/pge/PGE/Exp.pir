=head1 Title

PGE::Exp - PGE rule expression trees and code generator

=head1 DESCRIPTION

This file implements the expression objects used to build rule
expressions.  The classes currently include

    PGE::Exp       - base class
    PGE::Start     - start of rule
    PGE::End       - (successful) end of rule
    PGE::Literal   - match a literal string
    PGE::Scalar    - match a scalar
    PGE::Dot       - match any character
    PGE::CCShortcut - character class shortcuts (\d, \D, \w, etc.)
    PGE::CharClass - character classes (<[abcde]>, <-[abcde]>)
    PGE::WS        - <?ws> rule
    PGE::Anchor    - matching of ^, ^^, $, $$, \b, \B anchors
    PGE::Cut       - :: and :::
    PGE::Concat    - concatenation of expressions
    PGE::Alt       - alternations
    PGE::Group     - groups and captures

Each class has a C<gen> method that is used to generate PIR
code for that portion of the expression.  The code does not yet
have a lot of optimizations in it -- we're still focusing on
functionality and correctness at the moment.

=cut

.include "cclass.pasm"
.include "errors.pasm"
.include "interpinfo.pasm"

.const int PGE_INF = 2147483647                    # XXX: arbitrary limit
.const int PGE_CUT_GROUP = -1                      # cut current alt/group
.const int PGE_CUT_RULE = -2                       # cut current rule

.namespace [ "PGE::Exp" ]

.sub __onload
    .local pmc hashclass
    .local pmc expclass
    .local pmc hash
    hashclass = getclass "Hash"
    expclass = subclass hashclass, "PGE::Exp"
    $P0 = subclass expclass, "PGE::Exp::Start"
    $P0 = subclass expclass, "PGE::Exp::End"
    $P0 = subclass expclass, "PGE::Exp::Literal"
    $P0 = subclass expclass, "PGE::Exp::Scalar"
    $P0 = subclass expclass, "PGE::Exp::Dot"
    $P0 = subclass expclass, "PGE::Exp::CCShortcut"
    $P0 = subclass expclass, "PGE::Exp::CharClass"
    $P0 = subclass expclass, "PGE::Exp::WS"
    $P0 = subclass expclass, "PGE::Exp::Anchor"
    $P0 = subclass expclass, "PGE::Exp::Cut"
    $P0 = subclass expclass, "PGE::Exp::Concat"
    $P0 = subclass expclass, "PGE::Exp::Alt"
    $P0 = subclass expclass, "PGE::Exp::Group"
    hash = new Hash
    store_global "PGE::Exp::CCShortcut", "%slashcode", hash
    hash['\d'] = "$I0=is_digit target, pos \n unless $I0 goto %s_f"
    hash['\n'] = "$I0=is_newline target, pos \n unless $I0 goto %s_f"
    hash['\s'] = "$I0=is_whitespace target, pos \n unless $I0 goto %s_f"
    hash['\w'] = "$I0=is_wordchar target, pos \n unless $I0 goto %s_f"
    hash['\D'] = "$I0=is_digit target, pos \n if $I0 goto %s_f"
    hash['\N'] = "$I0=is_newline target, pos \n if $I0 goto %s_f"
    hash['\S'] = "$I0=is_whitespace target, pos \n if $I0 goto %s_f"
    hash['\W'] = "$I0=is_wordchar target, pos \n if $I0 goto %s_f"
    $P0 = new Integer
    store_global "PGE::Exp", "$_serno", $P0
    .return ()
.end

=head2 Functions

=item C<new(STR class [, PMC exp1 [, PMC exp2]])>

Creates and returns  a new C<Exp> object of C<class>, initializing
min/max/greedy/etc.  values and C<exp1> and C<exp2> objects if provided.

=cut

.sub "new"
    .param string expclass     :optional           # class from find_type
    .param int has_class       :opt_flag
    .param pmc exp1            :optional           # left expression
    .param int has_exp1        :opt_flag
    .param pmc exp2            :optional           # right expression
    .param int has_exp2        :opt_flag
    .local pmc me                                  # new expression object

    if has_class goto buildme
    expclass = "PGE::Exp"
  buildme:
    $I0 = find_type expclass
    me = new $I0
    me["min"] = 1
    me["max"] = 1
    me["isgreedy"] = 1
    me["iscut"] = 0

    unless has_exp1 goto end
    me["exp1"] = exp1
    unless has_exp2 goto end
    me["exp2"] = exp2
  end:
    .return (me)
.end

=item C<(min, max, isgreedy, iscut) = _getattributes()>

A method for quickly retrieving the min, max, and greediness
attributes of an expression object.

=cut

.sub "_getattributes" method
    .local int min, max, isgreedy, iscut
    min = self["min"]
    max = self["max"]
    isgreedy = self["isgreedy"]
    iscut = self["iscut"]
    .return (min, max, isgreedy, iscut)
.end

=item C<quant()>

Returns a string representing the quantifier values of the
current expression object.

=cut

.sub "quant" method
    .local string min, max
    $S0 = "{"
    min = self["min"]
    concat $S0, min
    concat $S0, ".."
    max = self["max"]
    concat $S0, max
    concat $S0, "}"
    $I0 = self["isgreedy"]
    if $I0 goto cut
    concat $S0, "?"
  cut:
    $I0 = self["iscut"]
    unless $I0 goto end
    concat $S0, ":"
  end:
    .return ($S0)
.end

=item C<analyze(PMC next, PMC pad)>

The analyze method is used to walk an expression tree and perform
a variety of optimizations and pre-processing in preparation for
generating the rule code.  The C<next> parameter identifies the
expression that will come after this one.  The C<pad> parameter
is a work pad hash that is carried from one object to the next.
Typical entries in the pad include:
    isarray: does this (group) object cause its subelements to repeat?
    reps: a hash of lexically repeated capture names

=cut

.sub "analyze" method
    .param pmc next
    .param pmc pad
    self["firstchars"] = ""
    .return ()
.end


=item C<firstchars([PMC exp1 [, PMC exp2]])>

The firstchars method sets the "firstchars" optimization hint
based on the concatenation of the firstchars of any expressions
provided.  If either of the expressions has an empty firstchars
hint, then we have have no firstchars either.

=cut

.sub "firstchars" method
    .param pmc exp1            :optional
    .param int has_exp1        :opt_flag
    .param pmc exp2            :optional
    .param int has_exp2        :opt_flag

    unless has_exp1 goto exp_1
    $S0 = exp1["firstchars"]
    unless $S0 > "" goto exp_1
    unless has_exp2 goto exp_1
    $S1 = exp2["firstchars"]
    unless $S1 > "" goto exp_1
    concat $S0, $S1
    goto end
  exp_1:
    $S0 = ""
  end:
    self["firstchars"] = $S0
    .return ()
.end

=item C<(STR, INT) = serno([STR prefix [, INT start]])>

This method simply returns integers and labels usable for
serialization, e.g., for generating unique labels and identifiers
within generated code.  The C<start> parameter allows the serial
number to be set to a given value.  XXX: I'm assuming overflow
won't be a problem, but is the use of the start parameter thread-safe?

=cut

.sub "serno" method
    .param string prefix       :optional
    .param int has_prefix      :opt_flag
    .param int start           :optional
    .param int has_start       :opt_flag

    if has_prefix goto serno_1
    prefix = "R"
  serno_1:
    $P0 = find_global "PGE::Exp", "$_serno"
    inc $P0
    unless has_start goto serno_2
    $P0 = start
  serno_2:
    $I0 = $P0
    $S0 = $I0
    concat prefix, $S0
    .return (prefix, $I0)
.end


=item C<emit(PMC code, STR fmt [, STR str1 [, STR str2]])>

Adds to the current code string, replacing %s by str1/str2.

=cut

.sub "emit"
    .param pmc code                                # append to code
    .param string fmt                              # string to output
    .param string str1         :optional           # first %s substitution
    .param string str2         :optional           # second %s substitution

    concat fmt, "\n"
    $I0 = index fmt, "%s"
    if $I0 < 0 goto emit_1
    substr fmt, $I0, 2, str1
    $I0 = index fmt, "%s"
    if $I0 < 0 goto emit_1
    substr fmt, $I0, 2, str2
  emit_1:
    concat code, fmt
    .return ()
.end

=item C<emitsub(PMC code, STR next, [STR save [, ...]])

Generate a subroutine call to C<next>, preserving and restoring
any save parameters in the process.  The special parameter "NOCUT"
says not to intercept cutting operations.

=cut

.sub "emitsub" method
    .param pmc code                                # append to code
    .param string next                             # sub label
    .param pmc save         :slurpy                # registers to save
    .local int nocut                               # flag for "NOCUT"
    .local pmc emit                                # emit subroutine
    .local int savec                               # number of save items

    emit = find_global "PGE::Exp", "emit"
    nocut = 0
    savec = elements save
    $I0 = 0
  saveregs_0:
    if $I0 >= savec goto emitsub_0
    $S0 = save[$I0]
    inc $I0
    if $S0 == "NOCUT" goto set_nocut
    emit(code, "    save %s", $S0)
    goto saveregs_0
  set_nocut:
    nocut = 1
    goto saveregs_0
  emitsub_0:
    emit(code, "    bsr %s", next)
    $I0 = savec - 1
  restoreregs_0:
    if $I0 < 0 goto generate_cut
    $S0 = save[$I0]
    dec $I0
    if $S0 == "NOCUT" goto restoreregs_0
    emit(code, "    restore %s", $S0)
    goto restoreregs_0
  generate_cut:
    if nocut goto end
    emit(code, "    if cutting != 0 goto fail")
  end:
    .return ()
.end

=item C<genliteral(PMC code, STR label, STR next)>

Generate code to match literal strings, quantified as necessary.
There are several types of rule expressions that result matching
literal strings, thus we write the code once here rather than
duplicate it in each expression.  This code assumes that the
literal to be matched has already been placed in the C<str> string
register.

=cut

.sub "genliteral" method
    .param pmc code
    .param string label
    .param string next
    .local pmc emit
    .local int min, max, isgreedy, iscut
    (min, max, isgreedy, iscut) = self."_getattributes"()
    emit = find_global "PGE::Exp", "emit"
    emit(code, "    litlen = length lit")
    unless min == 1 goto greedy
    unless max == 1 goto greedy
    emit(code, "    $S0 = substr target, pos, litlen")
    emit(code, "    if $S0 != lit goto fail")
    emit(code, "    pos += litlen")
    emit(code, "    goto %s", next)
    .return ()
  greedy:
    emit(code, "    rep = 0")
    unless isgreedy goto lazy
    emit(code, "  %s_lit1:", label)
    emit(code, "    if rep >= %s goto %s_lit2", max, label)
    emit(code, "    $S0 = substr target, pos, litlen")
    emit(code, "    if $S0 != lit goto %s_lit2", label)
    emit(code, "    inc rep")
    emit(code, "    pos += litlen")
    emit(code, "    goto %s_lit1", label)
    emit(code, "  %s_lit2:", label)
    emit(code, "    if rep < %s goto fail", min)
    unless iscut goto greedy_1
    emit(code, "    goto %s", next)
  greedy_1:
    emit(code, "    if rep == %s goto %s", min, next)
    self.emitsub(code, next, "pos", "rep", "litlen")
    emit(code, "    dec rep")
    emit(code, "    pos -= litlen")
    emit(code, "    goto %s_lit2", label)
    .return ()

  lazy:
    emit(code, "  %s_lit1:", label)
    emit(code, "    if rep < %s goto %s_lit2", min, label)
    unless iscut goto lazy_1
    emit(code, "    goto %s", next)
    goto lazy_2
  lazy_1:
    emit(code, "    if rep >= %s goto %s", max, next)
    self.emitsub(code, next, "pos", "rep", "lit", "litlen")
  lazy_2:
    emit(code, "  %s_lit2:", label)
    emit(code, "    $S0 = substr target, pos, litlen")
    emit(code, "    if $S0 != lit goto fail")
    emit(code, "    inc rep")
    emit(code, "    pos += litlen")
    emit(code, "    goto %s_lit1", label)
    .return ()
.end

.namespace [ "PGE::Exp::Start" ]

.sub analyze method
    .param pmc next
    .param pmc pad
    pad = new Hash                                 # create a new workpad
    $P0 = self["exp1"]
    $P0.analyze(self, pad)                         # analyze our subexp
    self.firstchars($P0)                           # set firstchars
    .return ()
.end

.sub "gen" method
    .param pmc code
    .param string label
    .param string next
    .local pmc emit
    .local pmc exp1
    .local string firstchars

    emit = find_global "PGE::Exp", "emit"
    emit(code, ".sub _pge_rule")
    emit(code, "    .param pmc mob")
    emit(code, "    .param pmc pos :optional")
    emit(code, "    .param int has_pos :opt_flag")
    emit(code, "    .local pmc yield")
    emit(code, "    $I0 = isa mob, \"PGE::Match\"")
    emit(code, "    if $I0 goto init_pos")
    emit(code, "  new_match:")
    emit(code, "    $P0 = new String")
    emit(code, "    assign $P0, mob")
    emit(code, "    $I0 = find_type \"PGE::Rule\"")
    emit(code, "    mob = new $I0")
    emit(code, "    setattribute mob, \"PGE::Match\\x0$:target\", $P0")
    emit(code, "    $P0 = new PerlInt")
    emit(code, "    setattribute mob, \"PGE::Match\\x0$:from\", $P0")
    emit(code, "    $P0 = new PerlInt")
    emit(code, "    $P0 = -1")
    emit(code, "    setattribute mob, \"PGE::Match\\x0$:pos\", $P0")
    emit(code, "  init_pos:")
    emit(code, "    if has_pos goto start_match")
    emit(code, "    $P0 = getattribute mob, \"PGE::Match\\x0$:pos\"")
    emit(code, "    pos = $P0")
    emit(code, "  start_match:")
    emit(code, "    newsub yield, .Coroutine, _pge_rule_coroutine")
    emit(code, "    setattribute mob, \"PGE::Match\\x0&:yield\", yield")
    emit(code, "    yield(mob, pos)")
    emit(code, "    .return (mob)")
    emit(code, ".end")
    emit(code, "")
    emit(code, ".sub _pge_rule_coroutine")
    emit(code, "    .param pmc mob")
    emit(code, "    .param int pos")
    emit(code, "    .local string target")
    emit(code, "    .local int lastpos")
    emit(code, "    .local int cutting")
    emit(code, "    .local int rep, maxrep")
    emit(code, "    .local int litlen")
    emit(code, "    .local string lit")
    emit(code, "    .local pmc gpad, cpad")
    emit(code, "    .local pmc cobcapt")
    emit(code, "    .local pmc capt")
    emit(code, "    .local pmc from")
    emit(code, "    .local int iscreator")
    emit(code, "    gpad = new PerlArray")
    emit(code, "    cpad = new PerlArray")
    emit(code, "    push gpad, -1")
    emit(code, "    push cpad, mob")
    emit(code, "    from = getattribute mob, \"PGE::Match\\x0$:from\"")
    emit(code, "    $P0 = getattribute mob, \"PGE::Match\\x0$:target\"")
    emit(code, "    target = $P0")
    emit(code, "    lastpos = length target")
    emit(code, "    if pos >= 0 goto try_at_pos")
    emit(code, "    pos = 0")
    emit(code, "  try_match:")
    emit(code, "    cutting = 0")
    emit(code, "    if pos > lastpos goto fail_forever")
    $I0 = exists self["firstchars"]
    unless $I0 goto gen_1
    firstchars = self["firstchars"]
    unless firstchars > "" goto gen_1
    $P0 = find_global "Data::Escape", "String"
    firstchars = $P0(firstchars, '"')
    emit(code, "    $S0 = substr target, pos, 1")
    emit(code, "    $I0 = index \"%s\", $S0", firstchars)
    emit(code, "    if $I0 < 0 goto try_again")
  gen_1:
    emit(code, "    from = pos")
    self.emitsub(code, label, "pos", "from", "lastpos", "NOCUT")
    #emit(code, "    if cutting == %s goto fail_forever", PGE_CUT_RULE)
    emit(code, "    if cutting == %s goto fail_forever", -2)  # XXX
    emit(code, "  try_again:")
    emit(code, "    inc pos")
    emit(code, "    goto try_match")
    emit(code, "  try_at_pos:")
    emit(code, "    cutting = 0")
    emit(code, "    from = pos")
    self.emitsub(code, label, "NOCUT")
    emit(code, "  fail_forever:")
    emit(code, "    .yield(-2)")
    emit(code, "    goto fail_forever")

    exp1 = self["exp1"]
    exp1.gen(code, label, next)

    emit(code, ".end")
    .return ()
.end

.namespace [ "PGE::Exp::End" ]

.sub "gen" method
    .param pmc code
    .param string label
    .param string next
    .local pmc emit
    emit = find_global "PGE::Exp", "emit"
    emit(code, "\n  %s:", label)
    emit(code, "    $P0 = getattribute mob, \"PGE::Match\\x0$:pos\"")
    emit(code, "    $P0 = pos")
    emit(code, "    .yield(pos)")
    emit(code, "    $P0 = -1")
    emit(code, "  fail:")
    emit(code, "    ret")
    .return ()
.end

.namespace [ "PGE::Exp::Literal" ]

.sub "analyze" method
    .param pmc next
    .param pmc pad
    $S0 = self["literal"]                          # set up firstchars
    $S0 = substr $S0, 0, 1
    self["firstchars"] = $S0
    $I0 = self["min"]                              # if we allow zero reps
    unless $I0 < 1 goto end                        # then add firstchars
    self.firstchars(self, next)                    # of the following exp
  end:
    .return ()
.end

.sub "gen" method
    .param pmc code
    .param string label
    .param string next
    .local pmc emit
    emit = find_global "PGE::Exp", "emit"
    $S0 = self."quant"()
    $S1 = self["literal"]
    $P0 = find_global "Data::Escape", "String"
    $S1 = $P0($S1, '"')
    emit(code, "\n  %s:  # literal %s", label, $S0)
    emit(code, "    lit = \"%s\"", $S1)
    self.genliteral(code, label, next)
    .return ()
.end


.namespace [ "PGE::Exp::Scalar" ]

.sub "gen" method
    .param pmc code
    .param string label
    .param string next
    .local pmc emit
    .local pmc cname
    .local int subp
    emit = find_global "PGE::Exp", "emit"
    $S0 = self."quant"()
    cname = self["cname"]
    $I0 = isa cname, "Integer"
    unless $I0 goto named
    subp = cname
    emit(code, "\n  %s:  # backref $%s %s", label, subp, $S0)
    emit(code, "    lit = ''")
    emit(code, "    $P0 = getattribute mob, \"PGE::Match\\x0@:capt\"")
    emit(code, "    isnull $P0, %s_1", label)
    emit(code, "    $P1 = $P0[%s]", subp)
    emit(code, "    lit = $P1[-1]")
    emit(code, "  %s_1:", label)
    .return self.genliteral(code, label, next)
  named:
    $P0 = find_global "Data::Escape", "String"
    $S1 = cname
    $S1 = $P0($S1)
    emit(code, "\n  %s:  # backref $<%s> %s", label, $S1, $S0)
    emit(code, "    lit = ''")
    emit(code, "    $P0 = getattribute mob, \"PGE::Match\\x0%:capt\"")
    emit(code, "    isnull $P0, %s_1", label)
    emit(code, "    $P1 = $P0[\"%s\"]", $S1)
    emit(code, "    lit = $P1[-1]")
    emit(code, "  %s_1:", label)
    .return self.genliteral(code, label, next)
.end


.namespace [ "PGE::Exp::Dot" ]

.sub "gen" method
    .param pmc code
    .param string label
    .param string next
    .local pmc emit
    .local int min, max, isgreedy, iscut
    (min, max, isgreedy, iscut) = self."_getattributes"()
    emit = find_global "PGE::Exp", "emit"
    $S0 = self."quant"()
    emit(code, "\n  %s: # dot %s", label, $S0)
    emit(code, "    maxrep = length target")
    emit(code, "    maxrep -= pos")
    unless min > 0 goto dot_1
    emit(code, "    if maxrep < %s goto fail", min)
  dot_1:
    unless min == max goto dot_2
    emit(code, "    pos += %s", min)
    emit(code, "    goto %s", next)
    .return ()
  dot_2:
    if max == PGE_INF goto greedy
    emit(code, "    if maxrep <= %s goto %s_1", max, label)
    emit(code, "    maxrep = %s", max)
    emit(code, "  %s_1:", label)
  greedy:
    unless isgreedy goto lazy
    emit(code, "    rep = maxrep")
    emit(code, "    pos += rep")
    emit(code, "  %s_2:", label)
    emit(code, "    if rep < %s goto fail", min)
    unless iscut goto greedy_1
    emit(code, "    goto %s", next)
    .return ()
  greedy_1:
    emit(code, "    if rep == %s goto %s", min, next)
    self.emitsub(code, next, "pos", "rep")
    emit(code, "    dec rep")
    emit(code, "    dec pos")
    emit(code, "    goto %s_2", label)
    .return ()
  lazy:
    emit(code, "    rep = %s", min)
    emit(code, "    pos += rep", min)
    emit(code, "  %s_3:", label)
    emit(code, "    if rep > maxrep goto fail")
    unless iscut goto lazy_1
    emit(code, "    goto %s", next)
    .return ()
  lazy_1:
    emit(code, "    if rep == maxrep goto %s", next)
    self.emitsub(code, next, "pos", "rep", "maxrep")
    emit(code, "    inc rep")
    emit(code, "    inc pos")
    emit(code, "    goto %s_3", label)
    .return ()
.end

.namespace [ "PGE::Exp::CCShortcut" ]

.sub gen method
    .param pmc code
    .param string label
    .param string next
    .local string token
    .local int min, max, isgreedy, iscut
    .local pmc emit
    .local pmc hash
    .local string test
    (min, max, isgreedy, iscut) = self."_getattributes"()
    hash = find_global "PGE::Exp::CCShortcut", "%slashcode"
    token = self["token"]
    test = hash[token]
    emit = find_global "PGE::Exp", "emit"
    emit(code, "\n  %s:", label)
    emit(code, "    rep = 0")
    unless isgreedy goto lazy
    emit(code, "  %s_1:", label)
    emit(code, "    if pos >= lastpos goto %s_f", label)
    emit(code, "    if rep >= %s goto %s_f", max, label)
    emit(code, test, label)
    emit(code, "    inc pos")
    emit(code, "    inc rep")
    emit(code, "    goto %s_1", label)
    emit(code, "  %s_f:", label)
    emit(code, "    if rep < %s goto fail", min)
    unless iscut goto greedy_1
    emit(code, "    goto %s", next)
    .return ()
  greedy_1:
    emit(code, "    if rep == %s goto %s", min, next)
    self.emitsub(code, next, "pos", "rep")
    emit(code, "    dec pos")
    emit(code, "    dec rep")
    emit(code, "    goto %s_f", label)
    .return ()
  lazy:
    emit(code, "  %s_0:", label)
    emit(code, "    if rep < %s goto %s_1", min, label)
    unless iscut goto lazy_1
    emit(code, "    goto %s", next)
    goto lazy_2
  lazy_1:
    emit(code, "    if rep >= %s goto %s", max, next)
    emit(code, "    if pos >= lastpos goto fail")
    self.emitsub(code, next, "pos", "rep")
  lazy_2:
    emit(code, "  %s_1:", label)
    emit(code, test, label)
    emit(code, "    inc rep")
    emit(code, "    inc pos")
    emit(code, "    goto %s_0", label)
    emit(code, "  %s_f:", label)
    emit(code, "    goto fail")
    .return ()
.end

.namespace [ "PGE::Exp::CharClass" ]

# Note:  The implementation interface for CharClass may change
# in the near future, so don't rely on this too heavily just yet.
# (pmichaud, 2005-05-11)

.sub gen method
    .param pmc code
    .param string label
    .param string next
    .local string token
    .local int min, max, isgreedy, iscut
    .local pmc emit
    .local string charclass, charmatch
    (min, max, isgreedy, iscut) = self."_getattributes"()
    emit = find_global "PGE::Exp", "emit"
    $P0 = find_global "Data::Escape", "String"
    charclass = self["charclass"]
    charclass = $P0(charclass, '"')
    charmatch = self["charmatch"]
    emit(code, "\n  %s:", label)
    emit(code, "    rep = 0")
    unless isgreedy goto lazy
    emit(code, "  %s_1:", label)
    emit(code, "    if pos >= lastpos goto %s_2", label)
    emit(code, "    if rep >= %s goto %s_2", max, label)
    emit(code, "    $S0 = substr target, pos, 1")
    emit(code, "    $I0 = index \"%s\", $S0", charclass)
    emit(code, "    %s $I0 == -1 goto %s_2", charmatch, label)
    emit(code, "    inc pos")
    emit(code, "    inc rep")
    emit(code, "    goto %s_1", label)
    emit(code, "  %s_2:", label)
    emit(code, "    if rep < %s goto fail", min)
    unless iscut goto greedy_1
    emit(code, "    goto %s", next)
    .return ()
  greedy_1:
    emit(code, "    if rep == %s goto %s", min, next)
    self.emitsub(code, next, "pos", "rep")
    emit(code, "    dec pos")
    emit(code, "    dec rep")
    emit(code, "    goto %s_2", label)
    .return ()
  lazy:
    emit(code, "  %s_0:", label)
    emit(code, "    if rep < %s goto %s_1", min, label)
    unless iscut goto lazy_1
    emit(code, "    goto %s", next)
    goto lazy_2
  lazy_1:
    emit(code, "    if rep >= %s goto %s", max, next)
    emit(code, "    if pos > lastpos goto fail")
    self.emitsub(code, next, "pos", "rep")
  lazy_2:
    emit(code, "  %s_1:", label)
    emit(code, "    $S0 = substr target, pos, 1")
    emit(code, "    $I0 = index \"%s\", $S0", charclass)
    emit(code, "    %s $I0 == -1 goto fail", charmatch)
    emit(code, "    inc rep")
    emit(code, "    inc pos")
    emit(code, "    goto %s_0", label)
    .return ()
.end

.namespace [ "PGE::Exp::WS" ]

.sub "analyze" method
    .param pmc next
    .param pmc pad
    .local string nextfc
    self["firstchars"] = ""
    nextfc = next["firstchars"]
    unless nextfc > "" goto end
    $I1 = length nextfc
    $I0 = find_cclass .CCLASS_WHITESPACE, nextfc, 0, $I1
    unless $I0 == $I1 goto end
    self["iscut"] = 1
  end:
    .return ()
.end

.sub "gen" method
    .param pmc code
    .param string label
    .param string next
    .local pmc emit
    emit = find_global "PGE::Exp", "emit"
    emit(code, "\n  %s: # <?ws>", label)
    emit(code, "    rep = 0")
    emit(code, "    if pos >= lastpos goto %s", next)
    emit(code, "    if pos < 1 goto %s_1", label)
    emit(code, "    $I0 = is_cclass %s, target, pos", .CCLASS_WORD)
    emit(code, "    unless $I0 goto %s_1", label)
    emit(code, "    $I0 = pos - 1")
    emit(code, "    $I1 = is_cclass %s, target, $I0", .CCLASS_WORD)
    emit(code, "    if $I1 goto fail")
    emit(code, "  %s_1:", label)
    emit(code, "    rep = pos")
    emit(code, "    pos = find_not_cclass %s, target, pos, lastpos", .CCLASS_WHITESPACE)
    $I0 = self["iscut"]
    unless $I0 goto backtrack
    emit(code, "    goto %s", next)
    goto end
  backtrack:
    emit(code, "    rep = pos - rep")
    emit(code, "  %s_2:", label)
    emit(code, "    if rep == 0 goto %s", next)
    self.emitsub(code, next, "pos", "rep")
    emit(code, "    dec rep")
    emit(code, "    dec pos")
    emit(code, "    goto %s_2", label)
  end:
    .return ()
.end


.namespace [ "PGE::Exp::Anchor" ]

.sub "analyze" method
    .param pmc next
    .param pmc pad
    self.firstchars(next)
    .return ()
.end

.sub "gen" method
    .param pmc code
    .param string label
    .param string next
    .local pmc emit
    .local string token
    emit = find_global "PGE::Exp", "emit"
    emit(code, "\n  %s:", label)
    token = self["token"]
    if token == "\\b" goto word
    if token == "\\B" goto word
    if token == '$$' goto eos
    if token == '$' goto eos
    emit(code, "    if pos == 0 goto %s", next)
    unless token == '^^' goto end
    emit(code, "    $I0 = pos - 1")
    emit(code, "    $I1 = is_newline target, $I0")
    emit(code, "    if $I1 goto %s", next)
    goto end
  eos:
    emit(code, "    if pos == lastpos goto %s", next)
    unless token == '$$' goto end
    emit(code, "    $I0 = is_newline target, pos")
    emit(code, "    if $I0 goto %s", next)
    goto end
  word:
    emit(code, "    $I0 = 0")
    emit(code, "    unless pos > 0 goto %s_1", label)
    emit(code, "    $I2 = pos - 1")
    emit(code, "    $I0 = is_wordchar target, $I2")
    emit(code, "  %s_1:", label)
    emit(code, "    $I1 = 0")
    emit(code, "    unless pos < lastpos goto %s_2", label)
    emit(code, "    $I1 = is_wordchar target, pos")
    emit(code, "  %s_2:", label)
    unless token == "\\b" goto word_1
    emit(code, "    if $I0 != $I1 goto %s", next)
    goto end
  word_1:
    emit(code, "    if $I0 == $I1 goto %s", next)
  end:
    emit(code, "    goto fail")
    .return ()
.end


.namespace [ "PGE::Exp::Concat" ]

.sub "analyze" method
    .param pmc next
    .param pmc pad
    .local pmc exp1, exp2
    exp2 = self["exp2"]
    exp2.analyze(next, pad)
    exp1 = self["exp1"]
    exp1.analyze(exp2, pad)
    self.firstchars(exp1)
    .return ()
.end

.sub "gen" method
    .param pmc code
    .param string label
    .param string next
    .local pmc emit
    .local pmc exp1, exp2
    emit = find_global "PGE::Exp", "emit"
    emit(code, "\n  %s:", label)
    ($S0, $I0) = self.serno()
    ($S1, $I1) = self.serno()
    exp1 = self["exp1"]
    exp1.gen(code, $S0, $S1)
    exp2 = self["exp2"]
    exp2.gen(code, $S1, next)
    .return ()
.end

.namespace [ "PGE::Exp::Cut" ]

.sub "analyze" method
    .param pmc next
    .param int pad
    self.firstchars(next)
    .return ()
.end

.sub "gen" method
    .param pmc code
    .param string label
    .param string next
    .param string token
    .local pmc emit
    .local string cutting
    token = self["token"]
    cutting = "gpad[-1]"                           # :: cut alternation
    unless token == ":::" goto cut_1               # ::: cut rule
    cutting = PGE_CUT_RULE
  cut_1:
    emit = find_global "PGE::Exp", "emit"
    emit(code, "\n  %s:", label)
    self.emitsub(code, next, "NOCUT")
    emit(code, "    cutting = %s", cutting)
    emit(code, "    goto fail")
    .return ()
.end

.namespace [ "PGE::Exp::Alt" ]

.sub "analyze" method
    .param pmc next
    .param pmc pad
    .local pmc exp1, exp2
    .local pmc creps

    $I0 = exists pad["creps"]                      # create creps hash array
    if $I0 goto creps_1                            # if not exists
    $P0 = new Hash
    pad["creps"] = $P0
  creps_1:
    creps = pad["creps"]
    creps = clone creps
    exp1 = self["exp1"]
    exp2 = self["exp2"]
    exp2.analyze(next, pad)
    pad["creps"] = creps
    exp1.analyze(next, pad)
    self.firstchars(exp1, exp2)
    .return ()
.end

.sub "gen" method
    .param pmc code
    .param string label
    .param string next
    .local pmc emit
    .local pmc exp1, exp2
    emit = find_global "PGE::Exp", "emit"
    emit(code, "\n  %s:", label)
    ($S0, $I0) = self.serno()
    ($S1, $I1) = self.serno()
    self.emitsub(code, $S0, "pos")
    emit(code, "    goto %s", $S1)
    exp1 = self["exp1"]
    exp1.gen(code, $S0, next)
    exp2 = self["exp2"]
    exp2.gen(code, $S1, next)
    .return ()
.end


.namespace [ "PGE::Exp::Group" ]

.sub "analyze" method
    .param pmc next
    .param pmc pad
    .local pmc exp1
    .local int isarray, psave
    .local pmc creps
    .local string cname

    self["firstchars"] = ""                        # no firstchars default

    $I0 = exists pad["creps"]                      # create creps hash array
    if $I0 goto creps_1                            # if not exists
    $P0 = new Hash
    pad["creps"] = $P0
  creps_1:
    creps = pad["creps"]                           # load creps hash
    $I0 = exists self["cname"]
    unless $I0 goto isarray_0                      # skip if no capture
    $P0 = self["cname"]
    cname = $P0
    $I0 = isa $P0, "Integer"                       # Integer = subpattern cap
    if $I0 goto creps_2
    cname = concat "%", cname
  creps_2:
    $I0 = exists creps[cname]                      # have seen capture name?
    unless $I0 goto creps_3                        #
    $P0 = creps[cname]                             # yes, so prev is now
    $P0["isarray"] = 1                             # an array capture
    self["isarray"] = 1                            # and so is self
  creps_3:
    creps[cname] = self                            # mark us for future ref

  isarray_0:
    psave = pad["isarray"]                         # get current isarray status
    isarray = self["isarray"]                      # combine with group's
    isarray |= psave                               #
    self["isarray"] = isarray                      # and pass to nested
    pad["isarray"] = isarray
    $I0 = self["cscope"]                           # new scope resets
    unless $I0 goto isarray_1                      # array status
    pad["isarray"] = 0
    isarray = 0                                    # each capt obj is single
    delete pad["creps"]                            # new lexical name scope
  isarray_1:
    $I0 = defined self["exp1"]
    unless $I0 goto end
    exp1 = self["exp1"]
    exp1.analyze(next, pad)
    pad["isarray"] = psave

  fc:
    $I0 = self["min"]                              # set up firstchars
    if $I0 > 0 goto fc_2
    self.firstchars(exp1, next)
    goto end
  fc_2:
    self.firstchars(exp1)
  end:
    pad["creps"] = creps
    .return ()
.end

.sub "gen" method
    .param pmc code
    .param string label
    .param string next
    .local pmc emit
    .local pmc exp1
    .local int min, max, isgreedy, iscut, iscapture, isarray, issubrule
    .local string sublabel
    .local string captname, captattr, captattrtype
    .local string rname
    .local int myserno
    emit = find_global "PGE::Exp", "emit"
    (min, max, isgreedy, iscut) = self."_getattributes"()
    (sublabel, myserno) = self.serno()
    iscapture = defined self["cname"]
    isarray = self["isarray"]
    rname = self["rname"]
    issubrule = exists self["rname"]
    unless iscapture goto init
    $P0 = self["cname"]
    $I0 = isa $P0, "Integer"
    unless $I0 goto captsubrule
  captsubpat:
    captname = $P0
    captattr = "PGE::Match\\x0@:capt"
    captattrtype = "PerlArray"
    goto init
  captsubrule:
    captname = $P0
    $P1 = find_global "Data::Escape", "String"
    captname = $P1(captname, '"')
    captname = concat '"', captname
    captname = concat captname, '"'
    captattr = "PGE::Match\\x0%:capt"
    captattrtype = "Hash"

  init:
    emit(code, "\n  %s:", label)
    emit(code, "    $I0 = gpad[-1]")
    emit(code, "    if $I0 == %s goto %s_1", myserno, label)
    emit(code, "    $P0 = cpad[-1]")
    emit(code, "    push cpad, $P0")
    unless iscapture goto init_1
    emit(code, "    cobcapt = getattribute $P0, \"%s\"", captattr)
    emit(code, "    isnull cobcapt, %s_i1", label)
    emit(code, "    goto %s_i2", label)
    emit(code, "  %s_i1:", label)
    emit(code, "    cobcapt = new %s", captattrtype)
    emit(code, "    setattribute $P0, \"%s\", cobcapt", captattr)
    emit(code, "  %s_i2:", label)
    emit(code, "    iscreator = 0")
    emit(code, "    $I0 = defined cobcapt[%s]", captname)
    emit(code, "    if $I0 goto %s_i3", label)
    emit(code, "    capt = new PerlArray")
    emit(code, "    $P0 = new Integer")
    emit(code, "    $P0 = %s", isarray)
    emit(code, "    setprop capt, \"isarray\", $P0")
    emit(code, "    cobcapt[%s] = capt", captname)
    emit(code, "    iscreator = 1")
    emit(code, "  %s_i3:", label)
    emit(code, "    capt = cobcapt[%s]", captname)
  init_1:
    emit(code, "    push gpad, capt")
    emit(code, "    push gpad, 0")
    emit(code, "    push gpad, %s", myserno)
    emit(code, "    save iscreator")
    emit(code, "    save cobcapt")
    emit(code, "    bsr %s_1", label)
    emit(code, "    restore cobcapt")
    emit(code, "    restore iscreator")
    emit(code, "    $I0 = pop gpad")
    emit(code, "    $I0 = pop gpad")
    emit(code, "    $P0 = pop gpad")
    emit(code, "    $P0 = pop cpad")
    unless iscapture goto init_2
    emit(code, "    unless iscreator goto %s_i4", label)
    emit(code, "    delete cobcapt[%s]", captname)
    emit(code, "  %s_i4:", label)
  init_2:
    emit(code, "    unless cutting == %s goto fail", myserno)
    emit(code, "    cutting = 0")
    emit(code, "    goto fail")
    emit(code, "  %s_1:", label)
    emit(code, "    rep = gpad[-2]")
    if issubrule goto greedy
    unless iscapture goto greedy
    emit(code, "    capt = gpad[-3]")
    emit(code, "    unless rep > 0 goto %s_2", label)
    emit(code, "    $P0 = capt[-1]")
    emit(code, "    $P1 = getattribute $P0, \"PGE::Match\\x0$:pos\"")
    emit(code, "    $P1 = pos")
  greedy:
    emit(code, "  %s_2:", label)
    unless isgreedy goto lazy
    emit(code, "    if rep >= %s goto %s_g1", max, label)
    emit(code, "    inc rep")
    emit(code, "    gpad[-2] = rep")
    self.emitsub(code, sublabel, "pos", "rep")
  greedy_1:
    emit(code, "    dec rep")
    emit(code, "  %s_g1:", label)
    emit(code, "    if rep < %s goto fail", min)
    emit(code, "    $I0 = pop gpad")
    emit(code, "    $I0 = pop gpad")
    emit(code, "    $P0 = pop gpad")
    emit(code, "    $P0 = pop cpad")
    self.emitsub(code, next, "capt", "rep", "$P0", "NOCUT")
    emit(code, "    push cpad, $P0")
    emit(code, "    push gpad, capt")
    emit(code, "    push gpad, rep")
    emit(code, "    push gpad, %s", myserno)
    unless iscut goto greedy_2
    emit(code, "    cutting = %s", myserno)
  greedy_2:
    emit(code, "    goto fail")
    goto subpat
  lazy:
    emit(code, "    if rep < %s goto %s_l1", min, label)
    emit(code, "    $I0 = pop gpad")
    emit(code, "    $I0 = pop gpad")
    emit(code, "    $P0 = pop gpad")
    emit(code, "    $P0 = pop cpad")
    self.emitsub(code, next, "capt", "rep", "pos", "$P0", "NOCUT")
    emit(code, "    push cpad, $P0")
    emit(code, "    push gpad, capt")
    emit(code, "    push gpad, rep")
    emit(code, "    push gpad, %s", myserno)
    unless iscut goto lazy_1
    emit(code, "    cutting = %s", myserno)
    emit(code, "    goto fail")
  lazy_1:
    emit(code, "  %s_l1:", label)
    emit(code, "    if rep >= %s goto fail", max)
    emit(code, "    inc rep")
    emit(code, "    gpad[-2] = rep")
    self.emitsub(code, sublabel, "rep")
    emit(code, "    goto fail")

  subpat:
    if issubrule goto subrule
    if iscapture goto subpat_1
    exp1 = self["exp1"]
    exp1.gen(code, sublabel, label)
    goto end
  subpat_1:
    emit(code, "  %s:", sublabel)
    emit(code, "    $P0 = mob.\"newat\"(mob,pos)")
    $I0 = self["cscope"]
    unless $I0 goto subpat_2
    emit(code, "    cpad[-1] = $P0")
  subpat_2:
    emit(code, "    push capt, $P0")
    emit(code, "    save capt")
    emit(code, "    bsr %s_s1", sublabel)
    emit(code, "    restore capt")
    emit(code, "    $P0 = pop capt")
    emit(code, "    ret")
    exp1 = self["exp1"]
    concat sublabel, "_s1"
    exp1.gen(code, sublabel, label)
    goto end
  subrule:
    emit(code, "  %s:", sublabel)
    emit(code, "    saveall")
    $I0 = 0
  subrule_1:
    $I1 = index rname, '::', $I0                   # see if we have grammar
    if $I1 == -1 goto subrule_2
    $I0 = $I1 + 2
    goto subrule_1
  subrule_2:
    if $I0 == 0 goto subrule_simple_name
    $S0 = substr rname, $I0                        # get rule name
    $I0 -= 2
    $S1 = substr rname, 0, $I0                     # get grammar name
    emit(code, "    $P2 = getclass \"%s\"", $S1)
    emit(code, "    $P2 = $P2.\"newat\"(mob, pos)")
    emit(code, "    $P1 = find_method $P2, \"%s\"", $S0)
    goto subrule_invoke
  subrule_simple_name:
    emit(code, "    $P2 = mob.\"newat\"(mob, pos)")
    emit(code, "    errorsoff %s", .PARROT_ERRORS_GLOBALS_FLAG)
#    emit(code, "    $P1 = find_lex \"%s\"", rname)
#    emit(code, "    $I0 = defined $P1")
#    emit(code, "    if $I0 goto %s_s1", label)
    emit(code, "    $I0 = can $P2, \"%s\"", rname)
    emit(code, "    if $I0 goto %s_s0", label)
    emit(code, "    $P1 = find_global \"%s\"", rname)
    emit(code, "    goto %s_s1", label)
    emit(code, "  %s_s0:", label)
    emit(code, "    $P1 = find_method $P2, \"%s\"", rname)
    emit(code, "  %s_s1:", label)
  subrule_invoke:
    emit(code, "    $P0 = $P1($P2, pos)")
    emit(code, "    pos = $P0.to()")
    emit(code, "    save pos")
    emit(code, "    save $P0")
    emit(code, "    restoreall")
    emit(code, "    restore $P0")
    emit(code, "    restore pos")
    emit(code, "    unless $P0 goto %s_s4", label)
    unless iscapture goto subrule_3
    emit(code, "    push capt, $P0")
  subrule_3:
    emit(code, "  %s_s2:", label)
    self.emitsub(code, label, "pos", "$P0", "NOCUT")
    emit(code, "    unless cutting == 0 goto %s_s3", label)
    emit(code, "    saveall")
    emit(code, "    $P0.next()")
    emit(code, "    pos = $P0.to()")
    emit(code, "    save pos")
    emit(code, "    restoreall")
    emit(code, "    restore pos")
    emit(code, "    if $P0 goto %s_s2", label)
    emit(code, "  %s_s3:", label)
    unless iscapture goto subrule_4
    emit(code, "    $P1 = pop capt")
  subrule_4:
    emit(code, "  %s_s4:", label)
    emit(code, "    ret")
  end:
    .return ()
.end

=head1 AUTHOR

Patrick Michaud (pmichaud@pobox.com) is the author and maintainer.
Patches and suggestions should be sent to the Perl 6 compiler list
(perl6-compiler@perl.org).

=cut

