# Copyright (C) 2006-2009, Parrot Foundation.
# $Id$

.sub greet
    .param pmc    name

    .local string name_str
    name_str = name

    .local pmc greeting
    greeting  = new 'String'
    greeting  = 'Hello, '
    greeting .= name_str
    greeting .= '!'

    .return( greeting )
.end

.sub sum
    .param pmc nums :slurpy

    .local int acc
    acc = 0

  loop:
    unless nums goto end_loop
    $I0 = shift nums
    acc += $I0
    goto loop
  end_loop:

    .return (acc)
.end

.namespace [ 'Elsewhere' ]

.sub greet
    .return( 'Hiya!' )
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
