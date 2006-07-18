/*
Copyright (C) 2001-2003, The Perl Foundation.
$Id$

=head1 NAME

src/utils.c - Some utility functions

=head1 DESCRIPTION

Prototypes are in F<src/misc.h>.

Opcode helper functions that don't really fit elsewhere.

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"

#define GRAPH_ELEMENT(PTR,REGS,X,Y)  (PTR)[((Y) * ((REGS) + 3)) + (X)]
#define ROOT_VERTEX(PTR,REGS,Y)      GRAPH_ELEMENT(PTR,REGS,REGS+0,Y)
#define REG_MOVE_VAL(PTR,REGS,Y)     GRAPH_ELEMENT(PTR,REGS,REGS+1,Y)
#define CYCLE_NODE(PTR,REGS,Y)        GRAPH_ELEMENT(PTR,REGS,REGS+2,Y)

/*

=item C<INTVAL intval_mod(INTVAL i2, INTVAL i3)>

NOTE: This "corrected mod" algorithm is based on the C code on page 70
of [1]. Assuming correct behavior of the built-in mod operator (%) with
positive arguments, this algorithm implements a mathematically
convenient version of mod, defined thus:

    x mod y = x - y * floor(x / y)

For more information on this definition of mod, see section 3.4 of [2],
pages 81-85.

References:

[1] Donald E. Knuth, *MMIXware: A RISC Computer for the Third
Millennium* Springer, 1999.

[2] Ronald L. Graham, Donald E. Knuth and Oren Patashnik, *Concrete
Mathematics*, Second Edition. Addison-Wesley, 1994.

=cut

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

    if (r) {    /* # 36003 */
        switch (s) {
            case 0:            break;
            case 1: r = r - z; break;
            case 2: r = z - r; break;
            case 3: r = -r;    break;
        }
    }

    return r;
}

/*

=item C<FLOATVAL
floatval_mod(FLOATVAL n2, FLOATVAL n3)>

Returns C<n2 mod n3>.

Includes a workaround for buggy code generation in the C<lcc> compiler.

=cut

*/

