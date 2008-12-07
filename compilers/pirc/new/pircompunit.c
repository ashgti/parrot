/*
 * $Id$
 * Copyright (C) 2007-2008, The Perl Foundation.
 */

/*

=head1 DESCRIPTION

This file contains functions to build the abstract syntax tree from
the PIR input as parsed by the parser implemented in F<pir.y>.

All data types that represent the AST nodes (C<expression>, C<target>,
C<constant>, C<argument>) are circular linked lists, where the C<root>
pointer (that is the pointer through which any list is accessible)
points to the I<last> item. Being circular linked, this means that to
get to the I<first> item on the list, you select C<<node->next>>.

Due to this organization, adding an element to a list can be done in
O(c) (constant) time.

Currently, no Parrot Byte Code is generated; instead, the generated
data structure can be printed, which results in a PASM representation
of the parsed PIR code. Through the symbol management, which is done
in F<pirsymbol.c>, a vanilla register allocator is implemented.



=head1 TODO

=over 4

=item * calculate offsets for global (sub) labels.

=item * fix local label offset calculation: make this work for all :flow (PARROT_JUMP_RELATIVE) ops.

=item * generate PackFiles (PBC).

=back

=cut

*/

#include "pircompunit.h"
#include "pircompiler.h"
#include "pirsymbol.h"
#include "piryy.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "parrot/oplib/ops.h"
#include "parrot/string_funcs.h"
#include "parrot/dynext.h"

#include <assert.h>

static unsigned const prime_numbers[] = {113 /* XXX think of more primes */ };

/*

=head1 FUNCTIONS

=over 4


=item C<void
panic(lexer_state * lexer, char * const message)>

Function to emit a final last cry that something's wrong and exit.

=cut

*/
/*
PARROT_DOES_NOT_RETURN
*/
void
panic(lexer_state * lexer, char const * const message) {
    fprintf(stderr, "Fatal: %s\n", message);
    release_resources(lexer);
    exit(EXIT_FAILURE);
}


/*

=item C<void
reset_register_allocator(lexer_state * const lexer)>

Reset the register numbers for all types. After this
function has been invoked, the next request for a new
(PASM) register will start at register 0 again (for all
types).

=cut

*/
void
reset_register_allocator(lexer_state * const lexer) {
    /* set register allocator to 0 for all register types. */
    lexer->curregister[INT_TYPE]    = 0;
    lexer->curregister[NUM_TYPE]    = 0;
    lexer->curregister[PMC_TYPE]    = 0;
    lexer->curregister[STRING_TYPE] = 0;
}


/*

=item C<void
set_namespace(lexer_state * const lexer, key * const ns)>

Set the current namespace in the lexer state, so that it is
available when needed, i.e. when a new subroutine node is
created.

=cut

*/
void
set_namespace(lexer_state * const lexer, key * const ns) {
    lexer->current_ns = ns;
}

/*

=item C<void
set_sub_outer(lexer_state * const lexer, char * const outersub)>

Set the lexically enclosing sub for the current sub.
Thus, set the :outer() argument to the current subroutine.

=cut

*/
void
set_sub_outer(lexer_state * const lexer, char const * const outersub) {
    CURRENT_SUB(lexer)->outer_sub = outersub;
    SET_FLAG(lexer->subs->flags, SUB_FLAG_OUTER);
}


/*

=item C<void
set_sub_vtable(lexer_state * const lexer, char const *vtablename)>

Set the :vtable() flag argument to the current subroutine. If C<vtablename>
is NULL, the name of the current sub is taken to be the vtable method name.
If the vtable method name (either specified or the current sub's name) is
in fact not a vtable method, an error message is emitted.

=cut

*/
void
set_sub_vtable(lexer_state * const lexer, char const * vtablename) {
    int vtable_index;

    if (vtablename == NULL)  /* the sub's name I<is> the vtablename */
        vtablename = CURRENT_SUB(lexer)->sub_name;

    /* get the index number of this vtable method */
    vtable_index = Parrot_get_vtable_index(lexer->interp,
                                           string_from_cstring(lexer->interp, vtablename,
                                                               strlen(vtablename)));

    /* now check whether the method name actually a vtable method */
    if (vtable_index == -1)
        yypirerror(lexer->yyscanner, lexer,
                   "'%s' is not a vtable method but was used with :vtable flag", vtablename);
    else {
        CURRENT_SUB(lexer)->vtable_index = vtable_index;
        SET_FLAG(lexer->subs->flags, SUB_FLAG_VTABLE);
    }
}

/*

=item C<void
set_sub_subid(lexer_state * const lexer, char * const subid)>

Set the name specified in the :subid flag on the sub.

=cut

*/
void
set_sub_subid(lexer_state * const lexer, char const * const subid) {
    CURRENT_SUB(lexer)->subid = subid;
    SET_FLAG(lexer->subs->flags, SUB_FLAG_SUBID);
}

/*

=item C<void
set_sub_methodname(lexer_state * const lexer, char * const methodname)>

Set the :method flag on a sub; if C<methodname> is not NULL, then it contains
the name by which the sub is stored as a method.

=cut

*/
void
set_sub_methodname(lexer_state * const lexer, char const * const methodname) {
    if (methodname) /* :method("foo") */
        CURRENT_SUB(lexer)->methodname = methodname;
    else /* :method without a value defaults to the subname. */
        CURRENT_SUB(lexer)->methodname = CURRENT_SUB(lexer)->sub_name;

    SET_FLAG(lexer->subs->flags, SUB_FLAG_METHOD);
}

/*

=item C<void
set_sub_instanceof(lexer_state * const lexer, char * const classname)>

Set the value of the C<:instanceof> flag on a sub. Note that this flag
is experimental, and not actually used at this point.

=cut

*/
void
set_sub_instanceof(lexer_state * const lexer, char const * const classname) {
    CURRENT_SUB(lexer)->instanceof = classname;
}

/*

=item C<void
set_sub_nsentry(lexer_state * const lexer, char const * const nsentry)>

Set the value of the C<:nsentry> flag on a sub. The value of C<nsentry> is the name
by which the sub is stored in the namespace.

=cut

*/
void
set_sub_nsentry(lexer_state * const lexer, char const * const nsentry) {
    CURRENT_SUB(lexer)->nsentry = nsentry;
}

/*

=item C<void
set_sub_flag(lexer_state * const lexer, sub_flag flag)>

Set a subroutine flag on the current sub.

=cut

*/
void
set_sub_flag(lexer_state * const lexer, sub_flag flag) {
    /* set the specified flag in the current subroutine */
    SET_FLAG(CURRENT_SUB(lexer)->flags, flag);

    /* if the sub is a method or a :vtable method, then also add a "self" parameter */
    if (TEST_FLAG(flag, (SUB_FLAG_VTABLE | SUB_FLAG_METHOD)))
        add_param(lexer, PMC_TYPE, "self");
}

