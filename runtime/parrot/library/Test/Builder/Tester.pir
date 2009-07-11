# $Id$

=head1 NAME

Test::Builder::Tester - Parrot extension for testing test modules

=head1 SYNOPSIS

    # load this library
    load_bytecode 'Test/Builder/Tester.pbc'

    # grab the subroutines you want to use
    .local pmc plan
    .local pmc test_out
    .local pmc test_diag
    .local pmc test_test

    plan      = get_hll_global [ 'Test'; 'Builder'; 'Tester' ], 'plan'
    test_out  = get_hll_global [ 'Test'; 'Builder'; 'Tester' ], 'test_out'
    test_diag = get_hll_global [ 'Test'; 'Builder'; 'Tester' ], 'test_diag'
    test_test = get_hll_global [ 'Test'; 'Builder'; 'Tester' ], 'test_test'

    # create a new Test::Builder object
    .local pmc tb_args
    .local pmc test

    tb_args = new 'Hash'
    test    = new [ 'Test'; 'Builder' ], tb_args

    # set your test plan
    plan( 4 )

    # test a passing test
    test_out( 'ok 1 - hi' )
    test.'ok'( 1, 'hi' )
    test_test( 'passing test')

    # test a test with some diagnostics
    test_out( 'ok 3 - A message' )
    test_diag( "some\nlines" )
    test.ok( 1, 'A message' )
    test.diag( 'some' )
    test.diag( 'lines' )
    test_test( 'passing test with diagnostics' )

    # clean up
    test.'finish'()

=head1 DESCRIPTION

Test::Builder::Tester is a pure-Parrot library for testing testing modules
built on L<Test::Builder>.  It allows you to describe the TAP output that they
will produce, showing any differences in description, directive, and
diagnostics.

This is a procedural library.

=head1 FUNCTIONS

This module defines the following public functions:

=over 4

=cut

.namespace [ 'Test'; 'Builder'; 'Tester'; 'Output' ]

.sub _initialize :load
    .local pmc tbto_class
    newclass tbto_class, [ 'Test'; 'Builder'; 'Tester'; 'Output' ]
    addattribute tbto_class, 'output'
    addattribute tbto_class, 'diagnostics'
.end

.sub init :vtable :method
    .local pmc output
    .local pmc diagnostics
    output      = new 'ResizablePMCArray'
    diagnostics = new 'ResizablePMCArray'

    setattribute self, "output", output
    setattribute self, "diagnostics", diagnostics

.end

.sub get_output :method
    .local pmc output

    getattribute output, self, "output"

    .return( output )
.end

.sub get_diagnostics :method
    .local pmc diagnostics

    getattribute diagnostics, self, "diagnostics"

    .return( diagnostics )
.end

.sub write :method
    .param string message

    .local pmc message_string
    message_string = new 'String'
    set message_string, message

    .local pmc output
    output = self.'get_output'()
    push output, message_string
.end

.sub diag :method
    .param string message

    .local pmc message_string
    message_string = new 'String'
    set message_string, message

    .local pmc diagnostics
    diagnostics = self.'get_diagnostics'()
    push diagnostics, message_string
.end

.sub output :method
    .local pmc output
    output = self.'get_output'()

    unless_null output, JOIN_LINES
    .return( '' )

  JOIN_LINES:
    .local string output_string
    output_string = join "\n", output
    set output, 0
    .return( output_string )
.end

.sub diagnostics :method
    .local pmc diagnostics
    diagnostics = self.'get_diagnostics'()

    unless_null diagnostics, JOIN_LINES
    .return( '' )

  JOIN_LINES:
    .local string diag_string
    diag_string = join "\n", diagnostics
    diagnostics = 0
    .return( diag_string )
.end

.namespace [ 'Test'; 'Builder'; 'Tester' ]

