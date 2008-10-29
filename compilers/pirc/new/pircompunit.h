/*
 * $Id$
 * Copyright (C) 2007-2008, The Perl Foundation.
 */

/*
 * This file contains the interface to the PIR compiler's back-end.
 * It defines all AST nodes that are used to build an abstract
 * syntax tree from the input.
 *
 */

#ifndef PARROT_PIR_PIRCOMPUNIT_H_GUARD
#define PARROT_PIR_PIRCOMPUNIT_H_GUARD

/* the 4 parrot types; use explicit values, they are used to index an array,
 * so this way we can be sure that works properly.
 * (don't assume enum's start counting at 0).
 */
typedef enum pir_types {
    INT_TYPE     = 0,
    NUM_TYPE     = 1,
    STRING_TYPE  = 2,
    PMC_TYPE     = 3,
    UNKNOWN_TYPE = 4  /* for uninitialized types */

} pir_type;



/* selector values for the expression value union */
typedef enum expr_types {
    EXPR_TARGET,
    EXPR_CONSTANT,
    EXPR_IDENT,
    EXPR_KEY

} expr_type;

/* target flags */
typedef enum target_flags {
    TARGET_FLAG_NAMED       = 1 << 0, /* named parameter or variable accepting return value */
    TARGET_FLAG_SLURPY      = 1 << 1, /* slurpy parameter or variable accepting return value */
    TARGET_FLAG_OPTIONAL    = 1 << 2, /* optional parameter */
    TARGET_FLAG_OPT_FLAG    = 1 << 3, /* parameter to check whether the previous (optional)
                                         parameter was passed */
    TARGET_FLAG_UNIQUE_REG  = 1 << 4, /* flag to tell register allocator to leave this poor target
                                         alone! */
    TARGET_FLAG_INVOCANT    = 1 << 5, /* XXX for MDD; to be implemented according to PDD27 */

    /* abuse this enum for these pir compiler internal flags; by adding them here, they can
     * be encoded in the same flag member variable of the target struct, preventing the need
     * for another structure field. As soon as there is a need for 32 target flags, these
     * flags must be encoded into a different flag member.
     */
    TARGET_FLAG_IS_REG      = 1 << 31 /* set if the target node represents a register; cleared if
                                         it's a .local/.param. */

} target_flag;

/* argument flags */
typedef enum arg_flags {
    ARG_FLAG_NAMED = 1 << 0, /* argument is passed by name */
    ARG_FLAG_FLAT  = 1 << 1  /* argument must be flattened */

} arg_flag;

/* instruction flags */
typedef enum instr_flags {
    INSTR_FLAG_BRANCH   = 1 << 0,  /* unconditional branch instruction */
    INSTR_FLAG_IFUNLESS = 1 << 1,  /* conditional branch instruction */
    INSTR_FLAG_ISXX     = 1 << 2   /* isXX variant of conditional branch instruction */

} instr_flag;

/* sub flags */
typedef enum sub_flags {
    SUB_FLAG_METHOD     = 1 << 0,  /* the sub is a method */
    SUB_FLAG_INIT       = 1 << 1,  /* the sub is run before :main when starting up */
    SUB_FLAG_LOAD       = 1 << 2,  /* the sub is run when the bytecode is loaded */
    SUB_FLAG_OUTER      = 1 << 3,  /* the sub is lexically nested */
    SUB_FLAG_MAIN       = 1 << 4,  /* execution of the program will start at this sub */
    SUB_FLAG_ANON       = 1 << 5,  /* this sub is shy and will not be stored in the global
                                      namespace */
    SUB_FLAG_POSTCOMP   = 1 << 6,  /* this sub will be executed after compilation */
    SUB_FLAG_IMMEDIATE  = 1 << 7,  /* similar to POSTCOMP above; check out PDD19 for difference */
    SUB_FLAG_VTABLE     = 1 << 8,  /* this sub overrides a vtable method */
    SUB_FLAG_LEX        = 1 << 9,  /* this sub needs a LexPad */
    SUB_FLAG_MULTI      = 1 << 10, /* this sub is a multi method/sub */
    SUB_FLAG_LEXID      = 1 << 11, /* this sub has a namespace-unaware identifier
                                      XXX this flag needed? XXX */
    SUB_FLAG_INSTANCEOF = 1 << 12  /* this sub has an :instanceof flag. XXX document this XXX */

} sub_flag;


