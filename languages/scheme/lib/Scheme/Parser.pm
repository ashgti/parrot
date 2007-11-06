# $Id$
# Copyright (C) 2001-2007, The Perl Foundation.

package Scheme::Parser;

# pragmata
use strict;
use warnings;
use 5.008;

our $VERSION   = '0.01';

use Data::Dumper;

# walk over the tokens
sub _build_tree {
    my ( $tokenizer ) = @_;

    if ( wantarray() ) {        # be greedy
        my @trees;
        while ( my $tree = _build_tree( $tokenizer ) ) {
            push @trees, $tree;
        }

        return @trees;
    }

    my $token = $tokenizer->();   # grap next token
   
    return unless $token;

    return if $token->[1] eq ')';

    if ( $token->[1] eq '(' ) {
        my @children = _build_tree( $tokenizer );

        if ( ! @children ) {
            # special case: empty list
            return { value => undef };
        }
        else {
            return { children => \@children };
        }
    } 

    my %special_function = (
            q{'}      => 'quote',
            q{`}      => 'quasiquote',
            q{,}      => 'unquote',
            q{,@}     => 'unquote-splicing',
    );
    if ( exists $special_function{$token->[1]}  ) {
        my $child = _build_tree( $tokenizer );

        return { children => [ { value => $special_function{$token->[1]} 
                               },
                               $child
                             ]
               };

    }

    # the atomic case
    return { type  => $token->[0],
             value => $token->[1],
           };
}


sub parse {
    my $tokenizer = shift;

    my @trees = _build_tree( $tokenizer );

    return undef unless @trees;

    return $trees[0] if scalar(@trees) == 1;

    # Implicit begin at toplevel
    return { children => [ { value => 'begin' },
                           @trees
                         ]
           };
}

1;

__END__

=head1 NAME

Scheme::Parser - The Scheme token parser

=head1 SYNOPSIS

  use Scheme::Parser;

  my $tree = Scheme::Parser::parse($tokenizer);

=head1 DESCRIPTION

The parser reads a list of tokens and turns it into a tree structure.
The Nodes of the tree are hash references with either the attribute C<value>
or the attribute C<children>. C<children> is an array reference with subbnodes. 

=head1 AUTHOR

Jeffrey Goff, drforr@hargray.com

=head1 SEE ALSO

L<Scheme>, L<Scheme::Tokenizer>, L<Scheme::Generator>

=cut