.sub _initialize :load
    load_bytecode 'Test/Builder.pbc'

    .local pmc test
    .local pmc output
    .local pmc test_output
    .local pmc expect_out
    .local pmc expect_diag
    .local pmc default_test
    .local pmc args

    # set the default output for the Test::Builder singleton
    test_output  = new [ 'Test'; 'Builder'; 'Tester'; 'Output' ]
    args         = new 'Hash'
    set args['output'], test_output

    default_test = new [ 'Test'; 'Builder' ], args
    default_test.'plan'( 'no_plan' )
    test_output.'output'()

    # create the Test::Builder object that this uses
    .local pmc tb_create
    tb_create   = get_hll_global [ 'Test'; 'Builder' ], 'create'

    args        = new 'Hash'
    output      = new [ 'Test'; 'Builder'; 'Output' ], args
    .local pmc results, testplan
    results    = new 'ResizablePMCArray'
    testplan   = new 'String'
    testplan   = ''

    set args['output'], output
    test        = tb_create( args )

    expect_out  = new 'ResizablePMCArray'
    expect_diag = new 'ResizablePMCArray'

    set_hll_global [ 'Test'; 'Builder'; 'Tester' ], '_test',         test
    set_hll_global [ 'Test'; 'Builder'; 'Tester' ], '_default_test', default_test
    set_hll_global [ 'Test'; 'Builder'; 'Tester' ], '_test_output',  test_output
    set_hll_global [ 'Test'; 'Builder'; 'Tester' ], '_expect_out',   expect_out
    set_hll_global [ 'Test'; 'Builder'; 'Tester' ], '_expect_diag',  expect_diag
.end

=item C<plan( num_tests )>

Sets the number of tests you plan to run, where C<num_tests> is an int.

=cut

.sub plan
    .param int tests

    .local pmc test
    test = get_hll_global [ 'Test'; 'Builder'; 'Tester' ], '_test'

    test.'plan'( tests )
.end

.sub line_num
.end

=item C<test_pass( test_string )>

Sets the expectation for a test to pass.  C<test_string> is the optional
description of the test.

=cut

.sub test_pass
    .param string description :optional
    .param int    have_desc   :opt_flag

    set_output( 'ok', description )
.end

=item C<test_fail( test_string )>

Sets the expectation for a test to fail.  C<test_string> is the optional
description of the test.

=cut

.sub test_fail
    .param string description :optional

    set_output( 'not ok', description )
.end

.sub set_output
    .param string test_type
    .param string description

    .local pmc test
    .local pmc results
    .local int result_count
    .local pmc next_result

    test         = get_hll_global [ 'Test'; 'Builder'; 'Tester' ], '_default_test'
    results      = test.'results'()
    result_count = results
    inc result_count

    next_result  = new 'String'
    set next_result, result_count

    .local pmc line_string
    line_string = new 'String'
    concat line_string, test_type
    concat line_string, ' '
    concat line_string, next_result

    .local int string_defined
    string_defined = length description
    unless string_defined goto SET_EXPECT_OUTPUT
    concat line_string, ' - '
    concat line_string, description

  SET_EXPECT_OUTPUT:
    .local pmc expect_out
    expect_out = get_hll_global [ 'Test'; 'Builder'; 'Tester' ], '_expect_out'

    push expect_out, line_string
.end

=item C<test_out( test_string )>

Sets the expected output for this test to a string.  This should be a line of
TAP output containing a combination of test number, status, description, and
directive.

=cut

.sub test_out
    .param string line

    .local pmc line_string
    line_string = new 'String'
    set line_string, line

    .local pmc expect_out
    expect_out = get_hll_global [ 'Test'; 'Builder'; 'Tester' ], '_expect_out'

    push expect_out, line_string
.end

=item C<test_err( test_string )>

Sets the expected diagnostic output for this test to a string.  This should be
a line of TAP output containing a test directive.

=cut

.sub test_err
    .param string line

    .local pmc line_string
    line_string = new 'String'
    set line_string, line

    .local pmc expect_diag
    expect_diag = get_hll_global [ 'Test'; 'Builder'; 'Tester' ], '_expect_diag'

    push expect_diag, line_string
.end

=item C<test_diag( test_string )>

