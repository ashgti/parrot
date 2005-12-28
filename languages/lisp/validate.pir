
.sub _VALIDATE_TOKEN
  .param string token
  .local string pkgname
  .local string symname
  .local pmc package
  .local pmc symbol
  .local pmc retv
  .local int flag
  .local pmc nil

  flag = _IS_INTEGER(token)		# attempt to parse token as an integer
  if flag goto INTEGER

  flag = _IS_FLOAT(token)		# attempt to parse token as a float
  if flag goto FLOAT

  goto QUALIFIED_SYMBOL			# else interpret it as a symbol

INTEGER:
  .INTEGER(retv,token)			# create a LispInteger object
  goto DONE

FLOAT:
  .FLOAT(retv,token)			# create a ListFloat object
  goto DONE

QUALIFIED_SYMBOL:
  (flag,pkgname,symname) = _IS_QUALIFIED(token)
  if flag != 1 goto SYMBOL

  retv = _LOOKUP_GLOBAL(pkgname, symname)
  if_null retv, SYMBOL_NOT_FOUND
  goto DONE

SYMBOL:
  symbol = _LOOKUP_GLOBAL("COMMON-LISP", "*PACKAGE*")
  if_null symbol, PACKAGE_NOT_FOUND

  package = symbol._get_value()		# Get the current package
  if_null package, PACKAGE_NOT_FOUND

  pkgname = package._get_name_as_string()
  symname = token

  retv = _LOOKUP_GLOBAL(pkgname, symname)
  if_null retv, SYMBOL_NOT_FOUND		# If not found, intern a new symbol

  goto DONE

SYMBOL_NOT_FOUND:
  null nil				# Intern a new global symbol
  retv = _GLOBAL_SYMBOL(pkgname, symname, nil, nil)

  goto DONE

PACKAGE_NOT_FOUND:
  .ERROR_0("internal-error", "the *PACKAGE* symbol could not be located")
  goto DONE

DONE:
  .return(retv)
.end

.sub _IS_INTEGER
  .param string token
  .param int retv
  .param int ndig
  .param int idx

  ndig = 0
  idx  = 0

SIGNS:
  rx_oneof token, idx, '+-', DIGIT	# check for +/- signs (optional)
  goto DIGIT

DIGIT:					# ensure the rest is all digits
  rx_is_d token, idx, DECIMAL
  ndig = ndig + 1
  goto DIGIT

DECIMAL:
  rx_literal token, idx, '.', EOS	# Check for an optional decimal point
  goto EOS

EOS:					# check to see if we're at string end
  rx_zwa_atend token, idx, FAIL
  goto MATCH

MATCH:
  if ndig == 0 goto FAIL		# ensure we had at least one digit
  retv = 1
  goto DONE

FAIL:
  retv = 0
  goto DONE

DONE:
  .return(retv)
.end

.sub _IS_FLOAT
  .param string token
  .param int retv
  .param int idx

  idx  = 0

SIGNS:
  rx_oneof token, idx, '+-', PREDIGITS	# check for +/- signs (optional)
  goto PREDIGITS

PREDIGITS:				# check for pre-decimal digits
  rx_is_d token, idx, DECIMAL
  goto PREDIGITS

DECIMAL:
  rx_literal token, idx, '.', FAIL	# check for a decimal point
  goto POSTDIGIT

POSTDIGIT:
  rx_is_d token, idx, FAIL		# check for at least one required digit
  goto POSTDIGITS

POSTDIGITS:				# check for option post-decimal digits
  rx_is_d token, idx, EOS
  goto POSTDIGITS

EOS:					# check to see if we're at string end
  rx_zwa_atend token, idx, FAIL
  goto MATCH

MATCH:
  retv = 1
  goto DONE

FAIL:
  retv = 0
  goto DONE

DONE:
  .return(retv)
.end

.sub _is_keyword
  .param string token
  .param int retv
  .param int idx

  idx  = 0

COLON:
  rx_literal token, idx, ':', FAIL	# check the initial ':' character
  goto NAME

NAME:
  rx_is_w token, idx, EOS		# check that we're not whitespace 
  goto NAME

EOS:					# check to see if we're at string end
  rx_zwa_atend token, idx, FAIL
  goto MATCH

MATCH:
  retv = 1
  goto DONE

FAIL:
  retv = 0
  goto DONE

DONE:
  .return(retv)
.end

.sub _IS_QUALIFIED
  .param string token
  .local string package
  .local string symbol
  .local string vchar
  .local int retv
  .local int idx1
  .local int idx2
  .local int idx3
  .local int type

  vchar = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!$%&*<=>?@^_~-./+"
  idx1   = 0

PACKAGE:
  rx_oneof token, idx1, vchar, COLON
  goto PACKAGE

COLON:
  idx2 = idx1				# Index of last valid symbol character

  rx_literal token, idx1, ':', FAIL	# If we don't have this -> not qualified

  idx3 = idx1				# Start of symbol character
  type = 0				# External symbol

  rx_literal token, idx1, ':', SYMBOL	# If we don't have this -> not external

  idx3 = idx1				# Start of symbol character
  type = 1				# Internal symbol

  goto SYMBOL

SYMBOL:
  rx_oneof token, idx1, vchar, EOS
  goto SYMBOL

EOS:
  rx_zwa_atend token, idx1, FAIL	# check to see if we're at string end
  goto MATCH

KEYWORD:
  package = "KEYWORD"
  goto KEYWORD_RETURN

MATCH:
  idx3 = idx3

KEYWORD_CHECK1:
  if idx2 != 0 goto NOT_KEYWORD
  if idx3 <= 2 goto KEYWORD
  goto NOT_KEYWORD

NOT_KEYWORD:
  substr package, token, 0, idx2

KEYWORD_RETURN:
  substr symbol, token, idx3, idx1

  retv = 1
  goto DONE

FAIL:
  package = ""
  symbol = ""
  type = 0
  retv = 0
  goto DONE

DONE:
  .return(retv,package,symbol,type)
.end
