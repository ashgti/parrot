package P6C::Tree;

=head1 B<P6C::Tree>

Post-process the parse tree into the form used by the rest of the
compiler.  This is faster than making a neat parse tree directly.
Each parse tree node class (i.e. grammar rule) should define a C<tree>
function that takes a raw parse tree node, and returns a processed
one.  Both of these objects are blessed into the same class (don't
call tree on a processed node, or val on a raw one, or bad things will
happen).  See Nodes.pm for descriptions of the various nodes --
there's mostly a one-to-one correspondence between them and the
grammar rules, but a few of the rules that got inlined away reappear
as node types, while some of the lower-level rules are not preserved.

Remember to update the appropriate tree function here if you change
the grammar.  The functions here appear in roughly the same order as
the grammar rules in Parser.pm.

=cut

use strict;
use Carp 'confess';
use P6C::Nodes;
use Data::Dumper;
use P6C::Util qw(unimp flatten_leftop);

######################################################################

=head2 Parse tree handling utilities

=over

=item B<infix_right_seq($seq)>

Turn the list output from a rightop directive into a binary tree in the
proper order.

=item B<infix_left_seq($seq)>

Same for a leftop directive

=item B<flatten_list($node)>

Turn a leftop or rightop list into a list of parse tree nodes by
calling C<tree> on each of its elements.

=item B<maybe_tree($node)>

Call tree on C<$node> if appropriate.  Useful for handling optional
items and rules with empty altnerations.

=cut

