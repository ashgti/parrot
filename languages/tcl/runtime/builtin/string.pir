###
# [string]

.HLL 'Tcl', 'tcl_group'
.namespace

.sub '&string'
  .param pmc argv :slurpy

  .local pmc retval

  $I3 = argv
  unless $I3 goto no_args

  .local string subcommand_name
  subcommand_name = shift argv

  .local pmc options
  options = new .ResizablePMCArray
  push options, 'bytelength'
  push options, 'compare'
  push options, 'equal'
  push options, 'first'
  push options, 'index'
  push options, 'is'
  push options, 'last'
  push options, 'length'
  push options, 'map'
  push options, 'match'
  push options, 'range'
  push options, 'repeat'
  push options, 'replace'
  push options, 'reverse'
  push options, 'tolower'
  push options, 'toupper'
  push options, 'totitle'
  push options, 'trim'
  push options, 'trimleft'
  push options, 'trimright'
  push options, 'wordend'
  push options, 'wordstart'

  .local pmc select_option
  select_option  = get_root_global ['_tcl'], 'select_option'
  .local string canonical_subcommand
  canonical_subcommand = select_option(options, subcommand_name)


  .local pmc subcommand_proc
  null subcommand_proc

  subcommand_proc = get_root_global ['_tcl'; 'helpers'; 'string'], canonical_subcommand
  if_null subcommand_proc, bad_args 
  .return subcommand_proc(argv)

bad_args:
  .return ('') # once all commands are implemented, remove this...

no_args:
  tcl_error 'wrong # args: should be "string option arg ?arg ...?"'

.end

.HLL '_Tcl', ''

.namespace [ 'helpers'; 'string' ]

.sub 'first'
  .param pmc argv

  .local int argc
  .local pmc retval

  argc = argv
  if argc > 3 goto bad_args
  if argc < 2 goto bad_args
  $S1 = argv[0]
  $S2 = argv[1]
  $I0 = 0
  if argc == 2 goto first_do
  $S3 = argv[2]
  .local pmc __index
  __index = get_root_global ['_tcl'], '__index'
  $I0 = __index($S3,$S2)
  if $I0 >0 goto first_do
  $I0 = 0 # XXX should this be done in __index?

first_do:
  .local int index_1
  index_1 = index $S2, $S1, $I0
  .return(index_1)

bad_args:
  tcl_error 'wrong # args: should be "string first subString string ?startIndex?"'

.end

.sub 'last'
  .param pmc argv

  .local int argc
  .local pmc retval

  argc = argv
  if argc > 3 goto bad_args
  if argc < 2 goto bad_args
  $S1 = argv[0]
  $S2 = argv[1]
  
  $I0 = length $S2
  if argc == 2 goto last_do
  
  $S3 = argv[2]
  .local pmc __index
  __index = get_root_global ['_tcl'], '__index'
  $I1 = __index($S3,$S2)

  if $I1 > $I0 goto last_do
  $I0 = $I1

last_do:
  .local int index_1
  index_1 = index $S2, $S1, 0
  if index_1 > $I0 goto not_found
  if index_1 < 0   goto not_found

iterate:	
  $I1 = index_1
  $I2 = $I1 + 1
  index_1 = index $S2, $S1, $I2
  if index_1 < 0   goto return
  if index_1 > $I0 goto return
  goto iterate

return:	
  .return($I1)

not_found:
  .return(-1)
  
bad_args:
  tcl_error 'wrong # args: should be "string last subString string ?startIndex?"'
.end

.sub 'index'
  .param pmc argv

  .local int index_1
  .local pmc retval
  .local int argc
  argc = elements argv
  if argc != 2 goto bad_index
  $S1 = argv[0]
  $S2 = argv[1]
  .local pmc __index
  __index = get_root_global ['_tcl'], '__index'
  $I0 = __index($S2,$S1)
  index_1 = length $S1
  inc index_1
  if $I0 > index_1 goto index_null
  if $I0 < 0 goto index_null
  $S0 = substr $S1, $I0, 1
  .return ($S0)

index_null:
  .return ('')

bad_index:
  tcl_error 'wrong # args: should be "string index string charIndex"'

done:
  .return (retval)
.end