/* types of uses for invocation structure, in some cases
 * the invocation structure contains just a set of return
 * values, no actual function invocation.
 */
typedef enum invoke_types {
    CALL_PCC,               /* a normal invocation */
    CALL_NCI,               /* an NCI invocation   */
    CALL_METHOD,            /* a method invocation */
    CALL_RETURN,            /* returning values    */
    CALL_YIELD,             /* yielding values     */
    CALL_TAILCALL,          /* a tail call         */
    CALL_METHOD_TAILCALL    /* a method tail call  */

} invoke_type;

/* use these macros for manipulating bits; don't do it yourself */
#define SET_FLAG(obj,flag)      obj |= flag
#define CLEAR_FLAG(obj,flag)    obj &= ~flag
#define TEST_FLAG(obj,flag)     (obj & flag)

/* macros to set the i-th bit */
#define BIT(i)          (1 << (i))
#define SET_BIT(M,B)    SET_FLAG(M,B)
#define TEST_BIT(M,B)   TEST_FLAG(M,B)
#define CLEAR_BIT(M,B)  CLEAR_FLAG(M,B)

#define NOT(X)          !(X)

/* selector for the value union */
typedef enum value_types {
    INT_VAL,
    NUM_VAL,
    PMC_VAL,
    STR_VAL,

} value_type;

/* for representing constant values */
typedef union value {
    char    *sval;
    double   nval;
    int      ival;
    char    *pval;

} value;

/* literal constants, possibly named */
typedef struct constant {
    char const      *name;     /* name of the constant, if declared as a constant */
    pir_type         type;     /* type of the constant */
    value            val;      /* value of the constant */
    struct constant *next;

} constant;

#define CONST_INTVAL(c) c->val.ival
#define CONST_NUMVAL(c) c->val.nval
#define CONST_PMCVAL(c) c->val.pval
#define CONST_STRVAL(c) c->val.sval

/* The expression node is used as a wrapper to represent target nodes (like .param, .local
 * and registers), constant nodes (either named or anonymous), XXX identifiers?? XXX,
 * or key nodes, such as ["x";42].
 */
typedef struct expression {
    union __expression_union {
        struct target  *t;
        constant       *c;
        char const     *id;
        struct key     *k;

    } expr;

    expr_type          type;  /* selector for __expression_union */

    struct expression *next;

} expression;

#define EXPR_CONST_INTVAL(e)    CONST_INTVAL(e->expr.c)
#define EXPR_CONST_NUMVAL(e)    CONST_NUMVAL(e->expr.c)

/* The key node is used to represent a key expression */
typedef struct key {
    expression *expr;      /* value of this key */
    struct key *next;      /* in ["x";"y"], there's 2 key nodes; 1 for "x", 1 for "y",
                              linked by "next" */
} key;


/* accessor macros for target name and (PIR) register number */
#define target_name(t)  t->u.name
#define target_regno(t) t->u.regno

/* The target node represents a .local, .param or register that can receive a value,
 * hence the name "target". When receiving arguments, it's a parameter, when receiving
 * return values, it's a local variable (or register).
 */
typedef struct target {

    /* XXX remove this union, change some bits in the parser, and let all target fields
     * that are stored in symbols or pir_regs, point to these fields.
     */
    union target_id { /* target is either a .local/.param or a PIR register; don't store both */
        char const *name;           /* if this is a declared local */
        int         regno;          /* if this is a register */
    } u;

   /* int            color; */         /* for register allocation; -1 means no reg. allocated. */
    target_flag    flags;          /* flags like :slurpy etc. */
    char const    *alias;          /* if this is a named parameter, this is the alias */
    char const    *lex_name;       /* if this is a lexical, this field contains the name */
    struct key    *key;            /* the key of this target, i.e. $P0[$P1], $P1 is key. */


    struct symreg *syminfo;

    struct target *next;

} target;


/* The argument node is used to represent arguments or return values.
 * the value field contains the actual value; furthermore it can have
 * flags, such as :flatten and :named; if :named is set, it can have
 * an alias, when the argument is passed (or returned) as a named
 * parameter (or return value).
 */
typedef struct argument {
    expression      *value; /* the value of this argument */
    int              flags; /* :flat or :named, if specified */
    char const      *alias; /* value of the :named flag */

    struct argument *next;  /* points to the next argument */

} argument;

