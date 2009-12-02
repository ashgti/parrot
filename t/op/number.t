#!parrot
# Copyright (C) 2001-2009, Parrot Foundation.
# $Id$

=head1 NAME

t/op/number.t - Number Registers

=head1 SYNOPSIS

    % prove t/op/number.t

=head1 DESCRIPTION

Tests the use of Parrot floating-point number registers.

=cut

.sub main :main
    .include 'test_more.pir'

    plan(173)
    test_set_n_nc()
    test_set_n()
    test_add_n_n_n()
    test_add_n_n()
    test_sub_n_n_n()
    test_sub_n_n()
    test_abs_n_i_ic_n_nc()
    test_mul_i()
    test_div_i()
    test_mod_n()
    test_cmod_n()
    test_eq_n_ic()
    test_eq_nc_ic()
    test_ne_n_ic()
    test_ne_n_nc_ic()
    test_lt_n_ic()
    test_lt_nc_ic()
    test_le_n_ic()
    test_le_nc_ic()
    test_gt_n_ic()
    test_gt_nc_ic()
    test_ge_n_ic()
    test_ge_nc_ic()
    test_if_n_ic()
    test_inc_n()
    test_dec_n()
    test_set_i_n()
    test_clearn()
    test_neg_n()
    test_neg_0_dot_0()
    test_mul_n_n()
    test_fact_n_i()
    test_fact_n_ic()
    test_exchange()
    test_op_n_nc_nc()
    test_lt_nc_nc_ic()
    test_string_gt_num()
    test_null()
    test_dot_dig_parsing()
    test_sqrt_n_n()
    test_exception_div_n_n_by_zero()
    test_exception_div_n_nc_by_zero()
    test_exception_div_n_n_n_by_zero()
    test_exception_div_n_nc_n_by_zero()
    test_exception_div_n_n_nc_by_zero()
    test_exception_fdiv_n_n_by_zero()
    test_exception_fdiv_n_nc_by_zero()
    test_exception_fdiv_n_n_n_by_zero()
    test_exception_fdiv_n_nc_n_by_zero()
    test_exception_fdiv_n_n_nc_by_zero()
    test_exception_cmod_n_n_n_by_zero()
    test_exception_cmod_n_nc_n_by_zero()
    test_exception_cmod_n_n_nc_by_zero()
    test_mod_n_n_n_by_zero()
    test_mod_n_nc_n_by_zero()
    test_mod_n_n_nc_by_zero()
    # END_OF_TESTS
.end

.macro exception_is ( M )
    .local pmc exception
    .local string message
    .get_results (exception)

    message = exception['message']
    is( message, .M, .M )
.endm

.sub test_set_n_nc
    set     $N0, 1.0
    set     $N1, 4.0
    set     $N2, 16.0
    set     $N3, 64.0
    set     $N4, 256.0
    set     $N5, 1024.0
    set     $N6, 4096.0
    set     $N7, 16384.0
    set     $N8, 65536.0
    set     $N9, 262144.0
    set     $N10, 1048576.0
    set     $N11, 4194304.0
    set     $N12, 16777216.0
    set     $N13, 67108864.0
    set     $N14, 268435456.0
    set     $N15, 1073741824.0
    set     $N16, 4294967296.0
    set     $N17, 17179869184.0
    set     $N18, 68719476736.0
    set     $N19, 274877906944.0
    set     $N20, 1099511627776.0
    set     $N21, 4398046511104.0
    set     $N22, 17592186044416.0
    set     $N23, 70368744177664.0
    set     $N24, 281474976710656.0
    set     $N25, 1.12589990684262e+15
    is( $N0, "1", 'set_n_nc' )
    is( $N1, "4", 'set_n_nc' )
    is( $N2, "16", 'set_n_nc' )
    is( $N3, "64", 'set_n_nc' )
    is( $N4, "256", 'set_n_nc' )
    is( $N5, "1024", 'set_n_nc' )
    is( $N6, "4096", 'set_n_nc' )
    is( $N7, "16384", 'set_n_nc' )
    is( $N8, "65536", 'set_n_nc' )
    is( $N9, "262144", 'set_n_nc' )
    is( $N10, "1048576", 'set_n_nc' )
    is( $N11, "4194304", 'set_n_nc' )
    is( $N12, "16777216", 'set_n_nc' )
    is( $N13, "67108864", 'set_n_nc' )
    is( $N14, "268435456", 'set_n_nc' )
    is( $N15, "1073741824", 'set_n_nc' )
    is( $N16, "4294967296", 'set_n_nc' )
    is( $N17, "17179869184", 'set_n_nc' )
    is( $N18, "68719476736", 'set_n_nc' )
    is( $N19, "274877906944", 'set_n_nc' )
    is( $N20, "1099511627776", 'set_n_nc' )
    is( $N21, "4398046511104", 'set_n_nc' )
    is( $N22, "17592186044416", 'set_n_nc' )
    is( $N23, "70368744177664", 'set_n_nc' )
    is( $N24, "281474976710656", 'set_n_nc' )
    is( $N25, "1.12589990684262e+15", 'set_n_nc' )
