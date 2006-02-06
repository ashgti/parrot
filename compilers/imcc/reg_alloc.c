/*
 * reg_alloc.c
 *
 * Register allocator:
 *
 * This is a brute force register allocator. It uses a graph-coloring
 * algorithm, but the implementation is very kludgy.
 *
 * It is a partial implementation of a Briggs-style register allocator
 * The following parts are just missing:
 *
 * - Renumbering
 * - Coaelesceing
 *
 */

#include <string.h>
#include <assert.h>
#include "imc.h"
#include "optimizer.h"

void graph_coloring_reg_alloc(Interp *interpreter, IMC_Unit * unit);
static void make_stat(IMC_Unit *, int *sets, int *cols);
static void imc_stat_init(IMC_Unit *);
static void print_stat(Parrot_Interp, IMC_Unit *);
static void build_reglist(Parrot_Interp, IMC_Unit * unit);
static void rebuild_reglist(Parrot_Interp, IMC_Unit * unit);
static void build_interference_graph(Parrot_Interp, IMC_Unit *);
static void compute_du_chain(IMC_Unit * unit);
static void compute_one_du_chain(SymReg * r, IMC_Unit * unit);
static int interferes(Interp *, IMC_Unit *, SymReg * r0, SymReg * r1);
static void map_colors(IMC_Unit *, int x, unsigned int * graph, char colors[], int typ, int);
static int try_allocate(Parrot_Interp, IMC_Unit *);
static void allocate_lexicals (Parrot_Interp, IMC_Unit *);
static void allocate_non_volatile (Parrot_Interp, IMC_Unit *);
#if 0
static int neighbours(int node);
#endif

static unsigned int* ig_get_word(int i, int j, int N, unsigned int* graph,
                                 int* bit_ofs)
{
    unsigned int bit = i * N + j;
    *bit_ofs = bit % sizeof(*graph);
    return &graph[bit / sizeof(*graph)];
}

static void ig_set(int i, int j, int N, unsigned int* graph)
{
    int bit_ofs;
    unsigned int* word = ig_get_word(i, j, N, graph, &bit_ofs);
    *word |= (1 << bit_ofs);
}

int ig_test(int i, int j, int N, unsigned int* graph);
int ig_test(int i, int j, int N, unsigned int* graph)
{
    int bit_ofs;
    unsigned int* word = ig_get_word(i, j, N, graph, &bit_ofs);
    return *word & (1 << bit_ofs);
}

static unsigned int* ig_allocate(int N)
{
    /* size is N*N bits, but we want don't want to allocate a partial
     * word, so round up to the nearest multiple of sizeof(int).
     */
    int need_bits = N * N;
    int num_words = (need_bits + sizeof(int) - 1) / sizeof(int);
    return (unsigned int*) mem_sys_allocate_zeroed(num_words * sizeof(int));
}

/* imc_reg_alloc is the main loop of the allocation algorithm. It operates
 * on a single compilation unit at a time.
 */