/* The invocation node represents an invocation statement, or a
 * return statement, but both have the same components; arguments
 * can be passed into a function (parameters) or out of a function,
 * which means they are return values.
 *
 * foo(1,2), (a,b) = foo(1,2), a = foo(1,2)
 *
 * .begin_call/.call PX/.end_call
 *
 * .return foo(), .return foo.bar(), .return x :flat
 */
typedef struct invocation {
    invoke_type         type;
    expression         *method;        /* method */
    target             *sub;           /* invoked sub, or the object on which method is invoked */
    target             *retcc;         /* return continuation, if any */
    target             *results;       /* targets that will receive return values */
    argument           *arguments;     /* values passed into the sub, or return values */

} invocation;

/* the instruction node represents a single parrot instruction */
typedef struct instruction {
    unsigned            offset;       /* sequence number of this instruction */
    char         const *label;        /* label of this instruction */
    char         const *opname;       /* name of the instruction, such as "print" and "set" */
    expression         *operands;     /* operands like "$I0" and "42" in "set $I0, 42" */
    int                 flags;        /* XXX used for checking if this op jumps */
    struct op_info_t   *opinfo;       /* pointer to the op_info containing this op's meta data */
    int                 opcode;       /* the opcode of one of this op */

    struct instruction *next;
} instruction;

/* initial size of a hashtable; note that this is a prime number.
 * XXX how to define next prime number, for when we want to resize the hashtable?
 * maybe implement a simple algorithm; shouldn't happen too often anyway.
 */
#define HASHTABLE_SIZE_INIT     113

/* a hashtable bucket for storing something */
typedef struct bucket {
    union __bucket_union {
        char const          *str;
        struct symbol       *sym;
        struct local_label  *loc;
        struct global_label *glob;
        struct constant     *cons;
    } u;

    struct bucket *next; /* link to next bucket, in case of hash clash */

} bucket;

/* accessors for the bucket union */
#define bucket_string(B)    B->u.str
#define bucket_symbol(B)    B->u.sym
#define bucket_local(B)     B->u.loc
#define bucket_global(B)    B->u.glob
#define bucket_constant(B)  B->u.cons

/* hashtable structure */
typedef struct hashtable {
    bucket   **contents;
    unsigned   size;
    unsigned   obj_count;

} hashtable;

/* forward declaration of structs */
struct symbol;
struct label;

/* a sub */
typedef struct subroutine {
    key                *name_space;    /* this sub's namespace */
    char const         *sub_name;      /* this sub's name */
    char const         *outer_sub;     /* this sub's outer subroutine, if any */
    char const         *lex_id;        /* this sub's lex_id, if any */
    char const         *vtable_method; /* name of vtable method that this sub's overriding if any */
    char const         *instanceof;    /* XXX document this XXX */
    int                 flags;         /* this sub's flags */

    /* XXX the whole multi stuff must be implemented */
    char              **multi_types;   /* data types of parameters if this is a multi sub */

    target             *parameters;    /* parameters of this sub */
    instruction        *statements;    /* statements of this sub */

    hashtable           symbols;
    hashtable           labels;        /* local labels */

    struct pir_reg     *registers[4];  /* used PIR registers in this sub (1 list for each type) */
    unsigned            regs_used[4];  /* number of PASM registers allocated for this sub */

    struct subroutine  *next;          /* pointer to next subroutine in the list */

} subroutine;

/* accessors for current sub and current instruction; makes code a bit more readable */
#define CURRENT_SUB(L)          L->subs
#define CURRENT_INSTRUCTION(L)  L->subs->statements

/* forward declaration */
struct lexer_state;

/* set the namespace that was just parsed */
void set_namespace(struct lexer_state * const lexer, key * const ns);

/* various set functions to set the value of a subroutine flag */
void set_sub_outer(struct lexer_state * const lexer, char const * const outersub);
void set_sub_vtable(struct lexer_state * const lexer, char const * vtablename);
void set_sub_lexid(struct lexer_state * const lexer, char const * const lexid);
void set_sub_instanceof(struct lexer_state * const lexer, char const * const classname);

/* install a new subroutine node */
void new_subr(struct lexer_state * const lexer, char const * const subname);

/* functions for setting argument flags or argument alias */
argument *set_arg_flag(argument * const arg, arg_flag flag);
argument *set_arg_alias(struct lexer_state * const lexer, char const * const alias);

/* constructors for constant nodes */
constant *new_named_const(struct lexer_state * const lexer, pir_type type,
                          char const * const name, ...);

constant *new_const(struct lexer_state * const lexer, pir_type type, ...);

