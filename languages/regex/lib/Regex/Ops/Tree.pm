package Regex::Ops::Tree;

# Copyright (C) 2002-2006, Parrot Foundation.
# $Id$

use base 'Exporter';
use strict;
use warnings;
use Carp qw(confess);

@Regex::Ops::Tree::EXPORT = qw(rop);

# Tree operators
#
# This package defines operators that can be used to construct a
# syntax tree for a regular expression.
#

# Core ops generated by parser
# ----------------------------
# match : Match a single codepoint
# charclass : Match a character class
# seq
# alternate : Match R or S at the same point?
# multi_match : Match m..n repetitions of R
# group : Capture a group
# scan : Scan through input until R matches
# atend : At the end of the input?
# advance : Unconditionally advance 1 char
# code : Embedded code, in some language

# Stuff that is used for optimization
# -----------------------------------
# nop : Do nothing (placeholder)
# check : Check to be sure we are n chars away from the end of the string

@Regex::Ops::Tree::_atom::ISA   = qw(Regex::Ops::Tree);
@Regex::Ops::Tree::_onearg::ISA = qw(Regex::Ops::Tree);

@Regex::Ops::Tree::match::ISA       = qw(Regex::Ops::Tree::_atom);
@Regex::Ops::Tree::charclass::ISA   = qw(Regex::Ops::Tree::_atom);
@Regex::Ops::Tree::classpieces::ISA = qw(Regex::Ops::Tree::_atom);
@Regex::Ops::Tree::seq::ISA         = qw(Regex::Ops::Tree);
@Regex::Ops::Tree::alternate::ISA   = qw(Regex::Ops::Tree);
@Regex::Ops::Tree::multi_match::ISA = qw(Regex::Ops::Tree);
@Regex::Ops::Tree::group::ISA       = qw(Regex::Ops::Tree::_onearg);
@Regex::Ops::Tree::call::ISA        = qw(Regex::Ops::Tree::_onearg);
@Regex::Ops::Tree::rule::ISA        = qw(Regex::Ops::Tree::_onearg);
@Regex::Ops::Tree::scan::ISA        = qw(Regex::Ops::Tree::_onearg);
@Regex::Ops::Tree::atend::ISA       = qw(Regex::Ops::Tree);
@Regex::Ops::Tree::advance::ISA     = qw(Regex::Ops::Tree::_atom);

@Regex::Ops::Tree::nop::ISA   = qw(Regex::Ops::Tree);
@Regex::Ops::Tree::check::ISA = qw(Regex::Ops::Tree);

@Regex::Ops::Tree::call::ISA = qw(Regex::Ops::Tree);
@Regex::Ops::Tree::code::ISA = qw(Regex::Ops::Tree);

# Construct a new op
sub op {
    my ( $class, $name, $args, %opts ) = @_;

    $class = ref($class) if ref $class;
    while (1) {
        last if UNIVERSAL::isa( "${class}::$name", 'Regex::Ops::Tree' );
        $class =~ s/::\w+$// or confess "Called op on invalid class $_[0]";
    }
    $class = "${class}::$name";
    my $self = bless {
        name => $name,
        args => $args || [],
        %opts
    }, $class;

    return $self->init();
}

# Default initialization, to be overridden in subclasses.
sub init { return shift(); }

# Exportable convenience function
sub rop {
    return __PACKAGE__->op(@_);
}

use vars qw(%MARKERS);

sub mark {
    my $name = shift || '';
    my $number = ++$MARKERS{$name};
    $number = '' if ( $number == 1 ) && ( $name ne '' );
    return bless( [ 'label', "\@$name$number" ], 'asm_op' );
}

############################################################################
# OPTIMIZATION INFORMATION
#
# All of this is only for computing information that may be useful in
# optimizing generated regular expressions. It is not needed for basic
# compilation.
#
# Probably the best way to understand this stuff is to look at
# where it's used, mostly in TreeOptimize.pm.
############################################################################

sub order_startset {
    my $start = shift;

    # Must maintain invariant that '' comes first, if it exists.
    my @null;
    push( @null, '' ) if exists $start->{''};
    delete $start->{''};
    return ( @null, keys %$start );
}

# Defaults
sub minlen   { confess "unimplemented" }
sub maxlen   { confess "unimplemented" }
sub startset { confess "unimplemented" }
sub hasback  { 0 }
sub dfa_safe { 0 }