.sub 'tolower'
  .param pmc argv

  .local int argc
  .local pmc retval

  argc = argv
  if argc > 3 goto bad_args
  if argc < 1 goto bad_args
  
  $S1 = argv[0]
  $I1 = length $S1 # it will be useful

  # If no range is specified, do to all the string
  $I2 = 0
  $I3 = $I1
  if argc == 1 goto tolower_do

  .local pmc __index
  __index = get_root_global ['_tcl'], '__index'

  $S2 = argv[1]
  $I2 = __index($S2, $S1)
  # if just the first is specified, the last is the same (tclsh says so)
  $I3 = $I2
  if argc == 2 goto tolower_do
  
  $S3 = argv[2]
  $I3 = __index($S3, $S1)

tolower_do:
  if $I2 > $I1  goto tolower_return
  if $I3 <= $I1 goto tolower_start
  $I3 = $I1

tolower_start:
  $I4 = $I3 - $I2
  $I4+= 1
  $S2 = substr $S1, $I2, $I4
  downcase $S2
  substr $S1, $I2, $I4, $S2

tolower_return:	
  .return($S1)

bad_args:
  tcl_error 'wrong # args: should be "string tolower string ?first? ?last?"'

.end




.sub 'toupper'
  .param pmc argv

  .local int argc
  .local pmc retval

  argc = argv
  if argc > 3 goto bad_args
  if argc < 1 goto bad_args
  
  $S1 = argv[0]
  $I1 = length $S1 # it will be useful

  # If no range is specified, do to all the string
  $I2 = 0
  $I3 = $I1
  if argc == 1 goto toupper_do

  .local pmc __index
  __index = get_root_global ['_tcl'], '__index'

  $S2 = argv[1]
  $I2 = __index($S2, $S1)
  # if just the first is specified, the last is the same (tclsh says so)
  $I3 = $I2
  if argc == 2 goto toupper_do
  
  $S3 = argv[2]
  $I3 = __index($S3, $S1)

toupper_do:
  if $I2 > $I1  goto toupper_return
  if $I3 <= $I1 goto toupper_start
  $I3 = $I1

toupper_start:
  $I4 = $I3 - $I2
  $I4+= 1
  $S2 = substr $S1, $I2, $I4
  upcase $S2
  substr $S1, $I2, $I4, $S2

toupper_return:	
  .return($S1)

bad_args:
  tcl_error 'wrong # args: should be "string toupper string ?first? ?last?"'

.end



.sub 'totitle'
  .param pmc argv

  .local int argc
  .local pmc retval

  argc = argv
  if argc > 3 goto bad_args
  if argc < 1 goto bad_args
  
  $S1 = argv[0]
  $I1 = length $S1 # it will be useful

  # If no range is specified, do to all the string
  $I2 = 0
  $I3 = $I1
  if argc == 1 goto totitle_do

  .local pmc __index
  __index = get_root_global ['_tcl'], '__index'

  $S2 = argv[1]
  $I2 = __index($S2, $S1)
  # if just the first is specified, the last is the same (tclsh says so)
  $I3 = $I2
  if argc == 2 goto totitle_do
  
  $S3 = argv[2]
  $I3 = __index($S3, $S1)

totitle_do:
  if $I2 > $I1  goto totitle_return
  if $I3 <= $I1 goto totitle_start
  $I3 = $I1

totitle_start:
  $I4 = $I3 - $I2
  $I4+= 1
  $S2 = substr $S1, $I2, $I4
  titlecase $S2
  substr $S1, $I2, $I4, $S2

totitle_return:	
  .return($S1)

bad_args:
  tcl_error 'wrong # args: should be "string totitle string ?first? ?last?"'

.end



.sub 'bytelength'
  .param pmc argv

  .local int argc
  argc = argv
  if argc != 1 goto bad_length
  $S0 = argv[0]
  $I0 = bytelength $S0
  .return($I0)

bad_length:
  tcl_error 'wrong # args: should be "string bytelength string"'
.end

.sub 'length'
  .param pmc argv

  .local int argc
  argc = argv
  if argc != 1 goto bad_length

  $S1 = argv[0]
  $I0 = length $S1
  .return($I0)

bad_length:
  tcl_error 'wrong # args: should be "string length string"'
.end

.sub 'range'
  .param pmc argv

  .local int argc
  argc = elements argv
  if argc != 3 goto bad_range

  .local string teh_string, first_s, last_s, result
  teh_string = shift argv
  result = '' # default result
  first_s = shift argv
  last_s  = shift argv

  .local int last_index
  last_index = length teh_string
  dec last_index

  .local pmc __index
  __index = get_root_global ['_tcl'], '__index'

  .local int first_i, last_i
  first_i = __index(first_s, teh_string)
  last_i  = __index(last_s, teh_string)

  if first_i > last_i goto done

  if first_i >= 0  goto range_top
  first_i = 0