.end

.sub test_set_n
    set     $N0, 42.0
    set     $N1, $N0
    is( $N1, "42", 'set_n' )
.end

.sub test_add_n_n_n
    set     $N0, 1.0
    add     $N1, $N0, $N0
    is( $N1, "2", 'add_n_n_n' )
    add     $N2, $N0, $N1
    is( $N2, "3", 'add_n_n_n' )
    add     $N2, $N2, $N2
    is( $N2, "6", 'add_n_n_n' )
.end

.sub test_add_n_n
    set     $N0, 1.0
    add     $N0, $N0
    is( $N0, "2", 'add_n_n' )
    set     $N1, 1.0
    add     $N0, $N1
    is( $N0, "3", 'add_n_n' )
    add     $N0, 3.0
    is( $N0, "6", 'add_n_n' )
.end

.sub test_sub_n_n_n
    set     $N0, 424242.0
    set     $N1, 4200.0
    sub     $N2, $N0, $N1
    is( $N2, "420042", 'sub_n_n_n' )
.end

.sub test_sub_n_n
    set     $N0, 424242.0
    set     $N1, 4200.0
    sub     $N0, $N1
    is( $N0, "420042", 'sub_n_n' )
    sub     $N0, $N0
    is( $N0, "0", 'sub_n_n' )
.end

.sub test_abs_n_i_ic_n_nc
    set     $I0, -1
    abs     $N0, $I0
    abs     $N1, -1
    set     $I1, 1
    abs     $N2, $I1
    abs     $N3, 1
    set     $N4, -1
    abs     $N4, $N4
    abs     $N5, -1.0
    set     $N6, 1.0
    abs     $N6, $N6
    abs     $N7, 1.0
    is( $N0, "1", 'abs(n, i|ic|n|nc)' )
    is( $N1, "1", 'abs(n, i|ic|n|nc)' )
    is( $N2, "1", 'abs(n, i|ic|n|nc)' )
    is( $N3, "1", 'abs(n, i|ic|n|nc)' )
    is( $N4, "1", 'abs(n, i|ic|n|nc)' )
    is( $N5, "1", 'abs(n, i|ic|n|nc)' )
    is( $N6, "1", 'abs(n, i|ic|n|nc)' )
    is( $N7, "1", 'abs(n, i|ic|n|nc)' )
.end

.sub test_mul_i
    set     $N0, 2.0
    mul     $N1, $N0, $N0
    mul     $N1, $N1, $N0
    mul     $N1, $N1, $N0
    mul     $N1, $N1, $N0
    mul     $N1, $N1, $N0
    mul     $N1, $N1, $N0
    mul     $N1, $N1, $N0
    is( $N1, "256", 'mul_i' )
.end

.sub test_div_i
    set     $N0, 10.0
    set     $N1, 2.0
    div     $N2, $N0, $N1
    is( $N2, "5", 'div_i' )
    set     $N3, 7.0
    set     $N4, 2.0
    div     $N3, $N3, $N4
    is( $N3, "3.5", 'div_i' )
    set     $N5, 9.0
    set     $N6, -4.0
    div     $N7, $N5, $N6
    is( $N7, "-2.25", 'div_i' )
.end

