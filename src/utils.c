/*  utils.c
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  CVS Info
 *     $Id$
 *  Overview:
 *     Some utility functions
 *  Data Structure and Algorithms:
 *  History:
 *     Initial version by leo 2003.09.09
 *  Notes:
 *     Prototypes are in misc.h
 *  References:
 */

#include "parrot/parrot.h"

/*
 * s. the comments in math.ops
 */

INTVAL
intval_mod(INTVAL i2, INTVAL i3)
{
    INTVAL y;
    INTVAL z = i3;
    int s = 0;
    INTVAL r;

    if (z == 0)
	return i2;

    y = i2;

    if (y < 0) { s += 2; y = -y; }
    if (z < 0) { s += 1; z = -z; }

    r = y % z;

    switch (s) {
	case 0 + 0:            break;
	case 0 + 1: r = r - z; break;
	case 2 + 0: r = z - r; break;
	case 2 + 1: r = -r;    break;
    }

    return r;
}

FLOATVAL
floatval_mod(FLOATVAL n2, FLOATVAL n3)
{
#ifdef __LCC__

  /* Another workaround for buggy code generation in the lcc compiler-
   * adding a temporary variable makes it pass the test.
   */
  FLOATVAL temp = n3 * floor(n2 / n3);

  return n3
     ? (n2 - temp)
     : n2;
#else
  return n3
     ? (n2 - n3 * floor(n2 / n3))
     : n2;
#endif
}

/*
 * currently undefined
 */
#ifndef PARROT_HAS_DRAND48

typedef unsigned short _rand_buf[3];
/*
 * s. man drand48, SuS V2
 *
 * X(n+1) = ( aX(n) + c ) mod 2^48
 *
 */
#define A_lo  0xE66D
#define A_mid 0xDEEC
#define A_hi  0x5
#define C     0xB
#define SEED_LO 0x330E

static _rand_buf a = { A_lo, A_mid, A_hi };
static _rand_buf last_rand;
static unsigned short c = C;

static void
next_rand(_rand_buf X)
{
    unsigned short lo, mid, hi;
    unsigned int t;

    /* 48 bit mul, one short at a time */
    t = X[0] * a[0] + c;
    lo = t & 0xffff;
    mid = (t >> 16) & 0xffff;

    t = X[1] * a[0] + X[0] * a[1] + mid;
    mid = t & 0xffff;
    hi = (t >> 16) & 0xffff;

    t = X[2] * a[0] + X[1] * a[1] + X[0] * a[2] + hi;

    X[0] = lo;
    X[1] = mid;
    X[2] = t & 0xffff;
}

static FLOATVAL
_erand48(_rand_buf buf)
{
    FLOATVAL r;
    next_rand(buf);
    r = (( buf[0] / 65536.0 + buf[1] ) / 65536.0 + buf[2]) / 65536.0;
    return r;
}

static FLOATVAL
_drand48(void)
{
    return _erand48(last_rand);
}

static long
_jrand48(_rand_buf buf)
{
    long ret;
    next_rand(buf);
    ret = buf[2] << 16 | buf[1];
    return ret;
}

static long
_nrand48(_rand_buf buf)
{
    return _jrand48(buf) & 0x7fffffff;
}

static long
_lrand48(void)
{
    return _nrand48(last_rand);
}

static long
_mrand48(void)
{
    return _jrand48(last_rand);
}

static void
_srand48(long seed)
{
    last_rand[0] = SEED_LO;
    last_rand[1] = seed & 0xffff;
    last_rand[2] = (seed >> 16) & 0xffff;
    /*
     * reinit a, c if changed by lcong48()
     */
}

#undef A_lo
#undef A_mid
#undef A_hi
#undef C

#else

#  define _drand48 drand48
#  define _erand48(b) erand48(b)

#  define _lrand48 lrand48
#  define _nrand48(b) nrand48(b)

#  define _mrand48 mrand48
#  define _jrand48(b) jrand48(b)

#  define _srand48 srand48

#endif

FLOATVAL
Parrot_float_rand(INTVAL how_random)
{
    return _drand48();          /* [0.0..1.0] */
}

INTVAL
Parrot_uint_rand(INTVAL how_random)
{
    return _lrand48();          /* [0..2^31] */
}

INTVAL
Parrot_int_rand(INTVAL how_random)
{
    return _mrand48();          /* [-2^31..2^31] */
}

INTVAL
Parrot_range_rand(INTVAL from, INTVAL to, INTVAL how_random)
{
    return (INTVAL) from + ((double)(to - from)) * Parrot_float_rand(how_random);
}

void
Parrot_srand(INTVAL seed)
{
    _srand48(seed);
}

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
