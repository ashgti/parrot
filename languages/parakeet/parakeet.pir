
# Parakeet is a stack machine language for the Parrot VM not entirely
# unlike the Forth programming language.  Parakeet is extremely simple
# and interactive.  Just run this file through Parrot and you will get
# an interpreter prompt.  From there, you can type in Parakeet code.


.macro IPROMPT "> " .endm
.macro CPROMPT "... " .endm

.const int Space = 32
.const int Tab = 9

# Registers used by machine.  These are hardwired to a register and
# all core words expect them in these places.  User words compiled in
# and executed save and restore these registers.

.macro STACK  P30 .endm     # the data stack
.macro CSTACK P29 .endm     # the control stack

.macro PIRC   P28 .endm     # the PIR compiler
.macro OBJ    P27 .endm     # a handy instance temp
.macro KLASS P26 .endm      # currently compiling class

.macro TOS P25 .endm        # Top Of Stack
.macro NOS P24 .endm        # Next On Stack

.macro STDIN P23 .endm      # standard input where source comes from
.macro INTS P22 .endm       # array of pre-created integers for speed
.macro FLAGS P21 .endm      # hash of flags for interpreter

.macro LEVEL I30 .endm      # current lexical level 0 is interpret > 0 is compile
.macro MODE I29 .endm       # current compile mode (func, class, meth)
.macro ITMP I28 .endm

.macro BODY S30 .endm       # Currently compiling word body
.macro CURR S29 .endm       # current word
.macro SRC  S28 .endm       # remaining source line to be interpreted
.macro NAMESPACE  S27 .endm # currently compiling namespace
.macro LOAD S26 .endm       # currently compiling module load body, non-interactive

.macro NumInts 10 .endm      # increase to turn on integer preallocation

.macro COMPILING .LEVEL > 0 .endm
.macro INTERPRETING .LEVEL == 0 .endm

.macro SAVEM  # this could probably be less bold...
  saveall
.endm

.macro RESTOREM
  restoreall
.endm

.macro SEEING
    .ITMP = .FLAGS["seeing"]
    unless .ITMP > 0 goto .$EndDebug
    print .BODY
    .local $EndDebug:
.endm

# compiling

.macro emit(s)
    concat .BODY, .s
.endm

.macro NEXT
    goto Next
.endm

.macro NEXTLINE goto NextLine .endm

# errors

.macro atLeastOne
    .ITMP = .STACK
    if .ITMP > 0 goto .$NoError
    new $P0, .Exception
    $P0["_message"] = "Stack is Empty!"
    throw $P0
    .local $NoError:
.endm

.macro atLeastTwo
    .ITMP = .STACK
    if .ITMP > 1 goto .$NoError
    new $P0, .Exception
    $P0["_message"] = "Stack is Empty!"
    throw $P0
    .local $NoError:
.endm

# be careful with the macros below, most of them trounce .TOS so don't
# rely on that register being preserved.

# stackies

.macro POP
  .atLeastOne
  pop .TOS, .STACK
.endm

.macro POP2
  .atLeastTwo
  pop .TOS, .STACK
  pop .NOS, .STACK
.endm

.macro PUSH
  push .STACK, .TOS
.endm

# bools

.macro TRUE
  .PUSHI(1)
.endm

.macro FALSE
  .PUSHI(0)
.endm

# ints

.macro getInt(x)
    lt .x, 0, .$createInt
    lt .x, .NumInts, .$fetchInt
  .local $createInt:
    .TOS = new .PerlInt
    .TOS = .x
    goto .$End
  .local $fetchInt:
    .TOS = .INTS[.x]
    clone .TOS, .TOS  # to prevent anyone modifying a "constant" int
  .local $End:
.endm

.macro PUSHI(x)
  .getInt(.x)
  .PUSH
.endm

# types of words

.macro __CORE 0 .endm
.macro __FUNC 1 .endm
.macro __VAR 2 .endm
.macro __CLASS 3 .endm
.macro __METH 4 .endm

# add words

.macro addCore(name, lbl, doc)
  $P1 = new .PerlString
  $P1 = .doc
  .getInt(.__CORE)
  $P0 = .TOS
  set_addr .ITMP, .lbl
  .getInt(.ITMP)
  setprop .TOS, "__type__", $P0
  setprop .TOS, "__doc__", $P1
  store_lex .LEVEL, .name, .TOS