/*

=item C<void
new_subr(lexer_state * const lexer, char * const subname)>

Create a new subroutine node, and set it as the "current"
subroutine, on which all other sub-related operations do
their thing.

=cut

*/
void
new_subr(lexer_state * const lexer, char const * const subname) {
    subroutine *newsub  = pir_mem_allocate_zeroed_typed(lexer, subroutine);
    int index;

    /* set the sub fields */
    newsub->sub_name    = subname;

    /* set default lexid */
    newsub->subid       = subname;

    /* take namespace of this sub of the lexer, which keeps track of that */
    newsub->name_space  = lexer->current_ns;

    newsub->parameters  = NULL;
    newsub->statements  = NULL;
    newsub->flags       = 0;
    newsub->codesize    = 0;
    newsub->startoffset = lexer->codesize; /* start offset in bytecode */

    init_hashtable(lexer, &newsub->symbols, HASHTABLE_SIZE_INIT);
    init_hashtable(lexer, &newsub->labels, HASHTABLE_SIZE_INIT);

    for (index = 0; index < 4; index++) { /* 4 is the number of Parrot types. */
        newsub->registers[index] = NULL; /* set all "register" tables to NULL */
        newsub->regs_used[index] = 0;    /* set all register counts to 0 */
    }

    /* link the new sub node into the list of subroutines */
    if (CURRENT_SUB(lexer) == NULL) { /* no subroutine yet, this is the first one */
        newsub->next = newsub; /* set next field to itself, making the list circular linked */
    }
    else { /* there is at least 1 other subroutine */
        /* lexer->subs points to "end of list", to the last added one */
        newsub->next             = CURRENT_SUB(lexer)->next; /* set newsub's next to the
                                                                first item in the list */
        CURRENT_SUB(lexer)->next = newsub;    /* set current sub's next to the new sub. */
    }
    CURRENT_SUB(lexer) = newsub;

    /* store the subroutine identifier as a global label */
    store_global_label(lexer, subname);

    /* vanilla register allocator is reset for each sub */
    reset_register_allocator(lexer);


}

/*

=item C<void
set_sub_name(struct lexer_state * const lexer, char const * const subname)>

Set the current subroutine's name to C<subname>.

=cut

*/
void
set_sub_name(struct lexer_state * const lexer, char const * const subname) {
    CURRENT_SUB(lexer)->sub_name = subname;
}


/*

=item C<static instruction *
new_instruction(char * const opname)>

Create a new instruction node and set C<opname> as the instruction.

=cut

*/
static instruction *
new_instruction(lexer_state * const lexer, char const * const opname) {
    instruction *ins = pir_mem_allocate_zeroed_typed(lexer, instruction);
    ins->opname      = opname;
    ins->opcode      = -1; /* make sure this field is properly initialized;
                              it must be >= 0 before being used */
    return ins;
}


/*

=item C<static void
new_statement(lexer_state * const lexer)>

Constructor for a statement. The newly allocated statement will be inserted
into the current subroutine's statements list.

=cut

*/
static void
new_statement(lexer_state * const lexer, char const * const opname) {
    instruction *instr = new_instruction(lexer, opname);

    /* Each instruction has a sequence number to be able to
     * calculate offsets for label branches.
     */

    /* the codesize so far will be the offset of this instruction. */
    instr->offset = lexer->codesize;

    /*
    fprintf(stderr, "offset of %s is: %d\n", opname, instr->offset);
    */

    if (CURRENT_INSTRUCTION(lexer) == NULL)
        instr->next = instr;
    else {
        instr->next = CURRENT_INSTRUCTION(lexer)->next;
        CURRENT_INSTRUCTION(lexer)->next = instr;
    }
    CURRENT_INSTRUCTION(lexer) = instr;
}

/*

=item C<void
set_curtarget(lexer_state * const lexer, target * const t)>

Sets the target C<t> as the current target in C<lexer> to
make it accessible to other parse actions. C<t> is returned.

=cut

*/
PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
target *
set_curtarget(lexer_state * const lexer, target * const t) {
    lexer->curtarget = t;
    return t;
}

/*

=item C<argument *
set_curarg(lexer_state * const lexer, argument *arg)>

Sets the argument C<arg> as the current argument in C<lexer>
to make it accessible to other parse actions. C<arg> is returned.

=cut

*/
PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
argument *
set_curarg(lexer_state * const lexer, argument * const arg) {
    lexer->curarg = arg;
    return arg;
}

/*

=item C<int
targets_equal(target const * const left, target const * const right)>

Returns true if C<left> equals C<right>, false otherwise. C<left> is
considered to be equal to C<right> if any of the following conditions
is true:

=over 4

=item * C<left> has a name, C<right> has a name, and these names are equal;

=item * C<left>'s type equals C<right>'s type, I<and> C<left>'s (PIR) register number
equals C<right>'s (PIR) register number, I<and> C<left>'s (PASM) register number
equals C<right>'s (PASM) register number.

=back

=cut

*/
PARROT_PURE_FUNCTION
PARROT_WARN_UNUSED_RESULT
int
targets_equal(target const * const left, target const * const right) {

    if (TEST_FLAG(left->flags, TARGET_FLAG_IS_REG)) {      /* if left is a reg */
        if (TEST_FLAG(right->flags, TARGET_FLAG_IS_REG)) { /* then right must be a reg */
            if ((left->s.reg->type == right->s.reg->type)        /* types must match */
            &&  (left->s.reg->regno == right->s.reg->regno           /* PIR regno must match */
            &&  (left->s.reg->color == right->s.reg->color)))    /* PASM regno must match */
                return TRUE;
        }
        else /* left is a reg, right is not */
            return FALSE;

    }
    else { /* left is not a reg */

        if (!TEST_FLAG(right->flags, TARGET_FLAG_IS_REG)  /* right must not be a reg */
        && (left->s.sym->name && right->s.sym->name       /* both must have a name */
        && STREQ(left->s.sym->name, right->s.sym->name))) /* and they must be equal */
            return TRUE;
    }

    return FALSE;
}

/*

=item C<target *
new_target(lexer_state * const lexer)>

Create a new target node. The node's next pointer is initialized to itself.

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
target *
new_target(lexer_state * const lexer) {
    target *t       = pir_mem_allocate_zeroed_typed(lexer, target);
    t->key          = NULL;
    t->next         = t; /* circly linked list */
    return t;
}

/*
=item C<void
set_target_key(target * const t, key * const k)>

Set the key C<k> on target C<t>. For instance:

 $P0[$P1]

[$P1] is the key of $P0.

=cut

*/
void
set_target_key(target * const t, key * const k) {
    t->key = k;
}

/*

=item C<target *
target_from_symbol(lexer_state * const lexer, symbol * const sym)>

Convert symbol C<sym> into a target node. The resulting target has
a pointer to C<sym>.

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
target *
target_from_symbol(lexer_state * const lexer, symbol * const sym) {
    target *t  = new_target(lexer);
    t->s.sym   = sym; /* set a pointer from target to symbol */
    t->flags   = sym->flags; /* copy the flags */

    return t;
}

/*

=item C<target *
add_target(lexer_state * const lexer, target *last, target * const t)>

Add a new target to the list pointed to by C<list>. C<list> points to
the last element, C<<last->next>> points to the first. The list is
circular linked. The newly added target C<t> is returned.

=cut

*/
PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
target *
add_target(lexer_state * const lexer, target *last, target * const t) {
    PARROT_ASSERT(last);
    PARROT_ASSERT(t);

    t->next    = last->next; /* points to first */
    last->next = t;
    last       = t;

    return t;
}

/*

=item C<target *
add_param(lexer_state * const lexer, pir_type type, char * const name)>

Add a parameter of type C<type> and named C<name> to the current
subroutine. The parameter will be declared as a local symbol in the
current subroutine, and a new register is allocated for it.

=cut

*/
PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
target *
add_param(lexer_state * const lexer, pir_type type, char const * const name) {
    target *targ = new_target(lexer);
    symbol *sym  = new_symbol(lexer, name, type);

    PARROT_ASSERT(CURRENT_SUB(lexer));

    if (CURRENT_SUB(lexer)->parameters == NULL) {
        CURRENT_SUB(lexer)->parameters = targ;
        targ->next = targ;
    }
    else {
        targ->next = CURRENT_SUB(lexer)->parameters->next;
        CURRENT_SUB(lexer)->parameters->next = targ;
        CURRENT_SUB(lexer)->parameters       = targ;
    }

    /* set the parameter just added as curtarget */
    lexer->curtarget = targ;

    /* parameters are just special .locals; enter them into the symbol table */
    declare_local(lexer, type, sym);
    /* parameters must always get a PASM register, even if they're not
     * "used"; in the generated PASM instructions, they're always used
     * (to store the incoming values).
     */
    assign_vanilla_register(lexer, sym);

    /* set a pointer from the target to the symbol object */
    targ->s.sym = sym;

    return targ;
}

