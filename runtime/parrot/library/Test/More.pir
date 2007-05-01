=head1 NAME

Test::More - Parrot extension for testing modules

=head1 SYNOPSIS

    # load this library
    load_bytecode 'library/Test/More.pir'

    # get the testing functions
    .local pmc exports, curr_namespace, test_namespace
    curr_namespace = get_namespace
    test_namespace = get_namespace [ "Test::More" ]
    exports = split " ", "plan diag ok is is_deeply like isa_ok"

    test_namespace."export_to"(curr_namespace, exports)

    # set a test plan
    plan( 12 )

    # run your tests
    ok( 1 )
    ok( 0, 'failing test with diagnostic' )

    is( 100, 100 )
    is( 200, 100, 'failing integer compare with diagnostic' )

    is( 1.001, 1.001, 'passing float compare with diagnostic' )
    is( 8.008, 4.004 )

    is( 'foo', 'foo', 'passing string compare with diagnostic' )
    is( 'foo', 'bar', 'failing string compare with diagnostic' )

    is( some_pmc, another_pmc, 'pmc comparison uses "eq" op' )

    diag( 'this may take a while' )
    is_deeply( some_deep_pmc, another_deep_pmc, 'deep structure comparison' )

    like( 'foo', 'f o**{2}', 'passing regex compare with diagnostic' )
    skip(1, 'reason for skipping')
    todo(0, 'this is a failed test', 'reason for todo')

    $P0 = getclass "Squirrel"
    $P0.new()

    isa_ok($P0, "Squirrel", "new Squirrel")

=head1 DESCRIPTION

C<Test::More> is a pure-Parrot library for testing modules.  It provides
the C<ok()>, C<is()>, C<is_deeply()>, and C<like()> comparison functions for
you.  It also provides the C<plan()> and C<diag()> helper functions. It uses
C<Test::Builder>, a simple, single backend for multiple test modules
to use within your tests.

=head1 FUNCTIONS

This class defines the following functions:

=over 4

=cut

.namespace [ 'Test::More' ]

.sub _initialize :load
    load_bytecode 'library/Test/Builder.pir'

    .local pmc test
    .local int test_type

    find_type test_type, 'Test::Builder'
    test = new test_type

    store_global 'Test::More', '_test', test
.end

=item C<plan( number_or_no_plan )>

Declares the number of tests you plan to run, either an integer greater than
zero or the string C<no_plan>.  This will throw an exception if you have
already declared a plan or if you pass an invalid argument.

=cut

.sub plan
    .param string tests

    .local pmc test
    find_global test, 'Test::More', '_test'
    test.plan( tests )
.end

=item C<ok( passed, description )>

Records a test as pass or fail depending on the truth of the integer C<passed>,
recording it with the optional test description in C<description>.

=cut

.sub ok
    .param int    passed
    .param string description     :optional

    .local pmc test
    find_global test, 'Test::More', '_test'

    test.ok( passed, description )
.end

=item C<nok( passed, description )>

Records a test as pass or fail depending on the falsehood of the integer
C<passed>, recording it with the optional test description in C<description>.

=cut

.sub nok
	.param int passed
	.param string description :optional

	.local pmc test
	find_global test, 'Test::More', '_test'

	.local int reverse_passed
	reverse_passed = not passed

	test.ok( reverse_passed, description )
.end

=item C<is( left, right, description )>

Compares the parameters passed as C<left> and C<right>, passing if they are
equal and failing otherwise.  This will report the results with the optional
test description in C<description>.

This is a multi-method, with separate implementations for int-int, float-float,
string-string, and PMC-PMC comparisons.  The latter uses the C<eq> opcode for
comparison.

If there is a mismatch, the current implementation takes the type of C<left> as
the proper type for the comparison, converting any numeric arguments to floats.
Note that there is a hard-coded precision check to avoid certain rounding
errors.  It's not entirely robust, but it's not completely awful either.

Patches very welcome.  Multi-dispatch is a bit tricky here.

This probably doesn't handle all of the comparisons you want, but it's easy to
add more.

=cut

.sub is :multi( int, int )
    .param int    left
    .param int    right
    .param string description :optional

    .local pmc test
    find_global test, 'Test::More', '_test'

    .local int pass
    pass       = 0

    if left == right goto pass_it
    goto report

  pass_it:
    pass = 1

  report:
    test.ok( pass, description )
    if pass goto done

    .local string diagnostic
    .local string l_string
    .local string r_string

    l_string    = left
    r_string    = right

    diagnostic = _make_diagnostic( l_string, r_string )
    test.diag( diagnostic )
  done:
.end

