package P6C::Builtins;

=head1 B<P6C::Builtins>

XXX: Pre-declare functions the hard way.  Once IMCC supports more
opcodes, this (and emit) will become unnecessary, as we can use a
"header file" to declare builtins in Perl 6 syntax, and add_code to
supply their function bodies.

=cut

use P6C::Context;
use P6C::Parser;
use strict;

# Squash warnings about builtins having empty bodies.  One entry for
# each builtin implemented below.
use vars '%builtin_names';
BEGIN {
    my @names = qw(print1 exit warn die print sleep time substr length
		   index map grep join reverse defined install_catch
		   pop_catch);
    @builtin_names{@names} = (1) x @names;
}

sub is_builtin {
    return exists $builtin_names{+shift};
}

# Make a blessed nonterminal just as if the parser had created it.
sub mkraw {
    my ($type, @info) = @_;
    return bless [ $type => @info ], "P6C::$type";
}

# HACK! It would be better to call into the regular parser for this,
# but it'll take me less time to implement it here than to figure out
# how to do that.
sub parse_sig {
    my ($sig_string) = @_;

    my @sigparams;
    foreach my $param_str (split(/\s*,\s*/, $sig_string)) {
        my $class;
        if (my ($type, $var) = $param_str =~ /(\S+) (\S+)/) {
            $class = mkraw(class => $type);
            $param_str = $var;
        }

        my $zone;
        if ($param_str =~ /^([\?\*\+])/) {
            $zone = mkraw(zone => $1);
            $param_str = substr($param_str, 1);
        }

        my $sigil = mkraw(sigil => substr($param_str, 0, 1));
        my $name = mkraw(varname => substr($param_str, 1));
        my $var = mkraw(variable => $sigil, undef, $name);

        push @sigparams, mkraw(sigparam => $class, $zone, $var, mkraw('traits'), undef);
    }

    my $sig = mkraw(signature => undef, undef, \@sigparams, undef);

    $sig = $sig->tree;
    return ($sig, new P6C::Context type => $sig->arg_context);
}

sub declare {
    my $hash = shift;
    for (qw(print1 exit sleep install_catch pop_catch)) {
        my ($sig, $ctx) = parse_sig('$a');
	$P6C::Context::CONTEXT{$_} = $ctx;
	$hash->{$_} = new P6C::IMCC::Sub params => $sig, rettype => [];
	$P6C::Parser::WANT{$_} = 'scalar_expr';
    }

    for (qw(time)) {
        my ($sig, $ctx) = parse_sig('');
	$P6C::Context::CONTEXT{$_} = $ctx;
	$hash->{$_} = new P6C::IMCC::Sub params => $sig, rettype => 'PerlInt';
	$P6C::Parser::WANT{$_} = 'no_args';
    }

    for (qw(print warn die)) {
        my ($sig, $ctx) = parse_sig('*@params');
	$P6C::Context::CONTEXT{$_} = $ctx;
	$hash->{$_} = new P6C::IMCC::Sub params => $sig, rettype => [];
	$P6C::Parser::WANT{$_} = 'bare_arglist';
    }
    for (qw(substr join)) {
        my ($sig, $ctx) = parse_sig('*@params');
	$P6C::Context::CONTEXT{$_} = $ctx;
	$hash->{$_} = new P6C::IMCC::Sub params => $sig,
                                         rettype => 'PerlString';
	$P6C::Parser::WANT{$_} = 'bare_arglist';
    }

    for (qw(index)) {
        my ($sig, $ctx) = parse_sig('*@params');
	$P6C::Context::CONTEXT{$_} = $ctx;
	$hash->{$_} = new P6C::IMCC::Sub params => $sig, rettype => 'PerlInt';
	$P6C::Parser::WANT{$_} = 'bare_arglist';
    }

    for (qw(length)) {
        my ($sig, $ctx) = parse_sig('$s');
	$P6C::Context::CONTEXT{$_} = $ctx;
	$hash->{$_} = new P6C::IMCC::Sub params => $sig,
                                         rettype => 'PerlInt';
	$P6C::Parser::WANT{$_} = 'bare_arglist';
    }

    for (qw(reverse)) {
        my ($sig, $ctx) = parse_sig('*@params');
	$P6C::Context::CONTEXT{$_} = $ctx;
	$hash->{$_} = new P6C::IMCC::Sub params => $sig,
                                         rettype => 'PerlArray';
	$P6C::Parser::WANT{$_} = 'bare_arglist';
    }

    for (qw(grep map)) {
        my ($sig, $ctx) = parse_sig('*@params');
	$P6C::Context::CONTEXT{$_} = $ctx;
	$hash->{$_} = new P6C::IMCC::Sub params => $sig,
                                         rettype => 'PerlArray';
	$P6C::Parser::WANT{$_} = "want_for_$_";
    }
}