.endm

.macro addVar(name, val, lv)
  .getInt(.__VAR)
  setprop .val, "__type__", .TOS
  store_lex .lv, .name, .val
.endm

.macro addClass(name, klz, lv)
  .getInt(.__CLASS)
  setprop .klz, "__type__", .TOS
  store_lex .lv, .name, .klz
.endm

.macro findWord(wrd)
  push_eh .$NotFound
  find_lex .TOS, .wrd
  clear_eh
  goto .$End
.local $NotFound:
    $P0 = new .Exception
    $P0["_message"] = "UnknownWord"
    throw $P0
.local $End:
.endm


# comparisons meta macros

# this hack on branching comparisons is so we can use PIR's .local
# macro target expansions. (way easier than keeping track of my own
# expansions)

.macro __EQ
    .POP2
    eq .NOS, .TOS, .$_IS
    .FALSE
    goto .$End
  .local $_IS:
    .TRUE
  .local $End:
.endm

.macro __NE
    .POP2
    ne .NOS, .TOS, .$_IS
    .FALSE
    goto .$End
  .local $_IS:
    .TRUE
  .local $End:
.endm

.macro __LT
    .POP2
    lt .NOS, .TOS, .$_IS
    .FALSE
    goto .$End
  .local $_IS:
    .TRUE
  .local $End:
.endm

.macro __GT
    .POP2
    gt .NOS, .TOS, .$_IS
    .FALSE
    goto .$End
  .local $_IS:
    .TRUE
  .local $End:
.endm

.macro __LE
    .POP2
    le .NOS, .TOS, .$_IS
    .FALSE
    goto .$End
  .local $_IS:
    .TRUE
  .local $End:
.endm

.macro __GE
    .POP2
    ge .NOS, .TOS, .$_IS
    .FALSE
    goto .$End
  .local $_IS:
    .TRUE
  .local $End:
.endm

.macro __ASSERT
    .POP
    if .TOS goto .$End
    $P0 = new .Exception
    $P0["_message"] = "AssertionError"
    throw $P0
  .local $End:
.endm


##########################################################
##
## Interpreter entry point.
##
##########################################################

.sub _main @MAIN
  .param PerlArray argv

  .STACK = new .PerlArray
  .CSTACK = new .PerlArray
  .INTS = new .PerlArray
  .FLAGS = new .PerlHash
  .BODY = ""
  .LOAD = ""

  .FLAGS["interactive"] = 1  # is the interpreter interactive?
  .FLAGS["seeing"] = 0       # print PIR blocks before compiling

  .ITMP = argv

  # name of the program
  .local string program_name
  program_name = shift argv

  if .ITMP > 1 goto OpenFile
  getstdin .STDIN

  # turn off output buffering

  getstdout $P0
  $P0."setbuf"(0)

  goto GotInput

  OpenFile:

  .FLAGS["interactive"] = 0
  .FLAGS["bye"] = 0

  $S0 = shift argv
  open .STDIN, $S0

  GotInput:

  .NAMESPACE = ""
  .LEVEL = 0

  newsub $P0, .Exception_Handler, _handler
  set_eh $P0

   new_pad .LEVEL

  compreg .PIRC, "PIR"

  # simple optimization hack, pre-create the first thousand or so
  # integer "constants".  See .getInt and .PUSHI.

  .ITMP = 0
  _fillInts:
    $P0 = new .PerlInt
    $P0 = .ITMP
    .INTS[.ITMP] = $P0
    inc .ITMP
    lt .ITMP, .NumInts, _fillInts

  # code objects

  .addCore("package", _PACKAGE, "Declare a package (unimplemented).")

  .addCore("func", _FUNC, "Defines a new function.")
  .addCore("class", _CLASS, "Defines a new class.")
  .addCore("meth", _METH, "Defines a new method.")
  .addCore("end", _END, "Ends the current function, class or method definition.")

  # inheritance and interfaces

  .addCore("extends", _EXTENDS, "Declare a superclass.")
  .addCore("implements", _IMPLEMENTS, "Declare a class implements an interface.")

  .addCore("attr", _ATTR, "Declare an attribute in a class.")
  .addCore("get", _GET, "Get an attribute from an object, push it on stack.")
  .addCore("set", _SET, "Set an attribute on an object.")

  # variables

  .addCore("var", _VAR, "Declare a local variable.")
  .addCore("self", _SELF, "Push self instance on stack.")

  # libraries

  .addCore("load-bytecode", _LOAD_BYTECODE, "Load bytecode from a Parrot file.")
  .addCore("include", _INCLUDE, "Include Parakeet code from another file.")

  # typish stuff

  .addCore("find-type", _FINDTYPE, "Pops type name and pushes type enumeration.")
  .addCore("new", _NEW, "Pops type enumeration and pushes new instance of that type.")
  .addCore("->", _BIND, "Binds an object to a word and executes it.")

  # wordish stuff