.sub is :multi( float, float )
    .param float  left
    .param float  right
    .param string description :optional

    .local pmc test
    find_global test, 'Test::More', '_test'

    .local int pass
    pass = 0

    eq left, right, pass_it
    goto report

  pass_it:
    pass = 1

  report:
    test.ok( pass, description )
    if pass goto done

    .local string diagnostic
    .local string l_string
    .local string r_string

    l_string    = left
    r_string    = right

    diagnostic = _make_diagnostic( l_string, r_string )
    test.diag( diagnostic )
  done:
.end

.sub is :multi( string, string )
    .param string left
    .param string right
    .param string description :optional

    .local pmc test
    find_global test, 'Test::More', '_test'

    .local int pass
    pass = 0

    eq left, right, pass_it
    goto report

  pass_it:
    pass = 1

  report:
    test.ok( pass, description )
    if pass goto done

    .local string diagnostic
    .local string l_string
    .local string r_string

    l_string    = left
    r_string    = right

    diagnostic = _make_diagnostic( l_string, r_string )
    test.diag( diagnostic )
  done:
.end

.sub is :multi()
    .param pmc    left
    .param pmc    right
    .param string description :optional

    .local pmc test
    find_global test, 'Test::More', '_test'

    .local int pass
    pass = 0

	.local string r_type
	r_type = typeof right

	if r_type == 'Float' goto num_compare
	if r_type == 'Int'   goto num_compare
	goto string_compare

  num_compare:
 	.local float l_val
 	.local float r_val
	l_val = left
	r_val = right

    if l_val == r_val goto pass_it

	# XXX - significant places?  I don't care :)
	.local float diff
	diff = l_val - r_val

	if diff < 0.000000000001 goto pass_it

  string_compare:
	.local string l_val
	.local string r_val
	l_val = left
	r_val = right
	eq l_val, r_val, pass_it
	goto report

  pass_it:
    pass = 1

  report:
    test.ok( pass, description )
    if pass goto done

    .local string diagnostic
    .local string l_string
    .local string r_string

    l_string    = left
    r_string    = right

    diagnostic = _make_diagnostic( l_string, r_string )
    test.diag( diagnostic )
  done:
.end

=item C<diag( diagnostic )>

Prints C<diagnostic> to the screen, without affecting test comparisons.

=cut

.sub diag
    .param string diagnostic

    .local pmc test
    find_global test, 'Test::More', '_test'
    test.diag( diagnostic )
.end


=item C<is_deeply( left, right, description )>

Compares the data structures passed as C<left> and C<right>.  If data
structures are passed, C<is_deeply> does a deep comparison by walking each
structure.  It passes if they are equal and fails otherwise.  This will
report the results with the optional test description in C<description>.

This only handles comparisons of array-like structures.  It shouldn't be too
hard to extend it for hash-like structures, too.

=cut

.sub is_deeply :multi( pmc, pmc )
    .param pmc left
    .param pmc right
    .param string description :optional

    .local int    result
    .local string diagnosis

    .local pmc position
    position = new .ResizablePMCArray

    .local pmc test
    find_global test, 'Test::More', '_test'

    .local int does_flag
    does_flag = does left, 'array'
    if does_flag goto compare_array

    does_flag = does left, 'hash'
    if does_flag goto compare_hash

    diagnosis  = typeof left
    diagnosis .= ' is not a nested data structure'
    result     = 0
    goto report_result

  compare_array:
    ( result, diagnosis ) = compare_array( left, right, position )
    goto report_result

  compare_hash:
    (result, diagnosis ) = compare_hash( left, right, position )
    goto report_result

  report_result:
    test.'ok'( result, description )

    unless result goto report_diagnostic
    .return( result )

  report_diagnostic:
    ne diagnosis, '', return_it

    .local string left
    .local string right
    right = pop position
    left  = pop position

    .local string nested_path
    nested_path = join '][', position

    diagnosis   = 'Mismatch'
    unless nested_path goto show_expected

    diagnosis  .= ' at ['
    diagnosis  .= nested_path
    diagnosis  .= ']'

  show_expected:
    diagnosis  .= ': expected '
    diagnosis  .= left
    diagnosis  .= ', received '
    diagnosis  .= right

  return_it:
    test.'diag'( diagnosis )
    .return( result )
.end

