=head1 B<Nodes>

This file declares the different node-types that occur in the parse
tree.  These types may be generated from the parser output (see
C<Tree.pm>), or by the compiler itself during a pass.

=over

=item B<sv_literal>

Represents a scalar literal value.  

=over

=item B<type>

The literal's type, either one of the scalar types, C<PerlHash> for an
anonymous hash reference, or C<PerlArray> for an anonymous array
reference.

=item B<lval>

The thing's value.  This is B<not> the same as the code-generating
C<val> method.  For scalar types, C<lval> is a string; for reference
types, a reference to the contents' parse tree.

=back

=cut

use Class::Struct P6C::sv_literal => { qw(type $ lval $) };

sub P6C::sv_literal::min_listlen {
    my $obj = shift;
    return 1 if P6C::Util::is_scalar($obj->type);
    return 0;
}

sub P6C::sv_literal::max_listlen {
    my $obj = shift;
    return 1 if P6C::Util::is_scalar($obj->type);
    return undef;
}

=item B<variable>

A variable.

=over

=item B<type>

Either one of the scalar types, C<PerlHash>, or C<PerlArray>.

=item B<global>

True if the variable is global (currently unimplemented).

=item B<implicit>

True for implicit block parameters, e.g. C<$^a>.

=item B<topical>

True for members of the current topic, e.g. C<$.foo>.

=item B<name>

The variable's name.  For simple variables, this is the literal name,
including sigil (e.g. C<$foo>).  For "complicated" variables, this is
a reference to their parse tree (currently unimplemented).

=back

=cut

use Class::Struct P6C::variable => { qw(type     $
					global   $
					implicit $
					topical  $
					name     $) };

sub P6C::variable::min_listlen {
    my $obj = shift;
    return 1 unless $obj->{ctx}->flatten;
    return scalar(P6C::Util::is_scalar($obj->type));
}

sub P6C::variable::max_listlen {
    my $obj = shift;
    return 1 unless $obj->{ctx}->flatten;
    return scalar(P6C::Util::is_scalar($obj->type));
}

=item B<Binop>

A node type representing a binary infix operation.  Note that
comparison operators are not binary, since they can be chained.  Both
comma and semicolon are binary.

=over

=item B<op>

For simple operators, the operator name as a string.  For hyper
operators, a reference to a C<hype> node.

=item B<l>, B<r>

The left and right operands of the operator.

=back

=cut

use Class::Struct P6C::Binop => { qw(op $ l $ r $) };

=item B<hype>

A "hyped" operator.  Its single member, C<op> is the normal operator
that has been lifted.  If it turns out that other things besides
operators can be hyped, op may refer to more complex nodes.

=cut

use Class::Struct P6C::hype => { qw(op $) };

=item B<apply_rhs>

The right-hand side of an apply operation (e.g. the "foo()" in "$a.foo()").

=over

=item B<prop>

The function- or property-name being accessed (e.g. "foo").

=item B<subscripts>

A reference to a list of subscripts, or an empty array if none are
present.  Subscripts may be either an C<indices> node, as for variable
subscripts, or "something else" representing a parameter list.  This
is kind of ugly, and may change once these things actually get
implemented.

=back

=cut

use Class::Struct P6C::apply_rhs => { qw(prop $ subscripts @) };

=item B<indices>

A single subscript, e.g. C<[@xs]>.  

=over

=item B<type>

The type of thing on which the subscript operates, either C<PerlHash>
or C<PerlArray>.

=item B<indices>

The parse tree for the subscript contents.

=back

=cut

use Class::Struct P6C::indices => { qw(type $ indices $) };

=item B<subscript_exp>

An item and one or more indices.

=over

=item B<thing>

The expression or variable to be subscripted.

=item B<subscripts>

A reference to an array of indices.

=back

=cut

use Class::Struct P6C::subscript_exp => { qw(thing $ subscripts @) };

=item B<incr>

An increment or decrement operator.

=over

=item B<post>

True if the operator is a postincrement.

=item B<op>

The operator.

=item B<thing>

The incremented expression.

=back

=cut

use Class::Struct P6C::incr => { qw(post $ op $ thing $) };