#  .addCore("lit", _LIT, "Pushes the next word literally.")
  .addCore("execute", _EXECUTE, "Pops code object and executes it.")

  # misc

  .addCore("bye", _BYE, "Exits Parakeet.")
  .addCore("see", _SEE, "Examine a word.")
  .addCore("seeing", _SEEING, "Turn on underlying PIR printing.")

  # debugging

  .addCore("debug", _DEBUG, "Toggle Parrot debugging information.")
  .addCore("debug-init", _DEBUG_INIT, "Init debugger.  Must call before other debug words")
  .addCore("debug-break", _DEBUG_BREAK, "Break into debugger.  Segfaults?")

  .addCore("bounds", _BOUNDS, "Toggle Parrot bounds checking.")
  .addCore("profile", _PROFILE, "Toggle Parrot profiling.")
  .addCore("trace", _TRACE, "Toggle Parrot instruction tracing.")

#   .addCore("interactive", _INTERACTIVE, "Swtiches to interactive mode.")
#   .addCore("non-interactive", _NINTERACTIVE, "Switches to non-interactive mode.")

  .addCore("assert", _ASSERT, "Throws error if top stack item is non-true.")

  # exceptions

  .addCore("throw", _THROW, "Pop exception and throw it.")

  # i/o

  .addCore("readline", _READLINE, "Read a line.")
  .addCore("read", _READ, "Read a character.")

  .addCore("print", _PRINT, "Pop stack item and print it.")
  .addCore("println", _PRINTLN, "Pop stack item and print it on own line.")

  .addCore("open", _OPEN, "unimplemented.")
  .addCore("close", _CLOSE, "unimplemented.")
  .addCore("peek", _PEEK, "unimplemented.")
  .addCore("stat", _STAT, "unimplemented.")
  .addCore("seek", _SEEK, "unimplemented.")
  .addCore("tell", _TELL, "unimplemented.")

  # stackies

  .addCore("drop", _DROP, "Pop and discard top of stack.")
  .addCore("dup", _DUP, "Duplicate top of stack.")
  .addCore("depth", _DEPTH, "Push the current stack depth.")

  # control flow

  .addCore("if", _IF, "Execute block if TOS is true.")
  .addCore("then", _THEN, "End of conditional block started with 'if'.")
  .addCore("else", _ELSE, "Begining of alternate conditional block started with 'if'.")
  .addCore("do", _DO, "Fast integer loop.")
  .addCore("loop", _LOOP, "Loop back to 'do'")
  .addCore("+loop", _PLOOP, "Loop back to do, incrementing index var by top of stack.")
  .addCore("for", _FOR, "Loop over iteration object on top of stack.")
  .addCore("next", _NEXT, "Loop back to 'for'.")

  # math

  .addCore("+", _ADD, "Add top stack items, push sum.")
  .addCore("-", _SUB, "Subtract top stack items, push difference.")
  .addCore("*", _MUL, "Multiple top stack items, push product.")
  .addCore("/", _DIV, "Divide top stack items, push result.")
  .addCore("%", _MOD, "Divide top stack items, push remainder.")

  # 'crementers

  .addCore("1+", _INC, "Increment top of stack.")
  .addCore("1-", _DEC, "Decrement top of stack.")

  # comparisons

  .addCore("cmp", _CMP, "Compare top two stack items.")
  .addCore("==", _EQ, "Compare top two stack items for equality.")
  .addCore("!=", _NE, "Compare top two stack items for inequality")
  .addCore(">", _GT, "Greater than comparison.")
  .addCore("<", _LT, "Less than comparison.")
  .addCore(">=", _GE, "Greater than or equal to comparison.")
  .addCore("<=", _LE, "Less than or equal to comparison.")

  # booleans

  .addCore("and", _AND, "Boolean And.")
  .addCore("or", _OR, "Boolean Or.")
  .addCore("xor", _XOR, "Boolean Xor.")
  .addCore("not", _NOT, "Boolean Not.")

  #######################################################
  # fetch a new line of code

  NextLine:
    .ITMP = length .BODY
    if .ITMP == 0 goto FreshLine
    if .COMPILING goto FreshLine
    .emit("invoke P1\n")
    .emit(".end\n")
    .SEEING
    compile $P0, .PIRC, .BODY
    .SAVEM
    invokecc $P0
    .RESTOREM
  FreshLine:
    .ITMP = .FLAGS["interactive"]
    unless .ITMP == 1 goto Prompted
    if .COMPILING goto CompilePrompt
    .ITMP = .STACK
    print .ITMP
    print .IPROMPT
    goto Prompted

  CompilePrompt:
    print .CPROMPT

  Prompted:
    .LOAD = ""
    unless .INTERPRETING goto Readline
    .BODY = ".sub xt\nnoop\n"
  Readline:
    readline .SRC, .STDIN
    length .ITMP, .SRC
    if .ITMP == 0 goto End
    dec .ITMP
    substr .SRC, .SRC, 0, .ITMP # slice off newline

  ########################################################
  # fetch the next word



  Next:
    .ITMP = .FLAGS["bye"]
    if .ITMP > 0 goto End
    .CURR = ""
    bsr CollectWord
    if .CURR == "" .NEXTLINE
    goto Comment

  Comment:
    substr $S0, .CURR, 0, 1
    unless $S0 == "#" goto Zero
    .NEXTLINE

  Zero:
    unless .CURR == "0" goto Integer
    .emit(".PUSHI(0)\n")
    goto Next

  Integer:
    .ITMP = .CURR
    unless .ITMP > 0 goto String
    .emit(".PUSHI(")
    .emit(.CURR)
    .emit(")\n")
    goto Next

  String:

    # This is lame-o.  figure out Rx4 to parse out string literals

    substr $S0, .CURR, 0, 1
    unless $S0 == "\"" goto Word
    substr .CURR, .CURR, 1              # slice leading quote
    substr $S0, .CURR, -1