.sub test_mod_n
    set     $N0, 5.0
    set     $N1, 0.0
    mod     $N2, $N0, $N1
    is( $N2, "5", 'mod_n' )
    set     $N0, 0.0
    set     $N1, 3.0
    mod     $N2, $N0, $N1
    is( $N2, "0", 'mod_n' )
    set     $N0, 5.0
    set     $N1, 3.0
    mod     $N2, $N0, $N1
    is( $N2, "2", 'mod_n' )
    set     $N0, 5.0
    set     $N1, -3.0
    mod     $N2, $N0, $N1
    is( $N2, "-1", 'mod_n' )
    set     $N0, -5.0
    set     $N1, 3.0
    mod     $N2, $N0, $N1
    is( $N2, "1", 'mod_n' )
    set     $N0, -5.0
    set     $N1, -3.0
    mod     $N2, $N0, $N1
    is( $N2, "-2", 'mod_n' )
.end

.sub test_cmod_n
    set     $N0, 5.000
    set     $N1, 3.000
    cmod    $N2, $N0, $N1
    is( $N2, "2", 'cmod_n' )
.end

.sub test_eq_n_ic
    set     $N0, 5.000001
    set     $N1, 5.000001
    set     $N2, 5.000002
    eq      $N0, $N1, ONE
    branch  ERROR
    ok( 0, 'test eq_n_ic bad' )
  ONE:
    ok( 1, 'eq_n_ic ok 1')
    eq      $N1, $N2, ERROR
    branch  TWO
    ok( 0, 'eq_n_ic bad 1' )
  TWO:
    ok( 1, 'eq_n_ic ok 2')
    goto END
  ERROR:
    ok( 0, 'eq_n_ic bad 2' )
  END:
.end

.sub test_eq_nc_ic
    set     $N0, 1.000001
    eq      $N0, 1, ERROR
    branch  ONE
    ok( 0, 'eq_nc_ic')
  ONE:
    ok( 1, 'eq_nc_ic ok 1')
    eq      $N0, 1.000001, TWO
    branch  ERROR
    ok( 0, 'eq_nc_ic')
  TWO:
    ok( 1, 'eq_nc_ic ok 2')
    goto END
  ERROR:
    ok( 0, 'eq_nc_ic')
  END:
.end

.sub test_ne_n_ic
    set     $N0, -22.222222
    set     $N1, -22.222222
    set     $N2, 0.0
    ne      $N0, $N2, ONE
    branch  ERROR
    ok( 0, 'ne_n_ic')
  ONE:
    ok( 1, 'ne_n_ic ok 1')
    ne      $N0, $N1, ERROR
    branch  TWO
    ok( 0, 'ne_n_ic')
  TWO:
    ok( 1, 'ne_n_ic ok 2')
    goto END
  ERROR:
    ok( 0, 'ne_n_ic')
  END:
.end

.sub test_ne_n_nc_ic
    set     $N0, 1073741824.0
    ne      $N0, 1073741824.0, nok1
    ok( 1, 'ne_n_nc_ic ok 1')
    branch  ONE
  nok1:
    ok( 0, 'ne_n_nc_ic')
  ONE:
    ne      $N0, 0.0, TWO
    branch  ERROR
  TWO:
    ok( 1, 'ne_n_nc_ic ok 2')
    goto END
  ERROR:
    ok( 0, 'ne_n_nc_ic')
  END:
.end

.sub test_lt_n_ic
    set     $N0, 1000.0
    set     $N1, 500.0
    set     $N2, 0.0
    set     $N3, 0.0
    lt      $N1, $N0, ONE
    branch  ERROR
    ok( 0, 'lt_n_ic')
  ONE:
    ok( 1, 'lt_n_ic ok 1')
    lt      $N0, $N1, ERROR
    branch  TWO
    ok( 0, 'lt_n_ic')
  TWO:
    ok( 1, 'lt_n_ic ok 2')
    lt      $N2, $N3, ERROR
    branch  THREE
    ok( 0, 'lt_n_ic')
  THREE:
    ok( 1, 'lt_n_ic ok 3')
    goto END
  ERROR:
    ok( 0, 'lt_n_ic')
  END:
.end

