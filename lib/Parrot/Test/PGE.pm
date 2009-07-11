# Copyright (C) 2004-2007, Parrot Foundation.
# $Id$

=head1 NAME

Parrot::Test::PGE - test functions for Perl 6 Grammar Engine

=head1 SYNOPSIS

In a .t file:

  use Parrot::Test tests => 2;
  use Parrot::Test::PGE;

  p6rule_is('abc', '^abc', 'BOS abc');
  p6rule_is("   int argc ",
    [
        [ type    => 'int | double | float | char' ],
        [ ident   => '\w+' ],
        [ _MASTER => ':w<type> <ident>' ],
    ],
    "simple subrules test");
  p6rule_isnt('abc', '^bc', 'BOS bc');
  p6rule_like('abcdef', 'bcd', qr/0: <bcd @ 1>/, '$0 capture');

=head1 DESCRIPTION

Parrot::Test::PGE provides functions for testing the grammar engine
and Perl 6 rules.

=cut

use strict;
use warnings;

require Parrot::Test;

=head2 Functions

=over 4

=item C<p6rule_is($target, $pattern, $description, @todo)>

Runs the target string against the Perl 6 pattern, passing the test
if they match.  Note that patterns should be specified as strings
and without leading/trailing pattern delimiters.

(Hint: if you try using qr// for the $pattern then you're misreading
what this does.)

subrules: In addition to a simple scalar string, the pattern can be a
reference to an array of arrays. Containing subrules that refer to each
other. In this form:

    [
        [ name1 => 'pattern 1' ],
        [ name2 => 'pattern 2' ],
        [ name3 => '<name1> pattern 3' ],
        [ _MASTER => '<name1> <name2> <name3>' ],
    ],

The last rule, labelled with _MASTER, is the rule that your target string
will be matched against. The 'outer rule' if you will.

=cut

sub p6rule_is {
    my ( $target, $pattern ) = ( shift, shift );

    unshift @_ => 'matched';
    unshift @_ => (
        ref $pattern
        ? Parrot::Test::PGE::_generate_subrule_pir( $target, $pattern )
        : Parrot::Test::PGE::_generate_pir_for( $target, $pattern )
    );

    goto &Parrot::Test::pir_output_is;
}

=item C<p6rule_isnt($target, $pattern, $description, @todo)>

Runs the target string against the Perl 6 pattern, passing the test if
they do not match. The same pattern argument syntax above applies here.

=cut

sub p6rule_isnt {
    my ( $target, $pattern ) = ( shift, shift );

    unshift @_ => 'failed';
    unshift @_ => (
        ref $pattern
        ? Parrot::Test::PGE::_generate_subrule_pir( $target, $pattern )
        : Parrot::Test::PGE::_generate_pir_for( $target, $pattern )
    );

    goto &Parrot::Test::pir_output_is;
}

=item C<p6rule_like($target, $pattern, $expected, $description, @todo)>

Runs the target string against the Perl 6 pattern, passing the test
if the output produced by the test code matches the C<$expected>
parameter.  Note that C<$expected> is a I<Perl 5> pattern.

=cut

sub p6rule_like {
    my ( $target, $pattern ) = ( shift, shift );

    unshift @_ => Parrot::Test::PGE::_generate_pir_for( $target, $pattern, 1 );

    goto &Parrot::Test::pir_output_like;
}

=item C<p6rule_error_like($target, $pattern, $expected, $description, @todo)>

Like C<p6rule_like()>, but expects Parrot/PGE to exit with an error.

=cut

sub p6rule_error_like {
    my ( $target, $pattern ) = ( shift, shift );

    unshift @_ => Parrot::Test::PGE::_generate_pir_for( $target, $pattern, 1 );

    goto &Parrot::Test::pir_error_output_like;
}

=item C<p6rule_throws($pattern, $expected, $description, @todo)>

Compiles the Perl 6 pattern, catching any thrown exceptions.  The test
passes if the pattern throws an exception and the exception message
matches the C<$expected> parameter.  Note that C<$expected> is a
I<Perl 5> pattern.

=cut

sub p6rule_throws {
    my ($pattern) = (shift);

    unshift @_ => Parrot::Test::PGE::_generate_pir_catch_for($pattern);

    goto &Parrot::Test::pir_output_like;
}

=item C<pgeglob_is($target, $pattern, $description, @todo)>

Runs the target string against the Perl 6 pattern, passing the test
if they match.  Note that patterns should be specified as strings
and without leading/trailing pattern delimiters.

(Hint: if you try using qr// for the $pattern then you're misreading
what this does.)

=cut

sub pgeglob_is {
    my ( $target, $pattern ) = ( shift, shift );

    unshift @_ => 'matched';
    unshift @_ => Parrot::Test::PGE::_generate_glob_for( $target, $pattern );

    goto &Parrot::Test::pir_output_is;
}

=item C<pgeglob_isnt($target, $pattern, $description, @todo)>

Runs the target string against the Perl 6 pattern, passing the test if
they do not match. The same pattern argument syntax above applies here.

=cut

