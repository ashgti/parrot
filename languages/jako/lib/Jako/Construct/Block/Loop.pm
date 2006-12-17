#
# Loop.pm
#
# Copyright (C) 2002-2005, The Perl Foundation.
# This program is free software. It is subject to the same license
# as the Parrot interpreter.
#
# $Id$
#

use strict;
use warnings;

package Jako::Construct::Block::Loop;

use Carp;

use Jako::Compiler;

use base qw(Jako::Construct::Block);

#
# compile()
#

sub compile {
    my $self = shift;
    my ($compiler) = @_;

    my $kind = $self->kind;
    my $peer = $self->peer;

    my $prefix;

    if ( $self->prefix ) {
        $prefix = $self->prefix;
    }
    else {
        $prefix = $self->peer ? $peer->prefix : $compiler->block_label($kind);
        $self->prefix($prefix);
    }

    my $namespace = $prefix;

    if ( $kind eq 'while' or $kind eq 'until' ) {
        my $test = ( $kind eq 'while' ) ? 'unless' : 'if';

        $compiler->emit("${prefix}_NEXT:");

        my $op    = $self->op;
        my $left  = $self->left->compile($compiler);
        my $right = $self->right->compile($compiler);
        $compiler->emit("  $test $left $op $right goto ${prefix}_LAST");

        $compiler->emit("${prefix}_REDO:");

        if ( $self->content ) {
            $compiler->emit(".namespace ${namespace}");
            $compiler->indent;
            $self->SUPER::compile($compiler);
            $compiler->outdent;
            $compiler->emit(".endnamespace ${namespace}");
        }
    }
    elsif ( $kind eq 'continue' ) {
        $compiler->emit("${prefix}_CONT:");

        if ( $self->content ) {
            $compiler->emit(".namespace ${namespace}_CONT");
            $compiler->indent;
            $self->SUPER::compile($compiler);
            $compiler->outdent;
            $compiler->emit(".endnamespace ${namespace}_CONT");
        }

        $compiler->emit("  goto ${prefix}_NEXT");
        $compiler->emit("${prefix}_LAST:");
    }
    else {
        $self->INTERNAL_ERROR( "Unrecognized kind of block '%s'", $kind );
    }

    return 1;
}

#
# sax()
#

sub sax {
    my $self = shift;
    my ($handler) = @_;

    if ( not $self->prefix and $self->peer and $self->peer->prefix ) {
        $self->prefix( $self->peer->prefix );
    }

    if ( $self->prefix ) {
        $handler->start_element(
            { Name => 'loop', Attributes => { kind => $self->kind, name => $self->prefix } } );
    }
    else {
        $handler->start_element( { Name => 'loop', Attributes => { kind => $self->kind } } );
    }

    if ( $self->op ) {
        $handler->start_element( { Name => 'block', Attributes => { kind => 'test' } } );
        $handler->start_element(
            { Name => 'op', Attributes => { kind => 'infix', name => $self->op } } );
        $self->left->sax($handler);
        $self->right->sax($handler);
        $handler->end_element( { Name => 'op' } );
        $handler->end_element( { Name => 'block' } );
    }

    $handler->start_element( { Name => 'block', Attributes => { kind => $self->kind } } );
    $_->sax($handler) foreach $self->content;
    $handler->end_element( { Name => 'block' } );

    $handler->end_element( { Name => $self->kind } );
}

1;
