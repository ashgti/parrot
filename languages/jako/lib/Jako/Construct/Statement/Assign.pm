#
# Assign.pm
#
# Copyright: 2002-2005 The Perl Foundation.  All Rights Reserved.
# This program is free software. It is subject to the same license
# as the Parrot interpreter.
#
# $Id$
#

use strict;
eval "use warnings";

package Jako::Construct::Statement::Assign;

use Carp;

use base qw(Jako::Construct::Statement);

sub new
{
  my $class = shift;
  my ($block, $left, $right) = @_;

  confess("Block (" . ref($block) . ") not!") unless UNIVERSAL::isa($block, 'Jako::Construct::Block');
  confess("Left (" . ref($left) . ") is not Value") unless UNIVERSAL::isa($left, 'Jako::Construct::Expression::Value');
  confess("Right (" . ref($right) . ") is not Value") unless UNIVERSAL::isa($right, 'Jako::Construct::Expression::Value');

  my $self = bless {
    BLOCK => $block,
    LEFT  => $left,
    RIGHT => $right
  }, $class;

  $block->push_content($self);

  return $self;
}

sub left  { return shift->{LEFT};  }
sub right { return shift->{RIGHT}; }


#
# compile()
#

sub compile
{
  my $self = shift;
  my ($compiler) = @_;

  my $left  = $self->left->value;
  my $right = $self->right->compile($compiler);

  $compiler->emit("  $left = $right");

  return 1;
}


#
# sax()
#

sub sax
{
  my $self = shift;
  my ($handler) = @_;

  $handler->start_element({ Name => 'assign' });
  $self->left->sax($handler);
  $self->right->sax($handler);
  $handler->end_element({ Name => 'assign' });
}


1;