#    print $S0
    unless $S0 == "\"" goto moreString  # trailing quote?
    chopn .CURR, 1                      # slice it
    goto gotString                      # we're done
  moreString:                           # otherwise,
    index $I1, .SRC, "\""               # search for trailing quote
    substr $S0, .SRC, 0, $I1, ""        # when found, slice rest of string off
    concat .CURR, $S0                   # and concat
#    print .CURR

  gotString:
    .emit(".TOS = new .PerlString\n.TOS = \"")
    .emit(.CURR)
    .emit("\"\n.PUSH\n")
    goto Next

  Word:
    newsub $P1, .Exception_Handler, _var_not_found
    set_eh $P1

    find_lex .TOS, .CURR
    goto FoundWord

  _var_not_found:
    clear_eh
    goto UnknownWord

  # at this point the word we are looking for is a lexical variable.
  # What to do with depends on it's "__type__" property.

  FoundWord:
    clear_eh
    getprop $P0, "__type__", .TOS

    if $P0 == .__CORE goto Core
    if $P0 == .__FUNC goto User
    if $P0 == .__VAR goto Variable
    if $P0 == .__CLASS goto Class

  Core:
    .ITMP = .TOS
    jump .ITMP

  User:
    .emit("find_lex .TOS, \"")
    .emit(.CURR)
    .emit("\"\n.SAVEM\ninvokecc .TOS\n.RESTOREM\n")
    goto Next

  Variable:
    .emit("find_lex .TOS, \"")
    .emit(.CURR)
    .emit("\"\n.PUSH\n")
    goto Next

  Class:
    .emit("find_type $I0, \"")
    .emit(.CURR)
    .emit("\"\n.PUSHI($I0)\n")
    goto Next

  UnknownWord:
    print "Unknown Word: "
    print .CURR
    print "\n"
    .LEVEL = 0
    .NEXTLINE

  _handler:
    print "An Exception was thrown: "
    set S0, P5["_message"]	# P5 is the exception object
    print S0
    print "\n"
    .LEVEL = 0
    .NEXTLINE

  End:
    # Current hack to really end the interpreter.
    # The problem is due to branching to different code segments.
    # Using "end" terminates the current run loop only, evaluating
    # compiled code branched to different code segments though, which
    # are ended but not the main one.
    # -leo
    clear_eh
    exit 0

