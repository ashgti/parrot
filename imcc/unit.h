

#ifndef IMCC_UNIT_H
#define IMCC_UNIT_H

/* A IMC compilation unit or atomic metadata item.
 * The unit holds the instruction list as well as
 * all of the compiler state info, (reg allocator, cfg, etc.)
 * for the unit of instructions.
 */

typedef enum {
    IMC_PASM = 0,
    IMC_SUB = 1,
    IMC_CLASS = 2
} IMC_Unit_Type;


typedef struct _IMC_Unit {
    IMC_Unit_Type type;
    Instruction * instructions;
    Instruction * last_ins;
    Symbol * sym;
    int bb_list_size;
    int n_basic_blocks;
    Basic_block **bb_list;
    Set** dominators;
    int n_loops;
    Loop_info ** loop_info;
    Edge * edge_list;
    /* register allocation */
    int n_spilled;
    SymReg * p31;
    SymReg** interference_graph;
    SymReg** reglist;
    int n_symbols;
    struct _IMC_Unit * prev;
    struct _IMC_Unit * next;
} IMC_Unit;


IMC_Unit * imc_open_unit(Parrot_Interp, IMC_Unit_Type);
void imc_close_unit(Parrot_Interp, IMC_Unit *);
IMC_Unit * imc_new_unit(IMC_Unit_Type);
IMC_Unit * imc_order_for_emit(IMC_Unit * list);

#endif