=item B<prefix>

A prefix operator.  Many things are prefix operators: filetests
(possibly combined), declared functions, and unary C<->, C<~>, C<\\>,
and C<!>.

=over

=item B<name>

The operator name.

=item B<args>

The argument tree.

=back

=cut

use Class::Struct P6C::prefix => { qw(name $ args $) };

=item B<context>

A single context operator (eg the * in *@arr).

=over

=item B<ctx>

The operator.

=item B<thing>

The operand.

=back

=cut

use Class::Struct P6C::context => { qw(ctx $ thing $) };

=item B<pair>

A pair (e.g. C<< a => "pair" >>).

=over

=item B<l>, B<r>

The left and right operands of the pair constructor.

=back

=cut

use Class::Struct P6C::pair => { qw(l $ r $) };

=item B<compare>

A comparison sequence.  Its single member, C<seq>, is a reference to a
list of operators and operands in left-to-right order.  For example,
C<1 lt 3 lt "three"> becomes C<[1, "lt", 3, "lt", "three"]>

=cut

use Class::Struct P6C::compare => { qw(seq @) };

=item B<ternary>

A ternary operation.

=over

=item B<if>

The test.

=item B<then>

The "true" branch.

=item B<else>

The "false" branch.

=back

=cut

use Class::Struct P6C::ternary => { qw(if $ then $ else $) };

=item B<scope_class>

Qualifiers that occur before the variables in a declaration.

=over

=item B<scope>

The variable scope, e.g. "my", "our", "temp".

=item B<class>

The variable class, e.g. "int".

=back

=cut

use Class::Struct P6C::scope_class => { qw(scope $ class $) };

=item B<decl>

A declaration of one or more variables, not including initializers.

=over

=item B<qual>

The variables' scope/class.

=item B<vars>

The variables' names.

=item B<props>

A list of properties (e.g. "is foo(42)").

=back

=cut

use Class::Struct P6C::decl => { qw(qual $ vars $ props @) };

=item B<property>

A single variable, class, or function property.

=over

=item B<name>

The property name.

=item B<args>

The argument list (for e.g. C<something(1, 2)>).

=back

=cut

use Class::Struct P6C::property => { qw(name $ args $) };

=item B<but>

A node representing a "but" clause, e.g. C<$foo = 23 but false but Inf>.

=over

=item B<thing>

The exceptional thing.

=item B<buts>

A reference to an array of exception clauses.

=back

=cut

use Class::Struct P6C::but => { qw(buts @ thing $) };

=item B<adverb>

An adverbial modifier (i.e. ':').

=over

=item B<thing>

The left-hand side of the colon.

=item B<adv>

The right-hand side.

=back

=cut

use Class::Struct P6C::adverb => { qw(adv $ thing $) };

=item B<signature>

A sub parameter list (not argument list).

=over

=item B<positional>

A reference to an array of required positional parameters, or an empty
array if none.

=item B<optional>

A reference to an array of optional positional parameters, i.e. those
occurring after the ';', or an empty array if none.

=item B<required_named>

A reference to an array of required named parameters.

=item B<slurpy_array>

The final "slurping" parameter, or C<undef> if none present.

=item B<slurpy_named>

The "slurping" parameter for named arguments, or C<undef> if none
present.

=item B<optional_named>

A reference to an array of optional named parameters.

=back

=cut

use Class::Struct P6C::signature => { qw(positional @
                                         optional @
                                         required_named @
                                         slurpy_array $
                                         slurpy_named $
                                         optional_named @
                                        ) };

=item B<sigparam>

A subroutine parameter.

=over

=item B<type>

The type of the parameter.

=item B<zone>

Which zone the parameter is in -- positional, optional, named, etc.

=item B<var>

The variable.

=item B<traits>

Its traits.

=item B<init>

An initializer expression, or C<undef> if none.

Private variable: 'no_named' means that named arguments are not used.
For the caller, this means that the extra hash containing named
arguments is not passed. For the callee, this means that the extra
hash is not expected, and therefore is not read off of the passed-in
arguments. Both of these had better agree!

=back

=cut

use Class::Struct P6C::sigparam => { qw(type $
                                        zone $
                                        var $
                                        traits @
                                        init $
                                       ) };