#  I need an rx expert!
#
#  CollectWord:
#     pushi
#     $I0 = 0  # index
#     $I1 = 0  # start
#     $I2 = 0  # length

#     .CURR = ""

#     $I3 = .SRC
#     if $I3 > 0 goto begin_
#     goto end_

#   begin_:
#     rx_is_s .SRC, $I0, beginword_
#     goto begin_

#   beginword_:
#     $I1 = $I0
#   moreword_:
#     rx_is_s .SRC, $I0, advanceword_
#     $I2 = $I0 - $I1
#     dec $I2
#     substr .CURR, .SRC, $I1, $I2, ""
#     print "|"
#     print .CURR
#     print "|\n"
#     goto end_

#   advanceword_:
#     rx_advance .SRC, $I0, end_
#     goto moreword_

#   end_:
#     popi
#     ret

  CollectWord:
    .CURR = ""
    length $I1, .SRC
    eq $I1, 0, DoneWhite
    set $I3, 0
    ord $I2, .SRC, $I3
    eq $I2, Space, EatWhite
    eq $I2, Tab, EatWhite
    branch DoneWhite
  EatWhite:
    inc $I3
    eq $I1, $I3, FinishWhite
    ord $I2, .SRC, $I3
    eq $I2, Space, EatWhite
    eq $I2, Tab, EatWhite
  FinishWhite:
    substr .SRC, 0, $I3, ""
  DoneWhite:
    length $I1, .SRC
    eq $I1, 0, DoneCollectWord
    set $I3, 0
  NextChar:
    eq $I3, $I1, EndDark
    ord $I2, .SRC, $I3
    eq $I2, Space, EndDark
    eq $I2, Tab, EndDark
    inc $I3
    branch NextChar
  EndDark:
    substr .CURR, .SRC, 0, $I3, ""
  DoneCollectWord:
    ret

################################################################
# Core word definitions follow
################################################################

  _CLASS:
    bsr CollectWord
    unless .LEVEL > 0 goto _CLASS1
    save .KLASS
    save .MODE
  _CLASS1:

    .MODE = .__CLASS
    save .BODY

    inc .LEVEL
    new_pad .LEVEL

    newclass .KLASS, .CURR

    save .NAMESPACE

    .NAMESPACE = classname .KLASS
    .BODY = ".namespace[\""
    .emit(.NAMESPACE)
    .emit("\"]\n")
    .NEXT

  _FUNC:

    bsr CollectWord

    inc .LEVEL
   new_pad .LEVEL

    unless .LEVEL > 0 goto _FUNC1
    save .KLASS
    save .MODE
  _FUNC1:

    .MODE = .__FUNC
    save .BODY
    save .CURR

    .BODY = ".sub _"
    .emit(.CURR)
    .emit("\n")
    .NEXT

  _METH:
    bsr CollectWord

    inc .LEVEL
    new_pad .LEVEL

    save .MODE
    .MODE = .__METH

    .emit(".sub _")
    .emit(.CURR)
    .emit(" method\n")
    .emit("inc .LEVEL\nnew_pad .LEVEL\n")
    .NEXT

  _END:
    if .COMPILING goto _ENDFUNC
    print "Not inside a definition.\n"
    .NEXT

  _ENDFUNC:

    if .MODE == .__CLASS goto _ENDCLASS
    if .MODE == .__METH goto _ENDMETH

    unless .LEVEL == 0 goto _DOENDFUNC
    print "Nothing to end!\n"
    .NEXT
  _DOENDFUNC:

    .emit("invoke P1\n")
    .emit(".end\n")

    .SEEING
    compile $P0, .PIRC, .BODY
    $P1 = new .PerlString
    $P1 = .BODY

    setprop $P0, "__asm__", $P1

    dec .LEVEL
    peek_pad $P1
    pop_pad

