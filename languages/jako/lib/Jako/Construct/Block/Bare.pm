#
# Bare.pm
#
# Copyright (C) 2002-2003 Gregor N. Purdy. All rights reserved.
# This program is free software. It is subject to the same license
# as the Parrot interpreter.
#
# $Id$
#

use strict;
eval "use warnings";

package Jako::Construct::Block::Bare;

use Carp;

use Jako::Compiler;

use base qw(Jako::Construct::Block);


#
# compile()
#

sub compile
{
  my $self = shift;
  my ($compiler) = @_;

  my $namespace = "BARE"; # TODO: Don't we need to do better than this?

  if ($self->content) {
    $compiler->emit(".namespace ${namespace}");
    $compiler->indent;
    foreach my $content ($self->contents) {
      $content->compile($compiler);
    }
    $compiler->outdent;
    $compiler->emit(".endnamespace ${namespace}");
  }

  return 1;
}


#
# sax()
#

sub sax
{
  my $self = shift;
  my ($handler) = @_;

  $handler->start_element({ Name => 'block', Attributes => { kind => $self->kind } });
  foreach my $content ($self->content) {
    $content->sax($handler);
  }
  $handler->end_element({ Name => 'block' });
}


1;