void
imc_reg_alloc(Interp *interpreter, IMC_Unit * unit)
{
    int first;
    char *function;

    if (!unit)
        return;
    if (!unit->instructions)
        return;
    imc_stat_init(unit);
    if (!(IMCC_INFO(interpreter)->optimizer_level &
                (OPT_PRE|OPT_CFG|OPT_PASM)) && unit->pasm_file)
        goto done;

    imcc_init_tables(interpreter);
    IMCC_INFO(interpreter)->allocated = 0;

#if IMC_TRACE
    fprintf(stderr, "reg_alloc.c: imc_reg_alloc\n");
    if (unit->instructions->r[1] && unit->instructions->r[1]->pcc_sub) {
        fprintf(stderr, "img_reg_alloc: pcc_sub (nargs = %d)\n",
            unit->instructions->r[1]->pcc_sub->nargs);
    }
#endif

    if (unit->instructions->n_r)
      function = unit->instructions->r[0]->name;
    else
      function = "(not a sub)";
    IMCC_debug(interpreter, DEBUG_IMC, "\n------------------------\n");
    IMCC_debug(interpreter, DEBUG_IMC, "processing sub %s\n", function);
    IMCC_debug(interpreter, DEBUG_IMC, "------------------------\n\n");

    if (IMCC_INFO(interpreter)->optimizer_level == OPT_PRE &&
            unit->pasm_file) {
        while (pre_optimize(interpreter, unit))
            ;
        goto done;
    }
    /*
     * all lexicals get a unique register
     */
    allocate_lexicals(interpreter, unit);

    /* build CFG and life info, and optimize iteratively */
    do {
        first = 1;
        do {
            while (pre_optimize(interpreter, unit));

            find_basic_blocks(interpreter, unit, first);
            build_cfg(interpreter, unit);
            first = 0;
        } while (cfg_optimize(interpreter, unit));

        compute_dominators(interpreter, unit);
        find_loops(interpreter, unit);
        if (IMCC_INFO(interpreter)->optimizer_level) {
            compute_dominance_frontiers(interpreter, unit);
        }

        build_reglist(interpreter, unit);
        life_analysis(interpreter, unit);
        allocate_non_volatile(interpreter, unit);
    } while (!IMCC_INFO(interpreter)->dont_optimize &&
            optimize(interpreter, unit));

    if (IMCC_INFO(interpreter)->debug & DEBUG_IMC)
        dump_symreg(unit);
    rebuild_reglist(interpreter, unit);
    graph_coloring_reg_alloc(interpreter, unit);

    if (IMCC_INFO(interpreter)->debug & DEBUG_IMC)
        dump_instructions(interpreter, unit);
done:
    if (IMCC_INFO(interpreter)->verbose  ||
            (IMCC_INFO(interpreter)->debug & DEBUG_IMC)) {
        print_stat(interpreter, unit);
    }
    else {
        make_stat(unit, NULL, unit->n_regs_used);
    }
}

void
free_reglist(IMC_Unit * unit)
{
#if IMC_TRACE
    fprintf(stderr, "reg_alloc.c: free_reglist\n");
#endif
    if (unit->interference_graph) {
        free(unit->interference_graph);
        unit->interference_graph =  0;
    }
    if (unit->reglist) {
        int i;
        for (i = 0; i < unit->n_symbols; i++)
            free_life_info(unit, unit->reglist[i]);
        free(unit->reglist);
        unit->reglist = NULL;
        unit->n_symbols = 0;
    }
}

void
graph_coloring_reg_alloc(Interp *interpreter, IMC_Unit * unit)
{

    build_interference_graph(interpreter, unit);

    try_allocate(interpreter, unit);
    IMCC_INFO(interpreter)->allocated = 1;
}

/* some statistics about register usage
 * printed with --verbose --verbose
 */
static void
make_stat(IMC_Unit * unit, int *sets, int *cols)
{
    /* register usage summary */
    char type[] = "INSP";
    int i, j;
    SymHash *hsh = &unit->hash;
    SymReg * r;

    for (i = 0; i < hsh->size; i++) {
        for (r = hsh->data[i]; r; r = r->next) {
            if (r->color > unit->max_color)
                unit->max_color = r->color;
            for (j = 0; j < 4; j++)
                if (r->set == type[j] && (r->type & VTREGISTER)) {
                    if (sets)
                        sets[j]++;
                    if (cols)
                        if (r->color > cols[j])
                            cols[j] = r->color;
                }
        }
    }
    if (cols) {
        for (j = 0; j < 4; j++)
            ++cols[j];
    }
}

/* registes usage of .pir */
static void
imc_stat_init(IMC_Unit * unit)
{
    int j;

    make_stat(unit, unit->n_vars_used, NULL);
    for (j = 0; j < 4; j++) {
        unit->n_regs_used[j] = -1;
        unit->first_avail[j] = 0;
    }
    /*
     * TODO move statistic into unit
     */
    memset(&ostat, 0, sizeof(ostat));
}