#    setprop $P0, "__dict__", $P1

    restore .CURR
    restore .BODY

    .getInt(.__FUNC)
    setprop $P0, "__type__", .TOS

    store_lex .LEVEL, .CURR, $P0

    if .LEVEL == 0 goto _ENDFUNC1
    restore .KLASS
    restore .BODY
    restore .MODE

  _ENDFUNC1:
    .NEXT

  _ENDMETH:
    .emit("pop_pad\ndec .LEVEL\n")
    .emit(".pcc_begin_return\n.pcc_end_return\n")
    .emit(".end\n")
   pop_pad
    dec .LEVEL
    restore .MODE
    .NEXT

  _ENDCLASS:
    .SEEING
    compile $P0, .PIRC, .BODY
    $P1 = new .PerlString
    $P1 = .BODY

    setprop .KLASS, "__asm__", $P1
    setprop .KLASS, "__bytecode__", $P0

    restore .BODY

    dec .LEVEL
    peek_pad $P1
    pop_pad
    setprop .KLASS, "__dict__", $P1

    $P1 = new .PerlInt
    $P1 = .__CLASS
    setprop .KLASS, "__type__", $P1

    $S0 = classname .KLASS

    store_lex .LEVEL, $S0, .KLASS

    restore .NAMESPACE

    if .LEVEL == 0 goto _ENDCLASS1
    restore .KLASS
    restore .MODE
  _ENDCLASS1:
    .NEXT

  _PACKAGE:
    .NEXT

  _EXTENDS:
    bsr CollectWord
    getclass $P1, .CURR
    addparent .KLASS, $P1
    .NEXT

  _IMPLEMENTS:
    bsr CollectWord
#    adddoes .KLASS, .CURR
    .NEXT

  _ATTR:
    bsr CollectWord
    addattribute .KLASS, .CURR
    .NEXT

  _GET:
    bsr CollectWord
    .POP
#    class $P0, .TOS
#    classname $S0, $P0
#    concat $S0, "\0"
#    concat $S0, .CURR
    getattribute $P0, .TOS, .CURR
    .TOS = $P0
    .PUSH
    .NEXT

  _SET:
    bsr CollectWord
    .POP2
#    class $P0, .TOS
#    classname $S0, $P0
#    concat $S0, "\0"
#    concat $S0, .CURR
    setattribute .TOS, .CURR, .NOS
    .NEXT

  _SEE:
    unless .INTERPRETING goto _BLIND
    bsr CollectWord

    newsub $P1, .Exception_Handler, _word_not_found2
    set_eh $P1
    find_lex $P0, .CURR
    getprop $P1, "__type__", $P0

    if $P1 == .__CORE goto SeeCore
    if $P1 == .__FUNC goto SeeUser
    if $P1 == .__VAR goto SeeVariable
    if $P1 == .__CLASS goto SeeUser

  SeeCore:
    getprop $P1, "__doc__", $P0
    print "Core Word: "
    print $P1
    print "\n"
    clear_eh
    .NEXT

  SeeUser:
    getprop $P1, "__asm__", $P0
    print $P1
    clear_eh
    .NEXT

  SeeVariable:
    print .CURR
    print " is a variable.\n"
    print "\n"
    clear_eh
    .NEXT

  SeeClass:
    print .CURR
    print " is a class.\n"
    print "\n"
    clear_eh
    .NEXT

  _word_not_found2:
    print "Can't see word: "
    print .CURR
    print "\n"
    clear_eh
    .NEXT

  _BLIND:
    goto Next

  _LOAD_BYTECODE:
    .emit("load_bytecode \"")
    .emit(.CURR)
    .emit("\"\n")
    .NEXT

  _INCLUDE:
    .NEXT

  _SEEING:
    .emit(".POP\n$I0 = .TOS\n")
    .emit(".FLAGS[\"seeing\"] = $I0\n")
    .NEXT

  _TRACE:
    .emit(".POP\n")
    .emit("$I0 = .TOS\n")
    .emit("trace $I0\n")
    .NEXT

  _DEBUG:
    .emit(".POP\n")
    .emit("$I0 = .TOS\n")
    .emit("debug $I0\n")
    .NEXT

  _DEBUG_INIT:
    .emit("debug_init\n")
    .NEXT

  _DEBUG_BREAK:
    .emit("debug_break\n")
    .NEXT

  _BOUNDS:
    .emit(".POP\n")
    .emit("$I0 = .TOS\n")
    .emit("bounds $I0\n")
    .NEXT

  _PROFILE:
    .emit(".POP\n")
    .emit("$I0 = .TOS\n")
    .emit("profile $I0\n")
    .NEXT