.sub test_lt_nc_ic
    set     $N0, 1000.0
    set     $N1, 500.0
    set     $N2, 0.0
    lt      $N0, 500.0, ERROR
    branch  ONE
    ok( 0, 'lt_nc_ic')
  ONE:
    ok( 1, 'lt_nc_ic ok 1')
    lt      $N1, 1000.0, TWO
    branch  ERROR
    ok( 0, 'lt_nc_ic')
  TWO:
    ok( 1, 'lt_nc_ic ok 2')
    lt      $N0, 0.0, ERROR
    branch  THREE
    ok( 0, 'lt_nc_ic')
  THREE:
    ok( 1, 'lt_nc_ic ok 3')
    goto END
  ERROR:
    ok( 0, 'lt_nc_ic')
  END:
.end

.sub test_le_n_ic
    set     $N0, 1000.0
    set     $N1, 500.0
    set     $N2, 0.0
    set     $N3, 0.0
    le      $N1, $N0, ONE
    branch  ERROR
    ok( 0, 'le_n_ic')
  ONE:
    ok( 1, 'le_n_ic ok 1')
    le      $N0, $N1, ERROR
    branch  TWO
    ok( 0, 'le_n_ic')
  TWO:
    ok( 1, 'le_n_ic ok 2')
    le      $N2, $N3, THREE
    branch  ERROR
    ok( 0, 'le_n_ic')
  THREE:
    ok( 1, 'le_n_ic ok 3')
    goto END
  ERROR:
    ok( 0, 'le_n_ic')
  END:
.end

.sub test_le_nc_ic
    set     $N0, 1000.0
    set     $N1, 500.0
    set     $N2, 0.0
    le      $N0, 500.0, ERROR
    branch  ONE
    ok( 0, 'le_nc_ic')
  ONE:
    ok( 1, 'le_nc_ic ok 1')
    le      $N1, 1000.0, TWO
    branch  ERROR
    ok( 0, 'le_nc_ic')
  TWO:
    ok( 1, 'le_nc_ic ok 2')
    le      $N2, 0.0, THREE
    branch  ERROR
    ok( 0, 'le_nc_ic')
  THREE:
    ok( 1, 'le_nc_ic ok 3')
    goto END
  ERROR:
    ok( 0, 'le_nc_ic')
  END:
.end

.sub test_gt_n_ic
    set     $N0, 500.0
    set     $N1, 1000.0
    set     $N2, 0.0
    set     $N3, 0.0
    gt      $N1, $N0, ONE
    branch  ERROR
    ok( 0, 'gt_n_ic')
  ONE:
    ok( 1, 'gt_n_ic ok 1')
    gt      $N0, $N1, ERROR
    branch  TWO
    ok( 0, 'gt_n_ic')
  TWO:
    ok( 1, 'gt_n_ic ok 2')
    gt      $N2, $N3, ERROR
    branch  THREE
    ok( 0, 'gt_n_ic')
  THREE:
    ok( 1, 'gt_n_ic ok 3')
    goto END
  ERROR:
    ok( 0, 'gt_n_ic')
  END:
.end

.sub test_gt_nc_ic
    set     $N0, 500.0
    set     $N1, 1000.0
    set     $N2, 0.0
    gt      $N0, 1000.0, ERROR
    branch  ONE
    ok( 0, 'gt_nc_ic')
  ONE:
    ok( 1, 'gt_nc_ic ok 1')
    gt      $N1, 500.0, TWO
    branch  ERROR
    ok( 0, 'gt_nc_ic')
  TWO:
    ok( 1, 'gt_nc_ic ok 2')
    gt      $N2, 0.0, ERROR
    branch  THREE
    ok( 0, 'gt_nc_ic')
  THREE:
    ok( 1, 'gt_nc_ic ok 3')
    goto END
  ERROR:
    ok( 0, 'gt_nc_ic')
  END:
.end