/* conversion functions, each wrapping its argument in an expression node */
expression *expr_from_const(struct lexer_state * const lexer, constant * const c);
expression *expr_from_target(struct lexer_state * const lexer, target * const t);
expression *expr_from_ident(struct lexer_state * const lexer, char const * const name);
expression *expr_from_key(struct lexer_state * const lexer, key * const k);

/* functions for argument node creation and storing */
argument *new_argument(struct lexer_state * const lexer, expression * const expr);
argument *add_arg(argument *arg1, argument * const arg2);

target *add_param(struct lexer_state * const lexer, pir_type type, char const * const name);
target *set_param_alias(struct lexer_state * const lexer, char const * const alias);
target *set_param_flag(struct lexer_state * const lexer, target * const t, target_flag flag);

target *set_curtarget(struct lexer_state * const lexer, target * const t);
argument *set_curarg(struct lexer_state * const lexer, argument * const arg);

/* target constructors */
target *add_target(struct lexer_state * const lexer, target *t1, target * const t);
target *new_reg(struct lexer_state * const lexer, pir_type type, int regno);
target *new_target(struct lexer_state * const lexer, pir_type type, char const * const name);

/* set a key on a target node */
void set_target_key(target * const t, key * const k);

/* functions for creating an invocation node and setting various fields */
invocation *invoke(struct lexer_state * const lexer, invoke_type, ...);
void set_invocation_type(invocation * const inv, invoke_type type);
void set_invocation_args(invocation * const inv, argument * const args);
void set_invocation_results(invocation * const inv, target * const results);

/* conversion functions that wrap their arguments into a target node */
target *target_from_string(struct lexer_state * const lexer, char const * const str);
target *target_from_ident(struct lexer_state * const lexer, pir_type type, char const * const id);
target *target_from_symbol(struct lexer_state * const lexer, struct symbol * const sym);

/* management functions for key nodes */
key *new_key(struct lexer_state * const lexer, expression * const expr);
key *add_key(struct lexer_state * const lexer, key *keylist, expression * const newkey);

void load_library(struct lexer_state * const lexer, char const * const library);
void set_hll(struct lexer_state * const lexer, char const * const hll);
void set_hll_map(struct lexer_state * const lexer, char const * const stdtype,
                 char const * const hlltype);

void set_sub_flag(struct lexer_state * const lexer, sub_flag flag);

/* constructor and functions for setting instruction fields */
void set_label(struct lexer_state * const lexer, char const * const label);
void set_instr(struct lexer_state * const lexer, char const * const opname);
void set_instrf(struct lexer_state * const lxr, char const * const op, char const * const fmt, ...);
void unshift_operand(struct lexer_state * const lexer, expression * const operand);
void push_operand(struct lexer_state * const lexer, expression * const operand);
void add_operands(struct lexer_state * const lexer, char const * const format, ...);

void get_operands(struct lexer_state * const lexer, int bitmask, ...);
expression *get_operand(struct lexer_state * const lexer, short n);
unsigned get_operand_count(struct lexer_state * const lexer);

void update_instr(struct lexer_state * const lexer, char const * const newop);
void update_op(struct lexer_state * const lexer, instruction * const instr, int newop);

void remove_operand(struct lexer_state * const lexer, unsigned index);
void remove_all_operands(struct lexer_state * const lexer);


void set_lex_flag(target * const t, char const * const lexname);
char const *get_inverse(char const * const instr);
void invert_instr(struct lexer_state * const lexer);

/* local declaration functions */
struct symbol *add_local(struct symbol * const list, struct symbol * const local);
struct symbol *new_local(struct lexer_state * const lexer, char const * const name, int unique);

/* compare two target nodes */
int targets_equal(target const * const t1, target const * const t2);

/* funtion to reset the register allocator */
void reset_register_allocator(struct lexer_state * const lexer);

/* to check whether given name is a parrot opcode */
int is_parrot_op(struct lexer_state * const lexer, char const * const name);

void close_sub(struct lexer_state * const lexer);
void fixup_global_labels(struct lexer_state * const lexer);
void set_instr_flag(struct lexer_state * const lexer, instr_flag flag);
void convert_inv_to_instr(struct lexer_state * const lexer, invocation * const inv);

void generate_get_params(struct lexer_state * const lexer);

void panic(struct lexer_state * lexer, char const * const message);

#endif /* PARROT_PIR_PIRCOMPUNIT_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */

