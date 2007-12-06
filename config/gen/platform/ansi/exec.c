/*
 * $Id$
 * Copyright (C) 2004-2007, The Perl Foundation.
 */

/*

=head1 NAME

config/gen/platform/ansi/exec.c

=head1 DESCRIPTION

TODO

=head2 Functions

=over 4

=cut

*/

/*

=item C<INTVAL Parrot_Run_OS_Command(Parrot_Interp interp, STRING *command)>

Spawn a subprocess

=cut

*/

INTVAL
Parrot_Run_OS_Command(Parrot_Interp interp, STRING *command)
{
    Parrot_warn(NULL, PARROT_WARNINGS_PLATFORM_FLAG,
            "Parrot_Run_OS_Command not implemented");
    return 0;
}

/*

=item C<INTVAL Parrot_Run_OS_Command_Argv(Parrot_Interp interp, PMC *cmdargs)>

RT#48260: Not yet documented!!!

=cut

*/

INTVAL
Parrot_Run_OS_Command_Argv(Parrot_Interp interp, PMC *cmdargs)
{
    Parrot_warn(NULL, PARROT_WARNINGS_PLATFORM_FLAG,
            "Parrot_Run_OS_Command_Argv not implemented");
    return 0;
}

/*

=item C<void Parrot_Exec_OS_Comman(Parrot_Interp interp, STRING *command)>

RT#48260: Not yet documented!!!

=cut

*/

void
Parrot_Exec_OS_Comman(Parrot_Interp interp, STRING *command)
{
  real_exception(interp, NULL, NOSPAWN, "Exec not implemented");
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