Sets the expected diagnostic output for this test to a string.  This should be
a line of TAP output containing a test directive.

This and C<test_err()> are effectively the same.

=cut

.sub test_diag
    .param string line

    .local pmc line_string
    line_string = new 'String'
    set line_string, line

    .local pmc expect_diag
    expect_diag = get_hll_global [ 'Test'; 'Builder'; 'Tester' ], '_expect_diag'

    push expect_diag, line_string
.end

=item C<test_test( test_description )>

Compares all of the expected test output and diagnostic output with the actual
test output.  This reports success or failure, using the giving string for the
test description, and prints a diagnostic message with the divergent test
output or diagnostic output.

=cut

.sub test_test
    .param string description

    .local int string_defined
    string_defined = length description
    if string_defined goto FETCH_GLOBALS
    description = ''

  FETCH_GLOBALS:
    .local pmc test
    .local pmc expect_out
    .local pmc expect_diag
    .local pmc test_output

    test          = get_hll_global [ 'Test'; 'Builder'; 'Tester' ], '_test'
    expect_out    = get_hll_global [ 'Test'; 'Builder'; 'Tester' ], '_expect_out'
    expect_diag   = get_hll_global [ 'Test'; 'Builder'; 'Tester' ], '_expect_diag'
    test_output   = get_hll_global [ 'Test'; 'Builder'; 'Tester' ], '_test_output'

    .local string received_out_string
    .local string received_diag_string
    .local string expected_out_string
    .local string expected_diag_string

    received_out_string  = test_output.'output'()
    received_diag_string = test_output.'diagnostics'()

  MAKE_EXPECTED_OUTPUT_STRING:
    .local int num_lines
    num_lines = expect_out
    ne num_lines, 0, JOIN_EO_STRING
    goto MAKE_EXPECTED_DIAG_STRING

  JOIN_EO_STRING:
    expected_out_string = join "\n", expect_out
    expect_out          = 0

  MAKE_EXPECTED_DIAG_STRING:
    num_lines = expect_diag
    ne num_lines, 0, JOIN_DIAG_STRING
    goto COMPARE_OUT_STRINGS

  JOIN_DIAG_STRING:
    expected_diag_string = join "\n", expect_diag
    expect_diag          = 0

    .local int diag_matches
    .local int output_matches
    diag_matches   = 1
    output_matches = 1

  COMPARE_OUT_STRINGS:
    eq received_out_string, expected_out_string, COMPARE_DIAG_STRINGS

    output_matches = 0
    goto FAIL_TEST

  COMPARE_DIAG_STRINGS:
    eq received_diag_string, expected_diag_string, PASS_TEST

    diag_matches = 0
    goto FAIL_TEST

  PASS_TEST:
    test.'ok'( 1, description )
    .return( 1 )

  FAIL_TEST:
    test.'ok'( 0, description )
    eq output_matches, 1, REPORT_DIAG_MISMATCH

  REPORT_OUTPUT_MISMATCH:
    .local string diagnostic
    diagnostic = "output mismatch\nhave: "
    concat diagnostic, received_out_string
    concat diagnostic, "\nwant: "
    concat diagnostic, expected_out_string
    concat diagnostic, "\n"
    test.'diag'( diagnostic )

    eq diag_matches, 1, RETURN

  REPORT_DIAG_MISMATCH:
    diagnostic = "diagnostic mismatch\nhave: '"
    concat diagnostic, received_diag_string
    concat diagnostic, "'\nwant: '"
    concat diagnostic, expected_diag_string
    concat diagnostic, "'\n"
    test.'diag'( diagnostic )

  RETURN:
    .return( 0 )
.end

=back

=head1 AUTHOR

Written and maintained by chromatic, C<< chromatic at wgz dot org >>, based on
the Perl 6 port he wrote, based on the original Perl 5 version written by Mark
Fowler.  Please send patches, feedback, and suggestions to the Perl 6 internals
mailing list.

=head1 COPYRIGHT

Copyright (C) 2005-2008, Parrot Foundation.

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