range_top:
  if last_i <= last_index goto range_do
  last_i = last_index
range_do:
  .local int repl_len
  repl_len = last_i - first_i
  inc repl_len

  result = substr teh_string, first_i, repl_len

done:
  .return(result)

bad_range:
  tcl_error 'wrong # args: should be "string range string first last"'
.end

.sub 'match'
  .param pmc argv

  .local int argc
  argc = argv

  .local int nocase
  nocase = 0
  if argc == 2 goto match_next
  if argc < 2 goto bad_match
  if argc > 3 goto bad_match
  $S0 = shift argv
  if $S0 == '-nocase' goto set_nocase
  if $S0 == '-nocas' goto set_nocase
  if $S0 == '-noca' goto set_nocase
  if $S0 == '-noc' goto set_nocase
  if $S0 == '-no' goto set_nocase
  if $S0 == '-n' goto set_nocase
  goto bad_option

set_nocase:
  nocase = 1

match_next:
  .local string pattern
  .local string the_string

  pattern = argv[0]
  the_string = argv[1]
  unless nocase goto match_continue
  pattern = downcase pattern

  the_string = downcase the_string

match_continue:
  .local pmc globber
  globber = compreg 'PGE::Glob'

  .local pmc rule, match 
  rule = globber.'compile'(pattern)
  match = rule(the_string)

  $I0 = istrue match
  .return ($I0)

bad_option:
  $S1 = 'bad option "'
  $S1 .= $S0
  $S1 .= '": must be -nocase'
  tcl_error $S1

bad_match:
  tcl_error 'wrong # args: should be "string match ?-nocase? pattern string"'
.end

.sub 'repeat'
  .param pmc argv

  .local int argc
  argc = argv

  if argc != 2 goto bad_repeat
  .local string the_string
  .local pmc    the_repeat
  the_string = argv[0]
  the_repeat = argv[1]
 
  .local pmc __integer
  __integer = get_root_global ['_tcl'], '__integer'
  the_repeat = __integer(the_repeat)

  $I0 = the_repeat
  if $I0 <= 0 goto nothing
  $S0 = repeat the_string, $I0
  .return($S0)

nothing:
  .return('')

bad_repeat:
  tcl_error 'wrong # args: should be "string repeat string count"'
.end

.sub 'map'
  .param pmc argv

  .local int argc
  argc = argv
  if argc == 0 goto bad_args
  if argc > 3 goto bad_args
  .local int nocase
  nocase = 0
  if argc == 2 goto setup
  $S0 = shift argv
  if $S0 != '-nocase' goto bad_option
  nocase = 1

setup:
  .local string the_string,mapstr,teststr,replacementstr
  .local pmc map_list
  .local int strpos,strlen,mappos,maplen,skiplen,mapstrlen,replacementstrlen

  .local pmc __list
  __list = get_root_global ['_tcl'], '__list'

  $P0 = argv[0]
  map_list = __list($P0)
  the_string = argv[1]

  maplen = map_list
  $I1 = maplen % 2
  if $I1 goto oddly_enough

  strpos = 0

outer_loop:
  strlen = length the_string
  if strpos >= strlen goto outer_done
  skiplen = 1
  mappos = 0

inner_loop:
  if mappos >= maplen goto inner_done
  mapstr = map_list[mappos]
  mapstrlen = length mapstr
  if mapstrlen == 0 goto inner_next

  teststr = substr the_string, strpos, mapstrlen
  # if nocase, tweak 'em both to lc.
  if nocase == 0 goto test
  downcase teststr
  downcase mapstr
test:
  if teststr != mapstr goto inner_next
  $I0 = mappos + 1
  replacementstr = map_list [ $I0 ]
  substr the_string, strpos, mapstrlen,replacementstr
  skiplen = length replacementstr
  goto outer_next

inner_next:
  mappos += 2
  goto inner_loop

inner_done:
outer_next:
  strpos += skiplen
  goto outer_loop

outer_done:
  .return (the_string)

oddly_enough:
  tcl_error 'char map list unbalanced'

bad_option:
  $S1 = 'bad option "'
  $S1 .= $S0
  $S1 .= '": must be -nocase'
  tcl_error $S1

bad_args:
  tcl_error 'wrong # args: should be "string map ?-nocase? charMap string"'
.end

.sub 'equal'
  .param pmc argv
  .local int argc
  argc = argv
  
  .local string a, b
  .local int length, nocase
  nocase = 0
  length = -1

  if argc < 2 goto bad_args
  if argc == 2 goto flags_done

  .local string flag