# Superclass for ops like scan(R) that contain a single subexpression.
# By default, all calls are propagated to the subexpression.
package Regex::Ops::Tree::_onearg;
sub minlen   { my ($op) = @_; $op->{args}->[0]->minlen() }
sub maxlen   { my ($op) = @_; $op->{args}->[0]->maxlen() }
sub startset { my ($op) = @_; $op->{args}->[0]->startset() }
sub hasback  { my ($op) = @_; $op->{args}->[0]->hasback() }
sub dfa_safe { my ($op) = @_; $op->{args}->[0]->dfa_safe() }

# Superclass for ops that match a single input atom (eg a character or
# character class.)
package Regex::Ops::Tree::_atom;

sub minlen   { 1 }
sub maxlen   { 1 }
sub dfa_safe { 1 }
sub hasback  { 1 }

# Sequences of regexes like RS
package Regex::Ops::Tree::seq;

sub init {
    my ($self) = @_;

    if ( @{ $self->{args} } == 0 ) {
        delete $self->{name};
        delete $self->{args};
        return $self->op( 'nop', [], %$self );
    }
    elsif ( @{ $self->{args} } == 1 && 2 == keys %$self ) {
        return $self->{args}->[0];
    }
    else {
        return $self;
    }
}

sub minlen {
    my $op     = shift;
    my $minlen = 0;
    $minlen += $_->minlen() foreach @{ $op->{args} };
    return $minlen;
}

sub maxlen {
    my $op     = shift;
    my $maxlen = 0;
    foreach my $kid ( @{ $op->{args} } ) {
        my $kidmax = $kid->maxlen();
        return undef if !defined $kidmax;
        $maxlen += $kidmax;
    }
    return $maxlen;
}

sub startset {
    my $op = shift;
    my %start;
    foreach ( @{ $op->{args} } ) {
        my @next = $_->startset();
        @start{@next} = ();

        # Stop unless NULLABLE.
        last unless ( @next && $next[0] eq '' );
    }

    return Regex::Ops::Tree::order_startset( \%start );
}

sub dfa_safe {
    my $op = shift;
    foreach ( @{ $op->{args} } ) {
        return 0 unless $_->dfa_safe();
    }
    return 1;
}

# R|S
package Regex::Ops::Tree::alternate;

sub init {
    my ($self) = @_;

    if ( @{ $self->{args} } == 0 ) {
        delete $self->{name};
        delete $self->{args};
        return $self->op( 'nop', [], %$self );
    }
    elsif ( @{ $self->{args} } == 1 && 2 == keys %$self ) {
        return $self->{args}->[0];
    }
    else {
        return $self;
    }
}

sub minlen {
    my $op = shift;
    my $min;
    foreach ( @{ $op->{args} } ) {
        my $len = $_->minlen();
        $min = $len if ( !defined $min ) || ( $min > $len );
    }
    return $min || 0;
}

sub maxlen {
    my $op  = shift;
    my $max = 0;
    foreach ( @{ $op->{args} } ) {
        my $len = $_->maxlen();
        return undef if !defined $len;
        $max = $len if $max < $len;
    }
    return $max;
}

sub startset {
    my $op = shift;
    my %start;
    foreach ( @{ $op->{args} } ) {
        @start{ $_->startset() } = ();
    }
    return Regex::Ops::Tree::order_startset( \%start );
}

# Returns true if at most one subexpression can ever hold at a given
# point in the input string. (Return value is conservative: this will
# return false if it is not sure.)
sub disjoint {
    my $op = shift;

    # For now, return true iff all subexpressions' startsets contain
    # only integers (no character classes or weirder things), and all
    # of those integers are different.
    my %start;
    foreach my $subop ( @{ $op->{args} } ) {
        my @subop_startset = $subop->startset();
        return 0 if grep { ref($_) || $_ !~ /^\d+$/ } @subop_startset;
        foreach (@subop_startset) {
            return 0 if exists( $start{$_} );
        }
    }

    return 1;
}

sub dfa_safe {
    my $op = shift;

    my $dfa_safe_subexprs = 1;
    foreach ( @{ $op->{args} } ) {
        $dfa_safe_subexprs = 0, last if !$_->dfa_safe();
    }

    return 1 if $dfa_safe_subexprs && $op->disjoint();

    # Insert better tests here

    return 0;
}

# R*, R+, R?, nongreedy variants of those
package Regex::Ops::Tree::multi_match;

sub minlen {
    my $op = shift;
    return 0 if $op->{args}->[0] <= 0;
    return $op->{args}->[0] * $op->{args}->[3]->minlen();
}