.sub test_ge_n_ic
    set     $N0, 500.0
    set     $N1, 1000.0
    set     $N2, 0.0
    set     $N3, 0.0
    ge      $N1, $N0, ONE
    branch  ERROR
    ok( 0, 'ge_n_ic')
  ONE:
    ok( 1, 'ge_n_ic ok 1')
    ge      $N0, $N1, ERROR
    branch  TWO
    ok( 0, 'ge_n_ic')
  TWO:
    ok( 1, 'ge_n_ic ok 2')
    ge      $N2, $N3, THREE
    branch  ERROR
    ok( 0, 'ge_n_ic')
  THREE:
    ok( 1, 'ge_n_ic ok 3')
    goto END
  ERROR:
    ok( 0, 'ge_n_ic')
  END:
.end

.sub test_ge_nc_ic
    set     $N0, 500.0
    set     $N1, 1000.0
    set     $N2, 0.0
    ge      $N0, 1000.0, ERROR
    branch  ONE
    ok( 0, 'ge_nc_ic')
  ONE:
    ok( 1, 'ge_nc_ic ok 1')
    ge      $N1, 500.0, TWO
    branch  ERROR
    ok( 0, 'ge_nc_ic')
  TWO:
    ok( 1, 'ge_nc_ic ok 2')
    ge      $N2, 0.0, THREE
    branch  ERROR
    ok( 0, 'ge_nc_ic')
  THREE:
    ok( 1, 'ge_nc_ic ok 3')
    goto END
  ERROR:
    ok( 0, 'ge_nc_ic')
  END:
.end

.sub test_if_n_ic
    set     $N0, 1000.0
    set     $N1, 500.0
    set     $N2, 0.0
    if      $N0, ONE
    branch  ERROR
    ok( 0, 'if_n_ic')
  ONE:
    ok( 1, 'if_n_ic ok 1')
    if      $N1, TWO
    branch  ERROR
    ok( 0, 'if_n_ic')
  TWO:
    ok( 1, 'if_n_ic ok 2')
    if      $N2, ERROR
    branch  THREE
    ok( 0, 'if_n_ic')
  THREE:
    ok( 1, 'if_n_ic ok 3')
    goto END
  ERROR:
    ok( 0, 'if_n_ic')
  END:
.end

.sub test_inc_n
    set     $N0, 0.0
    inc     $N0
    is( $N0, "1", 'inc_n' )
    inc     $N0
    inc     $N0
    inc     $N0
    inc     $N0
    is( $N0, "5", 'inc_n' )
.end

.sub test_dec_n
    set     $N0, 0.0
    dec     $N0
    is( $N0, "-1", 'dec_n' )
    dec     $N0
    dec     $N0
    dec     $N0
    dec     $N0
    is( $N0, "-5", 'dec_n' )
.end

.sub test_set_i_n
    set     $N0, 0.0
    set     $I0, $N0
    is( $I0, "0", 'set_i_n' )
    set     $N1, 2147483647.0
    set     $I1, $N1
    is( $I1, "2147483647", 'set_i_n' )
    set     $N2, -2147483648.0
    set     $I2, $N2
    is( $I2, "-2147483648", 'set_i_n' )
.end

.sub test_clearn
    set     $N0, 547972.0
    set     $N1, 547972.0
    set     $N2, 547972.0
    set     $N3, 547972.0
    set     $N4, 547972.0
    set     $N5, 547972.0
    set     $N6, 547972.0
    set     $N7, 547972.0
    set     $N8, 547972.0
    set     $N9, 547972.0
    set     $N10, 547972.0
    set     $N11, 547972.0
    set     $N12, 547972.0
    set     $N13, 547972.0
    set     $N14, 547972.0
    set     $N15, 547972.0
    set     $N16, 547972.0
    set     $N17, 547972.0
    set     $N18, 547972.0
    set     $N19, 547972.0
    set     $N20, 547972.0
    set     $N21, 547972.0
    set     $N22, 547972.0
    set     $N23, 547972.0
    set     $N24, 547972.0
    set     $N25, 547972.0
    set     $N26, 547972.0
    set     $N27, 547972.0
    set     $N28, 547972.0
    set     $N29, 547972.0
    set     $N30, 547972.0
    set     $N31, 547972.0
    clearn
    is( $N0, "0", 'clearn' )
    is( $N1, "0", 'clearn' )
    is( $N2, "0", 'clearn' )
    is( $N3, "0", 'clearn' )
    is( $N4, "0", 'clearn' )
    is( $N5, "0", 'clearn' )
    is( $N6, "0", 'clearn' )
    is( $N7, "0", 'clearn' )
    is( $N8, "0", 'clearn' )
    is( $N9, "0", 'clearn' )
    is( $N10, "0", 'clearn' )
    is( $N11, "0", 'clearn' )
    is( $N12, "0", 'clearn' )
    is( $N13, "0", 'clearn' )
    is( $N14, "0", 'clearn' )
    is( $N15, "0", 'clearn' )
    is( $N16, "0", 'clearn' )
    is( $N17, "0", 'clearn' )
    is( $N18, "0", 'clearn' )
    is( $N19, "0", 'clearn' )
    is( $N20, "0", 'clearn' )
    is( $N21, "0", 'clearn' )
    is( $N22, "0", 'clearn' )
    is( $N23, "0", 'clearn' )
    is( $N24, "0", 'clearn' )
    is( $N25, "0", 'clearn' )
    is( $N26, "0", 'clearn' )
    is( $N27, "0", 'clearn' )
    is( $N28, "0", 'clearn' )
    is( $N29, "0", 'clearn' )
    is( $N30, "0", 'clearn' )
    is( $N31, "0", 'clearn' )