/*

=item C<void
set_param_alias(lexer_state * const lexer, char * const alias)>

Set the argument of the :named flag for the current target
(parameter). Returns the current target (parameter).

=cut

*/
PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
target *
set_param_alias(lexer_state * const lexer, char const * const alias) {
    PARROT_ASSERT(lexer->curtarget != NULL);
    lexer->curtarget->alias = alias;
    SET_FLAG(lexer->curtarget->flags, TARGET_FLAG_NAMED);
    return lexer->curtarget;
}




/*

=item C<void
set_param_flag(lexer_state * const lexer, target * const param, target_flag flag)>

Set the flag C<flag> on parameter C<param>. The actual value
of C<flag> may encode several flags at a time. Returns C<param>.

=cut

*/
PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
target *
set_param_flag(lexer_state * const lexer, target * const param, target_flag flag) {
    SET_FLAG(param->flags, flag);

    /* note that param is always an identifier; registers are not allowed as parameters.
     * Therefore it's safe to reference param->s.sym, without checking for not
     * being a register.
     */

    /* :slurpy can only be set on a PMC parameter */
    if (TEST_FLAG(flag, TARGET_FLAG_SLURPY) && param->s.sym->type != PMC_TYPE)
        yypirerror(lexer->yyscanner, lexer,
                   "cannot set :slurpy flag on non-pmc %s", param->s.sym->name);

    /* :opt_flag can only be set on a int parameter */
    if (TEST_FLAG(flag, TARGET_FLAG_OPT_FLAG) && param->s.sym->type != INT_TYPE)
        yypirerror(lexer->yyscanner, lexer,
                   "cannot set :opt_flag flag on non-int %s", param->s.sym->name);

    return param;
}

/*

=item C<argument *
new_argument(expression * const expr)>

Create a new argument node which wraps C<expr>.

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
argument *
new_argument(lexer_state * const lexer, expression * const expr) {
    argument *arg = pir_mem_allocate_zeroed_typed(lexer, argument);
    arg->value    = expr;
    arg->next     = arg;
    return arg;
}



/*

=item C<argument *
add_arg(argument *arg1, argument * const arg2)>

Add argument C<arg2> at the end of the list pointed to by C<arg1>.
The list is circular linked, and C<arg1> points to the last item.
Being circular linked, C<<arg1->next>> points to the first item.

After adding the element, the I<root> pointer (that points to the last
element of the list) is updated, and returned.

=cut

*/
PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
argument *
add_arg(argument *arg1, argument * const arg2) {
    PARROT_ASSERT(arg1);
    PARROT_ASSERT(arg2);

    arg2->next = arg1->next; /* arg2->next is last one, must point to first item */
    arg1->next = arg2;
    arg1       = arg2;

    return arg1;
}

/*

=item C<void
set_arg_flag(argument * const arg, arg_flag flag)>

Set the flag C<flag> on argument C<arg>. Note the C<flag> may
encode multiple flags. C<arg> is returned.

=cut

*/
PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
argument *
set_arg_flag(argument * const arg, arg_flag flag) {
    SET_FLAG(arg->flags, flag);
    return arg;
}

/*

=item C<void
set_arg_alias(lexer_state * const lexer, char * const alias)>

Set the alias specified in C<alias> on the current argument, accessible
through C<lexer>. The alias is the name under which the argument is passed
(as a named argument, i.e., the value of the C<:named> flag).
The argument on which the alias is set is returned.

=cut

*/
PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
argument *
set_arg_alias(lexer_state * const lexer, char const * const alias) {
    PARROT_ASSERT(lexer->curarg != NULL);
    lexer->curarg->alias = alias;
    SET_FLAG(lexer->curarg->flags, ARG_FLAG_NAMED);
    return lexer->curarg;
}

/*

=item C<void
load_library(lexer_state * const lexer, char * const library)>

Load the library indicated by C<library>.

=cut

*/
void
load_library(lexer_state * const lexer, char const * const library) {
    /* see imcc.y:600 */
    STRING *libname       = string_from_cstring(lexer->interp, library, strlen(library));
    PMC    *ignored_value = Parrot_load_lib(lexer->interp, libname, NULL);
    UNUSED(ignored_value);
    Parrot_register_HLL_lib(lexer->interp, libname);
}


/*

=item C<void
set_label(lexer_state * const lexer, char * const label)>

Set the label C<label> on the current instruction.

=cut

*/
void
set_label(lexer_state * const lexer, char const * const labelname) {
    instruction *instr = CURRENT_INSTRUCTION(lexer);

    PARROT_ASSERT(instr);

    instr->label = labelname;

    /* if there is no instruction, then the label has the same index as the first
     * instruction to come. Like so:
     *
     * L1:
     * L2:
     * L3: print "hi"
     *     ...
     *     goto L1
     *     goto L2
     *     goto L3
     *
     * jumping to L1 is equivalent to jumping to L2 or L3; so when calculating
     * branch offsets, all three labels must yield the same offset.

    /* store the labelname and its offset */
    store_local_label(lexer, labelname, instr->offset);
}


/*

=item C<void
set_instr(lexer_state * const lexer, char const * const opname)>

Sets C<opname> to the current instruction, without operands. This is a
wrapper function for C<set_instrf> to prevent calls with an empty format
string.

=cut

*/
void
set_instr(lexer_state * const lexer, char const * const opname) {
    set_instrf(lexer, opname, "");
}

/*

=item C<void
update_instr(lexer_state * const lexer, char * const newop)>

Update the current instruction; the new opname is given by C<newop>.

=cut

*/
void
update_instr(lexer_state * const lexer, char const * const newop) {
    PARROT_ASSERT(CURRENT_INSTRUCTION(lexer));
    CURRENT_INSTRUCTION(lexer)->opname = newop;
}

/* This does not work as a function, so define this as a macro; this way, only need to maintain
 * it in one spot, and use it where ever you want.
 *
 * [It's not clear why this doesn't work in a separate function, but I suspect it has to do with
 * the for loop where it's used. --kjs]
 */
#define get_instr_var_arg(lexer, format, arg_ptr)  do {                                            \
    switch (format) {                                                                              \
        case 'I': /* identifier */                                                                 \
            expr = expr_from_ident(lexer, va_arg(arg_ptr, char *));                                \
            break;                                                                                 \
        case 'T': /* target */                                                                     \
            expr = expr_from_target(lexer, va_arg(arg_ptr, target *));                             \
            break;                                                                                 \
        case 'E': /* expression */                                                                 \
            expr = va_arg(arg_ptr, expression *);                                                  \
            break;                                                                                 \
        case 'C': /* constant */                                                                   \
            expr = expr_from_const(lexer, va_arg(arg_ptr, constant *));                            \
            break;                                                                                 \
        case 'i': /* integer */                                                                    \
            expr = expr_from_const(lexer, new_const(lexer, INT_TYPE, va_arg(arg_ptr, int)));       \
            break;                                                                                 \
        case 'n': /* number */                                                                     \
            expr = expr_from_const(lexer, new_const(lexer, NUM_TYPE, va_arg(arg_ptr, double)));    \
            break;                                                                                 \
        case 's': /* string */                                                                     \
            expr = expr_from_const(lexer, new_const(lexer, STRING_TYPE, va_arg(arg_ptr, char *))); \
            break;                                                                                 \
        default:                                                                                   \
            panic(lexer, "unknown format specifier in set_instrf()");                              \
            break;                                                                                 \
    }                                                                                              \
} while (0)