sub maxlen {
    my $op = shift;
    my ( $min, $max, $greedy, $R ) = @{ $op->{args} };
    my $sublen = $R->maxlen();
    if ( defined($max) && $max !~ /^-?\d+/ ) {
        return undef;
    }
    elsif ( !defined($max) || $max == -1 ) {
        return undef if !defined($sublen);    # [m..INF]*
        return undef if $sublen > 0;          # [m..sublen]*
        return 0;                             # [0..0]*
    }
    else {
        return undef if !defined($sublen);    # [m..INF] repeated up to N times
        return $max * $sublen;                # [m..sublen] repeated up to N times
    }
}

sub startset {
    my $op  = shift;
    my @sub = $op->{args}->[3]->startset();
    if ( @sub && $sub[0] ne '' ) {
        if ( $op->minlen() == 0 ) {
            return ( '', @sub );
        }
        else {
            return @sub;
        }
    }
    else {
        return @sub;
    }
}

package Regex::Ops::Tree::scan;
sub maxlen { undef }

# $ (not R$, just $)
package Regex::Ops::Tree::atend;

sub minlen   { 0 }
sub maxlen   { 0 }
sub startset { return () }
sub hasback  { 0 }
sub dfa_safe { 1 }

package Regex::Ops::Tree::nop;
sub minlen { 0 }
sub maxlen { 0 }

package Regex::Ops::Tree::check;
sub minlen { $_[0]->{args}->[1]->minlen(); }
sub maxlen { $_[0]->{args}->[1]->maxlen(); }

package Regex::Ops::Tree::rule;
sub minlen   { my ($op) = @_; $op->{args}->[1]->minlen() }
sub maxlen   { my ($op) = @_; $op->{args}->[1]->maxlen() }
sub startset { my ($op) = @_; $op->{args}->[1]->startset() }
sub hasback  { my ($op) = @_; $op->{args}->[1]->hasback() }
sub dfa_safe { my ($op) = @_; $op->{args}->[1]->dfa_safe() }

# Rule calls are totally unpredictable -- for now. I think some static
# analysis might not be too hard.
package Regex::Ops::Tree::call;
sub minlen   { 0 }
sub maxlen   { undef }
sub dfa_safe { 0 }
sub hasback  { 1 }
sub startset { undef }

# Embedded code is truly unpredictable. Although there will probably
# be pragmata for allowing code to specify that it won't muck with
# things.
package Regex::Ops::Tree::code;
sub minlen   { 0 }
sub maxlen   { undef }
sub dfa_safe { 0 }
sub hasback  { 0 }       # FIXME! code should be allowed to have BACK{} blocks
sub startset { undef }

########################################################################
# Rendering - only used for debugging for now
########################################################################

package Regex::Ops::Tree;    # Won't get used much

sub needparen { 0 }

