# Optimize the regular expression syntax tree before rendering it out
# as List ops. Most optimizations should go here, since the List form
# is really suitable only for peephole optimization.

package Regex::PreOptimize;
use Regex::Ops::Tree qw(rop);
use strict;

sub new {
    my ($proto, %opts) = @_;
    my $self = bless \%opts, (ref($proto) || $proto);
    $self->init();
    return $self;
}

sub init {
}

sub optimize_tree {
    my ($self, $t) = @_;
    $t = $self->pass1($t);
    $self->disable_implicit_checks($t);
    $t = $self->add_checks($t, 0, 0);
    return $t;
}

# Perform a few simple optimizations:
#
# 1. R(ST)    -> RST
# 2. aR|aS    -> a(R|S)
#    R|<null> -> R?
#    <null>|R -> R??
#
sub pass1 {
    my ($self, $t) = @_;
    my $type = $t->{name};
    if ($type eq 'seq') {
        # R.(S.T) => R.S.T
        # Or parenthetically, seq(R,seq(\alpha)) => seq(R, \alpha)
        #
	my @pre_pieces = map { $self->pass1($_) } @{ $t->{args} };
	my @pieces;
	foreach (@pre_pieces) {
	    if (ref $_ && $_->{name} eq 'seq') {
		push @pieces, @{$_->{args}};
	    } else {
		push @pieces, $_;
	    }
	}
	return rop('seq', \@pieces);
    } elsif ($type eq 'alternate') {
	# Pull out common prefixes, and convert alternation with null
	# expressions to optional matches:
        #
        # aR|aS => a(R|S)
        # R|    => R?
        # |R    => R??
        #
	my ($R, $S) = ($self->pass1($t->{args}->[0]),
		       $self->pass1($t->{args}->[1]));
	my ($R0, @Rrest);
	my ($S0, @Srest);
        @{$t->{args}} = ($R, $S);
	return $t if ! ref $R;
	return $t if ! ref $S;

	if ($R->{name} eq 'seq') {
            ($R0, @Rrest) = @{ $R->{args} };
	} else {
	    $R0 = $R;
	}

	if ($S->{name} eq 'seq') {
            ($S0, @Srest) = @{ $S->{args} };
	} else {
	    $S0 = $S;
	}

	return $t unless matchsame($R0, $S0);

	my $Rrest = rop('seq', \@Rrest);
	my $Srest = rop('seq', \@Srest);
	my $alt;
	if (@Rrest == 0) {
	    # (|S) --> S??
	    $alt = rop('multi_match', [ 0, 1, 0, $Srest ]);
	} elsif (@Srest == 0) {
	    # (R|) --> R?
	    $alt = rop('multi_match', [0, 1, 1, $Rrest]);
	} else {
	    $alt = rop('alternate', [ $Rrest, $Srest ]);
	}

	return $self->pass1(rop('seq', [ $R0, $alt ]));
    } else {
        # Find all subtrees, and recurse through them.
        foreach my $arg (@{ $t->{args} }) {
            if (UNIVERSAL::isa($arg, 'Regex::Ops::Tree')) {
                # $arg is a reference variable, remember.
                $arg = $self->pass1($arg);
            }
        }
        return $t;
    }
}