sub add_code {
    my $funcs = shift;
    # (see docs above)
}

sub emit {

print <<'END';

.pcc_sub _substr non_prototyped
    .param PerlArray named
    .param object params
    $P0 = params
# n paras
    $I0 = params
    $S0 = params[0]
    $I1 = params[1]
    if $I0 == 2 goto substr_2
    $I2 = params[2]
    if $I0 > 4 goto substr_die
    if $I0 < 2 goto substr_die
    length $I3, $S0
    $I4 = $I3
    if $I2 >= 0 goto substr_34
# len negative, leave -len of string
    $I3 = $I3 - $I1
    $I3 = $I3 + $I2
    $I2 = $I3
substr_34:
    $S1 = ""
# # offset >= len?
    if $I1 >= $I4 goto substr_ret
    if $I0 == 4 goto substr_4
substr_3:
    substr $S1, $S0, $I1, $I2
substr_ret:
    $P1 = new PerlString
    $P1 = $S1
    .pcc_begin_return
    .return $P1
    .pcc_end_return
substr_4:
    $S2 = params[3]
    substr $S1, $S0, $I1, $I2, $S2
    params[2] = $S1
    goto substr_ret
substr_2:
    length $I2, $S0
    sub $I2, $I2, $I1
    goto substr_3
substr_die:
    set $S0, "wrong number of args for substr"
    $P0 = new PerlArray
    $P0[0] = $S0
    $P1 = new PerlHash
    find_lex $P2, "&die"
    .pcc_begin non_prototyped
    .arg $P1
    .arg $P0
    .pcc_call $P2
substr_ret_label:
    .pcc_end
    goto substr_ret
    end
.end

.pcc_sub _length non_prototyped
    .param PerlArray named
    .param object s
    $S0 = s
    length $I0, $S0
    $P1 = new PerlInt
    set $P1, $I0
    .pcc_begin_return
    .return $P1
    .pcc_end_return
    end
.end

.pcc_sub _reverse non_prototyped
    .param PerlArray named
    .param object orig_array
    $I0 = orig_array
    dec $I0
    $I1 = 0
    $P1 = new PerlArray
reverse_loopstart:
    $P2 = orig_array[$I0]
    $P1[$I1] = $P2
    inc $I1
    dec $I0
    if 0 <= $I0 goto reverse_loopstart
    .pcc_begin_return
    .return $P1
    .pcc_end_return
    end
.end

.pcc_sub _join non_prototyped
    .param PerlArray named
    .param object params
    .local int num_params
    num_params = params
    if num_params > 1 goto join_next
# Empty args:
    $S0 = ""
    goto join_ret
# At least one arg:
join_next:
    .local string separator
    separator = params[0]	# separator
    $S0 = params[1]		# accumulated string
    .local int counter
    counter = 2			# arg number
    goto join_test
join_loopstart:
    $S2 = params[counter]
    concat $S0, separator
    concat $S0, $S2
    inc counter
join_test:
    if num_params != counter goto join_loopstart
join_ret:
    $P2 = new PerlString
    $P2 = $S0
    .pcc_begin_return
    .return $P2
    .pcc_end_return
    end
.end

.pcc_sub _index non_prototyped
    .param PerlArray named
    .param object params
    $I2 = params
    if $I2 < 2 goto index_numarg_error
    $S0 = params[0]
    $S1 =  params[1]
    $I0 = 0
    $P1 = new PerlInt
    $P1 = $I0
    if $I3 < 3 goto index_2_arg
    index $I0, $S0, $S1
    $P1 = $I0
    goto index_end
index_2_arg:
    $I1 = params[2]
    index $I0, $S0, $S1, $I1
    $P1 = $I0
index_end:
    .pcc_begin_return
    .return $P1
    .pcc_end_return
    end
index_numarg_error:
    $S0 = "wrong number of args for index"
    $P0 = new PerlArray
    $P0[0] = $S0
    find_lex $P2, "&die"
    .pcc_begin non_prototyped
    .arg $P0
    .pcc_call $P2
pcc_ret_label:
    .pcc_end
    goto index_end
.end

.pcc_sub _time non_prototyped
    .param PerlArray named
    $P1 = new PerlNum
    time $N1
    set $P1, $N1
    .pcc_begin_return
    .return $P1
    .pcc_end_return
    end
.end

.pcc_sub _sleep non_prototyped
    .param PerlArray named
    .param object wait
    $I0 = wait
    sleep $I0
    .pcc_begin_return
    .pcc_end_return
    end
.end

.pcc_sub _print1 non_prototyped
    .param PerlArray named
    .param object p
    print p
    print "\n"
    .pcc_begin_return
    .pcc_end_return
    end
.end

.pcc_sub _print non_prototyped
    .param PerlArray named
    .param object params
    .local int num_elem
    .local int counter
    num_elem = params
    counter = 0
print_loopstart:
    if counter == num_elem goto print_loopend
    $P0 = params[counter]
    print $P0
    inc counter
    goto print_loopstart
print_loopend:
    .pcc_begin_return
    .pcc_end_return
    end
.end

.pcc_sub _exit non_prototyped
    .param PerlArray named
    .param object message
    print message
    print "\n"
    end
.end

.pcc_sub _die non_prototyped
    .param PerlArray named
    .param object params

    # setup $!: ####################
    .local object dollar_bang
    dollar_bang = new PerlString
    .local int num_params
    num_params = params
    if num_params == 0 goto die_unknown
    $P1 = new PerlString
    .local int counter
    counter = 0
die_loopstart:
    if num_params == counter goto die_loopend
    $P1 = params[counter]
    dollar_bang = dollar_bang . $P1
    inc counter
    goto die_loopstart
die_unknown:
    dollar_bang = "Unknown error."
die_loopend:
    store_global "_SV__BANG_", dollar_bang

    # Look for a CATCH handler: ###
    .local object try_stack
    find_global try_stack, "_AV_catchers"
    $I0 = try_stack
    if $I0 == 0 goto die_nohandler

    # Remove top catch handler
    dec $I0
    $P0 = try_stack[$I0]
    try_stack = $I0
    store_global "_AV_catchers", try_stack
    invoke $P0

die_nohandler:
    print dollar_bang
    print "\nDied (no handler).\n"
    end
.end

.pcc_sub _warn non_prototyped
    .param PerlArray named
    .param object params
    find_lex $P0, "&print"
    .pcc_begin non_prototyped
    .arg params
    .pcc_call $P0
warn_ret_label:
    .result $P1
    .pcc_end
    .pcc_begin_return
    .return $P1
    .pcc_end_return
.end

.pcc_sub _grep non_prototyped
    .param PerlArray named
    .param object params
    $P2 = new PerlArray
    set I3, params
    lt I3, 2, __grep_die_numargs
    set $P0, params[0]
    typeof S0, $P0
    ne S0, "Sub", __grep_die_arg1
    set I0, 1
    set I1, 0
    set I2, params
__grep_loop_start:
# call closure with current array value as arg
    save $P0
    set $P3, params[I0]
    $P5 = new PerlArray
    set $P5, 1
    set $P5[0], $P3
    save $P5
    bsr __grep_closure
###    restore $P4
    set I3, $P4
    lt I3, 1, __grep_check_end
    dec I3
    set $P6, $P4[I3]
__grep_return_check:
    if $P6, __grep_match_ok
    branch __grep_check_end
__grep_match_ok:
    set $P2[I1], $P3
    inc I1
__grep_check_end:
    inc I0
    eq I0, I2, __grep_loop_end
    branch __grep_loop_start
__grep_loop_end:
    .pcc_begin_return
    .return $P2
    .pcc_end_return
__grep_die_numargs:
    set S0, "wrong number of args for grep"
    $P0 = new PerlArray
    set $P0[0], S0
    save $P0
    bsr _die
    branch __grep_loop_end
__grep_die_arg1:
    set S0, "First argument to grep must be a closure"
    $P0 = new PerlArray
    set $P0[0], S0
    save $P0
    bsr _die
    branch __grep_loop_end
__grep_closure:
    pushp
    save $P1
    invoke
    .pcc_begin_return
    .return $P2
    .pcc_end_return
.end

.pcc_sub _map non_prototyped
    .param PerlArray named
    .param object params
    $P2 = new PerlArray
    set I3, params
    lt I3, 2, __map_die_numargs
    set $P0, params[0]
    typeof S0, $P0
    ne S0, "Sub", __map_die_arg1
    set I0, 1
    set I1, 0
    set I2, params
__map_loop_start:
# call closure with current array value as arg
    save $P0
    set $P3, params[I0]
    $P5 = new PerlArray
    set $P5, 1
    set $P5[0], $P3
    save $P5
    bsr __map_closure
###    restore $P4
    set I3, $P4
    lt I3, 1, __map_check_end
    dec I3
    set $P6, $P4[I3]
__map_return_check:
    if $P6, __map_match_ok
    branch __map_check_end
__map_match_ok:
    set $P2[I1], $P6
    inc I1
__map_check_end:
    inc I0
    eq I0, I2, __map_loop_end
    branch __map_loop_start
__map_loop_end:
    .pcc_begin_return
    .return $P2
    .pcc_end_return
__map_die_numargs:
    set S0, "wrong number of args for map"
    $P0 = new PerlArray
    set $P0[0], S0
    save $P0
    bsr _die
    branch __map_loop_end
__map_die_arg1:
    set S0, "First argument to map must be a closure"
    $P0 = new PerlArray
    set $P0[0], S0
    save $P0
    bsr _die
    branch __map_loop_end
__map_closure:
    pushp
    save $P1
    invoke
    .pcc_begin_return
    .pcc_end_return
.end

.sub __setup
#  P5 is array of commandline args
#  -lt: should we really preserve any registers here?
    $P1 = new PerlArray
    $P2 = new PerlString
    set I0, P5
# $0 / $$PROGRAM_NAME
    set $P2, P5[0]
    store_global "_SV_0", $P2
    store_global "_SV_$PROGRAM_NAME", $P2

# @ARGS == @ARGV[1..@ARGV.length]
    set I1, 0
    set I2, 1
    dec I0
    set $P1, I0
    branch __setup_arg_end
__setup_arg:
    set S0, P5[I2]
    set $P1[I1], S0
    inc I1
    inc I2
__setup_arg_end:
    lt I1, I0, __setup_arg
    store_global "_AV_ARGS", $P1

# exception handling:
    $P0 = new PerlArray
    store_global "_AV_catchers", $P0
    $P0 = new PerlUndef
    store_global "_SV__BANG_", $P0
    ret # Only called from top level
.end

.pcc_sub _install_catch non_prototyped
    .param object continuation
    .local object try_stack
    find_global try_stack, "_AV_catchers"
    $I1 = try_stack
    try_stack[$I1] = continuation
    store_global "_AV_catchers", try_stack
    .pcc_begin_return
    .pcc_end_return
    end
.end

.pcc_sub _pop_catch non_prototyped
    .local object try_stack
    find_global try_stack, "_AV_catchers"
    $I1 = try_stack
    dec $I1
    try_stack = $I1
    store_global "_AV_catchers", try_stack
    .pcc_begin_return
    .pcc_end_return
    end
.end

END

}
1;
