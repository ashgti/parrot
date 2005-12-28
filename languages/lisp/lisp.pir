
.const int INVALID_CHAR	    = 0
.const int CONSTITUENT_CHAR = 1
.const int WHITESPACE_CHAR  = 2
.const int TERM_MACRO_CHAR  = 3
.const int NTERM_MACRO_CHAR = 4
.const int SESCAPE_CHAR     = 5
.const int MESCAPE_CHAR     = 6

.include "include/macros.pir"

.sub _main @MAIN
  .param pmc argv
  .local string names
  .local pmc symbol
  .local pmc stream
  .local pmc name
  .local pmc args
  .local pmc retv
  .local int argc
  .local int res

   _init_types()			# Initialize all the type classes.

   _init_cl()				# Initialize the built-in functions in
   _init_system()			# the SYSTEM and COMMON-LISP packages.

   new_pad 0				# Create the null lexical environment.


  .STRING(name, "lisp/bootstrap.l")	# Load the lisp bootstrap file.
  .LIST_1(args, name)
   _load(args)

   argc = argv

   if argc <= 1 goto READ_STDIN		# Check if we should read STDIN or load from file

   names = argv[1]			# Get the file name
  .STRING(name, names)

  .LIST_1(args, name)
   retv = _load(args)			# Load the specified file.

   end

READ_STDIN:
   symbol = _LOOKUP_GLOBAL("COMMON-LISP", "*STANDARD-INPUT*")
   stream = symbol._get_value()

   push_eh DEBUGGER			# Setup error handler for debug loop.
REP_LOOP:
   print "-> "				# Display the top level prompt.

  .LIST_1(args, stream)			# Read!
   retv = _read(args)

  .LIST_1(args, retv)			# Eval!
   retv = _eval(args)

   foldup retv

  .local int nretv
   nretv = retv

  .local pmc tmpval
  .local int i
   i = 0

PRINT_LOOP:
   tmpval = retv[i]

   print tmpval

   inc i
   if i == nretv goto PRINT_DONE

   print " ;\n"

   goto PRINT_LOOP

PRINT_DONE:
   print "\n"

   goto REP_LOOP

DEBUGGER:
  .local string message
  .local string msgtype
  .local pmc e

   e = P5

   message = e["_message"]

   print "*** ERROR: "
   print message
   print "\n"

   push_eh DEBUGGER

   goto REP_LOOP
.end

.include "types.pir"
.include "read.pir"
.include "eval.pir"
.include "system.pir"
.include "validate.pir"
.include "cl.pir"
.include "internals.pir"