/* and final */
static void
print_stat(Parrot_Interp interpreter, IMC_Unit * unit)
{
    int sets[4] = {0,0,0,0};

    char *function;
    if (unit->instructions->n_r)
        function = unit->instructions->r[0]->name;
    else
        function = "(not a function)";

    make_stat(unit, sets, unit->n_regs_used);
    IMCC_info(interpreter, 1,
            "sub %s:\n\tregisters in .pir:\t I%d, N%d, S%d, P%d\n",
            function,
            unit->n_vars_used[0], unit->n_vars_used[1],
            unit->n_vars_used[2], unit->n_vars_used[3]);
    IMCC_info(interpreter, 1,
            "\t%d labels, %d lines deleted, "
            "%d if_branch, %d branch_branch\n",
            ostat.deleted_labels, ostat.deleted_ins, ostat.if_branch,
            ostat.branch_branch);
    IMCC_info(interpreter, 1, "\t%d branch_cond_loop\n",
            ostat.branch_cond_loop);
    IMCC_info(interpreter, 1, "\t%d used once deleted\n",
            ostat.used_once);
    IMCC_info(interpreter, 1, "\t%d invariants_moved\n",
            ostat.invariants_moved);
    IMCC_info(interpreter, 1, "\tregisters needed:\t I%d, N%d, S%d, P%d\n",
            sets[0], sets[1], sets[2], sets[3]);
    IMCC_info(interpreter, 1,
            "\tregisters in .pasm:\t I%d, N%d, S%d, P%d - %d\n",
            unit->n_regs_used[0], unit->n_regs_used[1],
            unit->n_regs_used[2], unit->n_regs_used[3]);
    IMCC_info(interpreter, 1, "\t%d basic_blocks, %d edges\n",
            unit->n_basic_blocks, edge_count(unit));
}

/* sort list by line  nr */
static int
reg_sort_f(const void *a, const void *b)
{
    SymReg *ra = *(SymReg**) a;
    SymReg *rb = *(SymReg**) b;
    if (ra->first_ins->index < rb->first_ins->index) {
	return -1;
    }
    else if (ra->first_ins->index == rb->first_ins->index) {
        return 0;
    }
    else {
	return 1;
    }
}

static void
sort_reglist(IMC_Unit *unit)
{
    qsort(unit->reglist, unit->n_symbols, sizeof(SymReg*), reg_sort_f);
}

/*
 * Registers are now sorted according to the line on which their usage
 * starts, which means that they are sorted by basic block numbers too.
 *
 * Run through them and allocate all that don't overlap in one bunch.
 */

/* make a linear list of IDENTs and VARs, set n_symbols
 * TODO
 *   split the whole life analysis into 4, one per register kind
 *   registers of different kind never interfere, but the reglist
 *   has them all
 */

static void
build_reglist(Parrot_Interp interpreter, IMC_Unit * unit)
{
    int i, count, unused, n_symbols;
    SymHash *hsh = &unit->hash;
    SymReg * r;

    IMCC_info(interpreter, 2, "build_reglist\n");
    /* count symbols */
    if (unit->reglist)
        free_reglist(unit);
    count = unit->hash.entries;
    if (count == 0)
        return;
    unit->reglist = mem_sys_allocate(count * sizeof(SymReg*));

    for (i = count = 0; i < hsh->size; i++) {
        for (r = hsh->data[i]; r; r = r->next) {
            /* Add each symbol to reglist  */
            if (r->type & VTREGISTER) {
                unit->reglist[count++] = r;
            }
        }
    }
    unit->n_symbols = n_symbols = count;
    if (IMCC_INFO(interpreter)->debug & DEBUG_IMC)
        dump_symreg(unit);
    compute_du_chain(unit);
    /* we might have unused symbols here, from optimizations */
    for (i = count = unused = 0; i < n_symbols; i++) {
        if (!unit->reglist[i]->first_ins)
            unused++;
        else if (i == count)
            count++;
        else
            unit->reglist[count++] = unit->reglist[i];
    }
    n_symbols -= unused;
    unit->n_symbols = n_symbols;
    sort_reglist(unit);
}

/*
 * Exclude all already allocated registers (< first_avail)
 * from reglist. This reduced the size of the interference graph
 * significantly
 */
