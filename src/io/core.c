/*
Copyright (C) 2001-2008, Parrot Foundation.
$Id$

=head1 NAME

src/io/core.c - I/O subsystem core functions

=head1 DESCRIPTION

Core functions for initializing and destroying the I/O subsystem within an
interpreter.

=head2 Resource Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "io_private.h"

/* HEADERIZER HFILE: include/parrot/io.h */

/*
        The standard streams are:

                interp->piodata->table[PIO_STD*_FILENO].
*/

PIOOFF_T piooffsetzero;

/*

=item C<void Parrot_io_init(PARROT_INTERP)>

Sets up the interpreter's I/O storage and creates the C<STD*> handles.

Called when creating an interpreter.

=cut

*/

PARROT_EXPORT
void
Parrot_io_init(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_io_init)
    /* Has interp been initialized already? */
    if (interp->piodata) {
        /* memsub system is up and running: */
        /* Init IO stacks and handles for interp instance.  */
        PIO_INIT(interp);


        if (Interp_debug_TEST(interp, PARROT_START_DEBUG_FLAG)) {
            Parrot_io_eprintf(NULL, "I/O system initialized.\n");
        }

        return;
    }


    interp->piodata = mem_allocate_typed(ParrotIOData);
    if (interp->piodata == NULL)
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_PIO_ERROR,
            "PIO alloc piodata failure.");
    interp->piodata->table         =
        (PMC **)mem_sys_allocate_zeroed(PIO_NR_OPEN * sizeof (PMC *));

    if (!interp->piodata->table)
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_PIO_ERROR,
            "PIO alloc table failure.");
}

/*

=item C<void Parrot_io_finish(PARROT_INTERP)>

Closes the interpreter's IO resourses.  Called during its interpreter
destruction.

=cut

*/

PARROT_EXPORT
void
Parrot_io_finish(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_io_finish)
    /*
     * TODO free IO of std-handles
     */
    mem_sys_free(interp->piodata->table);
    interp->piodata->table = NULL;
    mem_sys_free(interp->piodata);
    interp->piodata = NULL;

}


/*

=item C<void Parrot_IOData_mark(PARROT_INTERP, ParrotIOData *piodata)>

Called from C<Parrot_gc_trace_root()> to mark the IO data live.

=cut

*/

PARROT_EXPORT
void
Parrot_IOData_mark(PARROT_INTERP, ARGIN(ParrotIOData *piodata))
{
    ASSERT_ARGS(Parrot_IOData_mark)
    INTVAL i;
    ParrotIOTable table = piodata->table;

    /* this was i < PIO_NR_OPEN, but only standard handles 0..2 need
     * to be kept alive AFAIK -leo
     */
    for (i = 0; i < 3; i++) {
        Parrot_gc_mark_PMC_alive(interp, table[i]);
    }
}


/*

=back

=head1 SEE ALSO

F<src/io/api.c>,
F<src/io/unix.c>,
F<src/io/win32.c>,
F<src/io/portable.c>,
F<src/io/buffer.c>,
F<src/io/io_private.h>.

=cut

*/



/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
