# Copyright: 2002-2005 The Perl Foundation.  All Rights Reserved.
# $Id$

.sub main @MAIN 
  .param pmc argv

  load_bytecode "Getopt/Long.pbc"
  .local pmc get_options
  find_global get_options, "Getopt::Long", "get_options"

  # Assemble specification for get_options
  # in an array of format specifiers
  .local pmc opt_spec    
  opt_spec = new ResizableStringArray
  push opt_spec, "language=s"

  # the program name is the first element in argv
  .local string program_name
  program_name = shift argv

  # Make a copy of argv, because this can easier be handled in get_options()
  # TODO: remove need for cloning
  .local pmc argv_clone
  argv_clone = clone argv

  .local pmc opt
  ( opt ) = get_options( argv_clone, opt_spec )

  # Now we do what the passed options tell
  .local int is_defined

  # Was '--language' passed ?
  is_defined = defined opt["language"]
  if is_defined goto HAS_LANGUAGE_PARAM
    print "Please pass --language=<language>\n"
    end
  HAS_LANGUAGE_PARAM:

  .local string language
  language = opt["language"]

  # Get the input as a string, don't care about buffer overflow yet
  read S0, 1000000

  # Assume that the input is PASM and compile it
  compreg P1, language
  P0 = P1( S0 )

  # invoke the compiled code
  P0()

.end
