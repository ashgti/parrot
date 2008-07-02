# $Id$

.loadlib 'php_group'

.include 'languages/pipp/src/common/php_MACRO.pir'

# steal builtins from Perl6
.sub 'print'
    .param pmc list            :slurpy
    .local pmc iter

    iter = new 'Iterator', list
  iter_loop:
    unless iter goto iter_end
    $P0 = shift iter
    print $P0
    goto iter_loop

  iter_end:
    .return (1)
.end

# steal builtins from Perl6
# TODO: put that into php_builtins.pir
.sub 'echo'
    .param pmc list            :slurpy

    .local pmc iter
    iter = new 'Iterator', list
  iter_loop:
    unless iter goto iter_end
    $P0 = shift iter
    print $P0
    goto iter_loop

  iter_end:
    .return (1)
.end

## autoincrement
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

.sub 'prefix:++'
    .param pmc a
    inc a
    .return (a)
.end

.sub 'prefix:--'
    .param pmc a
    dec a
    .return (a)
.end


## symbolic unary
.sub 'prefix:-'
    .param pmc a
    $P1 = a.'to_number'()
    neg $P1
    .return ($P1)
.end

.sub 'prefix:~' :multi(PhpString)
    .param pmc a
    bnots $P1
    .return ($P1)
.end

.sub 'prefix:~' :multi(_)
    .param int a
    $I0 = bnot a
    .RETURN_LONG($I0)
.end

.sub 'prefix:!'
    .param pmc a
    $I0 = isfalse a
    .RETURN_BOOL($I0)
.end


## multiplicative
.sub 'infix:*'
    .param pmc a
    .param pmc b
    $P1 = a.'to_number'()
    $P2 = b.'to_number'()
    $P0 = mul $P1, $P2
    .return ($P0)
.end

.sub 'infix:/'
    .param pmc a
    .param pmc b
    $P1 = a.'to_number'()
    $P2 = b.'to_number'()
    $P0 = div $P1, $P2
    .return ($P0)
.end

.sub 'infix:%'
    .param pmc a
    .param pmc b
    $P1 = a.'to_number'()
    $P2 = b.'to_number'()
    $P0 = mod $P1, $P2
    .return ($P0)
.end

.sub 'infix:&' :multi(PhpString,PhpString)
    .param pmc a
    .param pmc b
    $P0 = bands $P1, $P2
    .return ($P0)
.end

.sub 'infix:&' :multi(_,_)
    .param int a
    .param int b
    $I0 = band a, b
    .RETURN_LONG($I0)
.end

.sub 'infix:>>'
    .param int a
    .param int b
    $I0 = shr a, b
    .RETURN_LONG($I0)
.end

.sub 'infix:<<'
    .param int a
    .param int b
    $I0 = shl a, b
    .RETURN_LONG($I0)
.end


## additive
.sub 'infix:+'
    .param pmc a
    .param pmc b
    $P1 = a.'to_number'()
    $P2 = b.'to_number'()
    $P0 = add $P1, $P2
    .return ($P0)
.end

.sub 'infix:-'
    .param pmc a
    .param pmc b
    $P1 = a.'to_number'()
    $P2 = b.'to_number'()
    $P0 = sub $P1, $P2
    .return ($P0)
.end

.sub 'infix:.'
    .param pmc a
    .param pmc b
    $S1 = a
    $S2 = b
    $S0 = concat $S1, $S2
    .RETURN_STRING($S0)
.end

.sub 'infix:|' :multi(PhpString,PhpString)
    .param pmc a
    .param pmc b
    $P0 = bors $P1, $P2
    .return ($P0)
.end

.sub 'infix:|' :multi(_,_)
    .param int a
    .param int b
    $I0 = bor a, b
    .RETURN_LONG($I0)
.end

.sub 'infix:^' :multi(PhpString,PhpString)
    .param pmc a
    .param pmc b
    $P0 = bxors $P1, $P2
    .return ($P0)
.end

.sub 'infix:^' :multi(_,_)
    .param int a
    .param int b
    $I0 = bxor a, b
    .RETURN_LONG($I0)
.end


## logical
.sub 'infix:AND'
    .param pmc a
    .param pmc b
    $I1 = istrue a
    $I2 = istrue b
    $I0 = band $I1, $I2
    .RETURN_BOOL($I0)
.end

.sub 'infix:OR'
    .param pmc a
    .param pmc b
    $I1 = istrue a
    $I2 = istrue b
    $I0 = bor $I1, $I2
    .RETURN_BOOL($I0)
.end

.sub 'infix:XOR'
    .param pmc a
    .param pmc b
    $I1 = istrue a
    $I2 = istrue b
    $I0 = bxor $I1, $I2
    .RETURN_BOOL($I0)
.end


## comparison
.sub 'infix:<'
    .param pmc a
    .param pmc b
    $I0 = islt a, b
    .RETURN_BOOL($I0)
.end

.sub 'infix:<='
    .param pmc a
    .param pmc b
    $I0 = isle a, b
    .RETURN_BOOL($I0)
.end

.sub 'infix:=='
    .param pmc a
    .param pmc b
    $I0 = iseq a, b
    .RETURN_BOOL($I0)
.end

.sub 'infix:!='
    .param pmc a
    .param pmc b
    $I0 = isne a, b
    .RETURN_BOOL($I0)
.end

.sub 'infix:>'
    .param pmc a
    .param pmc b
    $I0 = isgt a, b
    .RETURN_BOOL($I0)
.end

.sub 'infix:>='
    .param pmc a
    .param pmc b
    $I0 = isge a, b
    .RETURN_BOOL($I0)
.end

.sub 'infix:==='
    .param pmc a
    .param pmc b
    $S1 = typeof a
    $S2 = typeof b
    if $S1 == $S2 goto L1
    .RETURN_FALSE()
  L1:
    $I0 = iseq a, b
    .RETURN_BOOL($I0)
.end

.sub 'infix:!=='
    .param pmc a
    .param pmc b
    $S1 = typeof a
    $S2 = typeof b
    if $S1 != $S2 goto L1
    .RETURN_TRUE()
  L1:
    $I0 = isne a, b
    .RETURN_BOOL($I0)
.end


.include 'languages/pipp/src/common/php_standard.pir'

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