# Note: the following fails to rewrite
#   aR|[a]S
# to
#   a(R|S)
#
sub matchsame {
    my ($R, $S) = @_;
    return 0 if ! ref $R;
    return 0 if ! ref $S;
    return 0 if $R->{name} ne $S->{name};
    my $type = $R->{name};
    if ($type eq 'match') {
	return $R->{args}[0] == $S->{args}[0];
    } elsif ($type eq 'classmatch') {
	return $R->{args}[0] eq $S->{args}[0];
    } elsif ($type eq 'start' || $type eq 'end') {
	return 0;
    } else {
        my @Rargs = @{ $R->{args} };
        my @Sargs = @{ $S->{args} };
	return 0 if (@Rargs != @Sargs);
	for (0..$#Rargs) { return 0 if $Rargs[$_] ne $Sargs[$_] };
	return 1;
    }
}

# sub add_checks
#
# Augments the tree with length checks, so that primitive matching
# operations (eg matching single characters or character ranges) won't
# have to do their own checking.
#
# Without this, /abc/ would compile to
#  check(1)
#  match(a)
#  check(1)
#  match(b)
#  check(1)
#  match(c)
#
# where check(n) is "am I closer than n away from the end of the string?"
#
# After this operation, /abc/ compiles to
#
#  check(3)
#  match(a)
#  match(b)
#  match(c)
#
# and something trickier like /ab|c[dxy]e/ compiles to
#
#          check(2)
#          match(a) or goto try_S
#          match(b) or goto try_S
#          goto next
#   try_S: check(3)
#          match(c) or fail
#          match([dxy]) or fail
#          match(e) or fail
#
# Note that the placement of the checks is *not* always optimal. But
# they should do a pretty decent job of making things fail early if
# you get too close to the end of the string.
#
# Args:
#  t - The Tree op being visited
#  guarantee - The number of atoms we are currently guaranteed to have for
#      the current node (NOT for this node and everything following it)
#  follow_min - The minimum number of atoms in nodes following the current
#
# FIXME: Once I add in the various 'cut' operations, it will no longer
# be correct to fail early (since failure can then be more dramatic!).
#
sub add_checks {
    my ($self, $t, $guarantee, $follow_min) = @_;

    # Have enough for any possible match of this subtree?
    return $t if (defined($t->maxlen()) && $guarantee >= $t->maxlen());

    # Need more for even the shortest match of this subtree?
    if ($t->minlen() > $guarantee) {
        my $newtree = $self->add_checks($t, $t->minlen(), $follow_min);
        return rop('check', [ $t->minlen() + $follow_min, $newtree ]);
    }

    # Shortest possible subtree match is guaranteed.
    my $type = $t->{name};

    if ($type eq 'seq' && ! $t->{add_check_visited}) {
        # Split the seq up into chunks that look like they're nicely
        # checkable.

        # Break the list of children up into contiguous chunks of
        # things with and without maximum lengths
        my $havemax = 0;
        my @chunks = ([]);

        foreach my $kid (@{ $t->{args} }) {
            if (defined($kid->maxlen()) xor $havemax) {
                # Change
                push @chunks, [];
                $havemax = 1 - $havemax;
            }
            push @{ $chunks[-1] }, $kid;
        }
        shift(@chunks) if @{ $chunks[0] } == 0;

        # If there were things with different types, create a new seq
        # that has a child seq for each chunk.
        if (@chunks > 1) {
            my @children = map { rop('seq', $_) } @chunks;
            $t = rop('seq', \@children);
        }

        $t->{add_check_visited} = 1;
        return $self->add_checks($t, $guarantee, $follow_min);

    } elsif ($type eq 'seq' && $t->{add_check_visited}) {
        my ($kid_follow_min, $kid_guarantee) = ($follow_min, $guarantee);

        foreach my $kid (@{ $t->{args} }) {
            $kid_follow_min += $kid->minlen();
        }

        foreach my $kid (@{ $t->{args} }) {
            $kid_follow_min -= $kid->minlen();

            $kid = $self->add_checks($kid, $kid_guarantee, $kid_follow_min);

            if (defined($kid->maxlen())) {
                $kid_guarantee -= $kid->maxlen();
            } else {
                $kid_guarantee = 0;
            }
        }
    } elsif ($type eq 'multi_match') {
        my ($min, $max, undef, $R) = @{ $t->{args} };
        if ($max < 0) { $guarantee = 0; }
        else { $guarantee /= $max; }
        $t->{args}->[3] = $self->add_checks($R, $guarantee, $follow_min);
        return $t;
    } else {
        # If we're scanning, we'll eat up any possible guarantee.
        $guarantee = 0 if $type eq 'scan';

        # Default for everything else is to pass the same guarantee
        # and follow_min to all children. (Which is reasonable, since
        # everything but seq tries to match its children at the same
        # starting point in the string -- think of alternation, for
        # example.)
        foreach my $arg (@{ $t->{args} }) {
            if (UNIVERSAL::isa($arg, 'Regex::Ops::Tree')) {
                # $arg is a reference variable, remember.
                $arg = $self->add_checks($arg, $guarantee, $follow_min);
            }
        }
    }

    return $t;
}

# By default, all atom match ops check to be sure there are enough
# atoms left in the input before looking at the next atom. This
# routine disables all of those checks. So you should call
# add_checks() to put the (hopefully fewer) checks back in.
sub disable_implicit_checks {
    my ($self, $t) = @_;
    $t->{nocheck} = 1;
    foreach my $arg (@{ $t->{args} }) {
        if (UNIVERSAL::isa($arg, 'Regex::Ops::Tree')) {
            $self->disable_implicit_checks($arg);
        }
    }
    return $t;
}

1;