flag_loop:
  flag = shift argv
  if flag == '-length' goto got_length
  if flag == '-nocase' goto got_nocase
  branch bad_args

got_length:
  length = shift argv
  branch gotten
got_nocase:
  nocase = 1
gotten:

  argc = argv
  if argc == 2 goto flags_done
  if argc < 2 goto bad_args
  branch flag_loop
flag_end:

flags_done:
  a = shift argv
  b = shift argv

  unless nocase goto skip_lower
    downcase a
    downcase b
skip_lower:

  if length == -1 goto skip_shorten
    a = substr a, 0, length
    b = substr b, 0, length
skip_shorten:

check:
  if a == b goto ret_one
  .return (0)
ret_one:
  .return (1)

bad_args:
  tcl_error 'wrong # args: should be "string equal ?-nocase? ?-length int? string1 string2"'

.end


# RT#40770: doesn't respect the -strict or -failindex switches
.sub 'is'
  .param pmc argv
  .local int argc
  argc = argv

  .local int strict
  strict = 0

  if argc < 2 goto bad_args

  .local int the_cclass

  .local string class,the_string
  class = argv[0]
  the_string = argv[1]

  .local pmc options
  options = new .ResizablePMCArray
  push options, 'alnum'
  push options, 'alpha'
  push options, 'ascii'
  push options, 'control'
  push options, 'boolean'
  push options, 'digit'
  push options, 'double'
  push options, 'false'
  push options, 'graph'
  push options, 'integer'
  push options, 'list'
  push options, 'lower'
  push options, 'print'
  push options, 'punct'
  push options, 'space'
  push options, 'true'
  push options, 'upper'
  push options, 'wideinteger'
  push options, 'wordchar'
  push options, 'xdigit'

  .local pmc select_option
  select_option  = get_root_global ['_tcl'], 'select_option'
  class = select_option(options, class, 'class')

  # RT#40771: Mdiep will probably want to change this to a hash-dispatch
  if class == 'alnum' goto alnum_check
  if class == 'alpha' goto alpha_check
  if class == 'ascii' goto ascii_check
  if class == 'control' goto control_check
  if class == 'boolean' goto boolean_check
  if class == 'digit' goto digit_check
  if class == 'double' goto double_check
  if class == 'false' goto false_check
  if class == 'graph' goto graph_check
  if class == 'integer' goto integer_check
  if class == 'list' goto list_check
  if class == 'lower' goto lower_check
  if class == 'print' goto print_check
  if class == 'punct' goto punct_check
  if class == 'space' goto space_check
  if class == 'true' goto true_check
  if class == 'upper' goto upper_check
  if class == 'wideinteger' goto integer_check # XXX implement this check
  if class == 'wordchar' goto wordchar_check
  if class == 'xdigit' goto xdigit_check

alnum_check:
  the_cclass = .CCLASS_ALPHANUMERIC
  goto cclass_check
alpha_check:
  the_cclass = .CCLASS_ALPHABETIC
  goto cclass_check
ascii_check:
  goto bad_args # RT#40772:
control_check:
  the_cclass = .CCLASS_CONTROL
  goto cclass_check
boolean_check:
  if the_string == 'yes' goto yep 
  if the_string == 'no' goto yep 
  if the_string == '1' goto yep 
  if the_string == '0' goto yep 
  if the_string == 'true' goto yep 
  if the_string == 'tru'  goto yep 
  if the_string == 'tr'   goto yep 
  if the_string == 't'    goto yep 
  if the_string == 'false' goto yep 
  if the_string == 'fals'  goto yep 
  if the_string == 'fal'   goto yep 
  if the_string == 'fa'    goto yep 
  if the_string == 'f'     goto yep 
  goto nope 
digit_check:
  the_cclass = .CCLASS_NUMERIC
  goto cclass_check
double_check:
  $P1 = get_root_global ['_tcl'], '__number'
  push_eh nope
    $P2 = $P1(the_string)
  clear_eh

  $I0 = typeof $P2
  if $I0 == .TclFloat goto yep
  if $I0 == .TclInt   goto yep
  goto nope
false_check:
  if the_string == 'no' goto yep 
  if the_string == '0' goto yep 
  if the_string == 'false' goto yep 
  if the_string == 'fals'  goto yep 
  if the_string == 'fal'   goto yep 
  if the_string == 'fa'    goto yep 
  if the_string == 'f'     goto yep 
  goto nope 