/*

=item C<void
set_instrf(lexer_state *lexer, char *opname, char const * const format, ...)>

Set the specified instruction, using the operands from the vararg list. The
number and types of operands is specified by C<format>. The type of the
operands is specified by a '%' character followed by a character, which
can be one of the following:

=over 4

=item I - an identifier, passed as a char pointer

=item T - a target node

=item E - an expression node

=item C - a constant node

=item i - an integer constant

=item n - a number constant

=item s - a string constant, passed as a char pointer

=back

An example is:

 set_instrf(lexer, "set", "%T%i", $1, $2);

This sets the instruction named C<set>, which takes two operands:
a target node (T) and an integer constant (i). C<$1> and C<$2>
refer to the (non-)terminals used in a Yacc/Bison specification.

=cut

*/
void
set_instrf(lexer_state * const lexer, char const * const opname, char const * const format, ...) {
    va_list  arg_ptr;       /* for the var. args */
    unsigned i;             /* loop iterator */
    size_t   format_length; /* length of the format string. */

    PARROT_ASSERT(format);
    format_length = strlen(format);
    PARROT_ASSERT(format_length % 2 == 0);


    /* create a new instruction node */
    new_statement(lexer, opname);

    /* retrieve the operands */
    va_start(arg_ptr, format);

    for (i = 0; i < format_length; i++) {
        expression *expr = NULL;
        /* make sure the format letter is preceded by a '%' */
        PARROT_ASSERT(*(format + i) == '%');
        ++i;

        get_instr_var_arg(lexer, *(format + i), arg_ptr);

        push_operand(lexer, expr);
    }
    va_end(arg_ptr);
}

/*

=item C<void
add_operands(lexer_state * const lexer, char const * const format, ...)>

Add operands to the current instruction. This is a variable argument function;
C<format> contains placeholders, see the macro C<get_instr_var_arg> above
for which placeholders. The number of placeholders should match the number
of operands passed to this function.

=cut

*/
void
add_operands(lexer_state * const lexer, char const * const format, ...) {
    va_list  arg_ptr;       /* for the var. args */
    unsigned i;             /* loop iterator */
    size_t   format_length; /* length of the format string. */

    PARROT_ASSERT(format);
    format_length = strlen(format);
    PARROT_ASSERT(format_length % 2 == 0);

    /* retrieve the operands */
    va_start(arg_ptr, format);

    for (i = 0; i < format_length; i++) {
        expression *expr = NULL;
        /* make sure the format letter is preceded by a '%' */
        PARROT_ASSERT(*(format + i) == '%');
        ++i;

        get_instr_var_arg(lexer, *(format + i), arg_ptr);

        push_operand(lexer, expr);
    }
    va_end(arg_ptr);
}


/*

=item C<void
set_instr_flag(lexer_state * const lexer, instr_flag flag)>

Set the flag C<flag> on the current instruction. Note that C<flag> may
encode different flags.

=cut

*/
void
set_op_labelflag(lexer_state * const lexer, int flag) {
    PARROT_ASSERT(CURRENT_INSTRUCTION(lexer));
    SET_FLAG(CURRENT_INSTRUCTION(lexer)->oplabelbits, flag);
}

/*

=item C<char *
get_inverse(char *instr)>

Returns the instruction with inversed semantics; for instance
C<if> becomes C<unless>, C<greater-than> becomes C<less-or-equals>.

=cut

*/
PARROT_CONST_FUNCTION
char const *
get_inverse(char const * const instr) {
         if (STREQ(instr, "if")) return "unless";
    else if (STREQ(instr, "gt")) return "le";
    else if (STREQ(instr, "ge")) return "lt";
    else if (STREQ(instr, "le")) return "gt";
    else if (STREQ(instr, "lt")) return "ge";
    else if (STREQ(instr, "ne")) return "eq";
    else if (STREQ(instr, "eq")) return "ne";

    PARROT_ASSERT(0); /* this should never happen */
    return NULL;
}

/*

=item C<void
invert_instr(lexer_state *lexer)>

Invert the current instruction. This function assumes there is an instruction
already in place.

=cut

*/
void
invert_instr(lexer_state * const lexer) {
    instruction * const  ins   = CURRENT_INSTRUCTION(lexer);
    char          const *instr;
    PARROT_ASSERT(ins);

    instr = ins->opname;
    instr = get_inverse(instr);
    /* and set the new instruction */
    ins->opname = instr;
}


/*

=item C<expression *
get_operand(lexer_state * const lexer, short n)>

Get the C<n>th operand from the current instruction. If there are no
operands, NULL is returned. Because the operands are stored in a
circular linked list, it is impossible to "run out"; for instance, if
there are 4 operands, and you request the 5th, then it will return
5 % 4 = the first operand.

Counting operands is done from 1 (not 0).

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
expression *
get_operand(lexer_state * const lexer, short n) {
    expression *operand = CURRENT_INSTRUCTION(lexer)->operands;

    if (operand == NULL)
        return NULL;


    /* initialize the iterator */
    operand = operand->next;

    PARROT_ASSERT(n >= 0);

    /* go to the nth operand and return that one. */
    while (--n > 0) {
        operand = operand->next;

    }

    PARROT_ASSERT(operand);

    return operand;
}


/*

=item C<void
get_operands(lexer_state * const lexer, int bitmask, ...)>

Get operands from the current instruction. C<bitmask> indicates which operands
are requested; if the C<i>th bit is set, the C<i>th operand is returned in a
vararg.

There must be as many varargs, as there are bits set in C<bitmask>.
This function assumes that the current instruction's C<opinfo> field is valid.

=cut

*/
void
get_operands(lexer_state * const lexer, int bitmask, ...) {
    instruction *instr   = CURRENT_INSTRUCTION(lexer);
    expression  *iter    = instr->operands;
    int          numargs;
    int          i;
    va_list      arg_ptr;

    PARROT_ASSERT(iter);
    iter = iter->next;

    /*
    PARROT_ASSERT(instr->opinfo);

    numargs = instr->opinfo->op_count;

    */
    va_start(arg_ptr, bitmask);

    /*
    for (i = 0; i < BIT(numargs - 1); i++) {
        */
    for (i = 0; i < 8; i++) {
        /* if this argument was requested, store it in the vararg OUT parameters. */
        if (TEST_BIT(bitmask, BIT(i))) {

            expression **arg = va_arg(arg_ptr, expression **);
            *arg = iter;
        }

        iter = iter->next;
    }

    va_end(arg_ptr);
}

/*

=item C<int
get_operand_count(lexer_state * const lexer)>

Returns the number of operands of the I<current> instruction.

=cut

*/
PARROT_WARN_UNUSED_RESULT
unsigned
get_operand_count(lexer_state * const lexer) {
    unsigned count = 0;
    expression *first, *operand;

    /* if no operands, return 0 */
    if (CURRENT_INSTRUCTION(lexer)->operands == NULL)
        return 0;

    /* initialize the first and the iterator */
    first = operand = CURRENT_INSTRUCTION(lexer)->operands->next;

    /* count the number of operands */
    do {
        ++count;
        operand = operand->next;
    }
    while (operand != first);

    return count;
}