# Nonportable
sub isplain {
    my $ord = shift;
    return 1 if $ord >= ord('a') && $ord <= ord('z');
    return 1 if $ord >= ord('A') && $ord <= ord('Z');
    return 1 if $ord >= ord('0') && $ord <= ord('9');
    return 1 if $ord =~ /^[~!@#%&_'":;>,<]$/;

    #    return 1 if $ord =~ /^[`~!@#$%^&*()\-_+{}\[\]\\|'":;\/?.>,<]$/;
    return 0;
}

sub Regex::Ops::Tree::match::render {
    my $op   = shift;
    my $atom = $op->{args}->[0];
    if ( $atom =~ /^\d+$/ ) {
        return chr($atom) if isplain($atom);
        return sprintf( "\\x%02x", $atom );    # Nonportable
    }
    else {
        die;
    }
}

sub Regex::Ops::Tree::charclass::render {
    die;
}

sub Regex::Ops::Tree::classpieces::render {
    die;
}

sub Regex::Ops::Tree::seq::needparen { 0 }

sub Regex::Ops::Tree::seq::render {
    my $op = shift;
    return join( '', map { $_->render() } @{ $op->{args} } );
}

sub Regex::Ops::Tree::alternate::needparen { 1 }

sub Regex::Ops::Tree::alternate::render {
    my $op  = shift;
    my $str = '';
    foreach my $case ( @{ $op->{args} } ) {
        $str .= "|" unless length($str) == 0;
        my $R = $case->render();
        $str .= $case->needparen() ? "(?:$R)" : $R;
    }

    return $str;
}

sub Regex::Ops::Tree::multi_match::needparen { 1 }

sub Regex::Ops::Tree::multi_match::render {
    my $op = shift;
    my ( $min, $max, $greedy, $R ) = @{ $op->{args} };
    my $base = $R->render();
    $base = "(?:$base)" if $R->needparen();
    if ( $min == 0 && $max == 1 ) {
        $base .= "?";
    }
    elsif ( $min == 0 && $max == -1 ) {
        $base .= "*";
    }
    elsif ( $min == 1 && $max == -1 ) {
        $base .= "+";
    }
    elsif ( $max == -1 ) {
        $base .= "{$min,}";
    }
    else {
        $base .= "{$min,$max}";
    }

    $base .= "?" unless $greedy;
    return $base;
}

sub Regex::Ops::Tree::group::needparen { 0 }

sub Regex::Ops::Tree::group::render {
    my $op  = shift;
    my $R   = $op->{args}->[0];
    my $str = $R->render();

    # Strip off (?:) from the subexpression if possible, so we can
    # render things as (R) instead of ((?:R))
    if ( $str =~ /^\(\?\:(.*)\)$/ ) {
        $str = $1;
    }
    return "($str)";
}

# FIXME: Should render the _absence_ of this op differently!
sub Regex::Ops::Tree::scan::needparen { 0 }
sub Regex::Ops::Tree::scan::render    { $_[0]->{args}->[0]->render() }

sub Regex::Ops::Tree::atend::needparen { 0 }
sub Regex::Ops::Tree::atend::render    { '$' }

sub Regex::Ops::Tree::check::needparen { 0 }
sub Regex::Ops::Tree::check::render    { $_[0]->{args}->[1]->render() }

sub Regex::Ops::Tree::rule::render {
    my $self = shift;
    my ( $name, $tree ) = @{ $self->{args} };
    my $expr = $tree->render;
    if ( $name eq 'default' ) {
        return $expr;
    }
    else {
        return "rule $name { $expr }";
    }
}

########################################################################
# Dumping trees, for debugging only
########################################################################

package Regex::Ops::Tree;

sub dump_tree {
    my ($op) = @_;
    my $ref = $op->reftree();
    return dump_ref($ref);
}

sub dump_ref {
    my ( $ref, $indent ) = @_;
    $indent ||= "";
    print $indent;
    if ( ref $ref ) {
        my ( $name, @children ) = @$ref;
        print $name, "\n";
        dump_ref( $_, $indent . "  " ) foreach (@children);
    }
    else {
        print $ref, "\n";
    }
}

sub annotated {
    my ( $op, $str ) = @_;
    my $min = $op->minlen();
    my $max = $op->maxlen();
    $max = "INF" if !defined $max;
    return "$str [$min..$max]";
}

# Defaults
sub Regex::Ops::Tree::reftree {
    my $op = shift;
    return $op->{name};
}

sub Regex::Ops::Tree::_onearg::reftree {
    my $op = shift;
    return [ annotated( $op, $op->{name} ), $op->{args}->[0]->reftree() ];
}

sub Regex::Ops::Tree::match::reftree {
    my $op = shift;
    return annotated( $op, "match(" . $op->render(@_) . ")" );
}

sub Regex::Ops::Tree::charclass::reftree {
    my $op = shift;
    return annotated( $op, "charclass " . $op->render(@_) );
}

sub Regex::Ops::Tree::classpieces::reftree {
    my $op = shift;
    return annotated( $op, "classpieces " . $op->render(@_) );
}

sub Regex::Ops::Tree::seq::reftree {
    my $op = shift;
    return [ annotated( $op, "seq" ), map { $_->reftree() } @{ $op->{args} } ];
}

sub Regex::Ops::Tree::alternate::reftree {
    my $op = shift;
    my $ref = [ annotated( $op, "alternate" ) ];
    push( @$ref, $_->reftree() ) foreach ( @{ $op->{args} } );
    return $ref;
}

sub Regex::Ops::Tree::multi_match::reftree {
    my $op = shift;

    my ( $min, $max, $greedy, $R ) = @{ $op->{args} };
    my $ref = [ annotated( $op, "multi_match($min .. $max)" ), $R->reftree() ];
    $ref->[0] .= "?" unless $greedy;

    return $ref;
}

sub Regex::Ops::Tree::check::reftree {
    my $op = shift;
    return [ annotated( $op, "check($op->{args}->[0])" ), $op->{args}->[1]->reftree() ];
}

sub Regex::Ops::Tree::rule::reftree {
    my $op = shift;
    my ( $rule, $tree ) = @{ $op->{args} };
    return [ annotated( $op, "rule($rule)" ), $tree->reftree() ];
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