.end

.sub test_neg_n
    neg $N0,3.0
    neg $N0,$N0
    neg $N0
    is( $N0, "-3", 'neg_n' )
.end

.sub test_neg_0_dot_0
    load_bytecode 'config.pbc'
    $P1 = _config()
    $P2 = $P1['has_negative_zero']
    unless $P2 goto negative_zero_todoed

    set $N1, 0
    neg $N1
    is( $N1, "-0", 'neg 0.0' )
    .return ()

  negative_zero_todoed:
    todo(0, '-0.0 not implemented, TT #313')
.end

.sub test_mul_n_n
    set $N0,3.0
    set $N1,4.0
    mul $N0,$N1
    is( $N0, "12", 'mul_n_n' )
.end

.sub test_fact_n_i
    set $I0, 3
    set $I1, 11
    set $I2, 0
    set $I3, -563
    fact $N5, $I0
    is( $N5, "6", 'fact_n_i' )
    fact $N6, $I1
    is( $N6, "39916800", 'fact_n_i' )
    fact $N7, $I2
    is( $N7, "1", 'fact_n_i' )
    fact $N8, $I3
    is( $N8, "1", 'fact_n_i' )
.end

.sub test_fact_n_ic
    fact $N5, 3
    is( $N5, "6", 'fact_n_ic' )
    fact $N6, 11
    is( $N6, "39916800", 'fact_n_ic' )
    fact $N7, 0
    is( $N7, "1", 'fact_n_ic' )
    fact $N8, -563
    is( $N8, "1", 'fact_n_ic' )
.end

.sub test_exchange
    set $N1, 1.234560
    set $N2, 9.876540
    exchange $N1, $N2
    is( $N1, "9.87654", 'exchange' )
    is( $N2, "1.23456", 'exchange' )
    set $N3, -100.200300
    exchange $N3, $N3
    is( $N3, "-100.2003", 'exchange' )
.end

.sub test_op_n_nc_nc
    add $N1, 2.0, 3.0
    is( $N1, "5", 'op_n_nc_nc' )
    sub $N1, 2.0, 4.0
    is( $N1, "-2", 'op_n_nc_nc' )
.end

.sub test_lt_nc_nc_ic
    lt 2.0, 1.0, nok
    ok( 1, 'lt_nc_nc_ic ok 1')
    lt 3.0, 4.0, ok_2
  nok:
    ok( 0, 'lt_nc_nc_ic')
    goto END
  ok_2:
    ok( 1, 'lt_nc_nc_ic ok 2')
  END:
.end

.sub test_string_gt_num
    set $S0, "1"
    set $S1, "12.0"
    set $S2, "-2.45"
    set $S3, "25e2"
    set $S4, "Banana"
    set $N0, $S0
    set $N1, $S1
    set $N2, $S2
    set $N3, $S3
    set $N4, $S4
    is( $N0, "1", 'string -> num' )
    is( $N1, "12", 'string -> num' )
    is( $N2, "-2.45", 'string -> num' )
    is( $N3, "2500", 'string -> num' )
    is( $N4, "0", 'string -> num' )