static void
rebuild_reglist(Parrot_Interp interpreter, IMC_Unit * unit)
{
    int i, count, unused, reg_set;
    SymReg * r;
    char types[] = "INSP", *p;

    UNUSED(interpreter);
    for (i = count = unused = 0; i < unit->n_symbols; i++) {
        r = unit->reglist[i];
        if (r->color == -1)
            goto use_it;
        p = strchr(types, r->set);
        if (!p)
            goto use_it;
        reg_set = p - types;
        if (r->color < unit->first_avail[reg_set]) {
            unused++;
            continue;
        }
use_it:
        if (i == count)
            count++;
        else
            unit->reglist[count++] = unit->reglist[i];
    }
    unit->n_symbols -= unused;
}

/* Creates the interference graph between the variables.
 *
 * Data structure is a 2-d array 'interference_graph' bitmap where
 * row/column indices represent the same index in the list of all
 * symbols (unit->reglist) in the current compilation unit.
 *
 * Two variables interfere when they are alive at the same time.
 */

static void
build_interference_graph(Parrot_Interp interpreter, IMC_Unit * unit)
{
    int x, y, n_symbols;
    unsigned int* interference_graph;

    n_symbols = unit->n_symbols;
    if (!n_symbols)
        return;

    /* Construct a graph N x N where N = number of symbolics.
     * This piece can be rewritten without the N x N array
     */
    interference_graph = ig_allocate(n_symbols);
    unit->interference_graph = interference_graph;

    /* Calculate interferences between each chain and populate the the Y-axis */
    for (x = 0; x < n_symbols; x++) {
        /* If symbol was never used in a statment, it can't interfere */
        if (!unit->reglist[x]->first_ins)
            continue;
        for (y = x + 1; y < n_symbols; y++) {
            if (!unit->reglist[y]->first_ins)
                continue;
            if (interferes(interpreter, unit,
                        unit->reglist[x], unit->reglist[y])) {
                ig_set(x, y, n_symbols, interference_graph);
                ig_set(y, x, n_symbols, interference_graph);
            }
        }
    }

    if (IMCC_INFO(interpreter)->debug & DEBUG_IMC)
        dump_interference_graph(unit);
}


/* Compute a DU-chain for each symbolic in a compilation unit
 */
static void
compute_du_chain(IMC_Unit * unit)
{
    Instruction * ins, *lastbranch;
    int i;

    lastbranch = 0;

    /* Compute last branch in this procedure, update instruction index */
    for (i = 0, ins = unit->instructions; ins; ins = ins->next) {
        ins->index = i++;
        if (ins->type == ITBRANCH)
            lastbranch = ins;
    }

    /* Compute du-chains for all symbolics */
    for (i = 0; i < unit->n_symbols; i++) {
        SymReg * r = unit->reglist[i];
        compute_one_du_chain(r, unit);
        /* what is this used for? -lt */
        if (r->type == VTIDENTIFIER
                && lastbranch
                && r->last_ins
                && r->last_ins->index < lastbranch->index)
            r->last_ins = lastbranch;
    }
}

static void
compute_one_du_chain(SymReg * r, IMC_Unit * unit)
{
    Instruction * ins;

    /* We cannot rely on computing the value of r->first when parsing,
     * since the situation can be changed at any time by the register
     * allocation algorithm
     */

    r->first_ins = 0;
    r->use_count = r->lhs_use_count = 0;
    for (ins = unit->instructions; ins; ins = ins->next) {
        int ro, rw;
        ro = instruction_reads(ins, r);
        rw = instruction_writes(ins, r);
        if (ro || rw) {
            if (!r->first_ins) {
                r->first_ins = ins;
            }
            r->last_ins = ins;
            if (rw)
                r->lhs_use_count++;
            r->use_count++;
            /* if this symbol is used in a different scope
             * assume usage
             */
            if (r->reg) {
                r->lhs_use_count++;
                r->use_count++;
            }
        }
    }
}

