/*
 * $Id$
 * Copyright (C) 2008, The Perl Foundation.
 */
#include <stdio.h>
#include <assert.h>
#include "parrot/parrot.h"
#include "parrot/embed.h"

#include "bcgen.h"


/* private bits of the bytecode generator */


typedef struct bc_const {
    int                index; /* index into constant table */
    PackFile_Constant *value;
    struct bc_const   *next;  /* linked list */

} bc_const;


/* Note that typedef of struct bytecode is already done in the header file */

struct bytecode {
    int          num_constants;  /* for assigning indices to the constants, also counter. */
    bc_const    *constants;      /* list of constants */
    PackFile    *packfile;       /* the actual packfile */
    opcode_t    *opcursor;       /* for writing ops into the code segment */
    Interp      *interp;         /* parrot interpreter */

};

/*typedef struct bytecode bytecode;
*/

static bc_const * new_const(bytecode * const bc);

/*

=head1 FUNCTIONS

=over 4

=cut

*/


/*

Create a new constant node in the linked list of constants.
A pointer to the node is returned.

*/
static bc_const *
new_const(bytecode * const bc) {
    bc_const *bcc = (bc_const *)mem_sys_allocate(sizeof (bc_const));
    bcc->value    = PackFile_Constant_new(bc->interp);
    bcc->index    = bc->num_constants++;
    bcc->next     = bc->constants;
    bc->constants = bcc;
    return bcc;
}

/*

Add a PMC constant to the constants list.

*/
int
add_pmc_const(bytecode * const bc, PMC * pmc) {
    bc_const *bcc     = new_const(bc);
    bcc->value->type  = PFC_PMC;
    bcc->value->u.key = pmc;
    return bcc->index;
}

/*

Add a String constant to the constants list.

*/
int
add_string_const(bytecode * const bc, char const * const str) {
    bc_const *bcc        = new_const(bc);
    bcc->value->type     = PFC_STRING;
    bcc->value->u.string = string_make(bc->interp, str, strlen(str), "ascii", PObj_constant_FLAG);
    return bcc->index;
}

/*

Add a number constant to the constants list.

*/
int
add_num_const(bytecode * const bc, double f) {
    /* STRING * const str   = string_from_cstring(interp, number_cstring, 0); */
    bc_const *bcc        = new_const(bc);
    bcc->value->type     = PFC_NUMBER;
    /* bcc->value->u.number = string_to_num(interp, str); */
    bcc->value->u.number = f;
    return bcc->index;
}

/*

Add a key constant to the constants list.

*/
int
add_key_const(bytecode * const bc, PMC *key) {
    bc_const *bcc     = new_const(bc);
    bcc->value->type  = PFC_KEY;
    bcc->value->u.key = key;
    return bcc->index;
}

/*

Get the PackFile_Constant on index C<key>.
XXX not efficient.

*/
PackFile_Constant *
get_const(bytecode * const bc, int key) {
    bc_const *iter = bc->constants;
    while (iter) {
        if (iter->index == key)
            return iter->value;
        iter = iter->next;
    }
    return NULL;
}

/*

=item new_bytecode

Create a new bytecode struct and return a pointer to it.

=cut

*/
bytecode *
new_bytecode(Interp *interp, char const * const filename, int bytes, int codesize) {
    PMC      *self;
    bytecode *bc      = (bytecode *)mem_sys_allocate(sizeof (bytecode));

    bc->packfile      = PackFile_new(interp, 0);
    bc->constants     = NULL;
    bc->interp        = interp;
    bc->num_constants = 0;

    Parrot_loadbc(interp, bc->packfile);

    /* create segments */
    interp->code      = PF_create_default_segs(interp, filename, 1);

    /* add interpreter globals to bytecode (?) */
    self              = VTABLE_get_pmc_keyed_int(interp, interp->iglobals, IGLOBALS_INTERPRETER);
    add_pmc_const(bc, self);

    interp->code->base.data = (opcode_t *)mem_sys_realloc(interp->code->base.data, bytes);
    interp->code->base.size = codesize;

    /* initialize the cursor to write opcodes into the code segment */
    bc->opcursor = (opcode_t *)interp->code->base.data;

    return bc;
}

/*

Write an op into the bytecode stream.

*/
void
emit_opcode(bytecode * const bc, opcode_t op) {
    *bc->opcursor++ = op;
}

/*

Write an integer argument into the bytecode stream.

*/
void
emit_int_arg(bytecode * const bc, int intval) {
    *bc->opcursor++ = intval;
}

/*

Emit the opcode by name. C<opname> must be a valid, signatured opname.
So, "print" is not valid, whereas "print_ic" is. The opcode of the
opname is looked up and written into the bytecode stream.

*/
void
emit_op_by_name(bytecode * const bc, char const * const opname) {
    int op = bc->interp->op_lib->op_code(opname, 1);
    if (op < 0) {
        /* error */
        fprintf(stderr, "'%s' is not an op", opname);
    }
    else
        emit_opcode(bc, op);
}

