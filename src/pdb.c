/*
Copyright (C) 2001-2003, The Perl Foundation.
$Id$

=head1 NAME

pdb - The Parrot debugger

=head1 SYNOPSIS

 pdb programfile

=head1 DESCRIPTION

=head2 Commands

=over 4

=item C<disassemble>

Disassemble the bytecode.

Use this if you have a PBC file but not the PASM.

=item C<load>

Load a source code file.

=item C<list> or C<l>

List the source code file.

=item C<run> or C<r>

Run the program.

=item C<break> or C<b>

Add a breakpoint.

=item C<watch> or C<w>

Add a watchpoint.

=item C<delete> or C<d>

Delete a breakpoint.

=item C<disable>

Disable a breakpoint.

=item C<enable>

Reenable a disabled breakpoint.

=item C<continue> or C<c>

Continue the program execution.

=item C<next> or C<n>

Run the next instruction

=item C<eval> or C<e>

Run an instruction.

=item C<trace> or C<t>

Trace the next instruction.

=item C<print> or C<p>

Print the interpreter registers.

=item C<stack> or C<s>

Examine the stack.

=item C<info>

Print interpreter information.

=item C<quit> or C<q>

Exit the debugger.

=item C<help> or C<h>

Print the help.

=back

=head2 Debug Ops

You can also debug Parrot code by using the C<debug_init>, C<debug_load>
and C<debug_break> ops in F<ops/debug.ops>.

=over 4

=cut

*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "../compilers/imcc/imc.h"
#include "../compilers/imcc/parser.h"
#include "parrot/embed.h"

static void PDB_printwelcome(void);

/*

=item C<int main(int argc, char *argv[])>

Reads the PASM or PBC file from argv[1], loads it, and then calls
Parrot_debug().

=cut

*/

extern void imcc_init(Parrot_Interp interp);

int
main(int argc, char *argv[])
{
    Parrot_Interp interp, debugger;
    char *filename;
    Parrot_PackFile pf;
    char *ext;
    int pasm_file;
    PDB_t *pdb;
    void *yyscanner;


    /*Parrot_set_config_hash();  TODO link with cfg */
    debugger = Parrot_new(NULL);
    pdb = (PDB_t *)mem_sys_allocate_zeroed(sizeof (PDB_t));
    /* attach pdb structure */
    debugger->pdb = pdb;

    interp = Parrot_new(debugger);
    interp->debugger = debugger;
    pdb->debugee = interp;

    Parrot_block_DOD(interp);
    Parrot_block_GC(interp);
    imcc_init(interp);

    do_yylex_init(interp, &yyscanner);

    if (argc < 2) {
        fprintf(stderr, "Usage: pdb programfile [program-options]\n");
        Parrot_exit(interp, 1);
    }

    filename = argv[1];
    ext = strrchr(filename, '.');
    if (ext && strcmp(ext, ".pbc") == 0) {

        pf = Parrot_readbc(interp, filename);

        if (!pf) {
            return 1;
        }

        Parrot_loadbc(interp, pf);
    }
    else {
        pf = PackFile_new(interp, 0);
        Parrot_loadbc(interp, pf);

        IMCC_push_parser_state(interp);
        IMCC_INFO(interp)->state->file = filename;

        if (!(imc_yyin_set(fopen(filename, "r"), yyscanner)))    {
            IMCC_fatal_standalone(interp, E_IOError,
                    "Error reading source file %s.\n",
                    filename);
        }
        pasm_file = 0;
        if (ext && strcmp(ext, ".pasm") == 0)
            pasm_file = 1;
        emit_open(interp, 1, NULL);
        IMCC_INFO(interp)->state->pasm_file = pasm_file;
        yyparse(yyscanner, (void *) interp);
        imc_compile_all_units(interp);

        imc_cleanup(interp, yyscanner);

        fclose(imc_yyin_get(yyscanner));
        PackFile_fixup_subs(interp, PBC_POSTCOMP, NULL);
    }
    Parrot_unblock_DOD(interp);
    Parrot_unblock_GC(interp);

    PDB_printwelcome();

    Parrot_runcode(interp, argc - 1, argv + 1);
    Parrot_exit(interp, 0);

    return 0;
}

/*

=item C<static void PDB_printwelcome()>

Prints out the welcome string.

=cut

*/

static void
PDB_printwelcome(void)
{
    fprintf(stderr, "Parrot Debugger 0.4.x\n");
    fprintf(stderr, "\nPlease note: ");
    fprintf(stderr, "the debugger is currently under reconstruction\n");
}

/*

=back

=head1 SEE ALSO

F<src/debug.c>, F<include/parrot/debug.h>.

=head1 HISTORY

=over 4

=item * Initial version by Daniel Grunblatt on 2002.5.19.

=item * Start of rewrite - leo 2005.02.16

The debugger now uses it's own interpreter. User code is run in
Interp* debugee. We have:

  debug_interp->pdb->debugee->debugger
    ^                            |
    |                            v
    +------------- := -----------+

Debug commands are mostly run inside the C<debugger>. User code
runs of course in the C<debugee>.

=back

=head1 TODO

=over 4

=item * Check the user input for bad commands, it's quite easy to make
it bang now, try listing the source before loading or disassembling it.

=item * Print the interpreter info.

=item * Make the user interface better (add comands
history/completion).

=item * Some other things I don't remember now because it's late.


=back

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