.end

.sub test_null
    set $N31, 12.5
    is( $N31, "12.5", 'null' )
    null $N31
    is( $N31, "0", 'null' )
.end

.sub test_dot_dig_parsing
    set $N0, .5
    is( $N0, "0.5", '.dig parsing' )
.end

# Don't check exact string representation. Last digit part can be different */
.sub test_sqrt_n_n
    $P0 = new 'Float'
    $N1 = 2
    $N2 = sqrt $N1
    $P0 = $N2
    is( $P0, 1.414213562373, 'sqrt_n_n',1e-6 )

    $N2 = sqrt 2.0
    $P0 = $N2
    is( $P0, 1.414213562373, 'sqrt_n_n',1e-6 )
.end

.sub test_exception_div_n_n_by_zero
    push_eh handler
    set $N0, 0
    set $N1, 10
    div $N1, $N0
  handler:
    .exception_is( 'Divide by zero' )
.end

.sub test_exception_div_n_nc_by_zero
    push_eh handler
    set $N1, 10
    div $N1, 0
  handler:
    .exception_is( 'Divide by zero' )
.end

.sub test_exception_div_n_n_n_by_zero
    push_eh handler
    set $N0, 0
    set $N1, 10
    div $N2, $N1, $N0
  handler:
    .exception_is( 'Divide by zero' )
.end

.sub test_exception_div_n_nc_n_by_zero
    push_eh handler
    set $N0, 0
    div $N2, 10, $N0
  handler:
    .exception_is( 'Divide by zero' )
.end

.sub test_exception_div_n_n_nc_by_zero
    push_eh handler
    set $N1, 10
    div $N2, $N1, 0
  handler:
    .exception_is( 'Divide by zero' )
.end

.sub test_exception_fdiv_n_n_by_zero
    push_eh handler
    set $N0, 0
    set $N1, 10
    fdiv $N1, $N0
  handler:
    .exception_is( 'Divide by zero' )
.end

.sub test_exception_fdiv_n_nc_by_zero
    push_eh handler
    set $N1, 10
    fdiv $N1, 0
  handler:
    .exception_is( 'Divide by zero' )
.end

.sub test_exception_fdiv_n_n_n_by_zero
    push_eh handler
    set $N0, 0
    set $N1, 10
    fdiv $N2, $N1, $N0
  handler:
    .exception_is( 'Divide by zero' )
.end

.sub test_exception_fdiv_n_nc_n_by_zero
    push_eh handler
    set $N0, 0
    fdiv $N2, 10, $N0
  handler:
    .exception_is( 'Divide by zero' )
.end

.sub test_exception_fdiv_n_n_nc_by_zero
    push_eh handler
    set $N1, 10
    fdiv $N2, $N1, 0
  handler:
    .exception_is( 'Divide by zero' )
.end

.sub test_exception_cmod_n_n_n_by_zero
    push_eh handler
    set $N0, 0
    set $N1, 10
    cmod $N2, $N1, $N0
  handler:
    .exception_is( 'Divide by zero' )
.end

.sub test_exception_cmod_n_nc_n_by_zero
    push_eh handler
    set $N0, 0
    cmod $N2, 10, $N0
  handler:
    .exception_is( 'Divide by zero' )
.end

.sub test_exception_cmod_n_n_nc_by_zero
    push_eh handler
    set $N1, 10
    cmod $N2, $N1, 0
  handler:
    .exception_is( 'Divide by zero' )
.end

.sub test_mod_n_n_n_by_zero
    set $N0, 0
    set $N1, 10
    mod $N2, $N1, $N0
    is( $N2, "10", 'mod_n_n_n by zero' )
.end

.sub test_mod_n_nc_n_by_zero
    set $N0, 0
    mod $N2, 10, $N0
    is( $N2, 10, 'mod_n_nc_n by zero' )
.end

.sub test_mod_n_n_nc_by_zero
    set $N1, 10
    mod $N2, $N1, 0
    is( $N2, '10', 'mod_n_n_nc by zero' )
.end

# Local Variables:
#   mode: pir
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