graph_check:
  the_cclass = .CCLASS_GRAPHICAL
  goto cclass_check
integer_check:
  $P1 = get_root_global ['_tcl'], '__number'
  push_eh nope
    $P2 = $P1(the_string)
  clear_eh

  $I0 = typeof $P2
  if $I0 == .TclInt goto yep
  goto nope
list_check:
  $P1 = get_root_global ['_tcl'], '__list'
  push_eh nope
    $P1(the_string)
  clear_eh
  goto yep
lower_check:
  the_cclass = .CCLASS_LOWERCASE
  goto cclass_check
print_check:
  the_cclass = .CCLASS_PRINTING
  goto cclass_check
punct_check:
  the_cclass = .CCLASS_PUNCTUATION
  goto cclass_check
space_check:
  the_cclass = .CCLASS_WHITESPACE
  goto cclass_check
true_check:
  if the_string == 'yes' goto yep 
  if the_string == '1' goto yep 
  if the_string == 'true' goto yep 
  if the_string == 'tru'  goto yep 
  if the_string == 'tr'   goto yep 
  if the_string == 't'    goto yep 
  goto nope 
upper_check:
  the_cclass = .CCLASS_UPPERCASE
  goto cclass_check
wordchar_check:
  the_cclass = .CCLASS_WORD
  goto cclass_check
xdigit_check:
  the_cclass = .CCLASS_HEXADECIMAL
  goto cclass_check

cclass_check:
  # Loop over the string. Die immediately if we fail.
  # RT#40773: Tie the index of the string into --failvar
  .local int len,ii
  len = length the_string
  ii = 0
loop:
  if ii == len goto yep
  $I0 = is_cclass the_cclass, the_string, ii
  unless $I0 goto nope
  inc ii
  goto loop

yep:
  .return(1)

nope:
  .return(0)

bad_args:
  tcl_error 'wrong # args: should be "string is class ?-strict? ?-failindex var? str"'
.end

                           
.sub 'replace'
  .param pmc argv

  .local int argc
  .local int low
  .local int high
  .local int len
  .local pmc retval

  .local pmc __index
  __index = get_root_global ['_tcl'], '__index'

  argc = argv
  if argc > 4 goto bad_args
  if argc < 3 goto bad_args

  .local string low_s, high_s, the_string
  .local int string_len
 
  the_string = argv[0]
  string_len = length the_string
  $S4 = ''
         
  low_s = argv[1]
  low = __index(low_s, the_string)

  if low >= string_len goto replace_done

  high_s = argv[2]
  high = __index(high_s, the_string)

  if high < low goto replace_done

  if low >= 0 goto low_ok
  low = 0

low_ok:
  len = length the_string
  if high <= len goto high_ok
  high = len

high_ok:        
  if argc == 1 goto replace_do
  $S4 = argv[3]

replace_do:
  len = high - low
  len += 1
  substr the_string, low, len, $S4         

replace_done:   
  .return(the_string)

bad_args:
  tcl_error 'wrong # args: should be "string replace string first last ?string?"'
.end

         
.sub 'trimleft'
  .param pmc argv

  .local int argc
  .local pmc retval

  argc = argv
  if argc > 2 goto bad_args
  if argc < 1 goto bad_args
  
  $S1 = argv[0]
  $S2 = " \t\r\n"

  if argc == 1 goto trimleft_do

  $S2 = argv[1]

trimleft_do:
  .local string char
  $I1 = length $S1
  unless $I1 goto trimleft_done

  char = substr $S1, 0, 1
  $I1 = index $S2, char

  if $I1 < 0 goto trimleft_done
  substr $S1, 0, 1, ''
  goto trimleft_do
         
trimleft_done:  
  .return($S1)

bad_args:
  tcl_error 'wrong # args: should be "string trimleft string ?chars?"'

.end


                  
.sub 'trimright'
  .param pmc argv

  .local int argc
  .local pmc retval

  argc = argv
  if argc > 2 goto bad_args
  if argc < 1 goto bad_args
  
  $S1 = argv[0]
  $S2 = " \t\r\n"

  if argc == 1 goto trimright_do

  $S2 = argv[1]

trimright_do:
  .local string char
  $I1 = length $S1
  unless $I1 goto trimright_done

  char = substr $S1, -1, 1
  $I1 = index $S2, char

  if $I1 < 0 goto trimright_done
  chopn $S1, 1
  goto trimright_do
         
trimright_done:  
  .return($S1)