static STRING *add_string_const_from_cstring(bytecode * const bc, char const * const str);
/*

XXX think of better name.
Add a string constant to the constants list, and return the STRING variant

*/
static STRING *
add_string_const_from_cstring(bytecode * const bc, char const * const str) {
    int index = add_string_const(bc, str);
    return get_const(bc, index)->u.string;
}




/*

Add a sub PMC to the constant table. This function initializes the sub PMC.

*/
void
add_sub_pmc(bytecode * const bc,
            char const * const subname, /* .sub foo --> "foo" */
            char const * const nsentry, /* .sub foo :nsentry('bar') --> "bar" */
            char const * const subid,   /* .sub foo :subid('baz') --> "baz" */
            int vtable_index,           /* vtable entry index */
            unsigned regs_used[],            /* register usage */
            int startoffset,
            int endoffset)
{
    Interp     *interp    = bc->interp;
    PMC        *sub_pmc   = pmc_new(bc->interp, enum_class_Sub);
    Parrot_sub *sub       = PMC_sub(sub_pmc);
    int   subconst_index;
    int   subname_index;


    int i;


    PackFile_Constant *subname_const;

    subname_index = add_string_const(bc, subname);
    subname_const = get_const(bc, subname_index);

    sub->start_offs       = startoffset;
    sub->end_offs         = endoffset;
    sub->namespace_name   = NULL;
    sub->HLL_id           = CONTEXT(interp)->current_HLL;

    sub->lex_info         = NULL;
    sub->outer_sub        = NULL;
    sub->vtable_index     = vtable_index;
    sub->multi_signature  = NULL;

    /* store register usage of this sub. */
    for (i = 0; i < 4; ++i)
        sub->n_regs_used[i] = regs_used[i];

    sub->name = subname_const->u.string;

    /* If there was a :nsentry, add it to the constants table, and set
     * the ns_entry_name attribute to that STRING. Default value is the sub's name.
     */
    if (nsentry)
        sub->ns_entry_name = add_string_const_from_cstring(bc, nsentry);
    else
        sub->ns_entry_name = subname_const->u.string;

    /* if there was a :subid, add it to the constants table, and set the subid
     * attribute to that STRING. Default value is the sub's name.
     */
    if (subid)
        sub->subid = add_string_const_from_cstring(bc, subid);
    else
        sub->subid = subname_const->u.string;



    Parrot_store_sub_in_namespace(bc->interp, sub_pmc);
    subconst_index = add_pmc_const(bc, sub_pmc);

    fprintf(stderr, "subconst index: %d\n", subconst_index);

    /* doesn't work?? */

    /* this /does/ work in pirc/bctest.c!! Why not here??
    */

    /*
    PackFile_FixupTable_new_entry(bc->interp, "main", enum_fixup_sub, subconst->index);
    */

}

/*

Walk the list of constants and store them in the constants segment of the packfile.
Instead of updating the constants table one by one, it's neater to add them in one go.
This prevents memory re-allocations, which are generally not the cheapest operations.

XXX check out if we can count the number of needed constants during the parse; that
would prevent the linked list thing. OTOH, that makes this API less flexible; client-code
doesn't want to be so inflexible.

*/
static void
emit_constants(bytecode * const bc) {
    bc_const *iter = bc->constants;

    /* allocate enough space */

    fprintf(stderr, "allocating space for %d constants\n", bc->num_constants);
    bc->interp->code->const_table->constants
                                   = mem_allocate_n_typed(bc->num_constants, PackFile_Constant *);

    /* walk the list and put all of them into the constant table. */
    while (iter) {
        bc->interp->code->const_table->constants[iter->index] = iter->value;
        iter = iter->next;
    }
}

/*

Write the bytecode to the file C<filename>.

*/
void
write_pbc_file(bytecode * const bc, char const * const filename) {
    size_t    size;
    opcode_t *packed;
    FILE     *fp;
    int       result;

    /* store the list of constants in the constants segment */
    emit_constants(bc);

    /* pack the packfile */
    size   = PackFile_pack_size(bc->interp, bc->interp->code->base.pf) * sizeof (opcode_t);
    packed = (opcode_t*) mem_sys_allocate(size);
    PackFile_pack(bc->interp, bc->interp->code->base.pf, packed);

    /* write to file */
    fp = fopen(filename, "wb");

    if (fp == NULL)
        fprintf(stderr, "Couldn't open %s\n", filename);

    result = fwrite(packed, size, 1, fp);

    if (result != 1)
        fprintf(stderr, "Couldn't write %s\n", filename);

    fclose(fp);

    /* done! */
}


/*

Test driver.

*/
/*
int
main(int argc, char **argv) {
    Interp *interp = Parrot_new(NULL);
    bytecode *bc   = new_bytecode(interp, "test.pir", 12, 3);
    int regs_used[4] = {0,0,0,0};

    add_sub_pmc(bc, "main", NULL, NULL, -1, regs_used);
    emit_op_by_name(bc, "print_ic");
    emit_int_arg(bc, 42);
    emit_op_by_name(bc, "end");


    fprintf(stderr, "writing pbc...");
    write_pbc_file(bc, "test.pbc");
    fprintf(stderr, "written pbc file\n");
}

*/


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