sub pgeglob_isnt {
    my ( $target, $pattern ) = ( shift, shift );

    unshift @_ => 'failed';
    unshift @_ => Parrot::Test::PGE::_generate_glob_for( $target, $pattern );

    goto &Parrot::Test::pir_output_is;
}

=item C<pgeglob_like($target, $pattern, $expected, $description, @todo)>

Runs the target string against the Perl 6 pattern, passing the test
if the output produced by the test code matches the C<$expected>
parameter.  Note that C<$expected> is a I<Perl 5> pattern.

=cut

sub pgeglob_like {
    my ( $target, $pattern ) = ( shift, shift );

    unshift @_ => Parrot::Test::PGE::_generate_glob_for( $target, $pattern, 1 );

    goto &Parrot::Test::pir_output_like;
}

package Parrot::Test::PGE;

sub _parrot_stringify {
    $_ = $_[0];
    s/\\(?!u)/\\\\/g;
    s/\n/\\n/g;
    s/\r/\\r/g;
    s/\"/\\"/g;
    return $_;
}

sub _generate_pir_for {
    my ( $target, $pattern, $captures ) = @_;
    $target  = _parrot_stringify($target);
    $pattern = _parrot_stringify($pattern);
    my $unicode = ( $target =~ /\\u/ ) ? "unicode:" : "";
    if ($captures) {
        $captures = qq(
            print "\\n"
            match."dump"("mob"," ","")\n);
    }
    else {
        $captures = "";
    }
    return qq(
        .sub _PGE_Test
            .local pmc p6rule_compile
            load_bytecode "PGE.pbc"
            load_bytecode "PGE/Dumper.pir"
            load_bytecode "PGE/Text.pir"
            load_bytecode "PGE/Util.pir"
            p6rule_compile = compreg "PGE::Perl6Regex"

            .local string target
            .local string pattern
            .local pmc rulesub
            .local pmc match
            target = $unicode"$target"
            pattern = "$pattern"
            rulesub = p6rule_compile(pattern)
            if_null rulesub, rule_fail
            match = rulesub(target)
            unless match goto match_fail
          match_success:
            print "matched"
            $captures
            goto end
          match_fail:
            print "failed"
            goto end
          rule_fail:
            print "rule error"
          end:
        .end\n);
}

sub _generate_pir_catch_for {
    my ($pattern) = @_;
    $pattern = _parrot_stringify($pattern);
    return qq(
        .sub _PGE_Test
            .local pmc p6rule_compile
            load_bytecode "PGE.pbc"
            load_bytecode "PGE/Dumper.pir"
            load_bytecode "PGE/Text.pir"
            load_bytecode "PGE/Util.pir"
            p6rule_compile = compreg "PGE::Perl6Regex"

            .local string pattern
            .local pmc rulesub
            pattern = "$pattern"
            push_eh handler
            rulesub = p6rule_compile(pattern)
            if_null rulesub, compile_fail
          compile_success:
            print "OK"
            goto end
          compile_fail:
            print "unknown compile error"
            goto end
          handler:
            .local pmc exception
            .local string message
            .get_results (exception, message)
            print message
          end:
        .end\n);
}

sub _generate_subrule_pir {
    my ( $target, $pattern ) = @_;
    $target = _parrot_stringify($target);

    # Beginning of the pir code
    my $pirCode = qq(
        .sub _PGE_Test
            .local pmc p6rule_compile
            load_bytecode "PGE.pbc"
            p6rule_compile = compreg "PGE::Perl6Regex"

            .local string target
            .local pmc rulesub
            .local pmc match
            .local string name
            .local string subpat

            target = "$target"\n\n);

    # Loop to create the subrules pir code
    for my $ruleRow (@$pattern) {
        my ( $name, $subpat ) = @$ruleRow;
        $subpat = _parrot_stringify($subpat);

        $pirCode .= qq(
            name = "$name"
            subpat = "$subpat"
            rulesub = p6rule_compile(subpat)\n);

        last if $name eq '_MASTER';

        $pirCode .= qq(
            store_global name, rulesub\n\n);
    }

    # End of the pir code
    $pirCode .= qq(
            match = rulesub(target)

            unless match goto match_fail
          match_success:
            print "matched"
            goto match_end
          match_fail:
            print "failed"
          match_end:
        .end\n);

    return $pirCode;
}

sub _generate_glob_for {
    my ( $target, $pattern, $captures ) = @_;
    $target  = _parrot_stringify($target);
    $pattern = _parrot_stringify($pattern);
    return qq(
        .sub _PGE_Test
            .local pmc glob_compile
            load_bytecode "PGE.pbc"
            load_bytecode "PGE/Glob.pbc"
            load_bytecode "PGE/Text.pbc"
            glob_compile = compreg "PGE::Glob"

            .local string target
            .local string pattern
            .local pmc rulesub
            .local pmc match
            .local pmc code
            .local pmc exp
            target = unicode:"$target"
            pattern = "$pattern"
            rulesub = glob_compile.'compile'(pattern)
            match = rulesub(target)
            unless match goto match_fail
          match_success:
            print "matched"
            goto match_end
          match_fail:
            print "failed"
          match_end:
        .end\n);
}

=back

=head1 AUTHOR

Patrick R. Michaud, pmichaud@pobox.com   18-Nov-2004

=cut

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