=item B<initializer>

A parameter initializer (B<not> an initializer in a variable declaration).

=over

=item B<op>

The initializing operator, probably C<=>.

=item B<expr>

The initialization expression.

=back

=cut

use Class::Struct P6C::initializer => { qw(op $ expr $) };

=item B<sub_def>

A subroutine definition or declaration. This is created for all of:
 method X ...
 sub X ...
 rule X ...

The sub_def contains only the name and properties; the signature and
body are stored in a P6C::closure object.

=over

=item B<qual>

Sub scope. (my/our/...)

=item B<name>

=item B<props>

An array of subroutine properties, or an empty array if none.

=item B<closure>

The closure associated with this name.

=back

=cut

use Class::Struct P6C::sub_def => { qw(qual $
                                       name $
                                       props @
                                       closure P6C::closure)
                                  };

=item B<closure>

A closure, which may be either an anonymous function or the parameters
and body of a named subroutine -- or merely a bare block.

=over

=item B<params>

The subroutine parameter list, or C<undef> if no parameter list was
given.  The appropriate implicit parameter list then depends on context.

=item B<rettype>

The subroutine's return type.

=item B<block>

The sequence of statements making up the closure body.  This is a
reference to an array of statements, or a single C<yadda> node for
C<...> definitions, or undef for a declaration.

=item B<bare>

True only for bare blocks, eg
  $x = { print "hi!" }
or just
  { print "hi!" }

Appears to be used only to determine that a particular block is an
anonymous subroutine (Addcontext.pm's is_anon_sub() says that it is an
anonymous subroutine unless it is marked noreturn, or it is a sub_def,
or it's both a bare block and in a "last statement" context. Urk?)

=item B<is_rule>

A boolean flag saying whether the closure is a rule (as opposed to a sub).

=back

=cut

use Class::Struct P6C::closure => { qw(params $
                                       rettype $
                                       block $
                                       bare $
                                       is_rule $) };

=item B<yadda>

A node representing a C<...> statement.  Its single member, C<msg>, is
either undef or an appropriate error message to be generated if the
statement is reached.

=cut

use Class::Struct P6C::yadda => { qw(msg $) }; # i.e. "..."

=item B<guard>

A guard (statement modifier?), e.g. the C<unless> in C<die unless $foo>.

=over

=item B<name>

The modifier name, either "if", "unless", "while", "until", or "for".

=item B<expr>

=item B<test>

=back

=cut

use Class::Struct P6C::guard => { qw(name $ test $ expr $) };

=item B<directive>

A "use", "package", or "module" directive

=over

=item B<name>

The directive name (e.g. "use").

=item B<thing>

The directive's object (e.g. "perl" in "use perl").

=item B<args>

Whatever else is on the directive line.

=back

=cut

use Class::Struct P6C::directive => { qw(name $ thing $ args $) };

=item B<label>

A statement label.  It will appear before the labeled statement in a
statement sequence.

=item B<loop>

A C<loop(;;) { ... }> statement.

=over

=item B<init>

=item B<test>

=item B<incr>

The initialization, test, and increment expressions in the loop header.

=item B<block>

The block (sequence of statements, not closure) controlled by the
loop.

=back

=cut

use Class::Struct P6C::loop => { qw(init $ test $ incr $ block $) };

=item B<label>

=over

=item B<name>

The string name of the label.

=back

=cut

use Class::Struct P6C::label => { qw(name $) };

=item B<class_def>

A class definition

=over

=item B<qual>

Class scope.

=item B<name>

=item B<props>

=item B<block>

=back

=cut

use Class::Struct P6C::class_def => { qw(qual $ name $ props @ block $) };

=item B<Regexes or rules>

=cut

use Class::Struct P6C::rule => { qw(pat $ mod % immediate $) };
use Class::Struct P6C::rx_alt => { qw(branches @) };
use Class::Struct P6C::rx_seq => { qw(things @) };
use Class::Struct P6C::rx_hypo => { qw(var $ val $) };
use Class::Struct P6C::rx_cut => { qw(level $) };
use Class::Struct P6C::rx_any => { };
use Class::Struct P6C::rx_meta => { qw(name $) };
use Class::Struct P6C::rx_zerowidth => { qw(name $) };
use Class::Struct P6C::rx_atom => { qw(capture $ atom $) };
use Class::Struct P6C::rx_mod => { qw(mod $ args $) };
use Class::Struct P6C::rx_repeat => { qw(min $ max $ greedy $ thing $ negated $) };
use Class::Struct P6C::rx_assertion => { qw(thing $ negated $) };
use Class::Struct P6C::rx_call => { qw(name $ args $) };
use Class::Struct P6C::rx_oneof => { qw(rep $ negated $) };
use Class::Struct P6C::rx_wordsep => [];

# These pseudo-types are introduced by the tree manipulation functions:

=item B<P6C::ValueList>

List operators and functions with named parameters currently receive
their arguments in different formats -- the former as a tree of binary
',' ops, the latter as an array.  This is a common class to do context
handling for both.

=cut

use Class::Struct P6C::ValueList => { qw(vals @) };

=item B<P6C::Register>

Gratuitous object? Not quite... It's a way of passing a temporary
register in place of an unexpanded rvalue.  Useful for
e.g. autoincrement, where we have the register lying around.

NOTE: this would be a good place to take care of conversions between
register types.  Right now things are always passed in P regs, but we
could do better by passing back a wrapped S, I, or N register, which
would be promoted if necessary.  On the other hand, we might do better
using context to do this.  Hopefully all will be clear once things get
a bit more developed.

=cut

use Class::Struct P6C::Register => { qw(reg $ type $) };
use strict;

######################################################################
# Misc per-node-class utility functions:

sub P6C::signature::min {
    my $x = shift;
    return @{$x->positional} + @{$x->required_named};
}

sub P6C::signature::max {
    my $x = shift;
    return undef if ($x->slurpy_array || $x->slurpy_named);
    return @{$x->positional}
         + @{$x->optional}
         + @{$x->required_named}
         + @{$x->optional_named};
}

sub P6C::signature::max_nonslurpy_positional {
    my $x = shift;
    return @{$x->positional}
         + @{$x->optional};
}

# Crazily wrongheaded
sub P6C::sigparam::context {
    my $param = shift;
    my $type = 'PerlUndef';
    if ($param->type =~ /^(PerlArray|PerlUndef|int|num|str)$/) {
        $type = $param->type;
    }
    my $ctx = new P6C::Context type => $type;
    $ctx->{name} = $param->var->name;
#    $ctx->flatten(1) if $param->zone eq 'slurpy';
    return $ctx;
}

sub P6C::signature::arg_context {
    my $x = shift;
    # FIXME: This discards the distinction between named params before
    # or after a slurpy array param. (Exegesis 6 uses the difference)
    my %named; # { param name => P6C::SigParam }
    foreach (@{ $x->required_named || [] },
             @{ $x->optional_named || [] })
    {
        my $name = $_->var->name;
        $name = substr($name, 1); # Trim off sigil
        $named{$name} = $_->context;
    }

    my $positional = [ map { $_->context } @{ $x->positional } ];
    my $optional = [ map { $_->context } @{ $x->optional } ];
    my $slurpy_array = $x->slurpy_array ? $x->slurpy_array->context : undef;
    my $slurpy_named = $x->slurpy_named ? $x->slurpy_named->context : undef;

    my $context = new P6C::SigContext positional => $positional,
                                      optional => $optional,
                                      slurpy_array => $slurpy_array,
                                      slurpy_named => $slurpy_named,
                                      named => \%named;
    return $context;
}

# Return whether the named variable is in the parameter list. This is
# used for figuring out whether a variable is a parameter, a local, or
# whatever.
sub P6C::signature::paramvar {
    my ($sig, $var) = @_;
    return 1 if grep { $_->var->name eq $var } @{ $sig->positional };
    return 1 if grep { $_->var->name eq $var } @{ $sig->optional };
    return 1 if grep { $_->var->name eq $var } @{ $sig->required_named };
    return 1 if grep { $_->var->name eq $var } @{ $sig->optional_named };
    return 1 if $sig->slurpy_array && $var eq $sig->slurpy_array->var->name;
    return 1 if $sig->slurpy_named && $var eq $sig->slurpy_named->var->name;
    return;
}

# Create a context for the ith positional argument, assuming that the
# context object refers to a P6C::SigContext object.
sub P6C::signature::indexed_param {
    my ($sig, $i) = @_;

    die "expected signature, got $sig"
      if ! UNIVERSAL::isa($sig, 'P6C::signature');

    if ($i < @{ $sig->positional }) {
        return $sig->positional->[$i];
    }
    $i -= @{ $sig->positional };

    if ($i < @{ $sig->optional }) {
        return $sig->optional->[$i];
    }
    $i -= @{ $sig->optional };

    if ($sig->slurpy_array) {
        return $sig->slurpy_array;
    } else {
        warn "no parameter found";
        return;
    }
}

sub P6C::compare::size {
    (@{shift->seq()} - 1) / 2
}

######################## Rendering (for debugging) ################

sub render_obj_or_literal {
    my $thing = shift;
    return UNIVERSAL::can($thing, 'render') ? $thing->render : $thing;
}

# $sep - separator
# $term - terminator
# $pre - prefix if list
# $post - suffix if list
sub render_list {
    my ($thing, $sep, $term, $pre, $post) = @_;
    if (UNIVERSAL::can($thing, 'render')) {
        $thing = [ $thing ];
    }
    my $str = '';
    $str .= $pre if defined($pre);
    my @lines = grep { defined $_ } map { $_->render } @$thing;
    if (defined $term) { $_ .= $term foreach (@lines) }
    $str .= join(defined($sep) ? $sep : "", @lines);
    $str .= $post if defined($post);
    return $str;
}

sub P6C::sub_def::render {
    my $obj = shift;
    my $str = '';
    $str = $obj->qual . " " if defined $obj->qual;
    $str .= $obj->name if defined $obj->name;
    $str .= $obj->closure->render;
    return $str;
}

sub P6C::closure::render {
    my $obj = shift;
    my $str = "";
    $str .= "sub " if $obj->bare;
    $str .= "(" . $obj->params->render . ")" if defined($obj->params);
    $str .= " {" . render_list($obj->block, undef, ";\n") . "}";
    return $str;
}

sub P6C::rule::render {
    my $obj = shift;
    return "/" . $obj->pat->render . "/";
}

sub P6C::rx_alt::render {
    my $obj = shift;
    return join("|", map { $_->render } @{ $obj->branches });
}

sub P6C::rx_seq::render {
    my $obj = shift;
    return join("", map { $_->render } @{ $obj->things });
}

sub P6C::rx_repeat::render {
    my $obj = shift;
    my $str = $obj->thing->render;
    if (! $obj->min && ! $obj->max) {
        $str .= "*";
    } elsif ($obj->min == 1 && ! $obj->max) {
        $str .= "+";
    } elsif ($obj->min && $obj->max && $obj->min == $obj->max) {
        $str .= "<" . $obj->min . ">";
    } else {
        $str .= "<";
        $str .= $obj->min if defined($obj->min);
        $str .= ",";
        $str .= $obj->max if defined($obj->max);
        $str .= ">";
    }
    $str .= "?" unless $obj->greedy;
    return $str;
}

sub P6C::rx_atom::render {
    my $obj = shift;
    my $atom = $obj->atom->render;
    return $obj->capture ? "($atom)" : $atom;
}

sub P6C::rx_any::render {
    my $obj = shift;
    return '.';
}

sub P6C::rx_oneof::render {
    my $obj = shift;
    my $str = '<[';
    $str .= "^" if $obj->negated;
    $str .= $obj->rep;
    $str .= "]>";
    return $str;
}

sub extregex_unadjust_call {
    my ($call) = @_;
    my $args = $call->args;
    die unless $args->isa('P6C::ValueList');
    my @arglist = @{ $args->vals };
    shift @arglist;
    shift @arglist;
    shift @arglist;
    shift @arglist;
    return P6C::rx_call->new(name => $call->name,
                             args => P6C::ValueList->new(vals => \@arglist));
}

sub P6C::rx_call::render {
    my $obj = shift;
    my $str = "<" . render_obj_or_literal($obj->name);
    if (defined $obj->args) {
        my $obj2 = extregex_unadjust_call($obj);
        $str .= "(";
        $str .= render_list($obj2->args, ", ");
        $str .= ")";
    }
    $str .= ">";
    return $str;
}

sub P6C::decl::render {
    my $obj = shift;
    my $str = $obj->qual->scope; # Works for 'my', probably nothing else
    $str .= " ";
    $str .= render_list($obj->vars, ", ", undef, "(", ")");
    $str .= " " . $_->render foreach (@{ $obj->props });
    return $str;
}

sub P6C::prefix::render {
    my $obj = shift;
    my $str = render_obj_or_literal($obj->name);
    if (defined($obj->args)) {
        if (ref $obj->args) {
            $str .= render_list($obj->args, ", ", undef, "(", ")");
        } else {
            $str .= "(" . $obj->args . ")";
        }
    }
    return $str;
}

sub P6C::Binop::render {
    my $obj = shift;
    return $obj->l->render . " " . $obj->op . " " . $obj->r->render;
}

sub P6C::subscript_exp::render {
    my $obj = shift;
    return $obj->thing->render . join("", map { $_->render } @{ $obj->subscripts });
}

sub P6C::indices::render {
    my $obj = shift;
    my $inner = $obj->indices->render;
    if ($obj->type eq 'PerlArray') {
        return "[$inner]";
    } elsif ($obj->type eq 'PerlHash') {
        return "{$inner}";
    } else {
        return "($inner)"
    }
}

sub P6C::sv_literal::render {
    my $obj = shift;
    return $obj->lval;
}

sub P6C::variable::render {
    my $obj = shift;
    my $str = "";
    $str .= "*" if $obj->global;
    $str .= substr($obj->name, 0, 1);
    $str .= "^" if $obj->implicit;
    $str .= substr($obj->name, 1);
    return $str;
}

sub P6C::yadda::render {
    return "...";
}

sub P6C::signature::render {
    my $obj = shift;
    my $str = '';
    $str .= join(" ", map { $_->render } @{ $obj->positional });
    $str .= join(" ", map { $_->render } @{ $obj->optional });
    $str .= join(" ", map { $_->render } @{ $obj->required_named });
    $str .= '*' . $obj->slurpy_array->render if $obj->slurpy_array;
    $str .= '*' . $obj->slurpy_named->render if $obj->slurpy_named;
    $str .= join(" ", map { $_->render } @{ $obj->optional_named });
    return $str;
}

sub P6C::sigparam::render {
    my $obj = shift;
    my $str = $obj->type . " ";
    if ($obj->zone eq 'positional') {
    } elsif ($obj->zone eq 'optional') {
        $str .= "?";
    } elsif ($obj->zone eq 'required_named') {
        $str .= "+"; # FIXME!
    } elsif ($obj->zone eq 'optional_named') {
        $str .= "+"; # FIXME!
    }
    $str .= $obj->var->render;
    $str .= render_list($obj->traits) if $obj->traits;
    $str .= " = " . $obj->init->render if $obj->init;
    return $str;
}

sub P6C::guard::render {
    my $obj = shift;
    return $obj->name . "(" . $obj->test->render . ") {\n"
      . $obj->expr->render . "\n"
      . "}";
}

sub P6C::directive::render {
    my $obj = shift;
    my $str = $obj->name . " " . $obj->thing;
    return $str . " " . $obj->args; # ???
}

# I think this one might have to be context sensitive to figure out
# the divider characters, but I'm not sure.
sub P6C::ValueList::render {
    my $obj = shift;
    return join(", ", map { $_->render } @{ $obj->vals });
}

sub P6C::compare::render {
    my $obj = shift;
    return join(" ", map { ref($_) ? $_->render : $_ } @{ $obj->seq });
}

sub P6C::context::render {
    my $obj = shift;
    return $obj->ctx . $obj->thing->render;
}

sub P6C::label::render {
    my $obj = shift;
    return $obj->name . ": ";
}

sub P6C::debug_info::render { return '' }

"Yep.";

=back

=cut