/* See if r0's chain interferes with r1. */
/* We currently decide that two vars interfere if they are both alive
 * at any point. This could be improved, requiring that one is alive
 * at the point of _definition_ of the other.
 */

static int
interferes(Interp *interpreter, IMC_Unit * unit, SymReg * r0, SymReg * r1)
{

    int i;

    /* Registers don't interfere with themselves
    */
    if (r0 == r1)
        return 0;

    /* Different register sets don't interfere with each other */
    if (r0->set != r1->set)
        return 0;
    /* If the first time r0 appears is in the same instruction as the
     * last appearance of r1, or after its last appearance, then they
     * can't interfere.
     *
     * Even if r0 and r1 are called in the same instruction, and even
     * if this instrucion does modify r0, if its value is never used
     * later, then they can share the same register.
     */
#if 1
    /* If they only overlap one instruction and one is used RHS only
     * and the other LHS, then that's ok
     *
     * But only, if that isn't inside a loop, tested via loop_depth
     * see also imcc/t/reg/alloc_2
     *
     * TODO no interferences, if the life range ends in this
     *      basic block, because it's end is e.g. a returncc
     */
    if (r0->first_ins->index == r1->last_ins->index &&
            instruction_writes(r0->first_ins, r0) &&
            instruction_reads(r1->last_ins, r1) &&
            !instruction_reads(r0->first_ins, r0)) {
        Basic_block *bb;
        bb = unit->bb_list[r0->first_ins->bbindex];
        if (bb->loop_depth == 0)
            return 0;
    }
    if (r1->first_ins->index == r0->last_ins->index &&
            instruction_writes(r1->first_ins, r1) &&
            instruction_reads(r0->last_ins, r0) &&
            !instruction_reads(r1->first_ins, r1)) {
        Basic_block *bb;
        bb = unit->bb_list[r1->first_ins->bbindex];
        if (bb->loop_depth == 0)
            return 0;
    }
#endif

    /* Now: */

    if (r0->life_info == NULL || r1->life_info == NULL) {
        PANIC("interferes: INTERNAL ERROR: Life range is NULL\n");
    }

    for (i=0; i < unit->n_basic_blocks; i++) {
        Life_range *l0, *l1;

        l0 = r0->life_info[i];
        l1 = r1->life_info[i];

        /* One or both are not alive in this block, so we have
         * no conflict
         */
        if (!l0->first_ins || !l1->first_ins)
            continue;

        /* If the registers don't overlap, i.e first_x > last_y
         * then no interference
         */
        if (l0->first_ins->index > l1->last_ins->index)
            continue;
        if (l1->first_ins->index > l0->last_ins->index)
            continue;


        return 1;
    }

    return 0;
}

/*
 * find available color for register #x in available colors
 */
static int
ig_find_color(Interp* interpreter, IMC_Unit *unit, int x, char *avail)
{
    int c;

    UNUSED(interpreter);
    UNUSED(x);
    for (c = 0; c < unit->n_symbols; c++)
        if (avail[c])
            return c;
    return -1;
}
/*
 * Color the graph, assigning registers to each symbol:
 *
 * We just proceed popping items from the stack and assigning
 * a free color to them.
 *
 * If we run out of colors, then we need to spill the top node.
 */

static int
try_allocate(Parrot_Interp interpreter, IMC_Unit * unit)
{
    int x;
    int color;
    char *avail;
    int t, n;
    unsigned int *graph = unit->interference_graph;
    SymReg ** reglist = unit->reglist;
    SymReg *r;

    /*
     * unit->n_symbols should be an upper limit of needed colors
     */
    n = unit->n_symbols;
    if (unit->max_color >= n)
        n = unit->max_color + 1;
    avail = mem_sys_allocate(n);

    for (x = 0; x < unit->n_symbols; ++x) {
        r = reglist[x];
        if (r->color >= 0)
            continue;
        for (t = 0; t < 4; t++) {
            int typ = "INSP"[t];
            int already_allocated = unit->first_avail[t];
            /*
             * don't even consider these regs
             */
            if (r->set == typ) {
                memset(avail, 1, n);
                map_colors(unit, x, graph, avail, typ, already_allocated );
                color = ig_find_color(interpreter, unit, x, avail);
                if (color >= 0) {
                    color += already_allocated;
                    r->color = color;

                    IMCC_debug(interpreter, DEBUG_IMC,
                            "#[%s] gets color [%d]\n",
                            r->name, color);
                    break;
                }

                if (r->color == -1) {
                    IMCC_fatal(interpreter, DEBUG_IMC,
                            "# no more colors - this should not happen\n");

                }
            }
        }
    }

    mem_sys_free(avail);
    return -1; /* we are totally finished */
}