/*

=item C<static constant *
create_const(lexer_state * const lexer, pir_type type, char * const name, va_list arg_ptr)>

Constant constructor; based on C<type>, retrieve a value of the
appropriate type from C<arg_ptr>.

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static constant *
create_const(lexer_state * const lexer, pir_type type, char const * const name, va_list arg_ptr) {
    constant *c = pir_mem_allocate_zeroed_typed(lexer, constant);
    c->name     = name;
    c->type     = type;
    c->next     = NULL;

    switch (type) {
        case INT_TYPE:
            c->val.ival = va_arg(arg_ptr, int);
            break;
        case NUM_TYPE:
            c->val.nval = va_arg(arg_ptr, double);
            break;
        case PMC_TYPE:
        case STRING_TYPE:
            c->val.sval = va_arg(arg_ptr, char *);
            break;
        case UNKNOWN_TYPE:
        default:
            panic(lexer, "unknown data type in create_const()");
            break;
    }

    return c;
}

/*


=item C<constant *
new_named_const(lexer_state * const lexer, pir_type type, char * const name, ...)>

Creates a new constant node of the given type, by the given name.
Wrapper function for C<create_const>.

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
constant *
new_named_const(lexer_state * const lexer, pir_type type, char const * const name, ...) {
    constant *c;
    va_list arg_ptr;
    va_start(arg_ptr, name);
    c = create_const(lexer, type, name, arg_ptr);
    va_end(arg_ptr);
    return c;
}

/*

=item C<constant *
new_pmc_const(char const * const type, char const * const name, constant * const value)>

Create a new PMC constant of type C<type>, name C<name> and having a value C<value>.
The type must be a string indicating a valid type name (e.g. "Sub"). The name will be the name
of the constant, and the value of the constant is passed as C<value>.

XXX if type is "Sub", value must be looked up, as it is the name of a subroutine.

=cut

*/
constant *
new_pmc_const(char const * const type, char const * const name, constant * const value) {
    value->name = name;
    /* XXX implement this. Not sure yet how to. */
    return value;
}

/*

=item C<constant *
new_const(lexer_state * const lexer, pir_type type, ...)>

Creates a new constant node of the given type.
Wrapper function for C<create_const>

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
constant *
new_const(lexer_state * const lexer, pir_type type, ...) {
    constant *c;
    va_list arg_ptr;
    va_start(arg_ptr, type);
    c = create_const(lexer, type, NULL, arg_ptr);
    va_end(arg_ptr);
    return c;
}

/*

=item C<static invocation *
new_invocation(lexer_state * const lexer)>

Returns a pointer to a new invocation object. In the current implementation,
there can only be one invocation object at any time. For that reason, the
lexer structure has a cache, containing such an invocation object. This way,
it can be reused over and over again, preventing the need to allocate new
invocation objects.

This function clears the invocation object, and returns a pointer to it.

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static invocation *
new_invocation(lexer_state * const lexer) {
    /*
    invocation *inv = (invocation *)malloc(sizeof (invocation));
    PARROT_ASSERT(inv);
    */
    /* optimization: return the address of the cached object */
    invocation *inv = &lexer->obj_cache.inv_cache;
    /* clear all fields */
    memset(inv, 0, sizeof (invocation));

    return inv;
}



/*

=item C<static expression *
new_expr(expr_type type)>

Create a new C<expression> node of the specified C<type>. The new
expression node is returned.

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static expression *
new_expr(lexer_state * const lexer, expr_type type) {
    expression *expr = pir_mem_allocate_zeroed_typed(lexer, expression);
    expr->type       = type;
    expr->next       = expr;
    return expr;
}

/*

=item C<target *
new_reg(lexer_state * const lexer, int type, int regno)>

Create a C<target> node from a register. Returns the newly created register.

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
target *
new_reg(lexer_state * const lexer, pir_type type, int regno) {
    target *t       = new_target(lexer);
    pir_reg *reg;

    color_reg(lexer, type, regno);
    reg = find_register(lexer, type, regno);
    t->s.reg = reg;

    /* set a flag on this target node saying it's a register */
    SET_FLAG(t->flags, TARGET_FLAG_IS_REG);
    return t;
}

/*

=item C<expression *
expr_from_target(target *t)>

convert a target to an expression node

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
expression *
expr_from_target(lexer_state * const lexer, target * const t) {
    expression *e = new_expr(lexer, EXPR_TARGET);
    e->expr.t     = t;
    return e;
}

/*

=item C<expression *
expr_from_const(constant * const c)>

Convert the constant C<c> to an expression node and returns the newly
created expression node.

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
expression *
expr_from_const(lexer_state * const lexer, constant * const c) {
    expression *e = new_expr(lexer, EXPR_CONSTANT);
    e->expr.c     = c;
    return e;
}

/*

=item C<expression *
expr_from_ident(char * const id)>

Convert a ident to an expression node and returns it.

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
expression *
expr_from_ident(lexer_state * const lexer, char const * const id) {
    expression *e = new_expr(lexer, EXPR_IDENT);
    e->expr.id    = id;
    return e;
}

/*

=item C<void
set_invocation_args(invocation * const inv, argument * const args)>

Set the args of an invocation onto the current invocation object.

=cut

*/
void
set_invocation_args(invocation * const inv, argument * const args) {
    inv->arguments = args;
}

/*

=item C<void
set_invocation_results(invocation * const inv, target * const results)>

Set the invocation results on the invocation object C<inv>.

=cut

*/
void
set_invocation_results(invocation * const inv, target * const results) {
    inv->results = results;
}

/*

=item C<void
set_invocation_type(invocation * const inv, invoke_type type)>

Set the invocation type on the invocation object C<inv>.

=cut

*/
void
set_invocation_type(invocation * const inv, invoke_type type) {
    inv->type = type;
}

/*

=item C<invocation *
invoke(lexer_state * const lexer, invoke_type type, ...)>

Create a new C<invocation> object of type C<type>. This can be one
of the C<invoke_types> enumeration. Based on the type, this function
expects a number of extra arguments to initialize some fields in
the newly created C<invocation> object.

The new invocation object is returned.

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
invocation *
invoke(lexer_state * const lexer, invoke_type type, ...) {
    va_list arg_ptr;

    invocation *inv = new_invocation(lexer);
    inv->type       = type;

    va_start(arg_ptr, type);
    switch (type) {
        case CALL_PCC:  /* .call $P0, $P1 */
            inv->sub   = va_arg(arg_ptr, target *);
            inv->retcc = va_arg(arg_ptr, target *);
            break;
        case CALL_NCI:  /* .nci_call $P0 */
            inv->sub   = va_arg(arg_ptr, target *);
            break;
        case CALL_METHOD: /* $P0.$P1() */
            inv->sub    = va_arg(arg_ptr, target *);
            inv->method = va_arg(arg_ptr, expression *);
            break;
        case CALL_RETURN:   /* no extra args */
        case CALL_YIELD: /* no extra args */
        case CALL_TAILCALL:  /* no extra args */
        case CALL_METHOD_TAILCALL:
            break;
        default:
            panic(lexer, "unknown invoke_type");
    }
    va_end(arg_ptr);

    return inv;
}




/*

=item C<void
set_lex_flag(target *t, char * const name)>

Set the lexical name C<name> on target C<t>.

=cut

*/
void
set_lex_flag(target * const t, char const * const name) {
    t->lex_name = name;
}

/*

=item C<void
set_hll(lexer_state * const lexer, char * const hll)>

Set the current HLL as specified in C<hll>.

Code taken from imcc.y; needs testing.

=cut

*/
void
set_hll(lexer_state * const lexer, char const * const hll) {
    STRING * const hll_name             = string_from_cstring(lexer->interp, hll, strlen(hll));
    CONTEXT(lexer->interp)->current_HLL = Parrot_register_HLL(lexer->interp, hll_name);
}


