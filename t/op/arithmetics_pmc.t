#!parrot
# Copyright (C) 2001-2009, Parrot Foundation.
# $Id$

=head1 NAME

t/op/arithmetics_pmc.t - Arithmetic Ops involving PMCs

=head1 SYNOPSIS

        % prove t/op/arithmetics_pmc.t

=head1 DESCRIPTION

Test handling C<dest> arg in 3-args arithmetic.

=cut

.sub main :main
    .include 'test_more.pir'
    .include "iglobals.pasm"

    plan(68)

    # Don't check BigInt or BigNum without gmp
    .local pmc interp     # a handle to our interpreter object.
    interp = getinterp
    .local pmc config
    config = interp[.IGLOBALS_CONFIG_HASH]
    .local string gmp
    gmp = config['gmp']

    run_tests_for('Integer')
    run_tests_for('Float')

    if gmp goto do_big_ones
        skip( 34, "will not test BigInt or BigNum without gmp" )
        goto end   

  do_big_ones:
    run_tests_for('BigInt')
    run_tests_for('BigNum')

  end:
.end

.sub run_tests_for
    .param pmc type
    test_add(type)
    test_divide(type)
    test_multiply(type)
    test_floor_divide(type)
    test_logical_and(type)
    test_concatenate(type)
    test_logical_xor(type)
    test_logical_or(type)
    test_bitwise_shr(type)
    test_bitwise_or(type)
    test_bitwise_shl(type)
    test_bitwise_xor(type)
    test_modulus(type)
    test_pow(type)
    test_subtract(type)
    test_bitwise_lsr(type)
    test_bitwise_and(type)
.end

.sub test_add
    .param pmc type

    $P0 = new type
    $P0 = 40
    $P1 = new type
    $P1 = 2
    $P2 = new type
    $P2 = 115200

    $P99 = $P2

    $S0 = "original dest is untouched in add for "
    $S1 = type
    concat $S0, $S1

    # ignore exceptions
    push_eh done
    add $P2, $P0, $P1

    $I0 = cmp $P99, 115200
    
    is( $I0, 0, $S0 )
    goto end

  done:
    ok(1, 'ignoring exceptions')
  end:
.end

.sub test_divide
    .param pmc type

    $P0 = new type
    $P0 = 40
    $P1 = new type
    $P1 = 2
    $P2 = new type
    $P2 = 115200

    $P99 = $P2

    $S0 = "original dest is untouched in divide for "
    $S1 = type
    concat $S0, $S1

    # ignore exceptions
    push_eh done
    div $P2, $P0, $P1

    $I0 = cmp $P99, 115200
    
    is( $I0, 0, $S0 )
    goto end

  done:
    ok(1, 'ignoring exceptions')
  end:
.end

.sub test_multiply
    .param pmc type

    $P0 = new type
    $P0 = 40
    $P1 = new type
    $P1 = 2
    $P2 = new type
    $P2 = 115200

    $P99 = $P2

    $S0 = "original dest is untouched in multiply for "
    $S1 = type
    concat $S0, $S1

    # ignore exceptions
    push_eh done
    mul $P2, $P0, $P1

    $I0 = cmp $P99, 115200
    
    is( $I0, 0, $S0 )
    goto end

  done:
    ok(1, 'ignoring exceptions')
  end:
.end

.sub test_floor_divide
    .param pmc type

    $P0 = new type
    $P0 = 40
    $P1 = new type
    $P1 = 2
    $P2 = new type
    $P2 = 115200

    $P99 = $P2

    $S0 = "original dest is untouched in floor_divide for "
    $S1 = type
    concat $S0, $S1

    # ignore exceptions
    push_eh done
    fdiv $P2, $P0, $P1

    $I0 = cmp $P99, 115200
    
    is( $I0, 0, $S0 )
    goto end

  done:
    ok(1, 'ignoring exceptions')
  end:
.end

.sub test_logical_and
    .param pmc type

    $P0 = new type
    $P0 = 40
    $P1 = new type
    $P1 = 2
    $P2 = new type
    $P2 = 115200

    $P99 = $P2

    $S0 = "original dest is untouched in logical_and for "
    $S1 = type
    concat $S0, $S1

    # ignore exceptions
    push_eh done
    and $P2, $P0, $P1

    $I0 = cmp $P99, 115200
    
    is( $I0, 0, $S0 )
    goto end

  done:
    ok(1, 'ignoring exceptions')
  end:
.end

.sub test_concatenate
    .param pmc type

    $P0 = new type
    $P0 = 40
    $P1 = new type
    $P1 = 2
    $P2 = new type
    $P2 = 115200

    $P99 = $P2

    $S0 = "original dest is untouched in concatenate for "
    $S1 = type
    concat $S0, $S1

    # ignore exceptions
    push_eh done
    concat $P2, $P0, $P1

    $I0 = cmp $P99, 115200
    
    is( $I0, 0, $S0 )
    goto end

  done:
    ok(1, 'ignoring exceptions')
  end:
.end

.sub test_logical_xor
    .param pmc type

    $P0 = new type
    $P0 = 40
    $P1 = new type
    $P1 = 2
    $P2 = new type
    $P2 = 115200

    $P99 = $P2

    $S0 = "original dest is untouched in logical_xor for "
    $S1 = type
    concat $S0, $S1

    # ignore exceptions
    push_eh done
    xor $P2, $P0, $P1

    $I0 = cmp $P99, 115200
    
    is( $I0, 0, $S0 )
    goto end

  done:
    ok(1, 'ignoring exceptions')
  end:
.end