.sub compare_array
    .param pmc l_array
    .param pmc r_array
    .param pmc position

    .local pmc test
    find_global test, 'Test::More', '_test'

    .local int l_count
    .local int r_count
    l_count = l_array
    r_count = r_array
    if l_count == r_count goto compare_contents

    .local string l_count_string
    .local string r_count_string
    l_count_string  = l_count
    l_count_string .= ' element'

    if l_count == 1 goto pluralization_done
    l_count_string .= 's'

  pluralization_done:
    r_count_string  = r_count

    push position, l_count_string
    push position, r_count_string

    .return( 0 )

  compare_contents:
    .local pmc l_iter
    .local pmc r_iter
    .local int count

    l_iter = new .Iterator, l_array
    r_iter = new .Iterator, r_array
    l_iter = 0
    r_iter = 0
    count  = 0

    .local pmc l_elem
    .local pmc r_elem
    .local int elems_equal

  iter_start:
    unless l_iter goto iter_end
    l_elem = shift l_iter
    r_elem = shift r_iter

    $S0 = typeof l_elem
    elems_equal = compare_elements( l_elem, r_elem, position )
    unless elems_equal goto elems_not_equal

    inc count
    goto iter_start

  elems_not_equal:
    unshift position, count
    .return( 0 )

  iter_end:
    .return( 1 )
.end

.sub compare_hash
    .param pmc l_hash
    .param pmc r_hash
    .param pmc position

    .local pmc test
    find_global test, 'Test::More', '_test'

    .local int l_count
    .local int r_count
    l_count = l_hash
    r_count = r_hash
    if l_count == r_count goto compare_contents

    .local string l_count_string
    .local string r_count_string
    l_count_string  = l_count
    l_count_string .= ' element'

    if l_count == 1 goto pluralization_done
    l_count_string .= 's'

  pluralization_done:
    r_count_string  = r_count

    push position, l_count_string
    push position, r_count_string

    .return( 0 )

  compare_contents:
    .local pmc l_iter
    .local pmc r_iter
    .local int count

    l_iter = new .Iterator, l_hash
    r_iter = new .Iterator, r_hash
    l_iter = 0
    r_iter = 0
    count  = 0

    .local pmc l_key
    .local pmc r_key
    .local pmc l_elem
    .local pmc r_elem
    .local int elems_equal

  iter_start:
    unless l_iter goto iter_end
    l_key  = shift l_iter
    r_key  = shift r_iter
    l_elem = l_hash[ l_key ]
    r_elem = r_hash[ r_key ]

    elems_equal = compare_elements( l_elem, r_elem, position )
    unless elems_equal goto elems_not_equal

    inc count
    goto iter_start

  elems_not_equal:
    unshift position, l_key
    .return( 0 )

  iter_end:
    .return( 1 )
.end

.sub compare_elements :multi( string, string, PMC )
    .param string left
    .param string right
    .param pmc position

    .local int equal

    eq left, right, are_equal

  are_not_equal:
    .return( 0 )

  are_equal:
    .return( 1 )
.end

.sub compare_elements :multi( int, int, PMC )
    .param int left
    .param int right
    .param pmc position

    .local int equal
    eq left, right, are_equal

  are_not_equal:
    push position, left
    push position, right
    .return( 0 )

  are_equal:
    .return( 1 )
.end

.sub compare_elements :multi( String, String, PMC )
    .param pmc left
    .param pmc right
    .param pmc position

    .local int equal
    eq left, right, are_equal

  are_not_equal:
    push position, left
    push position, right
    .return( 0 )

  are_equal:
    .return( 1 )
.end

.sub compare_elements :multi( Integer, Integer, PMC )
    .param pmc left
    .param pmc right
    .param pmc position

    .local int equal
    eq left, right, are_equal

  are_not_equal:
    push position, left
    push position, right
    .return( 0 )

  are_equal:
    .return( 1 )
.end

.sub compare_elements :multi( Array, Array, PMC )
    .param pmc left
    .param pmc right
    .param pmc position

    .local int equal
    equal = compare_array( left, right, position )
    .return( equal )
.end

.sub compare_elements :multi( Hash, Hash, PMC )
    .param pmc left
    .param pmc right
    .param pmc position

    .local int equal
    equal = compare_hash( left, right, position )
    .return( equal )
.end

.sub compare_elements :multi( Undef, Undef, PMC )
    .param pmc left
    .param pmc right
    .param pmc position

    .return( 1 )
.end

.sub compare_elements :multi( Undef, PMC, PMC )
    .param pmc left
    .param pmc right
    .param pmc position

    .local string l_undef
    l_undef = '(undef)'
    push position, l_undef
    push position, right
    .return( 0 )
.end

.sub compare_elements :multi( PMC, Undef, PMC )
    .param pmc left
    .param pmc right
    .param pmc position

    .local string r_undef
    r_undef = '(undef)'
    push position, left
    push position, r_undef
    .return( 0 )
.end