/*
 * map_colors: calculates what colors can be assigned to the x-th symbol.
 */
static void
map_colors(IMC_Unit* unit, int x, unsigned int *graph, char avail[],
        int typ, int already_allocated)
{
    int y = 0, n_symbols;
    SymReg * r;

    n_symbols = unit->n_symbols;
    for (y = 0; y < n_symbols; y++) {
        if (! ig_test(x, y, n_symbols, graph))
            continue;
        r = unit->reglist[y];
        if (   r
    	    && r->color != -1
	    && r->set == typ) {
            assert(r->color - already_allocated >= 0);
    	    avail[r->color - already_allocated] = 0;
    	}
    }
}

/*
 * find first available register of the given reg_set
 */

static int
first_avail(IMC_Unit *unit, int reg_set, Set **avail)
{
    int i, n, first;
    SymReg * r;
    SymHash *hsh;
    Set *allocated;

    n = unit->n_symbols;
    if (unit->max_color > n)
        n = unit->max_color;
    allocated = set_make(n);
    hsh = &unit->hash;
    for (i = 0; i < hsh->size; i++) {
        for (r = hsh->data[i]; r; r = r->next) {
            if (r->set != reg_set)
                continue;
            if (r->type & VTREGISTER) {
                if (r->color >= 0)
                    set_add(allocated, r->color);
            }
        }
    }
    first = set_first_zero(allocated);
    if (avail)
        *avail = allocated;
    else
        set_free(allocated);
    return first;
}


/*
 * allocate lexicals or non-volatile in ascending order
 */

static void
allocate_uniq(Parrot_Interp interpreter, IMC_Unit *unit, int usage)
{
    char type[] = "INSP";
    int i, j, reg_set, first_reg;
    SymReg * r;
    SymHash *hsh;
    Set *avail;

    hsh = &unit->hash;
    for (j = 0; j < 4; j++) {
        reg_set = type[j];
        first_reg = first_avail(unit, reg_set, &avail);
        for (i = 0; i < hsh->size; i++) {
            for (r = hsh->data[i]; r; r = r->next) {
                if (r->set != reg_set)
                    continue;
                if ((r->type & VTREGISTER) &&
                        r->color == -1 &&
                        (r->usage & usage)) {
                    if (set_contains(avail, first_reg))
                        first_reg = first_avail(unit, reg_set, NULL);
                    set_add(avail, first_reg);
                    r->color = first_reg++;
                    IMCC_debug(interpreter, DEBUG_IMC,
                            "allocate %s sym %c '%s'  color %d\n",
                            usage & U_LEXICAL ? "Lexical" : "Non-vol",
                            reg_set, r->name, r->color);
                }
            }
        }
        set_free(avail);
        unit->first_avail[j] = first_reg;
    }
    /*
     * TODO create allocation_threshold
     *      if there are less registers than threshold
     *      just allocate all and be done with it
     */
}

static void
allocate_lexicals(Parrot_Interp interpreter, IMC_Unit *unit)
{
    IMCC_debug(interpreter, DEBUG_IMC, "allocate lexicals\n");
    allocate_uniq(interpreter, unit, U_LEXICAL);
}

static void
allocate_non_volatile(Parrot_Interp interpreter, IMC_Unit *unit)
{
    IMCC_debug(interpreter, DEBUG_IMC, "allocate non_volatile\n");
    allocate_uniq(interpreter, unit, U_NON_VOLATILE);
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
