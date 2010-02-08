/*
 * $Id$
 * Copyright (C) 2004-2008, Parrot Foundation.
 */

/*

=head1 NAME

config\gen\platform\win32\exec.c

=head1 DESCRIPTION

Functions for dealing with child processes and Execs.

=head2 Functions

=over 4

=cut

*/

#include <process.h>

/*

=item C<INTVAL Parrot_Run_OS_Command(PARROT_INTERP, STRING *command)>

Spawn the subprocess specified in C<command>.
Waits for the process to complete, and then
returns the exit code in POSIX-compatibility mode.

=cut

*/

INTVAL
Parrot_Run_OS_Command(PARROT_INTERP, STRING *command)
{
    DWORD status = 0;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    int free_it = 0;
    char* cmd = (char *)mem_sys_allocate(command->strlen + 4);
    char* shell = Parrot_getenv(interp, Parrot_str_new(interp, "ComSpec", strlen("ComSpec")));
    char* cmdin = Parrot_str_to_cstring(interp, command);

    strcpy(cmd, "/c ");
    strcat(cmd, cmdin);
    Parrot_str_free_cstring(cmdin);

    memset(&si, 0, sizeof (si));
    si.cb = sizeof (si);
    memset(&pi, 0, sizeof (pi));

    /* Start the child process. */
    if (!CreateProcess(shell, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_NOSPAWN,
            "Can't spawn child process");

    WaitForSingleObject(pi.hProcess, INFINITE);

    if (!GetExitCodeProcess(pi.hProcess, &status)) {
        Parrot_warn(interp, PARROT_WARNINGS_PLATFORM_FLAG,
            "Process completed: Failed to get exit code.");
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    if (free_it) free(shell);
    mem_sys_free(cmd);

    /* Return exit code left shifted by 8 for POSIX emulation. */
    return status << 8;
}

/*

=item C<INTVAL Parrot_Run_OS_Command_Argv(PARROT_INTERP, PMC *cmdargs)>

Spawns a subprocess with the arguments provided in the C<cmdargs> PMC array.
The first array element should be the name of the process to spawn,
and the remainder of the array elements should be arguments.
Waits until the child process completes,
and returns the exit code in POSIX-compatibility mode.

=cut

*/

INTVAL
Parrot_Run_OS_Command_Argv(PARROT_INTERP, PMC *cmdargs)
{
    DWORD status = 0;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    int pmclen;
    int cmdlinelen = 1000;
    int cmdlinepos = 0;
    char *cmdline = (char *)mem_sys_allocate(cmdlinelen);
    int i;

    /* Ensure there's something in the PMC array. */
    pmclen = VTABLE_elements(interp, cmdargs);
    if (pmclen == 0)
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_NOSPAWN,
            "Empty argument array for spawnw");

    /* Now build command line. */
    for (i = 0; i < pmclen; i++) {
        STRING *s = VTABLE_get_string_keyed_int(interp, cmdargs, i);
        char *cs  = Parrot_str_to_cstring(interp, s);
        if (cmdlinepos + (int)s->strlen + 3 > cmdlinelen) {
            cmdlinelen += s->strlen + 4;
            cmdline = (char *)mem_sys_realloc(cmdline, cmdlinelen);
        }
        strcpy(cmdline + cmdlinepos, "\"");
        strcpy(cmdline + cmdlinepos + 1, cs);
        strcpy(cmdline + cmdlinepos + 1 + s->strlen, "\" ");
        cmdlinepos += s->strlen + 3;
    }

    /* Start the child process. */
    memset(&si, 0, sizeof (si));
    si.cb = sizeof (si);
    memset(&pi, 0, sizeof (pi));
    if (!CreateProcess(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_NOSPAWN,
            "Can't spawn child process");

    WaitForSingleObject(pi.hProcess, INFINITE);

    /* Get exit code. */
    if (!GetExitCodeProcess(pi.hProcess, &status)) {
        Parrot_warn(interp, PARROT_WARNINGS_PLATFORM_FLAG,
            "Process completed: Failed to get exit code.");
    }

    /* Clean up. */
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    mem_sys_free(cmdline);

    /* Return exit code left shifted by 8 for POSIX emulation. */
    return status << 8;
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

