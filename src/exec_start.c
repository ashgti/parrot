/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 NAME

src/exec_start.c - Call compiled code

=head1 SYNOPSIS

I<What goes here?>

=head1 DESCRIPTION

Exec is a tool for using the JIT at compile time to generate native
executables.

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "parrot/string.h"
#include "parrot/exec.h"
#include "jit.h"
#define JIT_EMIT 1
#include "jit_emit.h"
#include "parrot/embed.h"

opcode_t* run_compiled(PARROT_INTERP,
    opcode_t *cur_opcode, opcode_t *code_start);

/*

=item C<int main(int argc, char * argv[])>

The run-loop.

=cut

*/

int
main(int argc, char * argv[])
{
    /* long *             opp; */
    Interp *           interp;
    PackFile *  pf;
    opcode_t *         code_start;
    extern char *      program_code;
    /* extern long        opcode_map; */
    /* extern int         bytecode_offset; */
#if defined(JIT_CGP)
    extern void *      exec_prederef_code;
#endif
    /* extern int        Parrot_exec_run; */
    /* extern PackFile_Constant *exec_const_table; */
    /* extern PackFile_Constant const_table; */
    extern Interp interpre;

    /* s. exec.c */
    /* Parrot_exec_run = 1; */
    /* s. packfile.c (PackFile_ConstTable_unpack()) */
    /* exec_const_table = &const_table; */
    interp = Parrot_new(NULL);
    if (!interp) {
        return 1;
    }

    /* run_native = run_compiled; */
    /* TODO make also a shared variant of PackFile_new */
    pf = PackFile_new(interp, 0);

    if (!PackFile_unpack(interp, pf, (opcode_t *)(&program_code),
        sizeof (&program_code)))
    {
        printf("Can't unpack.\n");
        return 1;
    }
    Parrot_pbc_load(interp, pf);
    PackFile_fixup_subs(interp, PBC_PBC, NULL);

    /* opcode_map has the offset of each opcode in the compiled code
     * this modifies it to be address of the opcode.
     */
    /*
    opp = &opcode_map;
    for (i = 0; i < (int)interpre.code->base.size; i++) {
        opp[i] += (long)run_compiled;
    }
    */

#if defined(JIT_CGP)
    exec_init_prederef(interp, &exec_prederef_code);
#endif
    /* Parrot_runcore_switch(interp, Parrot_str_new_constant(interp, "exec"));
    interp->code->base.data =
        (opcode_t *)&((&program_code)[bytecode_offset]);
    Parrot_exec_run = 0; */
    Parrot_runcode(interp, argc, argv);
    /*
        run_compiled(interp,
            (opcode_t *)&((&program_code)[bytecode_offset]));
     */
    Parrot_exit(interp, 0);
}

/*

=back

=head1 SEE ALSO

F<include/parrot/exec.h>, F<src/exec_save.h>, F<src/exec.c>
and F<compilers/imcc/main.c>.

=head1 HISTORY

Initial version by Daniel Grunblatt on 2003.6.9

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