#   _INTERACTIVE:
#     save .SRC
#     save .STDIN
#     getstdin .STDIN
#     .FLAGS["interactive"] = 1
#     .NEXT

#   _NINTERACTIVE:
#     .FLAGS["interactive"] = 0
#     restore .STDIN
#     restore .SRC
#     .NEXT

   _ASSERT:
     .emit(".__ASSERT\n")
     .NEXT

  _BYE:
    .emit(".FLAGS[\"bye\"] = 1\n")
    .NEXT

  _VAR:
    bsr CollectWord
    # Also stash the var at compile time, so that later references
    # to it can be resolved.

    .getInt(0)
    $P0 = .TOS
    .getInt(.__VAR)
    setprop $P0, "__type__", .TOS
    store_lex -1, .CURR, $P0

    .emit(".POP\n$P0 = .TOS\n.getInt(.__VAR)\nsetprop $P0, \"__type__\", .TOS\nstore_lex -1, \"")
    .emit(.CURR)
    .emit("\", $P0\n")

    .NEXT

  _IS:
    bsr CollectWord
    .NEXT

  _SELF:
    .emit(".TOS = self\n")
    .emit(".PUSH\n")
    .NEXT

  _THROW:
    .emit(".POP\n")
    .emit("throw .TOS\n")
    .NEXT

  _OPEN:
    .NEXT

  _CLOSE:
    .NEXT

  _READLINE:
    .emit(".TOS = new .PerlString\n")
    .emit("readline $S0, .STDIN\n")
    .emit(".TOS = $S0\n")
    .emit(".PUSH\n")
    .NEXT

  _READ:
    .emit(".TOS = new .PerlString\n")
    .emit("read $S0, .STDIN\n")
    .emit(".TOS = $S0\n")
    .emit(".PUSH\n")
    .NEXT

  _PEEK:
    .NEXT

  _STAT:
    .NEXT

  _SEEK:
    .NEXT

  _TELL:
    .NEXT

  _PRINT:
    .emit(".POP\n")
    .emit("print .TOS\n")
    .NEXT

  _PRINTLN:
    .emit(".POP\n")
    .emit("print .TOS\n")
    .emit("print \"\\n\"\n")
    .NEXT

  _IF:
    inc .LEVEL
    .emit(".POP\n")
    .emit("unless .TOS, endif")
    $S0 = .LEVEL
    .emit($S0)
    .emit("\n")
  _ENDIF:
    .NEXT

  _ELSE:
  _ENDELSE:
    .NEXT

  _THEN:
    .emit("endif")
    $S0 = .LEVEL
    .emit($S0)
    .emit(":\n")
    dec .LEVEL
  _ENDTHEN:
    .NEXT

  _DO:
     bsr CollectWord
    inc .LEVEL
    .emit(".POP2\n")
    .emit("push .CSTACK, .NOS\n")
    .emit("push .CSTACK, .TOS\n")
    .emit("$P0 = new .PerlInt\nstore_lex -1, \"")
    .emit(.CURR)
    .emit("\", $P0\n")
    .emit("find_lex $P0, \"")
    .emit(.CURR)
    .emit("\"\nassign $P0, .TOS\n")
    save .CURR

    $P0 = new .PerlInt
    .getInt(.__VAR)
    setprop $P0, "__type__", .TOS
    store_lex -1, .CURR, $P0

    .emit("do")
    $S0 = .LEVEL
    .emit($S0)
    .emit(":\n")
  _ENDDO:
    .NEXT

  _LOOP:
    .emit("pop .TOS, .CSTACK\n")
    .emit("pop .NOS, .CSTACK\n")
    .emit("inc .TOS\n")

    restore .CURR

    .emit("find_lex $P0, \"")
    .emit(.CURR)
    .emit("\"\nassign $P0, .TOS\n")
    .emit("push .CSTACK, .NOS\n")
    .emit("push .CSTACK, .TOS\n")
    .emit("ne .TOS, .NOS,  do")

    $S0 = .LEVEL

    .emit($S0)
    .emit("\n")
    .emit("pop .TOS, .CSTACK\n")
    .emit("pop .NOS, .CSTACK\n")

    dec .LEVEL
  _ENDLOOP:
    .NEXT

  _PLOOP:
    .emit("pop .TOS, .CSTACK\n")
    .emit("pop .NOS, .CSTACK\n")
    .emit("pop $P0, .STACK\n")
    .emit(".TOS = .TOS + $P0\n")

    restore .CURR

    .emit("find_lex $P0, \"")
    .emit(.CURR)
    .emit("\"\nassign $P0, .TOS\n")
    .emit("push .CSTACK, .NOS\n")
    .emit("push .CSTACK, .TOS\n")
    .emit("ne .TOS, .NOS,  do")

    $S0 = .LEVEL

    .emit($S0)
    .emit("\n")
    .emit("pop .TOS, .CSTACK\n")
    .emit("pop .NOS, .CSTACK\n")

    dec .LEVEL
  _ENDPLOOP:
    .NEXT

  _FOR:
  _ENDFOR:
    .NEXT

  _NEXT:
  _ENDNEXT:
    .NEXT


  _NEW:
    .emit(".POP\n")
    .emit("$I0 = .TOS\n")
    .emit(".TOS = new $I0\n")
    .emit(".PUSH\n")
    .NEXT

  _EXECUTE:
    .emit(".POP\n")
    .emit(".SAVEM\n")
    .emit("invokecc .TOS\n")
    .emit(".RESTOREM\n")
    .NEXT

  _BIND:
    bsr CollectWord
    .emit(".POP\n")
    .emit("$S0 = \"_\"\n")
    .emit("fetchmethod $P0, .TOS, \"_")
    .emit(.CURR)
    .emit("\"\n")
    .emit(".SAVEM\n")
    .emit("invokecc $P0\n")
    .emit(".RESTOREM\n")
    .NEXT


  _FINDTYPE:
    .emit(".POP\n")
    .emit("$S0 = .TOS\n")
    .emit("find_type $I0, $S0\n")
    .emit(".TOS = $I0\n")
    .emit(".PUSH\n")
    .NEXT

  _DROP:
    .emit(".POP\n")
    .NEXT

  _DUP:
    .emit(".POP\n")
    .emit(".PUSH\n")
    .emit(".PUSH\n")
    .NEXT

  _DEPTH:
    .emit("$I0 = .STACK\n")
    .emit(".PUSHI($I0)\n")
    .NEXT

  _ADD:
    .emit(".POP2\n")
    .emit(".TOS = .NOS + .TOS\n")
    .emit(".PUSH\n")
    .NEXT

  _SUB:
    .emit(".POP2\n")
    .emit(".TOS = .NOS - .TOS\n")
    .emit(".PUSH\n")
    .NEXT

  _MUL:
    .emit(".POP2\n")
    .emit(".TOS = .NOS * .TOS\n")
    .emit(".PUSH\n")
    .NEXT

  _DIV:
    .emit(".POP2\n")
    .emit(".TOS = .NOS / .TOS\n")
    .emit(".PUSH\n")
    .NEXT

  _MOD:
    .emit(".POP2\n")
    .emit(".TOS = .NOS % .TOS\n")
    .emit(".PUSH\n")
    .NEXT

  _INC:
    .emit(".POP\n")
    .emit("inc .TOS\n")
    .emit(".PUSH\n")
    .NEXT

  _DEC:
    .emit(".POP\n")
    .emit("dec .TOS\n")
    .emit(".PUSH\n")
    .NEXT

  _CMP:
    .emit(".POP2\n")
    .emit("cmp $I0, .NOS, .TOS\n")
    .emit(".PUSHI($I0)\n")
    .NEXT

  _EQ:
    .emit(".__EQ\n")
    .NEXT

  _NE:
    .emit(".__NE")
    .NEXT

  _LT:
    .emit(".__LT")
    .NEXT

  _GT:
    .emit(".__GT")
    .NEXT

  _LE:
    .emit(".__LE")
    .NEXT

  _GE:
    .emit(".__GE")
    .NEXT

  _AND:
    .emit(".POP2\n")
    .emit("and .TOS, .NOS, .TOS\n")
    .emit(".PUSH\n")
    .NEXT

  _OR:
    .emit(".POP2\n")
    .emit("or .TOS, .NOS, .TOS\n")
    .emit(".PUSH\n")
    .NEXT

  _XOR:
    .emit(".POP2\n")
    .emit("xor .TOS, .NOS, .TOS\n")
    .emit(".PUSH\n")
    .NEXT

  _NOT:
    .emit(".POP\n")
    .emit("not .TOS, .TOS\n")
    .emit(".PUSH\n")
    .NEXT
.end