bad_args:
  tcl_error 'wrong # args: should be "string trimright string ?chars?"'

.end

# here, I might use trimleft and trim right, but I think it is
# better to implement it here as it should be faster
                  
.sub 'trim'
  .param pmc argv

  .local int argc
  .local pmc retval

  argc = argv
  if argc > 2 goto bad_args
  if argc < 1 goto bad_args
  
  $S1 = argv[0]
  $S2 = " \t\r\n"

  if argc == 1 goto trim_do1

  $S2 = argv[1]

trim_do1:
  .local string char

  $I1 = length $S1
  unless $I1 goto trim_do2

  char = substr $S1, -1, 1
  $I1 = index $S2, char

  if $I1 < 0 goto trim_do2
  chopn $S1, 1
  goto trim_do1

trim_do2:       
  $I1 = length $S1
  unless $I1 goto trim_done

  char = substr $S1, 0, 1
  $I1 = index $S2, char

  if $I1 < 0 goto trim_done
  substr $S1, 0, 1, ''
  goto trim_do2
         
trim_done:  
  .return($S1)

bad_args:
  tcl_error 'wrong # args: should be "string trim string ?chars?"'

.end
                  
.sub 'compare'
  .param pmc argv

  .local int argc
  .local pmc retval
  .local int size

  size = -1
  argc = argv

  if argc < 1 goto bad_args
  
  $S2 = pop argv
  $S1 = pop argv

args_processment:       
  argc = argv
  if argc == 0 goto args_processed
  $S4 = shift argv
  if $S4 == '-nocase' goto arg_nocase
  if $S4 == '-length' goto arg_length
  goto bad_args

args_processed:
  if $S1 == $S2 goto equal         
  if $S1 < $S2 goto smaller
  .return(1)

smaller:        
  .return(-1)

equal:
  .return(0)         
         
arg_nocase:
  downcase $S1
  downcase $S2
  goto args_processment

arg_length:
  if size != -1 goto bad_args         
  argc = elements argv
  if argc == 0 goto bad_args

  .local pmc __integer
  __integer = get_root_global ['_tcl'], '__integer'
  $S4  = shift argv
  size = __integer($S4)
  # "if -length is negative, it is ignored"
  if size < 0 goto args_processment         
  $S1 = substr $S1, 0, size
  $S2 = substr $S2, 0, size
  goto args_processment
         
bad_args:
  tcl_error 'wrong # args: should be "string compare ?-nocase? ?-length int? string1 string2"'

.end

.sub 'reverse'
  .param pmc argv
 
  .local int argc
  argc = elements argv
  if argc != 1 goto bad_args
 
  $S0 = shift argv
  $P0 = new .TclString
  $S0 = $P0.'reverse'($S0) 
  .return ($S0)

bad_args:
  tcl_error 'wrong # args: should be "string reverse string"'
.end

.sub 'wordend'
  .param pmc argv

  .local int argc
  argc = elements argv
  if argc != 2 goto bad_args

  .local string str
  .local int    idx
  str = argv[0]
  idx = argv[1]

  .local pmc __index
  __index = get_root_global ['_tcl'], '__index'
  idx = __index(idx, str)

  $I0 = length str
  $I0 -= idx

  $I0 = find_not_cclass .CCLASS_WORD, str, idx, $I0
  unless $I0 == idx goto return
  inc $I0

return:
  .return($I0)

bad_args:
  tcl_error 'wrong # args: should be "string wordend string index"'
.end

.sub 'wordstart'
  .param pmc argv

  .local int argc
  argc = elements argv
  if argc != 2 goto bad_args

  .local string str
  .local pmc    idx
  str = argv[0]
  idx = argv[1]

  .local pmc __index
  __index = get_root_global ['_tcl'], '__index'
  idx = __index(idx, str)
  
  .local int pos
  pos = idx
  # XXX should these checks be in __index itself?
  if pos >0 goto check_upper
  pos = 0
  goto pre_loop
check_upper:
  $I1 = length str
  dec $I1
  if pos <= $I1 goto pre_loop
  pos = $I1
pre_loop:
  .local int old_idx
  old_idx = pos
loop:
  if pos < 0 goto loop_done

  $I1 = is_cclass .CCLASS_WORD, str, pos
  unless $I1 goto loop_done

  dec pos
  goto loop
loop_done:
  if pos == old_idx goto ret_val
  inc pos
ret_val:  
  .return(pos)

bad_args:
  tcl_error 'wrong # args: should be "string wordstart string index"'
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
