#include <process.h>

/*
 * Spawn a subprocess
 *
 */
INTVAL
Parrot_Run_OS_Command(Parrot_Interp interpreter, STRING *command) {
    DWORD status = 0;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    int free_it = 0;
    char* cmd = mem_sys_allocate( command->strlen + 4 );
    char* shell = Parrot_getenv( "ComSpec", &free_it );
    char* cmdin = string_to_cstring(interpreter, command);

    strcpy( cmd, "/c " );
    strcat( cmd, cmdin );
    string_cstring_free( cmdin );

    memset( &si, 0, sizeof(si) );
    si.cb = sizeof(si);
    memset( &pi, 0, sizeof(pi) );

    /* Start the child process. */
    if(
        !CreateProcess( shell, cmd,
        NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi )
    ) {
        internal_exception(NOSPAWN, "Can't spawn child process");
    }

    WaitForSingleObject( pi.hProcess, INFINITE );

    if( !GetExitCodeProcess( pi.hProcess, &status ) ) {
        /* XXX njs Should call GetLastError for failure message? */
        Parrot_warn( interpreter, PARROT_WARNINGS_PLATFORM_FLAG,
            "Process completed: Failed to get exit code.");
    }
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
    if( free_it ) free( shell );
    mem_sys_free( cmd );

    return status;
}

void 
Parrot_Exec_OS_Command(Parrot_Interp interpreter, STRING *command)
{
	int status;
	char *in = string_to_cstring(interpreter, command);
	char *cmd = NULL;
	char **argv = mem_sys_allocate_zeroed(2 * sizeof(int));

	/* Grab string, extract command and parameters. */
	char *curPos = in;
	char *lastCommandStart = in;
	char seekChar = 0; 
	int argc = 1;
	while (*curPos)
	{
		/* If we don't have a seek character and this is a quote... */
		if (seekChar == 0 && (*curPos == '\'' || *curPos == '"'))
		{
			seekChar = *curPos;
			lastCommandStart = curPos;
		}

		/* If we don't have a seek character and this is not a space... */
		else if (seekChar == 0 && *curPos != ' ')
		{
			if (!seekChar)
				seekChar = ' ';
			lastCommandStart = curPos;
		}

		/* If we seek the seek character... */
		else if (*curPos == seekChar || (*(curPos + 1) == 0 && seekChar == ' '))
		{
			/* Copy what we found to a temporary string. */
			char *tmp;
			int lenFound = curPos - lastCommandStart;
			if (*(curPos + 1) == 0)
				lenFound++;
			tmp = mem_sys_allocate(1 + lenFound);
			memcpy(tmp, lastCommandStart, lenFound);
			*(tmp + lenFound) = 0;
			
			/* Is it command or argument? */
			if (cmd == NULL)
			{
				cmd = tmp;
				*argv = tmp;
			}
			else
			{
				/* Allocate space for another pointer in **argv. */
				argc++;
				argv = mem_sys_realloc(argv, (argc + 1) * sizeof(int));
				*(argv + (argc - 1)) = tmp;
				*(argv + argc) = NULL;
			}

			/* Clear seek character. */
			seekChar = 0;
		}

		/* Move to next character. */
		curPos ++;
	}

	/* If we still have a seek char, then the input was improper. */
	if (seekChar)
	{
		internal_exception(NOSPAWN, "Exec failed, invalid command string");
	}

	/* Now do the exec. */
	status = _execvp(cmd, argv);
	if (status) {
		internal_exception(NOSPAWN, "Exec failed, code %i", status);
	}
}