sub infix_right_seq {
    my $x = shift;
    if (@$x == 1) {
	return $x->[0]->tree;
    }
    my $r = $x->[$#{$x}]->tree;
    for (my $i = $#{$x} - 1; $i > 0; $i -= 2) {
	my $op = ref($x->[$i]) ? $x->[$i]->tree : $x->[$i];
	my $l = $x->[$i - 1]->tree;
	$r = new P6C::Binop l => $l, r => $r, op => $op;
    }
    return $r;
}

sub infix_left_seq {
    my $x = shift;
    my @x = @$x;
    if (@x == 1) {
	return $x[0]->tree;
    }
    my $l = $x[0]->tree;
    for (my $i = 1; $i < $#{$x}; $i += 2) {
	my $op = ref($x[$i]) ? $x[$i]->tree : $x[$i];
	unless (ref($x[$i + 1])) {
	    die Dumper(\@x);
	}
	my $r = $x[$i + 1]->tree;
	$l = new P6C::Binop l => $l, r => $r, op => $op;
    }
    return $l;
}

##############################
# Define handlers for the simple right- and left-associative operators
# here.
BEGIN {
    no strict 'refs';
    for (qw(apply match muldiv addsub bitshift bitand
	    bitor logand logor log_AND log_OR comma semi
	    _closure_args rx_charclass)) {
	*{'P6C::'.$_.'::tree'} = sub {
	    my $x = shift;
	    my $ret = infix_left_seq($x->[1]);
	    return $ret;
	};
    }
    for (qw(pow)) {
	*{'P6C::'.$_.'::tree'} = sub {
	    my $x = shift;
	    return infix_right_seq($x->[1]);
	};
    }
}

##############################
# Things that are strings
sub scalar_tree {
    shift->[1];
}

{
    no strict 'refs';
    for (qw(name namepart sigil scope directive)) {
	*{"P6C\::$_\::tree"} = \&scalar_tree;
    }
}

##############################
# Having a separate rule for hyping is too expensive.
sub operator_tree {
    local $_ = shift->[1];
    if (/^\^(.+)/) {
	return new P6C::hype op => $1;
    }
    return $_;
}

{
    no strict 'refs';
    for (qw(apply_op incr_op pow_op prefix_op filetest_op match_op
	    muldiv_op addsub_op bitshift_op compare_op bitand_op bitor_op
	    logand_op logor_op range_op assign_op comma_op semi_op
	    log_AND_op log_OR_op)) {
	*{"P6C\::$_\::tree"} = \&operator_tree;
    }
}

##############################
# _simple_ optional items:

sub maybe_tree {
    my $x = shift;
    my $t = ref($x);
    if (!$t || @$x == 0) {
	return undef;
    } elsif (ref($x) eq 'ARRAY' && @$x == 1) {
	return $x->[0]->tree;
    } elsif (ref($x) =~ /^P6C::/) {
	return $x->tree;
    } else {
	confess "maybe_tree: can't handle $t ($x)";
    }
}

sub flatten_list {		# flatten a comma-list
    my $x = shift;
    my @x = map { $_->tree } @$x;
    return [@x];
}

##############################
# Control constructs:

sub want_two_things {
    my $x = shift;
    return new P6C::ValueList vals => [$x->[1]->tree,
				       $x->[2]->tree];
}

{
    no strict 'refs';
    for (qw(for given when while grep map sort)) {
	*{'P6C::want_for_'.$_.'::tree'} = \&want_two_things;
    }
}

sub P6C::want_for_foreach::tree {
    my $x = shift;
    return new P6C::ValueList vals => [maybe_tree($x->[1]),
				       $x->[3]->tree,
				       $x->[5]->tree];
}

sub P6C::want_for_default::tree {
    my $x = shift;
    return $x->[1]->tree;
}

sub P6C::want_for_if::tree {
    my $x = shift;
    my @conditions = ([undef, $x->[1]->tree, $x->[2]->tree]);
    foreach (@{$x->[3]}) {
	push @conditions, [$_->[1], $_->[2]->tree, $_->[3]->tree];
    }
    if (ref($x->[4]) && @{$x->[4]} > 0) {
	push @conditions, ['else', 1, $x->[4][0][2]->tree];
    }
    return [@conditions];
}

##############################
# Literals
sub P6C::sv_literal::tree {
    my $x = shift;
    my ($type, $val);
    if ($x->[1] eq '{') {
	$type = 'PerlHash';
	$val = $x->[3]->tree;
    } elsif ($x->[1] eq '[') {
	$type = 'PerlArray';
	if (@{$x->[3]} > 0) {
	    $val = $x->[3][0]->tree;
	} else {
	    $val = undef;
	}
    } elsif (!ref($x->[1])) {
	$x->[1] =~ s/_//g;
	if ($x->[1] =~ /\./) {
	    $type = 'num';
	} else {
	    $type = 'int';
	}
	$val = $x->[1];
    } else {
	$type = 'str';
	$val = qq{"$x->[1][2]"};	# XXX: they're all just strings.
    }
    return new P6C::sv_literal type => $type, lval => $val;
}

sub P6C::av_seq::tree {
    shift->[1]->tree;
}

sub P6C::hv_seq::tree {
    my $x = shift;
    return [map { $_->tree } @{$x->[1]} ];
}

##############################
# Variables
sub varname {
    my $x = shift;
    if (@$x == 2) {		# plain Jane or punctuation
	if (ref $x->[1]) {
	    local $_ = $x->[1]->tree;
	    return (scalar(/^\*/), 0, scalar(/^\./), $_);
	}
	return (0, 0, 0, $x->[1]);
    } elsif (@$x == 4) {	# $^a and friends
	return (0, 1, 0, $x->[3]->tree);
    } else {			# ugly.
	return (@{$x->[1]} > 0, 0, 0, $x->[4][1]->tree);
    }
}

sub P6C::variable::tree {
    my %sigil_types = qw($ PerlUndef % PerlHash @ PerlArray & PerlSub);
    my $x = shift;
    my ($global, $implicit, $topical, $name) = varname($x->[3]);
    unless (ref($name)) {
	$name = $x->[1][1].$name; # tack sigil back on.
    }
    return P6C::variable->new(type => $sigil_types{$x->[1][1]},
			      name => $name,
			      global => $global,
			      topical => $topical,
			      implicit => $implicit);
}

sub P6C::scalar_var::tree {
    my $x = shift;
    my $sigil = $x->[1];
    $x->[1] = ['sigil', $sigil];
    return P6C::variable::tree($x);
}

*P6C::nonscalar_var::tree = *P6C::scalar_var::tree;

######################################################################
# Operators

##############################
sub P6C::hv_indices::tree {
    my $x = shift;
    if (!ref($x->[1])) {
	# stringify this.
	return new P6C::sv_literal type => 'PerlString',
				   lval => qq{"$x->[1]"};
    }
    return $x->[1]->tree;
}

##############################
sub P6C::arglist::tree {
    my $x = shift;
    if (@{$x->[2]} > 0) {
	return $x->[2][0]->tree;
    }
    return undef;		# XXX: probably bad
}

##############################
sub P6C::subscript::tree {
    my $x = shift;
    if (@$x == 4) {
	# i.e. $x(23)
	return new P6C::indices type => 'Sub', indices => maybe_tree($x->[2]);
    }
    my $type = ($x->[2] eq '{' ? 'PerlHash' : 'PerlArray');
    my $indices = $x->[5]->tree;
    return new P6C::indices type => $type, indices => $indices;
}

##############################
sub P6C::subscriptable::tree {
    my $x = shift;
    if (@$x == 2) {
	return $x->[1]->tree;
    } elsif ($x->[1] eq '(') {
	if (@{$x->[3]} > 0) {
	    return $x->[3][0]->tree;
	}
	return undef;		# XXX: probably bad
    } elsif ($x->[1] eq '.') {
	# XXX: fixme
	return ['topical', maybe_tree($x->[4])];
    } else {
	my $args = $x->[4]->tree;
	$args = new P6C::ValueList
	    vals => [$args ? flatten_leftop($args, ',') : ()];
	return P6C::prefix->new(name => $x->[1]->tree,
				args => $args);
    }
}

##############################
sub P6C::term::tree {
    my $x = shift;
    if (@$x == 2) {
	return $x->[1]->tree;
    }
    if (@$x == 5) {
	return P6C::prefix->new(name => 'readline',
				args => (@{$x->[3]} > 0 ? $x->[3][0]->tree
					: (new P6C::variable type => 'PerlIO',
					   name => '*ARGS')));
    }
    if (!ref($x->[1])) {
	return new P6C::context ctx => $x->[1],
	    thing => $x->[3]->tree;
    }
    my $thing = $x->[1]->tree;
    if (@{$x->[3]} == 0) {
	return $thing;
    }

    my @subsc;
    foreach (@{$x->[3]}) {
	push @subsc, $_->tree;
    }
    return new P6C::subscript_exp thing => $thing, subscripts => [@subsc];
}

##############################
# XXX: complete cruft
sub P6C::apply_rhs::tree {
    my $x = shift;
    my (@subsc, $prop);
    if (@$x == 2) {
	for (@{$x->[1]}) {
	    push @subsc, $_->tree;
	}
    } else {
	$prop = $x->[1]->tree;
	for (@{$x->[3]}) {
	    push @subsc, $_->tree;
	}
    }
    return new P6C::apply_rhs prop => $prop, subscripts => [@subsc];
}

##############################
sub P6C::incr::tree {
    my $x = shift;
    if (@$x == 4) {
	return new P6C::incr post => 0,
	    op => $x->[1]->tree,
		thing => $x->[3]->tree;
    }
    if (@{$x->[2]} > 0) {
	return new P6C::incr post => 1,
	    op => $x->[2][0]->tree,
		thing => $x->[1]->tree;
    }
    return $x->[1]->tree;
}

##############################
sub P6C::prefix::tree {
    my $x = shift;
    if (@$x == 2) {
	return $x->[1]->tree;
    }
    return P6C::prefix->new(name => $x->[1]->tree,
			    args => $x->[3]->tree);
}

##############################
sub P6C::pair::tree {
    my $x = shift;
    if (@$x == 5) {
	return new P6C::pair l => $x->[1]->tree,
	    r => $x->[4]->tree;
    }
    return new P6C::pair l => $x->[1]->tree,
	r => $x->[3]->tree;
}

##############################
sub P6C::maybe_pair::tree {
    my $x = shift;
    if (@$x == 5) {
	return new P6C::pair l => $x->[1]->tree,
	    r => $x->[4]->tree;
    }
    if (@{$x->[2]} > 0) {
	return new P6C::pair l => $x->[1]->tree,
			     r => $x->[2][0][-1]->tree;
    }
    return $x->[1]->tree;
}

##############################
sub P6C::compare::tree {
    my $x = shift;
    if (@{$x->[1]} == 1) {
	return $x->[1][0]->tree;
    }
    return new P6C::compare seq => [ map { $_->tree } @{$x->[1]} ];
}

##############################
sub P6C::range::tree {
    my $x = shift;
    if (@{$x->[2]} == 0) {
	return $x->[1]->tree;
    }
    my $op = $x->[2][0][1];
    my $r = $x->[2][0][2];
    return new P6C::Binop op => $op->tree,
			  l => $x->[1]->tree,
			  r => $r->tree;
}

##############################
sub P6C::ternary::tree {
    my $x = shift;
    if (@{$x->[2]} == 0) {
	return $x->[1]->tree;
    }
    my $then = $x->[2][0][2];
    my $else = $x->[2][0][4];
    return P6C::ternary->new(if => $x->[1]->tree,
			     then => $then->tree,
			     else => $else->tree);
}

##############################
sub P6C::scope_class::tree {
    my $x = shift;
    if (@$x == 2) {
	return new P6C::scope_class class => $x->[1]->tree;
    }
    if (@{$x->[3]} == 0) {
	return new P6C::scope_class scope => $x->[1]->tree;
    }
    return new P6C::scope_class scope => $x->[1]->tree,
	class => $x->[3][0]->tree;
}

##############################
sub P6C::class::tree {
    my $x = shift;
    $x->[1]->tree;
}

##############################
sub P6C::property::tree {
    my $x = shift;
    return P6C::property->new(name => $x->[1]->tree,
			      args => maybe_tree($x->[3]));
}

##############################
sub P6C::props::tree {
    my $x = shift;
    if (@$x == 1) {
	return [];
    }
    my @ret;
    push @ret, $x->[3]->tree;
    foreach my $ap (@{$x->[4]}) {
	push @ret, $ap->[$#{$ap}]->tree;
    }
    return [@ret];
}

##############################
sub P6C::decl::tree {
    my $x = shift;
    if ($x->[1] eq '(') {
	return P6C::decl->new(vars => P6C::Tree::flatten_list($x->[3]),
			      props => maybe_tree($x->[5]));
    }
    return P6C::decl->new(vars => $x->[1]->tree,
			  props => maybe_tree($x->[2]));
}

##############################
sub P6C::assign_lhs::tree {
    my $x = shift;
    if (@$x == 2) {
	return $x->[1]->tree;
    }
    if (@{$x->[2]} == 0) {
	return $x->[1]->tree;
    }
    my $decl = $x->[2]->tree;
    $decl->qual($x->[1]->tree);
    return $decl;
}

##############################
sub P6C::assign::tree {
    my $x = shift;
    # rightward-associating operator sequence
    my @things = ($x->[1], map { @{$_}[1,2] } @{$x->[2]});
    die if @things % 2 == 0;	# sanity check.
    return infix_right_seq(\@things);
}

##############################
sub P6C::scalar_expr::tree {
    my $x = shift;
    if (@{$x->[2]} > 0) {
	my @buts;
	foreach (@{$x->[2]}) {
	    push @buts, $_->[2]->tree;
	}
	return new P6C::but buts => [@buts], thing => $x->[1]->tree;
    }
    return $x->[1]->tree;
}

##############################
sub P6C::adverb::tree {
    my $x = shift;
    if (@{$x->[2]} > 0) {
	return P6C::adverb->new(adv => $x->[2][0][2]->tree,
				thing => $x->[1]->tree);
    }
    return $x->[1]->tree;
}

##############################
sub P6C::expr::tree {
    shift->[1]->tree;
}

######################################################################
# Parameters

sub P6C::params::tree {
    my $x = shift;
    if (@$x == 1) {
	return undef;
    }
    my ($req, $opt);
    if (ref($x->[4])) {
	$req = $x->[4]->tree;
    } else {
	$req = [];
    }
    if (ref($x->[5])) {
	$opt = $x->[5]->tree;
    } else {
	$opt = [];
    }
    if (ref($x->[6])) {
	my $arg = new P6C::variable type => 'PerlArray',
	    name => '@'.$x->[6][5]->tree;
	return P6C::params->new(req => $req, opt => $opt,
				rest => P6C::param->new(var => $arg));
    }
    return P6C::params->new(req => $req, opt => $opt);
}

##############################
sub P6C::maybe_params::tree {
    my $x = shift;
    if (!ref($x) || @{$x->[1][1]} == 0) {
	return [];
    }
    return [map { $_->tree } @{$x->[1][1]}];
}

##############################
sub P6C::opt_params::tree {
    my $x = shift;
    if (@$x == 1) {
	return [];
    }
    return $x->[4]->tree;
}

##############################
sub P6C::param::tree {
    my $x = shift;
    return new P6C::param qual => maybe_tree($x->[1]),
			  var => $x->[2]->tree,
			  props => maybe_tree($x->[3]),
			  init => maybe_tree($x->[$#{$x}]);
}

##############################
sub P6C::initializer::tree {
    my $x = shift;
    return new P6C::initializer op => $x->[1]->tree, expr => $x->[2]->tree;
}

######################################################################
# Statements

sub P6C::prog::tree {
    maybe_tree(shift->[2])
}

##############################
sub P6C::stmts::tree {
    my $x = shift;
    if (@$x == 1) {
	return [];
    }
    my @ret = ();
    foreach my $c (@{$x->[1]}) {
        my @child = $c->tree;	# stmt return array when -g
	push @ret, @child if (@child);   # Ignore empty statements
    }
    return [@ret];
}

##############################
sub P6C::maybe_label::tree {
    my $x = shift;
    return P6C::Tree::maybe_tree($x->[1]);
}

##############################
sub P6C::label::tree {
    shift->[1]->tree
}

sub stmt_guard {
    my $x = shift;
    if ($x->[1] eq 'for') {
	return ('for', $x->[2]->tree);
    }
    return ($x->[1], $x->[3]->tree);
}

##############################
# Debugging
sub P6C::debug_info::tree {
    my $x = shift;
    my ($file, $l, $c, $stmt, $txt) = @$x;
    my $di = bless [$file, $l, $c, $txt ], 'P6C::debug_info';
    return ($di, $stmt->tree);
}


##############################
sub P6C::stmt::tree {
    my $x = shift;
    if (@$x == 3) {		# expr
	if (@{$x->[2]} > 0) {	# with guard.
	    my ($name, $test) = stmt_guard($x->[2][0][2]);
	    return P6C::guard->new(name => $name,
				   test => $test,
				   expr => $x->[1]->tree);
	}
	return $x->[1]->tree;	# no guard.

    } elsif (@$x == 2) {       # empty statement
        return undef;

    } elsif ($x->[1] eq 'method') {
	my $sc = P6C::closure->new(params => maybe_tree($x->[4]),
				   block => $x->[6]->tree);
	return P6C::sub_def->new(qual => 'method', # XXX: fixme
				 name => $x->[3]->tree,
				 props => maybe_tree($x->[5]),
				 closure => $sc);

    } elsif ($x->[1] eq 'loop') {
	return P6C::loop->new(init =>  maybe_tree($x->[4]),
			     test =>  maybe_tree($x->[6]),
			     incr =>  maybe_tree($x->[8]),
			     block => maybe_tree($x->[10]));

    } elsif ($x->[2] eq 'class') {
	return P6C::class_def->new(qual => maybe_tree($x->[1]),
				   name => $x->[4]->tree,
				   props => maybe_tree($x->[6]),
				   block => $x->[7]->tree);

    } elsif ($x->[2] =~ /^(?:sub|rule)$/) {
	# Make sure we take care of declarations as well as
	# definitions:
	my $block = $x->[$#{$x}][1];
	my $sc = P6C::closure->new(params => maybe_tree($x->[5]),
				   block => ($block eq ';' ? undef
					     : $block->tree));
	return P6C::sub_def->new(qual => maybe_tree($x->[1]),
				 name => $x->[4]->tree,
				 props => maybe_tree($x->[7]),
				 closure => $sc);
	
    } elsif ($x->[2] eq ':') {	# label
	return new P6C::label name => $x->[1]->tree;

    } else {			# use/module/package directive
	return P6C::directive->new(name => $x->[1]->tree,
				   thing => $x->[3]->tree,
				   args => maybe_tree($x->[5]));
    }
}

##############################
sub P6C::block::tree {
    my $x = shift;
    if ($x->[2] eq '...') {
	return new P6C::yadda;
    }
    return maybe_tree($x->[2]) || [];
}

##############################
sub P6C::closure::tree {
    my $x = shift;
    my ($block, $arg_index);
    $block = $x->[$#{$x}]->tree;
    if (@$x == 2) {		# bare block
	return new P6C::closure params => undef, block => $block, bare => 1;
    } if (@$x == 5) {		# no parens
	$arg_index = 3;
    } else {			# with parens
	$arg_index = 4;
    }
    if (@{$x->[$arg_index]} == 0) {
	return new P6C::closure params => undef, block => $block;
    }

    my $p = $x->[$arg_index][0]->tree;

    return new P6C::closure params => $p, block => $block;
}

######################################################################
# Miscellaneous maybes:

sub P6C::maybe_decl::tree {
    my $x = shift;
    if (@$x == 1) {
	return undef;
    }
    if (@$x == 4) {
	return P6C::decl->new(qual => undef,
			      vars => [$x->[1]->tree],
			      props => maybe_tree($x->[3]));
    }
    return P6C::decl->new(qual => $x->[1]->tree,
			  vars => [$x->[3]->tree],
			  props => maybe_tree($x->[4]));
}

##############################
sub P6C::maybe_comma::tree {
    my $x = shift;
    if (@$x == 2 && ref $x->[1]) {
	return $x->[1]->tree;
    }
    return new P6C::ValueList vals => [];
}

sub P6C::bare_arglist::tree {
    my $x = shift;
    return $x->[2]->tree;
}

##############################
sub P6C::maybe_namepart::tree {
    my $x = shift;
    if (ref($x) && @$x == 2) {
	return $x->[1]->tree;
    }
    return undef;
}

##############################
sub P6C::nothing::tree {
    return new P6C::ValueList vals => [];
}

*P6C::no_args::tree = *P6C::nothing::tree;

##############################
sub P6C::rule::tree {
    my $x = shift;
    return new P6C::rule mod => maybe_tree($x->[1]), pat => $x->[3]->tree;
}

sub P6C::pattern::tree {
    my $x = shift;
    if (@$x == 4) {
	return new P6C::rule pat => $x->[2]->tree;
    }
    return new P6C::rule mod => maybe_tree($x->[3]), pat => $x->[5]->tree,
	immediate => ($x->[1] eq 'm');
}

sub P6C::rx_alt::tree {
    my $x = shift;
    return new P6C::rx_alt branches => [map { $_->tree } @{$x->[1]} ];
}

sub P6C::rx_seq::tree {
    my $x = shift;
    return new P6C::rx_seq things => [map { $_->tree } @{$x->[1]} ];
}

sub P6C::rx_maybe_hypo::tree {
    my $x = shift;
    if (@$x == 2) {
	if (ref $x->[1]) {
	    # rx_element
	    return $x->[1]->tree;
	} elsif ($x->[1] eq '^') {
	    return new P6C::rx_beg;
	} elsif ($x->[1] eq '$') {
	    return new P6C::rx_end;
	} elsif ($x->[1] eq '^^' || $x->[1] eq '$$') {
	    unimp 'BOL or EOL';
	} else {
	    # Cut
	    return new P6C::rx_cut level => length($x->[1]);
	}
    } elsif (@$x == 5) {
	# Scalar hypo
	return new P6C::rx_hypo var => $x->[1]->tree,
	    val => $x->[4]->tree;
    } else {
	# array hypo
	return new P6C::rx_hypo var => $x->[1]->tree,
	    val => new P6C::rx_atom(repeat => $x->[6]->tree,
				    atom => $x->[4]->tree);
    }
}

sub P6C::rx_element::tree {
    my $x = shift;
    my $ret;
    if (@$x == 4) {
	# codeblock
	$ret = new P6C::rx_atom atom => $x->[3]->tree;
    } else {
	# atom
	$ret = $x->[1]->tree;
	if (@{$x->[2]}) {
	    my $rep = $x->[2][0]->tree;
	    $rep->thing($ret);
	    $ret = $rep;
	}
    }
    return $ret;
}

sub P6C::rx_atom::tree {
    my $x = shift;
    my ($atom, $capture);
    if (@$x == 5) {
	if ($x->[1] =~ /\[\(/) {
	    # group, capturing or otherwise.
	    $atom = $x->[3]->tree;
	    $capture = $x->[1] eq '(';
	} else {
	    # modifiers
	    return $x->[3]->tree;
	}
    } elsif (@$x == 6) {
	# assertion
	my $ret = $x->[4]->tree;
	if (length($x->[3]) > 0) {
	    $ret->negated(!$ret->negated);
	}
	return $ret;
    } elsif (ref $x->[1]) {
	# variable
	$atom = $x->[1]->tree;
    } elsif ($x->[1] eq '.') {
	# metachar
	$atom = new P6C::rx_meta name => '.';

    } elsif ($x->[1] =~ /^\\(.+)/) {
	# metachar
	$atom = new P6C::rx_meta name => $1;
    } else {
	$atom = new P6C::sv_literal type => 'PerlString', lval => qq{"$x->[1]"};
    }
    return new P6C::rx_atom capture => $capture, atom => $atom;
}

sub P6C::rx_mod::tree {
    my $x = shift;
    my $ret = new P6C::rx_mod mod => $x->[2];
    if (@{$x->[4]} > 0) {
	$ret->args($x->[4][2]->tree);
    }
    return $ret;
}

sub repspec_item {
    my $i = shift;
    if (ref $i) {
	return $i->tree;
    } else {
	return $i;
    }
}

sub P6C::rx_repeat::tree {
    my $x = shift;
    my $greedy = length($x->[2]) == 0;
    my ($min, $max);
    if (@{$x->[1]} == 2) {
	my $t = $x->[1][1];
	if ($t eq '*') {
	    $min = 0; $max = undef;
	} elsif ($t eq '+') {
	    $min = 1; $max = undef;
	} else {
	    # '?'
	    $min = 0; $max = 1;	    
	}
    } else {
	my $repspec = $x->[1][4];
	if (@$repspec == 4) {
	    # two items
	    $min = repspec_item($repspec->[1][1]);
	    $max = repspec_item($repspec->[3][1]);
	} else {
	    # one number
	    $min = $max = 0 + $repspec->[1];
	}
    }
    return new P6C::rx_repeat min => $min, max => $max, greedy => $greedy;
}

sub P6C::rx_assertion::tree {
    my $x = shift;
    if (@$x == 2) {
	if (ref $x->[1]) {
	    # Variable, subrule, or character class
	    return $x->[1]->tree;
	} elsif ($x->[1] eq '.') {
	    # Single grapheme
	    unimp "Logical grapheme";
	} elsif ($x->[1] =~ /^'/) {
	    return new P6C::rx_assertion
		thing => P6C::sv_literal->new(type => 'str', lval => $x->[1]);
	} else {
	    die "internal error";
	}
    } elsif ($x->[1] eq '-') {
	# Negated assertion
	my $thing = $x->[3]->tree;
	$thing->negated(!$thing->negated);
	return $thing;
    } elsif ($x->[1] eq '(') {
	return new P6C::rx_assertion thing => $x->[2]->tree;
    } elsif ($x->[1] eq '{') {
	unimp "runtime-interpolated pattern";
    } else {
	die;
    }
}

sub P6C::rx_rulename::tree {
    my $x = shift;
    return $x->[1]->tree;
}

sub P6C::rx_call::tree {
    my $x = shift;
    my $pat = $x->[1]->tree;
    if ($x->[2] eq '(') {
	# Call with args
	return new P6C::rx_call name => $pat, args => $x->[4]->tree;
    } elsif ($x->[2] eq ':') {
	# Call with string
	my $strval = $x->[4];
	$strval =~ s/^\s*//;
	return new P6C::rx_call name => $pat,
	    args => new P6C::sv_literal type => 'str', lval => $strval;
    } else {
	# Call with pattern or no args
	my $args = maybe_tree($x->[2]);
	unless (defined $args) {
	    $args = new P6C::ValueList vals => [];
	}
	return new P6C::rx_call name => $pat, args => $args;
    }
}

sub P6C::rx_cc_neg::tree {
    my $x = shift;
    my $neg = $x->[1] eq '-';
    my $class = $x->[2];
    if (@$class == 4) {
	# Bracketed assertion
	my $ret = $class->[2]->tree;
	$ret->negated(!$ret->negated) if $neg;
	return $ret;

    } elsif (ref $class->[1]) {
	# Non-bracketed assertion
	return new P6C::rx_assertion
	    thing => new P6C::rx_call(name => $class->[1]->tree,
				      args => new P6C::ValueList(vals => [])),
	    negated => $neg;

    } else {
	# enumerated class
	return new P6C::rx_oneof
	    rep => substr($class, 1, length($class) - 2),
	    negated => $neg;
    }
}

1;