.sub test_logical_or
    .param pmc type

    $P0 = new type
    $P0 = 40
    $P1 = new type
    $P1 = 2
    $P2 = new type
    $P2 = 115200

    $P99 = $P2

    $S0 = "original dest is untouched in logical_or for "
    $S1 = type
    concat $S0, $S1

    # ignore exceptions
    push_eh done
    or $P2, $P0, $P1

    $I0 = cmp $P99, 115200
    
    is( $I0, 0, $S0 )
    goto end

  done:
    ok(1, 'ignoring exceptions')
  end:
.end

.sub test_bitwise_shr
    .param pmc type

    $P0 = new type
    $P0 = 40
    $P1 = new type
    $P1 = 2
    $P2 = new type
    $P2 = 115200

    $P99 = $P2

    $S0 = "original dest is untouched in bitwise_shr for "
    $S1 = type
    concat $S0, $S1

    # ignore exceptions
    push_eh done
    shr $P2, $P0, $P1

    $I0 = cmp $P99, 115200
    
    is( $I0, 0, $S0 )
    goto end

  done:
    ok(1, 'ignoring exceptions')
  end:
.end

.sub test_bitwise_or
    .param pmc type

    $P0 = new type
    $P0 = 40
    $P1 = new type
    $P1 = 2
    $P2 = new type
    $P2 = 115200

    $P99 = $P2

    $S0 = "original dest is untouched in bitwise_or for "
    $S1 = type
    concat $S0, $S1

    # ignore exceptions
    push_eh done
    bor $P2, $P0, $P1

    $I0 = cmp $P99, 115200
    
    is( $I0, 0, $S0 )
    goto end

  done:
    ok(1, 'ignoring exceptions')
  end:
.end

.sub test_bitwise_shl
    .param pmc type

    $P0 = new type
    $P0 = 40
    $P1 = new type
    $P1 = 2
    $P2 = new type
    $P2 = 115200

    $P99 = $P2

    $S0 = "original dest is untouched in bitwise_shl for "
    $S1 = type
    concat $S0, $S1

    # ignore exceptions
    push_eh done
    shl $P2, $P0, $P1

    $I0 = cmp $P99, 115200
    
    is( $I0, 0, $S0 )
    goto end

  done:
    ok(1, 'ignoring exceptions')
  end:
.end

.sub test_bitwise_xor
    .param pmc type

    $P0 = new type
    $P0 = 40
    $P1 = new type
    $P1 = 2
    $P2 = new type
    $P2 = 115200

    $P99 = $P2

    $S0 = "original dest is untouched in bitwise_xor for "
    $S1 = type
    concat $S0, $S1

    # ignore exceptions
    push_eh done
    bxor $P2, $P0, $P1

    $I0 = cmp $P99, 115200
    
    is( $I0, 0, $S0 )
    goto end

  done:
    ok(1, 'ignoring exceptions')
  end:
.end

.sub test_modulus
    .param pmc type

    $P0 = new type
    $P0 = 40
    $P1 = new type
    $P1 = 2
    $P2 = new type
    $P2 = 115200

    $P99 = $P2

    $S0 = "original dest is untouched in modulus for "
    $S1 = type
    concat $S0, $S1

    # ignore exceptions
    push_eh done
    mod $P2, $P0, $P1

    $I0 = cmp $P99, 115200
    
    is( $I0, 0, $S0 )
    goto end

  done:
    ok(1, 'ignoring exceptions')
  end:
.end

.sub test_pow
    .param pmc type

    $P0 = new type
    $P0 = 40
    $P1 = new type
    $P1 = 2
    $P2 = new type
    $P2 = 115200

    $P99 = $P2

    $S0 = "original dest is untouched in pow for "
    $S1 = type
    concat $S0, $S1

    # ignore exceptions
    push_eh done
    pow $P2, $P0, $P1

    $I0 = cmp $P99, 115200
    
    is( $I0, 0, $S0 )
    goto end

  done:
    ok(1, 'ignoring exceptions')
  end:
.end

.sub test_subtract
    .param pmc type

    $P0 = new type
    $P0 = 40
    $P1 = new type
    $P1 = 2
    $P2 = new type
    $P2 = 115200

    $P99 = $P2

    $S0 = "original dest is untouched in subtract for "
    $S1 = type
    concat $S0, $S1

    # ignore exceptions
    push_eh done
    sub $P2, $P0, $P1

    $I0 = cmp $P99, 115200
    
    is( $I0, 0, $S0 )
    goto end

  done:
    ok(1, 'ignoring exceptions')
  end:
.end

.sub test_bitwise_lsr
    .param pmc type

    $P0 = new type
    $P0 = 40
    $P1 = new type
    $P1 = 2
    $P2 = new type
    $P2 = 115200

    $P99 = $P2

    $S0 = "original dest is untouched in bitwise_lsr for "
    $S1 = type
    concat $S0, $S1

    # ignore exceptions
    push_eh done
    lsr $P2, $P0, $P1

    $I0 = cmp $P99, 115200
    
    is( $I0, 0, $S0 )
    goto end

  done:
    ok(1, 'ignoring exceptions')
  end:
.end

.sub test_bitwise_and
    .param pmc type

    $P0 = new type
    $P0 = 40
    $P1 = new type
    $P1 = 2
    $P2 = new type
    $P2 = 115200

    $P99 = $P2

    $S0 = "original dest is untouched in bitwise_and for "
    $S1 = type
    concat $S0, $S1

    # ignore exceptions
    push_eh done
    band $P2, $P0, $P1

    $I0 = cmp $P99, 115200
    
    is( $I0, 0, $S0 )
    goto end

  done:
    ok(1, 'ignoring exceptions')
  end:
.end

# Local Variables:
#   mode: pir
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir :
