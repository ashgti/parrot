# Copyright (C) 2005-2008, The Perl Foundation.
# $Id$


## Not sure what standard built-in library is for ECMAScript, but
## we need some output function for testing. For now this'll do.

.sub 'print'
    .param pmc args :slurpy
    .local pmc iter
    new iter, 'Iterator', args
  print_loop:
    unless iter goto end_print_loop
    $P1 = shift iter
    print $P1
    goto print_loop
  end_print_loop:
.end

## constructor for a object literals. It takes advantages of
## the Parrot Calling Conventions using :slurpy and :named flags,
## meaning that the parameter C<fields> is a hash, which is kinda
## what we want. For now.
##
.sub 'Object'
    .param pmc fields :slurpy :named
    .return (fields)
.end

# start of methods Put and CanPut.
# lots of work to do in JS' object model.
#.sub 'Put' :method
#    .param pmc P
#    .param pmc V
#    $I0 = self.'CanPut'(P)
#    unless $I0 goto stop
#
#  stop:
#    .return()
#.end
#
#.sub 'CanPut' :method
#
#.end

.sub 'Array'
    .param pmc args :slurpy
    $P0 = new 'Array'
    .return ($P0)
.end


## built-in functions
##

## probably add :vtable flag to these:
##
##.sub 'ToString' :multi(num)
##    .param num arg
##    #$P0 = new 'String'
##    $S0 = arg
##    .return ($S0)
##.end
##
##.sub 'ToString' :multi(int)
##    .param int arg
##    #$P0 = new 'String'
##    $S0 = arg
##    .return ($S0)
##.end
##
##.sub 'ToString' :multi(string)
##    .param string arg
##    .return (arg)
##.end
##
##.sub 'ToString' :multi(_)
##    .param pmc arg
##    $S0 = arg
##    .return ($S0)
##.end



## built-in operators
##

.sub 'infix:='
    .param pmc lhs
    .param pmc rhs
    assign lhs, rhs
    .return (lhs)
.end

## postfix operators
##
.sub 'postfix:++'
    .param pmc a
    $P0 = clone a
    inc a
    .return ($P0)
.end

.sub 'postfix:--'
    .param pmc a
    $P0 = clone a
    dec a
    .return ($P0)
.end

## infix operators
##

#.sub 'infix:||'
#    .param pmc left
#    .param pmc right
#.end
#
#
#.sub 'infix:&&'
#    .param pmc left
#    .param pmc right
#.end


.sub 'infix:|'
    .param pmc left
    .param pmc right
    $I1 = left
    $I2 = right
    bor $I0, $I1, $I2
    .return ($I0)
.end


.sub 'infix:^'
    .param pmc left
    .param pmc right
    $I1 = left
    $I2 = right
    bxor $I0, $I1, $I2
    .return ($I0)
.end


.sub 'infix:&'
    .param pmc left
    .param pmc right
    $I1 = left
    $I2 = right
    band $I0, $I1, $I2
    .return ($I0)
.end


.sub 'infix:=='
    .param pmc left
    .param pmc right
    iseq $I0, left, right
    .return ($I0)
.end


.sub 'infix:!='
    .param pmc left
    .param pmc right
    isne $I0, left, right
    .return ($I0)
.end


.sub 'infix:==='
    .param pmc left
    .param pmc right
.end


.sub 'infix:!=='
    .param pmc left
    .param pmc right
.end


.sub 'infix:<'
    .param pmc left
    .param pmc right
.end


.sub 'infix:>'
    .param pmc left
    .param pmc right
.end

.sub 'infix:<='
    .param pmc left
    .param pmc right
.end


.sub 'infix:>='
    .param pmc left
    .param pmc right
.end


.sub 'infix:instanceof'
    .param pmc left
    .param pmc right
.end


.sub 'infix:in'
    .param pmc left
    .param pmc right
.end


.sub 'infix:<<'
    .param pmc left
    .param pmc right
.end


.sub 'infix:>>'
    .param pmc left
    .param pmc right
.end


.sub 'infix:>>>'
    .param pmc left
    .param pmc right
.end


## prefix operators
##
.sub 'prefix:delete'
    .param pmc op
.end


.sub 'prefix:void'
    .param pmc op
.end

.sub 'prefix:typeof'
    .param pmc op
.end

.sub 'prefix:+'
    .param pmc op
    $N0 = op
    .return ($N0)
.end

.sub 'prefix:-'
    .param pmc op
    $N0 = op
    neg $N0
    .return ($N0)
.end

.sub 'prefix:++'
    .param pmc op
    inc op
    .return (op)
.end

.sub 'prefix:--'
    .param pmc op
    dec op
    .return (op)
.end

.sub 'prefix:~'
    .param pmc op
    bnot $P0, op
    .return ($P0)
.end

.sub 'prefix:!'
    .param pmc op
    istrue $I0, op
    not $I0
    .return ($I0)
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