/*

=item C<void
set_hll_map(lexer_state * const lexer, char * const stdtype, char * const maptype)>

Set a HLL PMC data type mapping; whenever Parrot needs to create a PMC object of
a type that is mapped to a user type, user type will be instantiated. For instance,
after the mapping:

 .HLL_map "Integer" = "Perl6Int"

whenever Parrot needs to create a Integer PMC, a Perl6Int is created instead.

Code taken from imcc.y; needs testing.

=cut

*/
void
set_hll_map(lexer_state * const lexer, char const * const stdtype, char const * const maptype) {
    Parrot_Context *ctx           = CONTEXT(lexer->interp);
    STRING * const  built_in_name = string_from_cstring(lexer->interp, stdtype, strlen(stdtype));
    STRING * const  language_name = string_from_cstring(lexer->interp, maptype, strlen(maptype));
    int             built_in_type = pmc_type(lexer->interp, built_in_name);
    int             language_type = pmc_type(lexer->interp, language_name);

    /* check if both the built-in and language types exist. */

    if (built_in_type <= 0)
        yypirerror(lexer->yyscanner, lexer, "type '%s' is not a built-in type", stdtype);

    if (language_type <= 0)
        yypirerror(lexer->yyscanner, lexer, "user type '%s' cannot be found", maptype);

    Parrot_register_HLL_type(lexer->interp, ctx->current_HLL, built_in_type, language_type);
}


/*

=item C<void
unshift_operand(lexer_state *lexer, expression *operand)>

Add the specified expression as the first operand of the current
instruction.

=cut

*/
void
unshift_operand(lexer_state * const lexer, expression * const operand) {
    expression *last = CURRENT_INSTRUCTION(lexer)->operands;
    if (last) {
        /* get the head of the list */
        expression *first = last->next;
        /* squeeze operand in between */
        operand->next     = first;
        last->next        = operand;
    }
    else {
        CURRENT_INSTRUCTION(lexer)->operands = operand;
    }
}

/*

=item C<void
push_operand(lexer_state * const lexer, expression * const operand)>

Add an operand at the end of the list of operands of the current instruction.

=cut

*/
void
push_operand(lexer_state * const lexer, NOTNULL(expression * const operand)) {
    PARROT_ASSERT(lexer->subs->statements);

    if (CURRENT_INSTRUCTION(lexer)->operands) {
        operand->next = CURRENT_INSTRUCTION(lexer)->operands->next;
        CURRENT_INSTRUCTION(lexer)->operands->next = operand;
    }
    CURRENT_INSTRUCTION(lexer)->operands = operand;
}

/*

=item C<void
remove_all_operands(lexer_state * const lexer)>

Remove all operands of the current instruction.

=cut

*/
void
remove_all_operands(NOTNULL(lexer_state * const lexer)) {
    CURRENT_INSTRUCTION(lexer)->operands = NULL;
}


/*

=item C<expression *
expr_from_key(key * const k)>

Wraps the key C<k> in an C<expression> node and returns that.

=cut

*/
expression *
expr_from_key(NOTNULL(lexer_state * const lexer), NOTNULL(key * const k)) {
    expression *e = new_expr(lexer, EXPR_KEY);
    e->expr.k     = k;
    return e;
}

/*

=item C<key *
new_key(expression * const expr)>

Wraps the expression C<expr> in a key node and returns that.

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
key *
new_key(NOTNULL(lexer_state * const lexer), NOTNULL(expression * const expr)) {
    key *k  = pir_mem_allocate_zeroed_typed(lexer, key);
    k->expr = expr;
    k->next = NULL;
    return k;
}





/*

=item C<add_key(key *keylist, expression * const exprkey)>

Adds a new, nested key (in C<exprkey>) to the current key,
pointed to by C<keylist>. C<keylist> is returned.

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
key *
add_key(NOTNULL(lexer_state * const lexer), NOTNULL(key * keylist),
        NOTNULL(expression * const exprkey))
{
    key *newkey = new_key(lexer, exprkey);
    key *list   = keylist;

    /* goto end of list */
    while (keylist->next != NULL)
        keylist = keylist->next;

    keylist->next = newkey;
    return list;
}

/*

=item C<symbol *
add_local(symbol *list, symbol *local)>

Add local C<local> to the list pointed to by C<list>. The new object
is inserted at the front of the list. C<list> is returned

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
symbol *
add_local(symbol * const list, symbol * const local) {
    local->next = list->next;
    list->next  = local;
    return list;
}


/*

=item C<symbol *
new_local(char * const name, int has_unique_reg)>

Create a new symbol node to represent the local C<name>.
If C<has_unique_reg> is true, the C<:unique_reg> flag is set.
The newly created symbol node is returned.

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
symbol *
new_local(lexer_state * const lexer, char const * const name, int has_unique_reg){
    symbol *s = new_symbol(lexer, name, UNKNOWN_TYPE);

    if (has_unique_reg)
        SET_FLAG(s->flags, TARGET_FLAG_UNIQUE_REG);

    return s;
}


/*

=item C<int
is_parrot_op(lexer_state * const lexer, char * const name)>

Check whether C<name> is a parrot opcode. C<name> can be either the short
or fullname of the opcode; for instance, C<print> is the short name, which
has several full names, such as C<print_i>, C<print_s>, etc., depending on
the arity and types of operands.

=cut

*/
int
is_parrot_op(lexer_state * const lexer, char const * const name) {
    int opcode = lexer->interp->op_lib->op_code(name, 0); /* check short name, e.g. "set" */

    /* if not found, try long name, e.g. "set_i_ic" */
    if (opcode < 0)
        opcode = lexer->interp->op_lib->op_code(name, 1); /* check long name */

    if (opcode >= 0)
        return TRUE;
    else
        return FALSE;

}






/*

=item C<static void
arguments_to_operands(lexer_state * const lexer, argument * const args)>

part of inv->instr conversion.

Add arguments as operands on the current instruction.

=cut

*/
static void
arguments_to_operands(lexer_state * const lexer, argument * const args) {
    argument *argiter;

    /* create a FixedIntegerArray object as first argument, which encodes
     * the number of arguments and their flags.
     */

    /* XXX in compilers/imcc/pcc.c there's a maximum number of 15 values;
     * do the same here to Get Things To Work, but fix later.
     */
    int flags_arg[15];
    int index         = 0;
    int forindex      = 0;
    int len;
    char *flagsstring, *strwriter;

    if (args == NULL) {
        push_operand(lexer, expr_from_const(lexer, new_const(lexer, PMC_TYPE, "")));
        return;
    }
    else {
        int numargs = 0;
        argiter = args;
        do {
            int flag = 0;
            expression *argvalue;

            argiter  = argiter->next;
            argvalue = argiter->value;
            /*
            fprintf(stderr, "converting arg to operand %d\n", ++numargs);
            */


            switch (argvalue->type) {
                case EXPR_TARGET:
                    if (TEST_FLAG(argvalue->expr.t->flags, TARGET_FLAG_IS_REG))
                        flag |= argvalue->expr.t->s.reg->type;
                    else
                        flag |= argvalue->expr.t->s.sym->type;
                    break;
                case EXPR_CONSTANT:
                    flag |= argvalue->expr.c->type;
                    break;
                default:
                    yypirerror(lexer->yyscanner, lexer, "invalid expression type for argument");
                    break;
            }
            /* store the flag for this argument */
            flags_arg[index++] = flag;
        }
        while (argiter != args);

        /* allocate space for each flag, + commas (index - 1) and 2 quotes */
        strwriter = flagsstring = (char *)mem_sys_allocate((index + index - 1 + 2) * sizeof (char));
        *strwriter++ = '"';

        while (forindex < index) {
            sprintf(strwriter++, "%d", flags_arg[forindex]);

            if (forindex < index - 1) {
                *strwriter++ = ',';
            }

            ++forindex;
        }
        /* write closing quote and NULL character */
        *strwriter++ = '"';
        *strwriter++ = '\0';

        /*
        fprintf(stderr, "args2operands: [%s]\n", flagsstring);
        */

        /* don't add it now, it will break tests. */
        /*
        push_operand(lexer, expr_from_const(lexer, new_const(lexer, STRING_TYPE, flagsstring)));
        */


        /* XXX Yes, this is a hacky attempt. Cleanups will follow. */
    }


    /* go over the arguments again, and add them as operands */
    argiter = args;

    do {
        argiter = argiter->next;
        /* how to handle named args? */
        push_operand(lexer, argiter->value);
    }
    while (argiter != args);
}