.sub compare_elements :multi( PMC, PMC, PMC )
    .param pmc left
    .param pmc right
    .param pmc position

    .local int does_flag
    .local int equal

  check_array:
    does_flag = does left, 'array'
    unless does_flag goto check_hash
    equal = compare_array( left, right, position )
    .return( equal )

  check_hash:
    does_flag = does left, 'hash'
    if does_flag goto compare_hash
    .return( 0 )

  compare_hash:
    equal = compare_hash( left, right, position )
    .return( equal )
.end

=item C<like( target, pattern, description )>

Similar to is, but using the Parrot Grammar Engine to compare the string
passed as C<target> to the pattern passed as C<pattern>.  It passes if the
pattern matches and fails otherwise.  This will report the results with the
optional test description in C<description>.

=cut

.sub like
    .param string target
    .param string pattern
    .param string description :optional

    .local pmc test
    find_global test, 'Test::More', '_test'

    .local pmc p6rule_compile
    load_bytecode "PGE.pbc"
    load_bytecode "PGE/Dumper.pir"
    load_bytecode "PGE/Text.pir"
    load_bytecode "PGE/Util.pir"
    p6rule_compile = compreg "PGE::P6Regex"

    .local string diagnostic
    .local int pass
    pass = 0

  match_pattern:
    .local pmc rulesub
    .local pmc match
    .local pmc code
    .local pmc exp
    (rulesub, code, exp) = p6rule_compile(pattern)
    if_null rulesub, rule_fail
    match = rulesub(target)
    unless match goto match_fail
  match_success:
    goto pass_it
  match_fail:
    diagnostic = "match failed"
    goto report
  rule_fail:
    diagnostic = "rule error"
    goto report

  pass_it:
    pass = 1

  report:
    test.ok( pass, description )
    if pass goto done

    test.diag( diagnostic )
  done:
.end

=item C<skip( how_many, why )>

Pass a number of tests, but with a comment that marks the test was
actually skipped.  Arguments are optional.

=cut

.sub skip :multi(int, string)
    .param int how_many
    .param string description

    .local pmc test
    find_global test, 'Test::More', '_test'
    test.'skip'(how_many, description)
.end

.sub skip :multi(int)
    .param int how_many

    .local pmc test
    find_global test, 'Test::More', '_test'
    test.'skip'(how_many)
.end

.sub skip :multi(string)
    .param string description

    .local pmc test
    find_global test, 'Test::More', '_test'
    test.'skip'(1, description)
.end

.sub skip :multi()
    .local pmc test
    find_global test, 'Test::More', '_test'
    test.'skip'()
.end

=item C<todo( passed, description, reason )>

Records a test as pass or fail (like C<ok>, but marks it as TODO so it always
appears as a success. This also records the optional C<description> of the test
and the C<reason> you have marked it as TODO.

=cut

.sub todo
    .param pmc args :slurpy

    .local pmc test
    find_global test, 'Test::More', '_test'

    test.todo( args :flat )
.end

=item C<isa_ok( object, class_name, object_name )>

Pass if the object C<isa> class of the given class name.  The object
name passed in is not a full description, but a name to be included in
the description. The description is presented as "<object_name> isa
<class>".

Good input: "C<new MyObject>", "C<return from bar()>"

Bad input: "C<test that the return from Foo is correct type>"

=cut

.sub isa_ok
    .param pmc thingy
    .param pmc class_name
    .param pmc object_name :optional
    .param int got_name :opt_flag

    .local pmc test
    find_global test, 'Test::More', '_test'

    .local string description, diagnostic
    description = "The object"
    unless got_name goto keep_default
    description = object_name
  keep_default:
    diagnostic = description
    description .= " isa "
    $S0 = class_name
    description .= $S0

    $I0 = isa thingy, class_name
    test.'ok'($I0, description)
    if $I0 goto out
    diagnostic .= " isn't a "
    $S1 = class_name
    diagnostic .= $S1
    diagnostic .= " it's a "
    $S2 = typeof thingy
    diagnostic .= $S2
    test.'diag'(diagnostic)
  out:
.end

.sub _make_diagnostic
    .param string received
    .param string expected
    .local string diagnostic

    diagnostic  = 'Received: '
    diagnostic .= received
    diagnostic .= "\nExpected: "
    diagnostic .= expected

    .return( diagnostic )
.end

=back

=head1 AUTHOR

Written and maintained by chromatic, C<< chromatic at wgz dot org >>, based on
the Perl 6 port he wrote, based on the original Perl 5 version he wrote with
ideas from Michael G. Schwern.  Please send patches, feedback, and suggestions
to the Perl 6 internals mailing list.

=head1 COPYRIGHT

Copyright (C) 2005 - 2006 The Perl Foundation.

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