FLOATVAL
floatval_mod(FLOATVAL n2, FLOATVAL n3)
{
#ifdef __LCC__

  /* Another workaround for buggy code generation in the lcc compiler-
   * adding a temporary variable makes it pass the test.
   */
  const FLOATVAL temp = n3 * floor(n2 / n3);

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

=back

=head2 Random Number Generator

Based on the C<rand48()> family of functions.

=over 4

=cut

*/

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

/*

=item C<static void
next_rand(_rand_buf X)>

Returns the next random number in C<X>.

=cut

*/

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

/*

=item C<static FLOATVAL
_erand48(_rand_buf buf)>

Returns a C<double> in the interval C<[0.0, 1.0)>.

=cut

*/

static FLOATVAL
_erand48(_rand_buf buf)
{
    FLOATVAL r;
    next_rand(buf);
    r = (( buf[0] / 65536.0 + buf[1] ) / 65536.0 + buf[2]) / 65536.0;
    return r;
}

/*

=item C<static FLOATVAL
_drand48(void)>

Returns a C<double> in the interval C<[0.0, 1.0)>.

=cut

*/

static FLOATVAL
_drand48(void)
{
    return _erand48(last_rand);
}

/*

=item C<static long
_jrand48(_rand_buf buf)>

Returns a C<long> in the interval C<[-2^31, 2^31)>.

=cut

*/

static long
_jrand48(_rand_buf buf)
{
    long ret;
    next_rand(buf);
    ret = buf[2] << 16 | buf[1];
    return ret;
}

/*

=item C<static long
_nrand48(_rand_buf buf)>

Returns a C<long> in the interval C<[0, 2^31)>.

=cut

*/

static long
_nrand48(_rand_buf buf)
{
    return _jrand48(buf) & 0x7fffffff;
}

/*

=item C<static long
_lrand48(void)>

Returns a C<long> in the interval C<[0, 2^31)>.

=cut

*/

static long
_lrand48(void)
{
    return _nrand48(last_rand);
}

/*

=item C<static long
_mrand48(void)>

Returns a C<long> in the interval C<[-2^31, 2^31)>.

=cut

*/

static long
_mrand48(void)
{
    return _jrand48(last_rand);
}

/*

=item C<static void
_srand48(long seed)>

Sets the high order 32 bits to the argument C<seed>. The low order 16
bits are set to the arbitrary value 0x330e.

=cut

*/

static void
_srand48(long seed)
{
    last_rand[0] = SEED_LO;
    last_rand[1] = (unsigned short)seed & 0xffff;
    last_rand[2] = (unsigned short)(seed >> 16) & 0xffff;
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

/*

=item C<FLOATVAL
Parrot_float_rand(INTVAL how_random)>

Returns a C<FLOATVAL> in the interval C<[0.0, 1.0)>.

C<how_random> is ignored.

=cut

*/

FLOATVAL
Parrot_float_rand(INTVAL how_random)
{
    return _drand48();          /* [0.0..1.0] */
}

/*

=item C<INTVAL
Parrot_uint_rand(INTVAL how_random)>

Returns an C<INTVAL> in the interval C<[0, 2^31)>.

C<how_random> is ignored.

=cut

*/

INTVAL
Parrot_uint_rand(INTVAL how_random)
{
    return _lrand48();          /* [0..2^31] */
}

/*

=item C<INTVAL
Parrot_int_rand(INTVAL how_random)>

Returns an C<INTVAL> in the interval C<[-2^31, 2^31)>.

C<how_random> is ignored.

=cut

*/

INTVAL
Parrot_int_rand(INTVAL how_random)
{
    return _mrand48();          /* [-2^31..2^31] */
}

/*

=item C<INTVAL
Parrot_range_rand(INTVAL from, INTVAL to, INTVAL how_random)>

Returns an C<INTVAL> in the range C<[from, to]>.

C<how_random> is ignored.

=cut

*/

INTVAL
Parrot_range_rand(INTVAL from, INTVAL to, INTVAL how_random)
{
    return (INTVAL)( from + ((double)(to - from)) * Parrot_float_rand(how_random) );
}

/*

=item C<void
Parrot_srand(INTVAL seed)>

Seeds the random number generator with C<seed>.

=cut

*/

void
Parrot_srand(INTVAL seed)
{
    _srand48(seed);
}

/*

=back

=head2 Array Functions

=over

=item C<void *
Parrot_make_la(Interp *interpreter, PMC *array)>

Creates a C array of C<long>s with one more element than the number of
elements in C<*array>. The elements are then copied from C<*array> to
the new array, and the last (extra) element is set to 0.

Used in C<src/nci.c>.

=cut

*/

void *
Parrot_make_la(Interp *interpreter, PMC *array) {
    const INTVAL arraylen = VTABLE_elements(interpreter, array);
    INTVAL cur;

    /* Allocate the array and set the last element to 0. Since we
       always allocate one element more than we use we're guaranteed
       to actually have an array, even if the inbound array is
       completely empty
    */
    long * const out_array = mem_sys_allocate((sizeof(long)) * (arraylen + 1));
    out_array[arraylen] = 0;
    /*    printf("Long array has %i elements\n", arraylen);*/
    for (cur = 0; cur < arraylen; cur++) {
        out_array[cur] = VTABLE_get_integer_keyed_int(interpreter, array, cur);
    }

    return out_array;
}

/*

=item C<void
Parrot_destroy_la(long *array)>

Use this to destroy an array created with C<Parrot_make_la()>.

=cut

*/

void
Parrot_destroy_la(long *array) {
    mem_sys_free(array);
}

/*

=item C<void *
Parrot_make_cpa(Interp *interpreter, PMC *array)>

Creates a C array of C<char *>s with one more element than the number of
elements in C<*array>. The elements are then copied from C<*array> to
the new array, and the last (extra) element is set to 0.

Currently unused.

=cut

*/

void *
Parrot_make_cpa(Interp *interpreter, PMC *array) {
    const INTVAL arraylen = VTABLE_elements(interpreter, array);
    INTVAL cur;

    /* Allocate the array and set the last element to 0. Since we
       always allocate one element more than we use we're guaranteed
       to actually have an array, even if the inbound array is
       completely empty
    */
    char ** const out_array = mem_sys_allocate((sizeof(char *)) * (arraylen + 1));
    out_array[arraylen] = 0;

    /*    printf("String array has %i elements\n", arraylen);*/
    for (cur = 0; cur < arraylen; cur++) {
        out_array[cur] = string_to_cstring(interpreter, VTABLE_get_string_keyed_int(interpreter, array, cur));
        /*        printf("Offset %i is %s\n", cur, out_array[cur]);*/
    }

    return out_array;
}

/*

=item C<void
Parrot_destroy_cpa(char **array)>

Use this to destroy an array created with C<Parrot_make_cpa()>.

=cut

*/

void
Parrot_destroy_cpa(char **array) {
    UINTVAL offset = 0;
    /* Free each piece */
    while (array[offset] != NULL) {
        string_cstring_free(array[offset++]);
    }
    /* And then the holding array */
    mem_sys_free(array);
}
/*

=item C<PMC* tm_to_array(Parrot_Interp, struct tm *)>

Helper to convert a B<struct tm *> to an Array

=cut

*/

/* &gen_from_enum(tm.pasm) */
typedef enum {
    TM_SEC,
    TM_MIN,
    TM_HOUR,
    TM_MDAY,
    TM_MON,
    TM_YEAR,
    TM_WDAY,
    TM_YDAY,
    TM_ISDST
} tm_struct_enum;
/* &end_gen */

PMC*
tm_to_array(Parrot_Interp interpreter, const struct tm *tm)
{
  PMC * const Array = pmc_new(interpreter, enum_class_Array);
  VTABLE_set_integer_native(interpreter, Array, 9);
  VTABLE_set_integer_keyed_int(interpreter, Array, 0, tm->tm_sec);
  VTABLE_set_integer_keyed_int(interpreter, Array, 1, tm->tm_min);
  VTABLE_set_integer_keyed_int(interpreter, Array, 2, tm->tm_hour);
  VTABLE_set_integer_keyed_int(interpreter, Array, 3, tm->tm_mday);
  VTABLE_set_integer_keyed_int(interpreter, Array, 4, tm->tm_mon + 1);
  VTABLE_set_integer_keyed_int(interpreter, Array, 5, tm->tm_year + 1900);
  VTABLE_set_integer_keyed_int(interpreter, Array, 6, tm->tm_wday);
  VTABLE_set_integer_keyed_int(interpreter, Array, 7, tm->tm_yday);
  VTABLE_set_integer_keyed_int(interpreter, Array, 8, tm->tm_isdst);
  return Array;
}

INTVAL
Parrot_byte_index(Interp *interpreter, const STRING *base,
        const STRING *search, UINTVAL start_offset)
{
    char *base_start, *search_start;
    INTVAL current_offset;
    INTVAL max_possible_offset;
    INTVAL searchlen = search->strlen;
    search_start = search->strstart;
    max_possible_offset = (base->strlen - search->strlen);

    for (current_offset = start_offset; current_offset <= max_possible_offset;
            current_offset++) {
        base_start = (char *)base->strstart + current_offset;
        if (!memcmp(base_start, search_start, searchlen)) {
            return current_offset;
        }
    }

    return -1;
}

INTVAL
Parrot_byte_rindex(Interp *interpreter, const STRING *base,
        const STRING *search, UINTVAL start_offset)
{
    char *base_start, *search_start;
    INTVAL current_offset;
    UINTVAL max_possible_offset;
    INTVAL searchlen = search->strlen;
    search_start = search->strstart;
    max_possible_offset = (base->strlen - search->strlen);
    if (start_offset && start_offset < max_possible_offset) {
        max_possible_offset = start_offset;
    }
    for (current_offset = max_possible_offset; current_offset >= 0;
            current_offset--) {
        base_start = (char *)base->strstart + current_offset;
        if (!memcmp(base_start, search_start, searchlen)) {
            return current_offset;
        }
    }

    return -1;
}

/*
  
=item C<static int
find_first_indegree(int *graph,int node_count, int dest)>
  
Finds the first indegree for the given node.
   
=cut
      
*/
static int
find_first_indegree(int *graph, int node_count, int dest)
{
    int i = 0;
    for (i = 0; i < node_count; i++) {
        if (GRAPH_ELEMENT(graph, node_count, i, dest) > 0) {
            return i;
        }
    }
    return -1;
}

/*
 
=item C<static int
find_root(int *graph ,int node_count, int src, int dest)>
          
Finds the root vertex of the graph.

=cut

*/
static int
find_root(int *graph, int node_count, int src, int dest)
{
    int in_degree;
    if (GRAPH_ELEMENT(graph, node_count, src, dest) == 2) {
        CYCLE_NODE(graph, node_count, dest) = 1;    
        GRAPH_ELEMENT(graph, node_count, src, dest) = 1;
        return dest;
    }
    ROOT_VERTEX(graph, node_count, src) = 0;
    GRAPH_ELEMENT(graph, node_count, src, dest) = 2;
    in_degree = find_first_indegree(graph, node_count, src);
    if (in_degree == -1) {
        ROOT_VERTEX(graph, node_count, dest) = 0;
        GRAPH_ELEMENT(graph, node_count, src, dest) = 1;
        return src;
    }
    return find_root(graph, node_count, in_degree, src);
}

/*

=item C<static void
emit_mov(reg_move_func mov, Interp *interpreter, void *info, int emit,
          int emit_temp, int dest, int src, int temp, int * map)>
          
   
Emit the move instructions
  
=cut
       
*/
static void
emit_mov(reg_move_func mov, Interp * interpreter, void *info, int emit,
         int emit_temp, int dest, int src, int temp)
{
    if (emit > -1) {
        if (emit_temp) {
            mov(interpreter, dest, temp, info);
        }
        else if (src != dest) {
            mov(interpreter, dest, src, info);
        }
    }
}

/*
 
=item C<static int
reorder_move(int *graph, INT node_count, int *map, INT * val,
             int src, int prev, int depth, reg_move_func mov,
             Interp *interpreter, void *info, int temp)>
                          
   
This method reorders the move operations.OA
   
=cut
       
*/
static int
reorder_move(int *graph, int node_count, int src, int prev, int depth, 
            reg_move_func mov, Interp * interpreter, void *info, int temp)
{
    int i, x;
    REG_MOVE_VAL(graph, node_count, src) = 1;
  
    for (i = 0; i < node_count; i++) {
        if (GRAPH_ELEMENT(graph, node_count, src, i) > 0) {
            if (REG_MOVE_VAL(graph, node_count, i) == 1) {
                emit_mov(mov, interpreter, info, prev, 0, i, src, temp);
                emit_mov(mov, interpreter, info, prev, depth <= 1, src, prev,
                         temp);
                return 1;
            }
            if (REG_MOVE_VAL(graph, node_count, i) != 2) {
                depth++;
                x = reorder_move(graph, node_count, i, src, depth,
                                     mov, interpreter, info, temp);
                depth--;
                emit_mov(mov, interpreter, info, prev,
                         x && (depth <= 1), src, prev, temp);
                return x;
            }
        }
    }
    REG_MOVE_VAL(graph, node_count, src) = 2;
    emit_mov(mov, interpreter, info, prev, 0, src, prev, temp);
    return 0;
}

/*

=item C<typedef int (*reg_move_func)(Interp*, unsigned char d, unsigned char s, void *);>

=item C<void Parrot_register_move(Interp *, int n_regs,
        unsigned char *dest_regs, unsigned char *src_regs,
        unsigned char temp_reg, 
        reg_move_func mov, 
        reg_move_func mov_alt, 
        void *info)>

Move C<n_regs> from the given register list C<src_regs> to C<dest_regs>.

  n_regs    ... amount of registers to move
  dest_regs ... list of register numbers 0..255
  src_regs  ... list of register numbers 0..255
  temp_reg  ... a register number not in one of these lists
  mov       ... a register move function to be called to move one register
  mov_alt   ... a register move function to be called to move one register
                which triese fetching from an alternate src (or NULLfunc):
    
    (void)  (mov)(interp, dest, src, info);       
    moved = (mov_alt)(interp, dest, src, info);

Some C<dest_regs> might be the same as C<src_regs>, which makes this a bit 
non-trivial, because if the destination is already clobbered, using it
later as source doesn"t work. E.g.

  0 <- 1
  1 <- 0     # register 0 already clobbered

or

  2 <- 0
  0 <- 1
  3 <- 2      # register 2 already clobbered - reorder moves

To handle such cases, we do:

  a) rearrange the order of moves (not possible in the first case)
     and/or if that failed:
  b) if an alternate move function is available, it may fetch the
     source from a different (non-clobbered) location - call it.
     if the function returns 0 also use c)
  c) if no alternate move function is available, use the temp reg   

The amount of register moves should of course be minimal.  

TODO The current implementation will not work for following cases

Talked to Leo and he said those cases are not likely (Vishal Soni).
1. I0->I1 I1->I0 I0->I3
2. I1->I2 I3->I2

TODO: Add tests for the above conditions.
=cut
  
*/
void
Parrot_register_move(Interp * interpreter, int n_regs,
                     unsigned char *dest_regs, unsigned char *src_regs,
                     unsigned char temp_reg,
                     reg_move_func mov, reg_move_func mov_alt, void *info)
{
    int i, uniq_reg_cnt;
    int *reg_move_graph;
    uniq_reg_cnt=0;

    if (n_regs == 0)
        return;

    for (i = 0; i < n_regs; i++) {
        if (src_regs[i] > uniq_reg_cnt) {
            uniq_reg_cnt=src_regs[i];
        }
        
        if (dest_regs[i] > uniq_reg_cnt) {
            uniq_reg_cnt=dest_regs[i];
        }   
    }
    uniq_reg_cnt++;
    
    reg_move_graph = (int *)
        mem_sys_allocate_zeroed(sizeof(int) * uniq_reg_cnt *
                                (uniq_reg_cnt + 3));

    for (i = 0; i < n_regs; i++) {
        GRAPH_ELEMENT(reg_move_graph, uniq_reg_cnt, src_regs[i],
                      dest_regs[i]) = 1;
        ROOT_VERTEX(reg_move_graph, uniq_reg_cnt,
                    find_root(reg_move_graph, uniq_reg_cnt, src_regs[i],
                              dest_regs[i])) = 1;
        GRAPH_ELEMENT(reg_move_graph, uniq_reg_cnt, src_regs[i],
                      dest_regs[i]) = 1;
    }
    for (i = 0; i < uniq_reg_cnt; i++) {
        if (ROOT_VERTEX(reg_move_graph, uniq_reg_cnt, i) > 0) {
            if (GRAPH_ELEMENT(reg_move_graph, uniq_reg_cnt, i, i) == 1) {
                /* mov(interpreter, i, i, info); */
            }
            else {
                if (CYCLE_NODE(reg_move_graph, uniq_reg_cnt, i)) {
                    mov(interpreter, temp_reg, i, info);
                }
                reorder_move(reg_move_graph, uniq_reg_cnt, i, -1, 0, mov,
                         interpreter, info, temp_reg);
            }
        }
    }

    mem_sys_free(reg_move_graph);
}


/*

=back

=head1 HISTORY

Initial version by leo 2003.09.09.

=cut

*/

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