/*

=item C<static void
targets_to_operands(lexer_state * const lexer, target * const targets)>

Convert a list of targets pointed to by C<targets> into operands; each
C<target> node is added as an operand to the current instruction. If
C<targets> is NULL, an empty string is added as an operand.

=cut

*/
static void
targets_to_operands(lexer_state * const lexer, target * const targets) {
    target *iter;

    if (targets == NULL) {
        push_operand(lexer, expr_from_const(lexer, new_const(lexer, PMC_TYPE, "")));
        return;
    }

    iter = targets;

    do {
        iter = iter->next;
        push_operand(lexer, expr_from_target(lexer, iter));
    }
    while (iter != targets);
}


/*

=item C<static target *
generate_unique_pir_reg(lexer_state * const lexer, pir_type type)>

Generate a unique, temporary PIR register of type C<type>. It uses the C<reg>
constructor to create a target node, and using C<pir_reg_generator>
field of C<lexer> the PIR register number is specified. Because the positive
PIR register numbers can be used in PIR code, we use the negative numbers
here, for the reg() constructor this doesn't matter; a PIR register is always
mapped to a PASM register, so using negative PIR register is safe.

=cut

*/
static target *
generate_unique_pir_reg(lexer_state * const lexer, pir_type type) {
    return new_reg(lexer, type, --lexer->pir_reg_generator);
}



/*

=item C<static void
new_sub_instr(lexer_state * const lexer, int opcode, char * const opname)>

Create a new instruction node, and initialize the opcode and opinfo on that
node. This function can be used to create an instruction of which the signature
is known beforehand, without the need to compute the signature during runtime.
This is useful for generating special subroutine instructions, such as
C<get_params_pc> etc.

=cut

*/
static void
new_sub_instr(lexer_state * const lexer, int opcode, char const * const opname, ...) {
    new_statement(lexer, opname);
    CURRENT_INSTRUCTION(lexer)->opinfo = &lexer->interp->op_info_table[opcode];
    CURRENT_INSTRUCTION(lexer)->opcode = opcode;

    /* XXX how to calculate size of var-arg ops? */

    /* count number of ints needed to store this instruction in bytecode */
    lexer->codesize += CURRENT_INSTRUCTION(lexer)->opinfo->op_count;
}

/*

=item C<static void
update_op(lexer_state * const lexer, instruction * const instr, int newop)>

Update the instruction C<instr>; it is replaced by the op with opcode C<newop>.
The C<opinfo>, C<opname> and C<opcode> fields of C<instr> are updated.

=cut

*/
void
update_op(NOTNULL(lexer_state * const lexer), NOTNULL(instruction * const instr), int newop) {
    /* Deduct number of ints needed for the old instruction, if there is one.
     * This is necessary during strength reduction and other optimizations, once
     * the opinfo is retrieved, we also update the codesize field in the lexer.
     */
    if (instr->opinfo)
        lexer->codesize -= instr->opinfo->op_count;
    /* else the instruction was already set; decrement the codesize, as it was added already */

    /* now get the opinfo structure, update the name, and update the opcode. */
    instr->opinfo = &lexer->interp->op_info_table[newop];
    instr->opname = instr->opinfo->full_name;
    instr->opcode = newop;

    /* add codesize needed for the new instruction. */
    lexer->codesize += instr->opinfo->op_count;
}



/*

=item C<static void
save_global_reference(lexer_state * const lexer, instruction *instr, char * const label)>

Store the instruction C<instr>, which references the global label C<label> in a list.
After the parse phase, this instruction can be patched, if C<label> can be resolved
during compile time.

=cut

*/
static void
save_global_reference(lexer_state * const lexer, instruction * const instr, char const * const label) {
    global_fixup *ref = pir_mem_allocate_zeroed_typed(lexer, global_fixup);

    ref->instr = instr;
    ref->label = label;

    /* order of these entries does not matter, so just store it at the beginning */
    ref->next = lexer->global_refs;
    lexer->global_refs = ref;
}

/*

=item C<void
convert_inv_to_instr(lexer_state * const lexer, invocation * const inv)>

Convert an C<invocation> structure into a series of instructions.

XXX Some of the conversion should be done during emit_pbc(), because only at that
point is there a bytecode object around, in which PMCs can be emitted.
Subs are stored as PMCs, and we need to look up the PMC constant and emit
its index as an operand. Also, the first operands of the special PCC instructions
must be generated once the bytecode object is around, because they use a FixedIntegerArray
to encode flags/types of the rest of the operands.

One solution would be to mark these instructions, and fix them during emitting bytecode.
This needs more thought.

=cut

*/
void
convert_inv_to_instr(lexer_state * const lexer, invocation * const inv) {
    switch (inv->type) {
        case CALL_PCC:
            new_sub_instr(lexer, PARROT_OP_set_args_pc, "set_args_pc");
            arguments_to_operands(lexer, inv->arguments);

            new_sub_instr(lexer, PARROT_OP_get_results_pc, "get_results_pc");
            targets_to_operands(lexer, inv->results);

            /* if the target is a register, invoke that. */
            if (TEST_FLAG(inv->sub->flags, TARGET_FLAG_IS_REG)) {
                target *sub = new_reg(lexer, PMC_TYPE, inv->sub->s.reg->color);
                if (inv->retcc) { /* return continuation present? */
                    new_sub_instr(lexer, PARROT_OP_invoke_p_p, "invoke_p_p");
                    add_operands(lexer, "%T%T", inv->sub, inv->retcc);
                }
                else {
                    new_sub_instr(lexer, PARROT_OP_invokecc_p, "invokecc_p");
                    add_operands(lexer, "%T", sub);
                }
            }
            else { /* find the global label in the current file, or find it during runtime */
                target *sub        = generate_unique_pir_reg(lexer, PMC_TYPE);
                global_label *glob = find_global_label(lexer, inv->sub->s.sym->name);

                if (glob) {
                    /* XXX fix pmc const stuff */
                    new_sub_instr(lexer, PARROT_OP_set_p_pc, "set_p_pc");
                    add_operands(lexer, "%T%i", sub, glob->const_nr);
                }
                else { /* find it during runtime (hopefully, otherwise exception) */
                    new_sub_instr(lexer, PARROT_OP_find_sub_not_null_p_sc,
                                  "find_sub_not_null_p_sc");

                    add_operands(lexer, "%T%s", sub, inv->sub->s.sym->name);

                    /* save the current instruction in a list; entries in this list will be
                     * fixed up, if possible, after the parsing phase.
                     *
                     * Instead of the instruction
                     *
                     *   set_p_pc
                     *
                     * that is generated when the global label C<glob> was found (see above),
                     * another instructions is generated. After the parse, we'll re-try
                     * to find the global label that is referenced. For now, just generate
                     * this instruction to do the resolving of the label during runtime:
                     *
                     *   find_sub_not_null_p_sc
                     *
                     */
                    save_global_reference(lexer, CURRENT_INSTRUCTION(lexer), inv->sub->s.sym->name);
                }

                new_sub_instr(lexer, PARROT_OP_invokecc_p, "invokecc_p");
                add_operands(lexer, "%T", sub);

            }
            break;
        case CALL_RETURN:
            new_sub_instr(lexer, PARROT_OP_set_returns_pc, "set_returns_pc");
            arguments_to_operands(lexer, inv->arguments);

            new_sub_instr(lexer, PARROT_OP_returncc, "returncc");
            break;
        case CALL_NCI:
            set_instr(lexer, "invokecc_p");
            break;
        case CALL_YIELD:
            new_sub_instr(lexer, PARROT_OP_set_returns_pc, "set_returns_pc");
            arguments_to_operands(lexer, inv->arguments);

            new_sub_instr(lexer, PARROT_OP_yield, "yield");
            break;
        case CALL_TAILCALL:
            new_sub_instr(lexer, PARROT_OP_set_args_pc, "set_args_pc");
            arguments_to_operands(lexer, inv->arguments);

            new_sub_instr(lexer, PARROT_OP_tailcall_p, "tailcall_pc");
            break;
        case CALL_METHOD:
            new_sub_instr(lexer, PARROT_OP_set_args_pc, "set_args_pc");
            arguments_to_operands(lexer, inv->arguments);
            /* in a methodcall, the invocant object is passed as the first argument */
            unshift_operand(lexer, expr_from_target(lexer, inv->sub));

            new_sub_instr(lexer, PARROT_OP_get_results_pc, "get_results_pc");
            targets_to_operands(lexer, inv->results);

            new_sub_instr(lexer, PARROT_OP_callmethodcc_p_sc, "callmethodcc_p_sc");
            add_operands(lexer, "%T%E", inv->sub, inv->method);

            break;
        case CALL_METHOD_TAILCALL:
            new_sub_instr(lexer, PARROT_OP_set_args_pc, "set_args_pc");
            arguments_to_operands(lexer, inv->arguments);

            /* check out the type of the method expression; it may be a PMC or a STRING. */
            if (inv->method->type == EXPR_TARGET)
                new_sub_instr(lexer, PARROT_OP_tailcallmethod_p_p, "tailcallmethod_p_p");
            else if (inv->method->type == EXPR_CONSTANT)
                new_sub_instr(lexer, PARROT_OP_tailcallmethod_p_p, "tailcallmethod_p_sc");
            else
                panic(lexer, "unknown expression type in tailcallmethod instruction");

            break;
        default:
            panic(lexer, "Unknown invocation type in convert_inv_to_instr()");
            break;
    }

}

/*

=item C<static label *
new_label(lexer_state * const lexer, char const * const labelid, int offset)>

Constructor for a label operand.

=cut

*/
PARROT_MALLOC
static label *
new_label(lexer_state * const lexer, char const * const labelid, int offset) {
    label *l  = pir_mem_allocate_zeroed_typed(lexer, label);
    l->name   = labelid;
    l->offset = offset;
    return l;
}

/*

=item C<static void
fixup_local_labels(subroutine * const sub)>

Fix up all local labels in the subroutine C<sub>. Labels are stored as
identifiers in an expression node (the C<id> field in the C<expr> union);
the label's offset is subtracted from the current instruction's offset,
and the operand of the branch instruction is changed into this numeric
representation of the label.

=cut

*/
static void
fixup_local_labels(lexer_state * const lexer) {
    instruction *iter = CURRENT_SUB(lexer)->statements;

    /* if there's no instruction in the current sub, then do nothing. */
    if (iter == NULL)
        return;

    do {
        iter = iter->next; /* init pointer to first instruction */

        /* depending on what kind of branching instruction, get the right operand
         * that contains the label.
         */
        if (iter->oplabelbits) { /* this is a quick global check if any label bits have been set
                                  * if no label at all, skip this whole block.
                                  */

            /* now check for each operand */
            expression *operand = iter->operands;

            /* iter->operands is a circular linked list, point to the *last* operand. */

            /* Note that since oplabelbits has at least 1 bit set (otherwise it wouldn't
             * have been evaluated as "true" in the if statement above), we can be
             * sure there's at least one operand. Don't do silly tests here anymore.
             * (hence the do-while statement, no initial test.)
             */
            int flag = 0;

            do {
                operand = operand->next;

                if (TEST_FLAG(iter->oplabelbits, BIT(flag))) {
                    /* the current operand is a label; fix it up. No, not a date. */
                    char const * labelid    = operand->expr.id;
                    unsigned     offset     = find_local_label(lexer, labelid);
                    unsigned     curr_instr = iter->offset;

                    /* convert the label identifier into a real label object */
                    operand->expr.l = new_label(lexer, labelid, offset - curr_instr);
                    operand->type   = EXPR_LABEL;
                }

                ++flag;

            }
            while (operand != iter->operands);

        }

    }
    while (iter != lexer->subs->statements); /* iterate over all instructions */
}

/*

=item C<void
fixup_global_labels(lexer_state * const lexer)>

Fix references to global labels. The lexer keeps a list of instructions that
can be patched. These instructions currently look up the global label during
runtime (this is the op C<find_sub_not_null>). If the global label can be
resolved now, then this op is changed into the C<set_p_pc> op, which loads
the PMC constant representing the sub into a register. Obviously, being a
simple register set instruction, this is faster than a look up.

=cut

*/
void
fixup_global_labels(lexer_state * const lexer) {
    global_fixup *iter = lexer->global_refs;

    while (iter) {
        global_label *glob = find_global_label(lexer, iter->label);

        /* if found, then fix it; if not, that's fine, the right instruction to find the
         * sub during runtime is in place already.
         */
        if (glob) {
            expression *new_second_operand;

            /* iter->instr should be changed into this instruction: set_p_pc */

            update_op(lexer, iter->instr, PARROT_OP_set_p_pc);

            /* now change the second operand. It was a PASM register, and a string, but
             * it should be the PASM register and a constant PMC (that represents the
             * sub to be invoked). This constant PMC is referenced by a number, stored
             * in the C<global_label> struct.
             */

            /* create an operand that refers to a constant PMC */
            new_second_operand = expr_from_const(lexer, new_const(lexer, INT_TYPE, glob->const_nr));
            /* link it into the list of operands; the /current/ second operand should be removed,
             * so insert the new expression as second operand, and make sure the old second
             * operand is no longer in the list.
             */
            new_second_operand->next = iter->instr->operands->next;
            iter->instr->operands->next->next = new_second_operand;

        }
        iter = iter->next;
    }
}

/*

=item C<void
close_sub(lexer_state * const lexer)>

Finalize the subroutine. Generate the final instructions in the current
subroutine; if the C<:main> flag was set on the subroutine, this is the
C<end> instruction; otherwise, a I<normal> C<return> sequence is generated.

Then, all local labels are fixed up; i.e., all label identifiers are converted
into their offsets.

=cut

*/
void
close_sub(lexer_state * const lexer) {
    int opcode;

    /* a :main-marked sub ends with the "end" instruction;
     * otherwise it's this pair:
     *
     *    set_returns_pc
     *    returncc
     */


    if (TEST_FLAG(lexer->subs->flags, SUB_FLAG_MAIN)) {
        new_sub_instr(lexer, PARROT_OP_end, "end");
    }
    else {
        /* XXX if there was already a return sequence explicitly, we shouldn't do this. */

        new_sub_instr(lexer, PARROT_OP_set_returns_pc, "set_returns_pc");
        new_sub_instr(lexer, PARROT_OP_returncc, "returncc");
    }

    /* fix up all local branch labels */
    fixup_local_labels(lexer);

    /* store end offset in bytecode of this subroutine */
    CURRENT_SUB(lexer)->endoffset = lexer->codesize;
}


/*

=back

=cut

*/

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */


