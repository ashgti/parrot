/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 NAME

src/debug.c - Parrot debugging

=head1 DESCRIPTION

This file implements Parrot debugging and is used by C<parrot_debugger>,
the Parrot debugger, and the C<debug> ops.

=head2 Functions

=over 4

=cut

*/

#include <stdio.h>
#include <stdlib.h>
#include "parrot/parrot.h"
#include "parrot/extend.h"
#include "parrot/embed.h"
#include "parrot/oplib.h"
#include "parrot/debugger.h"
#include "parrot/oplib/ops.h"
#include "pmc/pmc_key.h"
#include "parrot/runcore_api.h"
#include "parrot/runcore_trace.h"
#include "debug.str"
#include "pmc/pmc_continuation.h"
#include "pmc/pmc_callcontext.h"

/* Hand switched debugger tracing
 * Set to 1 to enable tracing to stderr
 * Set to 0 to disable
 */
#define TRACE_DEBUGGER 0

#if TRACE_DEBUGGER
#  define TRACEDEB_MSG(msg) fprintf(stderr, "%s\n", (msg))
#else
#  define TRACEDEB_MSG(msg)
#endif

/* Length of command line buffers */
#define DEBUG_CMD_BUFFER_LENGTH 255

/* Easier register access */
#define IREG(i) REG_INT(interp, (i))
#define NREG(i) REG_NUM(interp, (i))
#define SREG(i) REG_STR(interp, (i))
#define PREG(i) REG_PMC(interp, (i))

typedef struct DebuggerCmd DebuggerCmd;
typedef struct DebuggerCmdList DebuggerCmdList;


/* HEADERIZER HFILE: include/parrot/debugger.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static void chop_newline(ARGMOD(char * buf))
        __attribute__nonnull__(1)
        FUNC_MODIFIES(* buf);

static void close_script_file(PARROT_INTERP)
        __attribute__nonnull__(1);

static unsigned short condition_regtype(ARGIN(const char *cmd))
        __attribute__nonnull__(1);

PARROT_CAN_RETURN_NULL
static PDB_breakpoint_t * current_breakpoint(ARGIN(PDB_t * pdb))
        __attribute__nonnull__(1);

static void debugger_cmdline(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PARROT_OBSERVER
static STRING * GDB_P(PARROT_INTERP, ARGIN(const char *s))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PARROT_OBSERVER
static STRING * GDB_print_reg(PARROT_INTERP, int t, int n)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static const DebuggerCmd * get_cmd(ARGIN_NULLOK(const char **cmd));

PARROT_WARN_UNUSED_RESULT
static unsigned long get_uint(ARGMOD(const char **cmd), unsigned int def)
        __attribute__nonnull__(1)
        FUNC_MODIFIES(*cmd);

PARROT_WARN_UNUSED_RESULT
static unsigned long get_ulong(ARGMOD(const char **cmd), unsigned long def)
        __attribute__nonnull__(1)
        FUNC_MODIFIES(*cmd);

static void list_breakpoints(ARGIN(PDB_t *pdb))
        __attribute__nonnull__(1);

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static const char * nextarg(ARGIN_NULLOK(const char *command));

static void no_such_register(PARROT_INTERP,
    char register_type,
    UINTVAL register_num)
        __attribute__nonnull__(1);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static const char * parse_int(ARGIN(const char *str), ARGOUT(int *intP))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*intP);

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static const char* parse_key(PARROT_INTERP,
    ARGIN(const char *str),
    ARGOUT(PMC **keyP))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*keyP);

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static const char * parse_string(PARROT_INTERP,
    ARGIN(const char *str),
    ARGOUT(STRING **strP))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*strP);

PARROT_CANNOT_RETURN_NULL
static const char * skip_command(ARGIN(const char *str))
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static const char * skip_whitespace(ARGIN(const char *cmd))
        __attribute__nonnull__(1);

#define ASSERT_ARGS_chop_newline __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(buf))
#define ASSERT_ARGS_close_script_file __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_condition_regtype __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(cmd))
#define ASSERT_ARGS_current_breakpoint __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(pdb))
#define ASSERT_ARGS_debugger_cmdline __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_GDB_P __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(s))
#define ASSERT_ARGS_GDB_print_reg __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_get_cmd __attribute__unused__ int _ASSERT_ARGS_CHECK = (0)
#define ASSERT_ARGS_get_uint __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(cmd))
#define ASSERT_ARGS_get_ulong __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(cmd))
#define ASSERT_ARGS_list_breakpoints __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(pdb))
#define ASSERT_ARGS_nextarg __attribute__unused__ int _ASSERT_ARGS_CHECK = (0)
#define ASSERT_ARGS_no_such_register __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_parse_int __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(str) \
    , PARROT_ASSERT_ARG(intP))
#define ASSERT_ARGS_parse_key __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(str) \
    , PARROT_ASSERT_ARG(keyP))
#define ASSERT_ARGS_parse_string __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(str) \
    , PARROT_ASSERT_ARG(strP))
#define ASSERT_ARGS_skip_command __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(str))
#define ASSERT_ARGS_skip_whitespace __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(cmd))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

/*
 *  Command functions and help dispatch
 */

typedef void (* debugger_func_t)(PDB_t * pdb, const char * cmd);

static int nomoreargs(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    if (*skip_whitespace(cmd) == '\0')
        return 1;
    else {
        Parrot_io_eprintf(pdb->debugger, "Spurious arg\n");
        return 0;
    }
}

static void dbg_assign(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_assign");

    PDB_assign(pdb->debugee, cmd);
}

static void dbg_break(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_break");

    PDB_set_break(pdb->debugee, cmd);
}

static void dbg_continue(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_continue");

    PDB_continue(pdb->debugee, cmd);
}

static void dbg_delete(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_delete");

    PDB_delete_breakpoint(pdb->debugee, cmd);
}

static void dbg_disable(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_disable");

    PDB_disable_breakpoint(pdb->debugee, cmd);
}

static void dbg_disassemble(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_disassemble");

    PDB_disassemble(pdb->debugee, cmd);
}

static void dbg_echo(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_echo");

    if (! nomoreargs(pdb, cmd))
        return;

    if (pdb->state & PDB_ECHO) {
        TRACEDEB_MSG("Disabling echo");
        pdb->state &= ~PDB_ECHO;
    }
    else {
        TRACEDEB_MSG("Enabling echo");
        pdb->state |= PDB_ECHO;
    }
}

static void dbg_enable(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    PDB_enable_breakpoint(pdb->debugee, cmd);
}

static void dbg_eval(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    PDB_eval(pdb->debugee, cmd);
}

static void dbg_gcdebug(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_gcdebug");

    if (! nomoreargs(pdb, cmd))
        return;

    if (pdb->state & PDB_GCDEBUG) {
        TRACEDEB_MSG("Disabling gcdebug mode");
        pdb->state &= ~PDB_GCDEBUG;
    }
    else {
        TRACEDEB_MSG("Enabling gcdebug mode");
        pdb->state |= PDB_GCDEBUG;
    }
}

static void dbg_help(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_help");

    PDB_help(pdb->debugee, cmd);
}

static void dbg_info(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_info");

    if (! nomoreargs(pdb, cmd))
        return;

    PDB_info(pdb->debugger);
}

static void dbg_list(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_list");

    PDB_list(pdb->debugee, cmd);
}

static void dbg_listbreakpoints(PDB_t * pdb, SHIM(const char * cmd)) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_list");

    list_breakpoints(pdb);
}

static void dbg_load(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_load");

    PDB_load_source(pdb->debugee, cmd);
}

static void dbg_next(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_next");

    PDB_next(pdb->debugee, cmd);
}

static void dbg_print(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_print");

    PDB_print(pdb->debugee, cmd);
}

static void dbg_quit(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_quit");

    if (! nomoreargs(pdb, cmd))
        return;

    pdb->state |= PDB_EXIT;
    pdb->state &= ~PDB_STOPPED;
}

static void dbg_run(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_run");

    PDB_init(pdb->debugee, cmd);
    PDB_continue(pdb->debugee, NULL);
}

static void dbg_script(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_script");

    PDB_script_file(pdb->debugee, cmd);
}

static void dbg_stack(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_stack");

    if (! nomoreargs(pdb, cmd))
        return;

    PDB_backtrace(pdb->debugee);
}

static void dbg_trace(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_trace");

    PDB_trace(pdb->debugee, cmd);
}

static void dbg_watch(PDB_t * pdb, const char * cmd) /* HEADERIZER SKIP */
{
    TRACEDEB_MSG("dbg_watch");

    PDB_watchpoint(pdb->debugee, cmd);
}

struct DebuggerCmd {
    debugger_func_t func;
    PARROT_OBSERVER const char * const shorthelp;
    PARROT_OBSERVER const char * const help;
};

static const DebuggerCmd
    cmd_assign = {
        & dbg_assign,
        "assign to a register",
"Assign a value to a register. For example:\n\
    a I0 42\n\
    a N1 3.14\n\
The first command sets I0 to 42 and the second sets N1 to 3.14."
    },
    cmd_break = {
        & dbg_break,
        "add a breakpoint",
"Set a breakpoint at a given line number (which must be specified).\n\n\
Optionally, specify a condition, in which case the breakpoint will only\n\
activate if the condition is met. Conditions take the form:\n\n\
           if [REGISTER] [COMPARISON] [REGISTER or CONSTANT]\n\n\
\
For example:\n\n\
           break 10 if I4 > I3\n\n\
           break 45 if S1 == \"foo\"\n\n\
The command returns a number which is the breakpoint identifier."
    },
    cmd_continue = {
        & dbg_continue,
        "continue the program execution",
"Continue the program execution.\n\n\
Without arguments, the program runs until a breakpoint is found\n\
(or until the program terminates for some other reason).\n\n\
If a number is specified, then skip that many breakpoints.\n\n\
If the program has terminated, then \"continue\" will do nothing;\n\
use \"run\" to re-run the program."
    },
    cmd_delete = {
        & dbg_delete,
        "delete a breakpoint",
"Delete a breakpoint.\n\n\
The breakpoint to delete must be specified by its breakpoint number.\n\
Deleted breakpoints are gone completely. If instead you want to\n\
temporarily disable a breakpoint, use \"disable\"."
    },
    cmd_disable = {
        & dbg_disable,
        "disable a breakpoint",
"Disable a breakpoint.\n\n\
The breakpoint to disable must be specified by its breakpoint number.\n\
Disabled breakpoints are not forgotten, but have no effect until re-enabled\n\
with the \"enable\" command."
    },
    cmd_disassemble = {
        & dbg_disassemble,
        "disassemble the bytecode",
"Disassemble code"
    },
    cmd_echo = {
        & dbg_echo,
        "toggle echo of script commands",
"Toggle echo mode.\n\n\
In echo mode the script commands are written to stderr before executing."
    },
    cmd_enable = {
        & dbg_enable,
        "reenable a disabled breakpoint",
"Re-enable a disabled breakpoint."
    },
    cmd_eval = {
        & dbg_eval,
        "run an instruction",
"No documentation yet"
    },
    cmd_gcdebug = {
        & dbg_gcdebug,
        "toggle gcdebug mode",
"Toggle gcdebug mode.\n\n\
In gcdebug mode a garbage collection cycle is run before each opcocde,\n\
same as using the gcdebug core."
    },
    cmd_help = {
        & dbg_help,
        "print this help",
"Print a list of available commands."
    },
    cmd_info = {
        & dbg_info,
        "print interpreter information",
"Print information about the current interpreter"
    },
    cmd_list = {
        & dbg_list,
        "list the source code file",
"List the source code.\n\n\
Optionally specify the line number to begin the listing from and the number\n\
of lines to display."
    },
    cmd_listbreakpoints = {
        & dbg_listbreakpoints,
        "list breakpoints",
"List breakpoints."
    },
    cmd_load = {
        & dbg_load,
        "load a source code file",
"Load a source code file."
    },
    cmd_next = {
        & dbg_next,
        "run the next instruction",
"Execute a specified number of instructions.\n\n\
If a number is specified with the command (e.g. \"next 5\"), then\n\
execute that number of instructions, unless the program reaches a\n\
breakpoint, or stops for some other reason.\n\n\
If no number is specified, it defaults to 1."
    },
    cmd_print = {
        & dbg_print,
        "print the interpreter registers",
"Print register: e.g. \"p i2\"\n\
Note that the register type is case-insensitive. If no digits appear\n\
after the register type, all registers of that type are printed."
    },
    cmd_quit = {
        & dbg_quit,
        "exit the debugger",
"Exit the debugger"
    },
    cmd_run = {
        & dbg_run,
        "run the program",
"Run (or restart) the program being debugged.\n\n\
Arguments specified after \"run\" are passed as command line arguments to\n\
the program.\n"
    },
    cmd_script = {
        & dbg_script,
        "interprets a file as user commands",
"Interprets a file s user commands.\n\
Usage:\n\
(pdb) script file.script"
    },
    cmd_stack = {
        & dbg_stack,
        "examine the stack",
"Print a stack trace of the parrot VM"
    },
    cmd_trace = {
        & dbg_trace,
        "trace the next instruction",
"Similar to \"next\", but prints additional trace information.\n\
This is the same as the information you get when running Parrot with\n\
the -t option.\n"
    },
    cmd_watch = {
        & dbg_watch,
        "add a watchpoint",
"Add a watchpoint"
    };

struct DebuggerCmdList {
    PARROT_OBSERVER const char * const name;
    char shortname;
    PARROT_OBSERVER const DebuggerCmd * const cmd;
};

DebuggerCmdList DebCmdList [] = {
    { "assign",      'a',  &cmd_assign },
    { "break",       '\0', &cmd_break },
    { "continue",    '\0', &cmd_continue },
    { "delete",      'd',  &cmd_delete },
    { "disable",     '\0', &cmd_disable },
    { "disassemble", '\0', &cmd_disassemble },
    { "e",           '\0', &cmd_eval },
    { "echo",        '\0', &cmd_echo },
    { "enable",      '\0', &cmd_enable },
    { "eval",        '\0', &cmd_eval },
    { "f",           '\0', &cmd_script },
    { "gcdebug",     '\0', &cmd_gcdebug },
    { "help",        '\0', &cmd_help },
    { "info",        '\0', &cmd_info },
    { "L",           '\0', &cmd_listbreakpoints },
    { "list",        'l',  &cmd_list },
    { "load",        '\0', &cmd_load },
    { "next",        '\0', &cmd_next },
    { "print",       '\0', &cmd_print },
    { "quit",        '\0', &cmd_quit },
    { "run",         '\0', &cmd_run },
    { "script",      '\0', &cmd_script },
    { "stack",       's',  &cmd_stack },
    { "trace",       '\0', &cmd_trace },
    { "watch",       '\0', &cmd_watch }
};

/*

=item C<static const DebuggerCmd * get_cmd(const char **cmd)>

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static const DebuggerCmd *
get_cmd(ARGIN_NULLOK(const char **cmd))
{
    ASSERT_ARGS(get_cmd)
    if (cmd && *cmd) {
        const char * const start = skip_whitespace(*cmd);
        const char *next = start;
        char c;
        unsigned int i, l;
        int found = -1;
        int hits = 0;

        *cmd = start;
        for (; (c= *next) != '\0' && !isspace((unsigned char)c); ++next)
            continue;
        l = next - start;
        if (l == 0)
            return NULL;
        for (i= 0; i < sizeof (DebCmdList) / sizeof (DebuggerCmdList); ++i) {
            const DebuggerCmdList * const cmdlist = DebCmdList + i;
            if (l == 1 && cmdlist->shortname == (*cmd)[0]) {
                hits = 1;
                found = i;
                break;
            }
            if (strncmp(*cmd, cmdlist->name, l) == 0) {
                if (strlen(cmdlist->name) == l) {
                    hits = 1;
                    found = i;
                    break;
                }
                else {
                    ++hits;
                    found = i;
                }
            }
        }
        if (hits == 1) {
            *cmd = skip_whitespace(next);
            return DebCmdList[found].cmd;
        }
    }
    return NULL;
}

/*

=item C<static const char * skip_whitespace(const char *cmd)>

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static const char *
skip_whitespace(ARGIN(const char *cmd))
{
    ASSERT_ARGS(skip_whitespace)
    while (*cmd && isspace((unsigned char)*cmd))
        ++cmd;
    return cmd;
}

/*

=item C<static unsigned long get_uint(const char **cmd, unsigned int def)>

=cut

*/


PARROT_WARN_UNUSED_RESULT
static unsigned long
get_uint(ARGMOD(const char **cmd), unsigned int def)
{
    ASSERT_ARGS(get_uint)
    char *cmdnext;
    unsigned int result = strtoul(skip_whitespace(* cmd), & cmdnext, 0);
    if (cmdnext != *cmd)
        *cmd = cmdnext;
    else
        result = def;
    return result;
}

/*

=item C<static unsigned long get_ulong(const char **cmd, unsigned long def)>

=cut

*/


PARROT_WARN_UNUSED_RESULT
static unsigned long
get_ulong(ARGMOD(const char **cmd), unsigned long def)
{
    ASSERT_ARGS(get_ulong)
    char *cmdnext;
    unsigned long result = strtoul(skip_whitespace(* cmd), & cmdnext, 0);
    if (cmdnext != * cmd)
        * cmd = cmdnext;
    else
        result = def;
    return result;
}

/*

=item C<static void chop_newline(char * buf)>

If the C string argument end with a newline, delete it.

=cut

*/

static void
chop_newline(ARGMOD(char * buf))
{
    ASSERT_ARGS(chop_newline)
    const size_t l = strlen(buf);

    if (l > 0 && buf [l - 1] == '\n')
        buf [l - 1] = '\0';
}

/*

=item C<static const char * nextarg(const char *command)>

Returns the position just past the current argument in the PASM instruction
C<command>. This is not the same as C<skip_command()>, which is intended for
debugger commands. This function is used for C<eval>.

=cut

*/

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static const char *
nextarg(ARGIN_NULLOK(const char *command))
{
    ASSERT_ARGS(nextarg)
    /* as long as the character pointed to by command is not NULL,
     * and it is either alphanumeric, a comma or a closing bracket,
     * continue looking for the next argument.
     */
    if (command) {
        while (isalnum((unsigned char) *command) || *command == ',' || *command == ']')
            command++;

        /* eat as much space as possible */
        command = skip_whitespace(command);
    }

    return command;
}

/*

=item C<static const char * skip_command(const char *str)>

Returns the pointer past the current debugger command. (This is an
alternative to the C<skip_command()> macro above.)

=cut

*/

PARROT_CANNOT_RETURN_NULL
static const char *
skip_command(ARGIN(const char *str))
{
    ASSERT_ARGS(skip_command)
    /* while str is not null and it contains a command (no spaces),
     * skip the character
     */
    while (*str && !isspace((unsigned char) *str))
        str++;

    /* eat all space after that */
    return skip_whitespace(str);
}

/*

=item C<static const char * parse_int(const char *str, int *intP)>

Parse an C<int> out of a string and return a pointer to just after the C<int>.
The output parameter C<intP> contains the parsed value.

=cut

*/

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static const char *
parse_int(ARGIN(const char *str), ARGOUT(int *intP))
{
    ASSERT_ARGS(parse_int)
    char *end;

    *intP = strtol(str, &end, 0);

    return end;
}

/*

=item C<static const char * parse_string(PARROT_INTERP, const char *str, STRING
**strP)>

Parse a double-quoted string out of a C string and return a pointer to
just after the string. The parsed string is converted to a Parrot
C<STRING> and placed in the output parameter C<strP>.

=cut

*/

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static const char *
parse_string(PARROT_INTERP, ARGIN(const char *str), ARGOUT(STRING **strP))
{
    ASSERT_ARGS(parse_string)
    const char *string_start;

    /* if this is not a quoted string, there's nothing to parse */
    if (*str != '"')
        return NULL;

    /* skip the quote */
    str++;

    string_start = str;

    /* parse while there's no closing quote */
    while (*str && *str != '"') {
        /* skip any potentially escaped quotes */
        if (*str == '\\' && str[1])
            str += 2;
        else
            str++;
    }

    /* create the output STRING */
    *strP = string_make(interp, string_start, (UINTVAL)(str - string_start),
        NULL, 0);

    /* skip the closing quote */
    if (*str)
        str++;

    return str;
}

/*

=item C<static const char* parse_key(PARROT_INTERP, const char *str, PMC
**keyP)>

Parse an aggregate key out of a string and return a pointer to just
after the key. Currently only string and integer keys are allowed.

=cut

*/

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static const char*
parse_key(PARROT_INTERP, ARGIN(const char *str), ARGOUT(PMC **keyP))
{
    ASSERT_ARGS(parse_key)
    /* clear output parameter */
    *keyP = NULL;

    /* make sure it's a key */
    if (*str != '[')
        return NULL;

    /* Skip [ */
    str++;

    /* if this is a string key, create a Parrot STRING */
    if (*str == '"') {
        STRING *parrot_string;
        str   = parse_string(interp, str, &parrot_string);
        *keyP = key_new_string(interp, parrot_string);
    }
    /* if this is a numeric key */
    else if (isdigit((unsigned char) *str)) {
        int value;
        str   = parse_int(str, &value);
        *keyP = key_new_integer(interp, (INTVAL) value);
    }
    /* unsupported case; neither a string nor a numeric key */
    else {
        return NULL;
    }

    /* hm, but if this doesn't match, it's probably an error */
    /* XXX str can be NULL from parse_string() */
    if (*str != ']')
        return NULL;

    /* skip the closing brace on the key */
    return ++str;
}

/*

=item C<static void debugger_cmdline(PARROT_INTERP)>

Debugger command line.

Gets and executes commands, looping until the debugger state
is changed, either to exit or to start executing code.

=cut

*/

static void
debugger_cmdline(PARROT_INTERP)
{
    ASSERT_ARGS(debugger_cmdline)
    TRACEDEB_MSG("debugger_cmdline");

    /*while (!(interp->pdb->state & PDB_EXIT)) {*/
    while (interp->pdb->state & PDB_STOPPED) {
        const char * command;
        interp->pdb->state &= ~PDB_TRACING;
        PDB_get_command(interp);
        command = interp->pdb->cur_command;
        if (command[0] == '\0')
            command = interp->pdb->last_command;

        PDB_run_command(interp, command);
    }
    TRACEDEB_MSG("debugger_cmdline finished");
}

/*

=item C<static void close_script_file(PARROT_INTERP)>

Close the script file, returning to command prompt mode.

=cut

*/

static void
close_script_file(PARROT_INTERP)
{
    ASSERT_ARGS(close_script_file)
    TRACEDEB_MSG("Closing debugger script file");
    if (interp->pdb->script_file) {
        fclose(interp->pdb->script_file);
        interp->pdb->script_file = NULL;
        interp->pdb->state|= PDB_STOPPED;
        interp->pdb->last_command[0] = '\0';
        interp->pdb->cur_command[0] = '\0';
    }
}

/*

=item C<void Parrot_debugger_init(PARROT_INTERP)>

Initializes the Parrot debugger, if it's not already initialized.

=cut

*/

PARROT_EXPORT
void
Parrot_debugger_init(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_debugger_init)
    TRACEDEB_MSG("Parrot_debugger_init");

    if (! interp->pdb) {
        PDB_t          *pdb      = mem_allocate_zeroed_typed(PDB_t);
        Parrot_Interp   debugger = Parrot_new(interp);
        interp->pdb              = pdb;
        debugger->pdb            = pdb;
        pdb->debugee             = interp;
        pdb->debugger            = debugger;

        /* Allocate space for command line buffers, NUL terminated c strings */
        pdb->cur_command = (char *)mem_sys_allocate_zeroed(DEBUG_CMD_BUFFER_LENGTH + 1);
        pdb->last_command = (char *)mem_sys_allocate_zeroed(DEBUG_CMD_BUFFER_LENGTH + 1);
        pdb->file = mem_allocate_zeroed_typed(PDB_file_t);
    }

    /* PDB_disassemble(interp, NULL); */

    interp->pdb->state     |= PDB_RUNNING;
}

/*

=item C<void Parrot_debugger_destroy(PARROT_INTERP)>

Destroy the current Parrot debugger instance.

=cut

*/

PARROT_EXPORT
void
Parrot_debugger_destroy(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_debugger_destroy)
    /* Unfinished.
       Free all debugger allocated resources.
     */
    PDB_t *pdb = interp->pdb;

    TRACEDEB_MSG("Parrot_debugger_destroy");

    PARROT_ASSERT(pdb);
    PARROT_ASSERT(pdb->debugee == interp);

    mem_sys_free(pdb->last_command);
    mem_sys_free(pdb->cur_command);

    mem_sys_free(pdb);
    interp->pdb = NULL;
}

/*

=item C<void Parrot_debugger_load(PARROT_INTERP, STRING *filename)>

Loads a Parrot source file for the current program.

=cut

*/

PARROT_EXPORT
void
Parrot_debugger_load(PARROT_INTERP, ARGIN_NULLOK(STRING *filename))
{
    ASSERT_ARGS(Parrot_debugger_load)
    char *file;

    TRACEDEB_MSG("Parrot_debugger_load");

    if (!interp->pdb)
        Parrot_ex_throw_from_c_args(interp, NULL, 0, "No debugger");

    file = Parrot_str_to_cstring(interp, filename);
    PDB_load_source(interp, file);
    Parrot_str_free_cstring(file);
}

/*

=item C<void Parrot_debugger_start(PARROT_INTERP, opcode_t * cur_opcode)>

Start debugger.

=cut

*/

PARROT_EXPORT
void
Parrot_debugger_start(PARROT_INTERP, ARGIN(opcode_t * cur_opcode))
{
    ASSERT_ARGS(Parrot_debugger_start)
    TRACEDEB_MSG("Parrot_debugger_start");

    if (!interp->pdb)
        Parrot_ex_throw_from_c_args(interp, NULL, 0, "No debugger");

    interp->pdb->cur_opcode = interp->code->base.data;

    if (interp->pdb->state & PDB_ENTER) {
        if (!interp->pdb->file) {
            /* PDB_disassemble(interp, NULL); */
        }
        interp->pdb->state &= ~PDB_ENTER;
    }

    interp->pdb->cur_opcode = cur_opcode;

    interp->pdb->state |= PDB_STOPPED;

    debugger_cmdline(interp);

    if (interp->pdb->state & PDB_EXIT) {
        TRACEDEB_MSG("Parrot_debugger_start Parrot_exit");
        Parrot_exit(interp, 0);
    }
    TRACEDEB_MSG("Parrot_debugger_start ends");
}

/*

=item C<void Parrot_debugger_break(PARROT_INTERP, opcode_t * cur_opcode)>

Breaks execution and drops into the debugger.  If we are already into the
debugger and it is the first call, set a breakpoint.

When you re run/continue the program being debugged it will pay no attention to
the debug ops.

=cut

*/

PARROT_EXPORT
void
Parrot_debugger_break(PARROT_INTERP, ARGIN(opcode_t * cur_opcode))
{
    ASSERT_ARGS(Parrot_debugger_break)
    TRACEDEB_MSG("Parrot_debugger_break");

    if (!interp->pdb)
        Parrot_ex_throw_from_c_args(interp, NULL, 0, "No debugger");

    if (!interp->pdb->file)
        Parrot_ex_throw_from_c_args(interp, NULL, 0, "No file loaded to debug");

    if (!(interp->pdb->state & PDB_BREAK)) {
        TRACEDEB_MSG("Parrot_debugger_break - in BREAK state");
        new_runloop_jump_point(interp);
        if (setjmp(interp->current_runloop->resume)) {
            fprintf(stderr, "Unhandled exception in debugger\n");
            return;
        }

        interp->pdb->state     |= PDB_BREAK;
        interp->pdb->state     |= PDB_STOPPED;
        interp->pdb->cur_opcode = (opcode_t *)cur_opcode + 1;

        /*PDB_set_break(interp, NULL);*/

        debugger_cmdline(interp);
    }
    else {
        interp->pdb->cur_opcode = (opcode_t *)cur_opcode + 1;
        /*PDB_set_break(interp, NULL);*/
    }
    TRACEDEB_MSG("Parrot_debugger_break done");
}

/*

=item C<void PDB_get_command(PARROT_INTERP)>

Get a command from the user input to execute.

It saves the last command executed (in C<< pdb->last_command >>), so it
first frees the old one and updates it with the current one.

Also prints the next line to run if the program is still active.

The user input can't be longer than DEBUG_CMD_BUFFER_LENGTH characters.

The input is saved in C<< pdb->cur_command >>.

=cut

*/

void
PDB_get_command(PARROT_INTERP)
{
    ASSERT_ARGS(PDB_get_command)
    unsigned int  i;
    int           ch;
    char         *c;
    PDB_t        * const pdb = interp->pdb;

/***********************************
   **** Testing ****
   Do not delete yet
   the commented out
   parts
***********************************/

    /* flush the buffered data */
    fflush(stdout);

    TRACEDEB_MSG("PDB_get_command");

    PARROT_ASSERT(pdb->last_command);
    PARROT_ASSERT(pdb->cur_command);

    if (interp->pdb->script_file) {
        FILE *fd = interp->pdb->script_file;
        char buf[DEBUG_CMD_BUFFER_LENGTH+1];
        const char *ptr;

        do {
            if (fgets(buf, DEBUG_CMD_BUFFER_LENGTH, fd) == NULL) {
                close_script_file(interp);
                return;
            }
            ++pdb->script_line;
            chop_newline(buf);
#if TRACE_DEBUGGER
            fprintf(stderr, "script (%lu): '%s'\n", pdb->script_line, buf);
#endif

            /* skip spaces */
            ptr = skip_whitespace(buf);

            /* skip blank and commented lines */
       } while (*ptr == '\0' || *ptr == '#');

        if (pdb->state & PDB_ECHO)
            Parrot_io_eprintf(pdb->debugger, "[%lu %s]\n", pdb->script_line, buf);

#if TRACE_DEBUGGER
        fprintf(stderr, "(script) %s\n", buf);
#endif

        strcpy(pdb->cur_command, buf);
    }
    else {

        /* update the last command */
        if (pdb->cur_command[0] != '\0')
            strcpy(pdb->last_command, pdb->cur_command);

        i = 0;

        c = pdb->cur_command;

        /*Parrot_io_eprintf(pdb->debugger, "\n(pdb) ");*/
        Parrot_io_eprintf(pdb->debugger, "\n");

        /* skip leading whitespace */
/*
        do {
            ch = fgetc(stdin);
        } while (isspace((unsigned char)ch) && ch != '\n');
*/
        {
        Interp * interpdeb = interp->pdb->debugger;
        STRING * readline = CONST_STRING(interpdeb, "readline_interactive");
        STRING * prompt = CONST_STRING(interpdeb, "(pdb) ");
        STRING *s= Parrot_str_new(interpdeb, NULL, 0);
        PMC *tmp_stdin = Parrot_io_stdhandle(interpdeb, 0, NULL);

        Parrot_pcc_invoke_method_from_c_args(interpdeb,
            tmp_stdin, readline,
            "S->S", prompt, & s);
        {
        char * const aux = Parrot_str_to_cstring(interpdeb, s);
        strcpy(c, aux);
        Parrot_str_free_cstring(aux);
        }
        ch = '\n';
        }

        /* generate string (no more than buffer length) */
/*
        while (ch != EOF && ch != '\n' && (i < DEBUG_CMD_BUFFER_LENGTH)) {
            c[i++] = (char)ch;
            ch     = fgetc(tmp_stdin);
        }

        c[i] = '\0';
*/
        if (ch == -1)
            strcpy(c, "quit");
    }
}

/*

=item C<void PDB_script_file(PARROT_INTERP, const char *command)>

Interprets the contents of a file as user input commands

=cut

*/

PARROT_EXPORT
void
PDB_script_file(PARROT_INTERP, ARGIN(const char *command))
{
    ASSERT_ARGS(PDB_script_file)
    FILE *fd;

    TRACEDEB_MSG("PDB_script_file");

    /* If already executing a script, close it */
    close_script_file(interp);

    TRACEDEB_MSG("Opening debugger script file");

    fd = fopen(command, "r");
    if (!fd) {
        Parrot_io_eprintf(interp->pdb->debugger,
                "Error reading script file %s.\n",
                command);
        return;
    }
    interp->pdb->script_file = fd;
    interp->pdb->script_line = 0;
    TRACEDEB_MSG("PDB_script_file finished");
}

/*

=item C<int PDB_run_command(PARROT_INTERP, const char *command)>

Run a command.

Hash the command to make a simple switch calling the correct handler.

=cut

*/

PARROT_IGNORABLE_RESULT
int
PDB_run_command(PARROT_INTERP, ARGIN(const char *command))
{
    ASSERT_ARGS(PDB_run_command)
    PDB_t        * const pdb = interp->pdb;
    const DebuggerCmd *cmd;

    /* keep a pointer to the command, in case we need to report an error */

    const char * cmdline = command;

    TRACEDEB_MSG("PDB_run_command");
    cmd = get_cmd(& cmdline);

    if (cmd) {
        (* cmd->func)(pdb, cmdline);
        return 0;
    }
    else {
        if (*cmdline == '\0') {
            return 0;
        }
        else {
            Parrot_io_eprintf(pdb->debugger,
                        "Undefined command: \"%s\"", command);
            if (pdb->script_file)
                Parrot_io_eprintf(pdb->debugger, " in line %lu", pdb->script_line);
            Parrot_io_eprintf(pdb->debugger, ".  Try \"help\".");
            close_script_file(interp);
            return 1;
        }
    }
}

/*

=item C<void PDB_next(PARROT_INTERP, const char *command)>

Execute the next N operation(s).

Inits the program if needed, runs the next N >= 1 operations and stops.

=cut

*/

void
PDB_next(PARROT_INTERP, ARGIN_NULLOK(const char *command))
{
    ASSERT_ARGS(PDB_next)
    PDB_t  * const pdb = interp->pdb;
    Interp *debugee;

    TRACEDEB_MSG("PDB_next");

    /* Init the program if it's not running */
    if (!(pdb->state & PDB_RUNNING))
        PDB_init(interp, command);

    /* Get the number of operations to execute if any */
    pdb->tracing = get_ulong(& command, 1);

    /* Erase the stopped flag */
    pdb->state &= ~PDB_STOPPED;

    /* Testing use of the debugger runloop */
    #if 0

    /* Execute */
    for (; n && pdb->cur_opcode; n--)
        DO_OP(pdb->cur_opcode, pdb->debugee);

    /* Set the stopped flag */
    pdb->state |= PDB_STOPPED;

    /* If program ended */

    if (!pdb->cur_opcode)
        (void)PDB_program_end(interp);
    #endif

    debugee     = pdb->debugee;

    new_runloop_jump_point(debugee);
    if (setjmp(debugee->current_runloop->resume)) {
        Parrot_io_eprintf(pdb->debugger, "Unhandled exception while tracing\n");
        pdb->state |= PDB_STOPPED;
        return;
    }

    Parrot_runcore_switch(pdb->debugee, CONST_STRING(interp, "debugger"));

    TRACEDEB_MSG("PDB_next finished");
}

/*

=item C<void PDB_trace(PARROT_INTERP, const char *command)>

Execute the next N operations; if no number is specified, it defaults to 1.

=cut

*/

void
PDB_trace(PARROT_INTERP, ARGIN_NULLOK(const char *command))
{
    ASSERT_ARGS(PDB_trace)
    PDB_t *  const pdb = interp->pdb;
    Interp        *debugee;

    TRACEDEB_MSG("PDB_trace");

    /* if debugger is not running yet, initialize */
    /*
    if (!(pdb->state & PDB_RUNNING))
        PDB_init(interp, command);
    */

    /* get the number of ops to run, if specified */
    pdb->tracing = get_ulong(& command, 1);

    /* clear the PDB_STOPPED flag, we'll be running n ops now */
    pdb->state &= ~PDB_STOPPED;
    debugee     = pdb->debugee;

    /* execute n ops */
    new_runloop_jump_point(debugee);
    if (setjmp(debugee->current_runloop->resume)) {
        Parrot_io_eprintf(pdb->debugger, "Unhandled exception while tracing\n");
        pdb->state |= PDB_STOPPED;
        return;
    }

    pdb->state |= PDB_TRACING;
    Parrot_runcore_switch(pdb->debugee, CONST_STRING(interp, "debugger"));

    /* Clear the following when done some testing */

    /* we just stopped */
    pdb->state |= PDB_STOPPED;

    /* If program ended */
    if (!pdb->cur_opcode)
        (void)PDB_program_end(interp);
    pdb->state |= PDB_RUNNING;
    pdb->state &= ~PDB_STOPPED;

    TRACEDEB_MSG("PDB_trace finished");
}

/*

=item C<static unsigned short condition_regtype(const char *cmd)>

=cut

*/

static unsigned short
condition_regtype(ARGIN(const char *cmd))
{
    ASSERT_ARGS(condition_regtype)
    switch (*cmd) {
      case 'i':
      case 'I':
        return PDB_cond_int;
      case 'n':
      case 'N':
        return PDB_cond_num;
      case 's':
      case 'S':
        return PDB_cond_str;
      case 'p':
      case 'P':
        return PDB_cond_pmc;
      default:
        return 0;
    }
}

/*

=item C<PDB_condition_t * PDB_cond(PARROT_INTERP, const char *command)>

Analyzes a condition from the user input.

=cut

*/

PARROT_CAN_RETURN_NULL
PDB_condition_t *
PDB_cond(PARROT_INTERP, ARGIN(const char *command))
{
    ASSERT_ARGS(PDB_cond)
    PDB_condition_t *condition;
    const char      *auxcmd;
    char             str[DEBUG_CMD_BUFFER_LENGTH + 1];
    unsigned short   cond_argleft;
    unsigned short   cond_type;
    unsigned char    regleft;
    int              i, reg_number;

    TRACEDEB_MSG("PDB_cond");

    /* Return if no more arguments */
    if (!(command && *command)) {
        Parrot_io_eprintf(interp->pdb->debugger, "No condition specified\n");
        return NULL;
    }

    command = skip_whitespace(command);
#if TRACE_DEBUGGER
    fprintf(stderr, "PDB_trace: '%s'\n", command);
#endif

    cond_argleft = condition_regtype(command);

    /* get the register number */
    auxcmd = ++command;
    regleft = (unsigned char)get_uint(&command, 0);
    if (auxcmd == command) {
        Parrot_io_eprintf(interp->pdb->debugger, "Invalid register\n");
            return NULL;
    }

    /* Now the condition */
    command = skip_whitespace(command);
    switch (*command) {
      case '>':
        if (*(command + 1) == '=')
            cond_type = PDB_cond_ge;
        else
            cond_type = PDB_cond_gt;
        break;
      case '<':
        if (*(command + 1) == '=')
            cond_type = PDB_cond_le;
        else
            cond_type = PDB_cond_lt;
        break;
      case '=':
        if (*(command + 1) == '=')
            cond_type = PDB_cond_eq;
        else
            goto INV_COND;
        break;
      case '!':
        if (*(command + 1) == '=')
            cond_type = PDB_cond_ne;
        else
            goto INV_COND;
        break;
      case '\0':
        if (cond_argleft != PDB_cond_str && cond_argleft != PDB_cond_pmc) {
            Parrot_io_eprintf(interp->pdb->debugger, "Invalid null condition\n");
            return NULL;
        }
        cond_type = PDB_cond_notnull;
        break;
      default:
  INV_COND:
        Parrot_io_eprintf(interp->pdb->debugger, "Invalid condition\n");
        return NULL;
    }

    /* if there's an '=', skip it */
    if (*(command + 1) == '=')
        command += 2;
    else
        command++;

    command = skip_whitespace(command);

    /* return if no notnull condition and no more arguments */
    if (!(command && *command) && (cond_type != PDB_cond_notnull)) {
        Parrot_io_eprintf(interp->pdb->debugger, "Can't compare a register with nothing\n");
        return NULL;
    }

    /* Allocate new condition */
    condition = mem_allocate_zeroed_typed(PDB_condition_t);

    condition->type = cond_argleft | cond_type;

    if (cond_type != PDB_cond_notnull) {

        if (isalpha((unsigned char)*command)) {
            /* It's a register - we first check that it's the correct type */

            unsigned short cond_argright = condition_regtype(command);

            if (cond_argright != cond_argleft) {
                Parrot_io_eprintf(interp->pdb->debugger, "Register types don't agree\n");
                mem_sys_free(condition);
                return NULL;
            }

            /* Now we check and store the register number */
            auxcmd = ++command;
            reg_number = (int)get_uint(&command, 0);
            if (auxcmd == command) {
                Parrot_io_eprintf(interp->pdb->debugger, "Invalid register\n");
                    mem_sys_free(condition);
                    return NULL;
            }

            if (reg_number < 0) {
                Parrot_io_eprintf(interp->pdb->debugger, "Out-of-bounds register\n");
                mem_sys_free(condition);
                return NULL;
            }

            condition->value         = mem_allocate_typed(int);
            *(int *)condition->value = reg_number;
        }
        /* If the first argument was an integer */
        else if (condition->type & PDB_cond_int) {
            /* This must be either an integer constant or register */
            condition->value             = mem_allocate_typed(INTVAL);
            *(INTVAL *)condition->value  = (INTVAL)atoi(command);
            condition->type             |= PDB_cond_const;
        }
        else if (condition->type & PDB_cond_num) {
            condition->value               = mem_allocate_typed(FLOATVAL);
            *(FLOATVAL *)condition->value  = (FLOATVAL)atof(command);
            condition->type               |= PDB_cond_const;
        }
        else if (condition->type & PDB_cond_str) {
            for (i = 1; ((command[i] != '"') && (i < DEBUG_CMD_BUFFER_LENGTH)); i++)
                str[i - 1] = command[i];
            str[i - 1] = '\0';
#if TRACE_DEBUGGER
            fprintf(stderr, "PDB_break: '%s'\n", str);
#endif
            condition->value = string_make(interp, str, (UINTVAL)(i - 1),
                NULL, 0);

            condition->type |= PDB_cond_const;
        }
        else if (condition->type & PDB_cond_pmc) {
            /* TT #1259: Need to figure out what to do in this case.
             * For the time being, we just bail. */
            Parrot_io_eprintf(interp->pdb->debugger, "Can't compare PMC with constant\n");
            mem_sys_free(condition);
            return NULL;
        }

    }

    return condition;
}

/*

=item C<void PDB_watchpoint(PARROT_INTERP, const char *command)>

Set a watchpoint.

=cut

*/

void
PDB_watchpoint(PARROT_INTERP, ARGIN(const char *command))
{
    ASSERT_ARGS(PDB_watchpoint)
    PDB_t           * const pdb = interp->pdb;
    PDB_condition_t * const condition = PDB_cond(interp, command);

    if (!condition)
        return;

    /* Add it to the head of the list */
    if (pdb->watchpoint)
        condition->next = pdb->watchpoint;
    pdb->watchpoint = condition;
    fprintf(stderr, "Adding watchpoint\n");
}

/*

=item C<void PDB_set_break(PARROT_INTERP, const char *command)>

Set a break point, the source code file must be loaded.

=cut

*/

void
PDB_set_break(PARROT_INTERP, ARGIN_NULLOK(const char *command))
{
    ASSERT_ARGS(PDB_set_break)
    PDB_t            * const pdb      = interp->pdb;
    PDB_breakpoint_t *newbreak;
    PDB_breakpoint_t **lbreak;
    PDB_line_t       *line = NULL;
    long              bp_id;
    opcode_t         *breakpos = NULL;

    unsigned long ln = get_ulong(& command, 0);

    TRACEDEB_MSG("PDB_set_break");

    /* If there is a source file use line number, else opcode position */


    if (pdb->file) {
        TRACEDEB_MSG("PDB_set_break file");

        if (!pdb->file->size) {
            Parrot_io_eprintf(pdb->debugger,
                "Can't set a breakpoint in empty file\n");
            return;
        }

        /* If no line number was specified, set it at the current line */
        if (ln != 0) {
            unsigned long i;

            /* Move to the line where we will set the break point */
            line = pdb->file->line;

            for (i = 1; ((i < ln) && (line->next)); i++)
                line = line->next;

            /* Abort if the line number provided doesn't exist */
            if (line == NULL || !line->next) {
                Parrot_io_eprintf(pdb->debugger,
                    "Can't set a breakpoint at line number %li\n", ln);
                return;
            }
        }
        else {
            /* Get the line to set it */
            line = pdb->file->line;

            TRACEDEB_MSG("PDB_set_break reading ops");
            while (line->opcode != pdb->cur_opcode) {
                line = line->next;
                if (!line) {
                    Parrot_io_eprintf(pdb->debugger,
                       "No current line found and no line number specified\n");
                    return;
                }
            }
        }
        /* Skip lines that are not related to an opcode */
        while (line && !line->opcode)
            line = line->next;
        /* Abort if the line number provided doesn't exist */
        if (!line) {
            Parrot_io_eprintf(pdb->debugger,
                "Can't set a breakpoint at line number %li\n", ln);
            return;
        }

        breakpos = line->opcode;
    }
    else {
        TRACEDEB_MSG("PDB_set_break no file");
        breakpos = interp->code->base.data + ln;
    }

    TRACEDEB_MSG("PDB_set_break allocate breakpoint");
    /* Allocate the new break point */
    newbreak = mem_allocate_zeroed_typed(PDB_breakpoint_t);

    if (command) {
        /*command = skip_command(command);*/
    }
    else {
        Parrot_ex_throw_from_c_args(interp, NULL, 1,
            "NULL command passed to PDB_set_break");
    }

    /* if there is another argument to break, besides the line number,
     * it should be an 'if', so we call another handler. */
    if (command && *command) {
        command = skip_whitespace(command);
        while (! isspace((unsigned char)*command))
            ++command;
        command = skip_whitespace(command);
        newbreak->condition = PDB_cond(interp, command);
    }

    /* Set the address where to stop */
    newbreak->pc   = breakpos;

    /* No next breakpoint */
    newbreak->next = NULL;

    /* Don't skip (at least initially) */
    newbreak->skip = 0;

    /* Add the breakpoint to the end of the list */
    bp_id = 1;
    lbreak = & pdb->breakpoint;
    while (*lbreak) {
        bp_id = (*lbreak)->id + 1;
        lbreak = & (*lbreak)->next;
    }
    newbreak->prev = *lbreak;
    *lbreak = newbreak;
    newbreak->id = bp_id;

    /* Show breakpoint position */

    Parrot_io_eprintf(pdb->debugger, "Breakpoint %li at", newbreak->id);
    if (line)
        Parrot_io_eprintf(pdb->debugger, " line %li", line->number);
    Parrot_io_eprintf(pdb->debugger, " pos %li\n", newbreak->pc - interp->code->base.data);
}

/*

=item C<static void list_breakpoints(PDB_t *pdb)>

=cut

*/

static void
list_breakpoints(ARGIN(PDB_t *pdb))
{
    ASSERT_ARGS(list_breakpoints)

    PDB_breakpoint_t **lbreak;
    for (lbreak = & pdb->breakpoint; *lbreak; lbreak = & (*lbreak)->next) {
        PDB_breakpoint_t *br = *lbreak;
        Parrot_io_eprintf(pdb->debugger, "Breakpoint %li at", br->id);
        Parrot_io_eprintf(pdb->debugger, " pos %li", br->pc - pdb->debugee->code->base.data);
        if (br->skip == -1)
            Parrot_io_eprintf(pdb->debugger, " (disabled)");
        Parrot_io_eprintf(pdb->debugger, "\n");
    }
}

/*

=item C<void PDB_init(PARROT_INTERP, const char *command)>

Init the program.

=cut

*/

void
PDB_init(PARROT_INTERP, SHIM(const char *command))
{
    ASSERT_ARGS(PDB_init)
    PDB_t * const pdb = interp->pdb;

    /* Restart if we are already running */
    if (pdb->state & PDB_RUNNING)
        Parrot_io_eprintf(pdb->debugger, "Restarting\n");

    /* Add the RUNNING state */
    pdb->state |= PDB_RUNNING;
}

/*

=item C<void PDB_continue(PARROT_INTERP, const char *command)>

Continue running the program. If a number is specified, skip that many
breakpoints.

=cut

*/

void
PDB_continue(PARROT_INTERP, ARGIN_NULLOK(const char *command))
{
    ASSERT_ARGS(PDB_continue)
    PDB_t * const pdb = interp->pdb;
    unsigned long ln = 0;

    TRACEDEB_MSG("PDB_continue");

    /* Skip any breakpoint? */
    if (command)
        ln = get_ulong(& command, 0);

    if (ln != 0) {
        if (!pdb->breakpoint) {
            Parrot_io_eprintf(pdb->debugger, "No breakpoints to skip\n");
            return;
        }

        PDB_skip_breakpoint(interp, ln);
    }

    /* Run while no break point is reached */
    /*
    while (!PDB_break(interp))
        DO_OP(pdb->cur_opcode, pdb->debugee);
    */

    #if 0
    pdb->tracing           = 0;
    Parrot_runcore_switch(pdb->debugee, CONST_STRING(interp, "debugger"));

    new_internal_exception(pdb->debugee);
    if (setjmp(pdb->debugee->exceptions->destination)) {
        Parrot_io_eprintf(pdb->debugee, "Unhandled exception while debugging: %Ss\n",
            pdb->debugee->exceptions->msg);
        pdb->state |= PDB_STOPPED;
        return;
    }
    runops_int(pdb->debugee, pdb->debugee->code->base.data - pdb->cur_opcode);
    if (!pdb->cur_opcode)
        (void)PDB_program_end(interp);
    #endif
    pdb->state |= PDB_RUNNING;
    pdb->state &= ~PDB_BREAK;
    pdb->state &= ~PDB_STOPPED;
}

/*

=item C<PDB_breakpoint_t * PDB_find_breakpoint(PARROT_INTERP, const char
*command)>

Find breakpoint number N; returns C<NULL> if the breakpoint doesn't
exist or if no breakpoint was specified.

=cut

*/

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PDB_breakpoint_t *
PDB_find_breakpoint(PARROT_INTERP, ARGIN(const char *command))
{
    ASSERT_ARGS(PDB_find_breakpoint)
    const char *oldcmd = command;
    const unsigned long n = get_ulong(&command, 0);
    if (command != oldcmd) {
        PDB_breakpoint_t *breakpoint = interp->pdb->breakpoint;

        while (breakpoint && breakpoint->id != n)
            breakpoint = breakpoint->next;

        if (!breakpoint) {
            Parrot_io_eprintf(interp->pdb->debugger, "No breakpoint number %ld", n);
            return NULL;
        }

        return breakpoint;
    }
    else {
        /* Report an appropriate error */
        if (*command)
            Parrot_io_eprintf(interp->pdb->debugger, "Not a valid breakpoint");
        else
            Parrot_io_eprintf(interp->pdb->debugger, "No breakpoint specified");

        return NULL;
    }
}

/*

=item C<void PDB_disable_breakpoint(PARROT_INTERP, const char *command)>

Disable a breakpoint; it can be reenabled with the enable command.

=cut

*/

void
PDB_disable_breakpoint(PARROT_INTERP, ARGIN(const char *command))
{
    ASSERT_ARGS(PDB_disable_breakpoint)
    PDB_breakpoint_t * const breakpoint = PDB_find_breakpoint(interp, command);

    /* if the breakpoint exists, disable it. */
    if (breakpoint)
        breakpoint->skip = -1;
}

/*

=item C<void PDB_enable_breakpoint(PARROT_INTERP, const char *command)>

Reenable a disabled breakpoint; if the breakpoint was not disabled, has
no effect.

=cut

*/

void
PDB_enable_breakpoint(PARROT_INTERP, ARGIN(const char *command))
{
    ASSERT_ARGS(PDB_enable_breakpoint)
    PDB_breakpoint_t * const breakpoint = PDB_find_breakpoint(interp, command);

    /* if the breakpoint exists, and it was disabled, enable it. */
    if (breakpoint && breakpoint->skip == -1)
        breakpoint->skip = 0;
}

/*

=item C<void PDB_delete_breakpoint(PARROT_INTERP, const char *command)>

Delete a breakpoint.

=cut

*/

void
PDB_delete_breakpoint(PARROT_INTERP, ARGIN(const char *command))
{
    ASSERT_ARGS(PDB_delete_breakpoint)
    PDB_breakpoint_t * const breakpoint = PDB_find_breakpoint(interp, command);
    const PDB_line_t *line;
    long bp_id;

    if (breakpoint) {
        if (!interp->pdb->file)
            Parrot_ex_throw_from_c_args(interp, NULL, 0, "No file loaded");

        line = interp->pdb->file->line;
        while (line->opcode != breakpoint->pc)
            line = line->next;

        /* Delete the condition structure, if there is one */
        if (breakpoint->condition) {
            PDB_delete_condition(interp, breakpoint);
            breakpoint->condition = NULL;
        }

        /* Remove the breakpoint from the list */
        if (breakpoint->prev && breakpoint->next) {
            breakpoint->prev->next = breakpoint->next;
            breakpoint->next->prev = breakpoint->prev;
        }
        else if (breakpoint->prev && !breakpoint->next) {
            breakpoint->prev->next = NULL;
        }
        else if (!breakpoint->prev && breakpoint->next) {
            breakpoint->next->prev  = NULL;
            interp->pdb->breakpoint = breakpoint->next;
        }
        else {
            interp->pdb->breakpoint = NULL;
        }
        bp_id = breakpoint->id;
        /* Kill the breakpoint */
        mem_sys_free(breakpoint);

        Parrot_io_eprintf(interp->pdb->debugger, "Breakpoint %li deleted\n", bp_id);
    }
}

/*

=item C<void PDB_delete_condition(PARROT_INTERP, PDB_breakpoint_t *breakpoint)>

Delete a condition associated with a breakpoint.

=cut

*/

void
PDB_delete_condition(SHIM_INTERP, ARGMOD(PDB_breakpoint_t *breakpoint))
{
    ASSERT_ARGS(PDB_delete_condition)
    if (breakpoint->condition->value) {
        if (breakpoint->condition->type & PDB_cond_str) {
            /* 'value' is a string, so we need to be careful */
            PObj_external_CLEAR((STRING*)breakpoint->condition->value);
            PObj_on_free_list_SET((STRING*)breakpoint->condition->value);
            /* it should now be properly garbage collected after
               we destroy the condition */
        }
        else {
            /* 'value' is a float or an int, so we can just free it */
            mem_sys_free(breakpoint->condition->value);
            breakpoint->condition->value = NULL;
        }
    }

    mem_sys_free(breakpoint->condition);
    breakpoint->condition = NULL;
}

/*

=item C<void PDB_skip_breakpoint(PARROT_INTERP, unsigned long i)>

Skip C<i> times all breakpoints.

=cut

*/

void
PDB_skip_breakpoint(PARROT_INTERP, unsigned long i)
{
    ASSERT_ARGS(PDB_skip_breakpoint)
#if TRACE_DEBUGGER
        fprintf(stderr, "PDB_skip_breakpoint: %li\n", i);
#endif

    interp->pdb->breakpoint_skip = i;
}

/*

=item C<char PDB_program_end(PARROT_INTERP)>

End the program.

=cut

*/

char
PDB_program_end(PARROT_INTERP)
{
    ASSERT_ARGS(PDB_program_end)
    PDB_t * const pdb = interp->pdb;

    TRACEDEB_MSG("PDB_program_end");

    /* Remove the RUNNING state */
    pdb->state &= ~PDB_RUNNING;

    Parrot_io_eprintf(pdb->debugger, "Program exited.\n");
    return 1;
}

/*

=item C<char PDB_check_condition(PARROT_INTERP, const PDB_condition_t
*condition)>

Returns true if the condition was met.

=cut

*/

PARROT_WARN_UNUSED_RESULT
char
PDB_check_condition(PARROT_INTERP, ARGIN(const PDB_condition_t *condition))
{
    ASSERT_ARGS(PDB_check_condition)
    PMC *ctx = CURRENT_CONTEXT(interp);

    TRACEDEB_MSG("PDB_check_condition");

    PARROT_ASSERT(ctx);

    if (condition->type & PDB_cond_int) {
        INTVAL   i,  j;
        if (condition->reg >= Parrot_pcc_get_regs_used(interp, ctx, REGNO_INT))
            return 0;
        i = CTX_REG_INT(ctx, condition->reg);

        if (condition->type & PDB_cond_const)
            j = *(INTVAL *)condition->value;
        else
            j = REG_INT(interp, *(int *)condition->value);

        if (((condition->type & PDB_cond_gt) && (i >  j)) ||
            ((condition->type & PDB_cond_ge) && (i >= j)) ||
            ((condition->type & PDB_cond_eq) && (i == j)) ||
            ((condition->type & PDB_cond_ne) && (i != j)) ||
            ((condition->type & PDB_cond_le) && (i <= j)) ||
            ((condition->type & PDB_cond_lt) && (i <  j)))
                return 1;

        return 0;
    }
    else if (condition->type & PDB_cond_num) {
        FLOATVAL k,  l;

        if (condition->reg >= Parrot_pcc_get_regs_used(interp, ctx, REGNO_NUM))
            return 0;
        k = CTX_REG_NUM(ctx, condition->reg);

        if (condition->type & PDB_cond_const)
            l = *(FLOATVAL *)condition->value;
        else
            l = REG_NUM(interp, *(int *)condition->value);

        if (((condition->type & PDB_cond_gt) && (k >  l)) ||
            ((condition->type & PDB_cond_ge) && (k >= l)) ||
            ((condition->type & PDB_cond_eq) && (k == l)) ||
            ((condition->type & PDB_cond_ne) && (k != l)) ||
            ((condition->type & PDB_cond_le) && (k <= l)) ||
            ((condition->type & PDB_cond_lt) && (k <  l)))
                return 1;

        return 0;
    }
    else if (condition->type & PDB_cond_str) {
        STRING  *m, *n;

        if (condition->reg >= Parrot_pcc_get_regs_used(interp, ctx, REGNO_STR))
            return 0;
        m = CTX_REG_STR(ctx, condition->reg);

        if (condition->type & PDB_cond_notnull)
            return ! STRING_IS_NULL(m);

        if (condition->type & PDB_cond_const)
            n = (STRING *)condition->value;
        else
            n = REG_STR(interp, *(int *)condition->value);

        if (((condition->type & PDB_cond_gt) &&
                (Parrot_str_compare(interp, m, n) >  0)) ||
            ((condition->type & PDB_cond_ge) &&
                (Parrot_str_compare(interp, m, n) >= 0)) ||
            ((condition->type & PDB_cond_eq) &&
                (Parrot_str_compare(interp, m, n) == 0)) ||
            ((condition->type & PDB_cond_ne) &&
                (Parrot_str_compare(interp, m, n) != 0)) ||
            ((condition->type & PDB_cond_le) &&
                (Parrot_str_compare(interp, m, n) <= 0)) ||
            ((condition->type & PDB_cond_lt) &&
                (Parrot_str_compare(interp, m, n) <  0)))
                    return 1;

        return 0;
    }
    else if (condition->type & PDB_cond_pmc) {
        PMC *m;

        if (condition->reg >= Parrot_pcc_get_regs_used(interp, ctx, REGNO_PMC))
            return 0;
        m = CTX_REG_PMC(ctx, condition->reg);

        if (condition->type & PDB_cond_notnull)
            return ! PMC_IS_NULL(m);
        return 0;
    }
    else
        return 0;
}

/*

=item C<static PDB_breakpoint_t * current_breakpoint(PDB_t * pdb)>

Returns a pointer to the breakpoint at the current position,
or NULL if there is none.

=cut

*/

PARROT_CAN_RETURN_NULL
static PDB_breakpoint_t *
current_breakpoint(ARGIN(PDB_t * pdb))
{
    ASSERT_ARGS(current_breakpoint)
    PDB_breakpoint_t *breakpoint = pdb->breakpoint;
    while (breakpoint) {
        if (pdb->cur_opcode == breakpoint->pc)
            break;
        breakpoint = breakpoint->next;
    }
    return breakpoint;
}

/*

=item C<char PDB_break(PARROT_INTERP)>

Returns true if we have to stop running.

=cut

*/

PARROT_WARN_UNUSED_RESULT
char
PDB_break(PARROT_INTERP)
{
    ASSERT_ARGS(PDB_break)
    PDB_t            * const pdb = interp->pdb;
    PDB_condition_t  *watchpoint = pdb->watchpoint;
    PDB_breakpoint_t *breakpoint;

/*
    TRACEDEB_MSG("PDB_break");
*/

    /* Check the watchpoints first. */
    while (watchpoint) {
        if (PDB_check_condition(interp, watchpoint)) {
            pdb->state |= PDB_STOPPED;
            return 1;
        }

        watchpoint = watchpoint->next;
    }

    /* If program ended */
    if (!pdb->cur_opcode)
        return PDB_program_end(interp);

    /* If the program is STOPPED allow it to continue */
    if (pdb->state & PDB_STOPPED) {
        pdb->state &= ~PDB_STOPPED;
        return 0;
    }

    breakpoint = current_breakpoint(pdb);
    if (breakpoint) {
        /* If we have to skip breakpoints, do so. */
        if (pdb->breakpoint_skip) {
            TRACEDEB_MSG("PDB_break skipping");
            pdb->breakpoint_skip--;
            return 0;
        }

        if (breakpoint->skip < 0)
            return 0;

        /* Check if there is a condition for this breakpoint */
        if ((breakpoint->condition) &&
            (!PDB_check_condition(interp, breakpoint->condition)))
                return 0;

        TRACEDEB_MSG("PDB_break stopping");

        /* Add the STOPPED state and stop */
        pdb->state |= PDB_STOPPED;
        return 1;
    }

    return 0;
}

/*

=item C<char * PDB_escape(const char *string, UINTVAL length)>

Escapes C<">, C<\r>, C<\n>, C<\t>, C<\a> and C<\\>.

The returned string must be freed.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PARROT_MALLOC
char *
PDB_escape(ARGIN(const char *string), UINTVAL length)
{
    ASSERT_ARGS(PDB_escape)
    const char *end;
    char       *_new, *fill;

    length = length > 20 ? 20 : length;
    end    = string + length;

    /* Return if there is no string to escape*/
    if (!string)
        return NULL;

    fill = _new = (char *)mem_sys_allocate(length * 2 + 1);

    for (; string < end; string++) {
        switch (*string) {
          case '\0':
            *(fill++) = '\\';
            *(fill++) = '0';
            break;
          case '\n':
            *(fill++) = '\\';
            *(fill++) = 'n';
            break;
          case '\r':
            *(fill++) = '\\';
            *(fill++) = 'r';
            break;
          case '\t':
            *(fill++) = '\\';
            *(fill++) = 't';
            break;
          case '\a':
            *(fill++) = '\\';
            *(fill++) = 'a';
            break;
          case '\\':
            *(fill++) = '\\';
            *(fill++) = '\\';
            break;
          case '"':
            *(fill++) = '\\';
            *(fill++) = '"';
            break;
          default:
            *(fill++) = *string;
            break;
        }
    }

    *fill = '\0';

    return _new;
}

/*

=item C<int PDB_unescape(char *string)>

Do inplace unescape of C<\r>, C<\n>, C<\t>, C<\a> and C<\\>.

=cut

*/

int
PDB_unescape(ARGMOD(char *string))
{
    ASSERT_ARGS(PDB_unescape)
    int l = 0;

    for (; *string; string++) {
        l++;

        if (*string == '\\') {
            char *fill;
            int i;

            switch (string[1]) {
              case 'n':
                *string = '\n';
                break;
              case 'r':
                *string = '\r';
                break;
              case 't':
                *string = '\t';
                break;
              case 'a':
                *string = '\a';
                break;
              case '\\':
                *string = '\\';
                break;
              default:
                continue;
            }

            fill = string;

            for (i = 1; fill[i + 1]; i++)
                fill[i] = fill[i + 1];

            fill[i] = '\0';
        }
    }

    return l;
}

/*

=item C<size_t PDB_disassemble_op(PARROT_INTERP, char *dest, size_t space, const
op_info_t *info, const opcode_t *op, PDB_file_t *file, const opcode_t
*code_start, int full_name)>

Disassembles C<op>.

=cut

*/

size_t
PDB_disassemble_op(PARROT_INTERP, ARGOUT(char *dest), size_t space,
        ARGIN(const op_info_t *info), ARGIN(const opcode_t *op),
        ARGMOD_NULLOK(PDB_file_t *file), ARGIN_NULLOK(const opcode_t *code_start),
        int full_name)
{
    ASSERT_ARGS(PDB_disassemble_op)
    int         j;
    size_t     size = 0;
    int        specialop = 0;

    /* Write the opcode name */
    const char * p = full_name ? info->full_name : info->name;

    TRACEDEB_MSG("PDB_disassemble_op");

    if (! p)
        p= "**UNKNOWN**";
    strcpy(dest, p);
    size += strlen(p);

    dest[size++] = ' ';

    /* Concat the arguments */
    for (j = 1; j < info->op_count; j++) {
        char      buf[256];
        INTVAL    i = 0;

        PARROT_ASSERT(size + 2 < space);

        switch (info->types[j - 1]) {
          case PARROT_ARG_I:
            dest[size++] = 'I';
            goto INTEGER;
          case PARROT_ARG_N:
            dest[size++] = 'N';
            goto INTEGER;
          case PARROT_ARG_S:
            dest[size++] = 'S';
            goto INTEGER;
          case PARROT_ARG_P:
            dest[size++] = 'P';
            goto INTEGER;
          case PARROT_ARG_IC:
            /* If the opcode jumps and this is the last argument,
               that means this is a label */
            if ((j == info->op_count - 1) &&
                (info->jump & PARROT_JUMP_RELATIVE)) {
                if (file) {
                    dest[size++] = 'L';
                    i            = PDB_add_label(file, op, op[j]);
                }
                else if (code_start) {
                    dest[size++] = 'O';
                    dest[size++] = 'P';
                    i            = op[j] + (op - code_start);
                }
                else {
                    if (op[j] > 0)
                        dest[size++] = '+';
                    i = op[j];
                }
            }

            /* Convert the integer to a string */
            INTEGER:
            if (i == 0)
                i = (INTVAL) op[j];

            PARROT_ASSERT(size + 20 < space);

            size += sprintf(&dest[size], INTVAL_FMT, i);

            break;
          case PARROT_ARG_NC:
            {
                /* Convert the float to a string */
                const FLOATVAL f = interp->code->const_table->constants[op[j]]->u.number;
                Parrot_snprintf(interp, buf, sizeof (buf), FLOATVAL_FMT, f);
                strcpy(&dest[size], buf);
                size += strlen(buf);
            }
            break;
          case PARROT_ARG_SC:
            dest[size++] = '"';
            if (interp->code->const_table->constants[op[j]]-> u.string->strlen) {
                char * const unescaped =
                    Parrot_str_to_cstring(interp, interp->code->
                           const_table->constants[op[j]]->u.string);
                char * const escaped =
                    PDB_escape(unescaped, interp->code->const_table->
                           constants[op[j]]->u.string->strlen);
                if (escaped) {
                    strcpy(&dest[size], escaped);
                    size += strlen(escaped);
                    mem_sys_free(escaped);
                }
                Parrot_str_free_cstring(unescaped);
            }
            dest[size++] = '"';
            break;
          case PARROT_ARG_PC:
            Parrot_snprintf(interp, buf, sizeof (buf), "PMC_CONST(%d)", op[j]);
            strcpy(&dest[size], buf);
            size += strlen(buf);
            break;
          case PARROT_ARG_K:
            dest[size - 1] = '[';
            Parrot_snprintf(interp, buf, sizeof (buf), "P" INTVAL_FMT, op[j]);
            strcpy(&dest[size], buf);
            size += strlen(buf);
            dest[size++] = ']';
            break;
          case PARROT_ARG_KC:
            {
                PMC * k = interp->code->const_table->constants[op[j]]->u.key;
                dest[size - 1] = '[';
                while (k) {
                    switch (PObj_get_FLAGS(k)) {
                      case 0:
                        break;
                      case KEY_integer_FLAG:
                        Parrot_snprintf(interp, buf, sizeof (buf),
                                    INTVAL_FMT, VTABLE_get_integer(interp, k));
                        strcpy(&dest[size], buf);
                        size += strlen(buf);
                        break;
                      case KEY_number_FLAG:
                        Parrot_snprintf(interp, buf, sizeof (buf),
                                    FLOATVAL_FMT, VTABLE_get_number(interp, k));
                        strcpy(&dest[size], buf);
                        size += strlen(buf);
                        break;
                      case KEY_string_FLAG:
                        dest[size++] = '"';
                        {
                            char * const temp = Parrot_str_to_cstring(interp,
                                    VTABLE_get_string(interp, k));
                            strcpy(&dest[size], temp);
                            Parrot_str_free_cstring(temp);
                        }
                        size += Parrot_str_byte_length(interp,
                                VTABLE_get_string(interp, (k)));
                        dest[size++] = '"';
                        break;
                      case KEY_integer_FLAG|KEY_register_FLAG:
                        Parrot_snprintf(interp, buf, sizeof (buf),
                                    "I" INTVAL_FMT, VTABLE_get_integer(interp, k));
                        strcpy(&dest[size], buf);
                        size += strlen(buf);
                        break;
                      case KEY_number_FLAG|KEY_register_FLAG:
                        Parrot_snprintf(interp, buf, sizeof (buf),
                                    "N" INTVAL_FMT, VTABLE_get_integer(interp, k));
                        strcpy(&dest[size], buf);
                        size += strlen(buf);
                        break;
                      case KEY_string_FLAG|KEY_register_FLAG:
                        Parrot_snprintf(interp, buf, sizeof (buf),
                                    "S" INTVAL_FMT, VTABLE_get_integer(interp, k));
                        strcpy(&dest[size], buf);
                        size += strlen(buf);
                        break;
                      case KEY_pmc_FLAG|KEY_register_FLAG:
                        Parrot_snprintf(interp, buf, sizeof (buf),
                                    "P" INTVAL_FMT, VTABLE_get_integer(interp, k));
                        strcpy(&dest[size], buf);
                        size += strlen(buf);
                        break;
                      default:
                        dest[size++] = '?';
                        break;
                    }
                    GETATTR_Key_next_key(interp, k, k);
                    if (k)
                        dest[size++] = ';';
                }
                dest[size++] = ']';
            }
            break;
          case PARROT_ARG_KI:
            dest[size - 1] = '[';
            Parrot_snprintf(interp, buf, sizeof (buf), "I" INTVAL_FMT, op[j]);
            strcpy(&dest[size], buf);
            size += strlen(buf);
            dest[size++] = ']';
            break;
          case PARROT_ARG_KIC:
            dest[size - 1] = '[';
            Parrot_snprintf(interp, buf, sizeof (buf), INTVAL_FMT, op[j]);
            strcpy(&dest[size], buf);
            size += strlen(buf);
            dest[size++] = ']';
            break;
          default:
            Parrot_ex_throw_from_c_args(interp, NULL, 1, "Unknown opcode type");
        }

        if (j != info->op_count - 1)
            dest[size++] = ',';
    }

    /* Special decoding for the signature used in args/returns.  Such ops have
       one fixed parameter (the signature vector), plus a varying number of
       registers/constants.  For each arg/return, we show the register and its
       flags using PIR syntax. */
    if (*(op) == PARROT_OP_set_args_pc || *(op) == PARROT_OP_set_returns_pc)
        specialop = 1;

    /* if it's a retrieving op, specialop = 2, so that later a :flat flag
     * can be changed into a :slurpy flag. See flag handling below.
     */
    if (*(op) == PARROT_OP_get_results_pc || *(op) == PARROT_OP_get_params_pc)
        specialop = 2;

    if (specialop > 0) {
        char buf[1000];
        PMC * const sig = interp->code->const_table->constants[op[1]]->u.key;
        const int n_values = VTABLE_elements(interp, sig);
        /* The flag_names strings come from Call_bits_enum_t (with which it
           should probably be colocated); they name the bits from LSB to MSB.
           The two least significant bits are not flags; they are the register
           type, which is decoded elsewhere.  We also want to show unused bits,
           which could indicate problems.
        */
        PARROT_OBSERVER const char * const flag_names[] = {
                                     "",
                                     "",
                                     " :unused004",
                                     " :unused008",
                                     " :const",
                                     " :flat", /* should be :slurpy for args */
                                     " :unused040",
                                     " :optional",
                                     " :opt_flag",
                                     " :named",
                                     NULL
        };


        /* Register decoding.  It would be good to abstract this, too. */
        PARROT_OBSERVER static const char regs[] = "ISPN";

        for (j = 0; j < n_values; j++) {
            size_t idx = 0;
            const int sig_value = VTABLE_get_integer_keyed_int(interp, sig, j);

            /* Print the register name, e.g. P37. */
            buf[idx++] = ',';
            buf[idx++] = ' ';
            buf[idx++] = regs[sig_value & PARROT_ARG_TYPE_MASK];
            Parrot_snprintf(interp, &buf[idx], sizeof (buf)-idx,
                            INTVAL_FMT, op[j+2]);
            idx = strlen(buf);

            /* Add flags, if we have any. */
            {
                int flag_idx = 0;
                int flags = sig_value;

                /* End when we run out of flags, off the end of flag_names, or
                 * get too close to the end of buf.
                 * 100 is just an estimate of all buf lengths added together.
                 */
                while (flags && idx < sizeof (buf) - 100) {
                    const char * const flag_string
                                    = (specialop == 2  && STREQ(flag_names[flag_idx], " :flat"))
                                    ? " :slurpy"
                                    : flag_names[flag_idx];

                    if (! flag_string)
                        break;
                    if (flags & 1 && *flag_string) {
                        const size_t n = strlen(flag_string);
                        strcpy(&buf[idx], flag_string);
                        idx += n;
                    }
                    flags >>= 1;
                    flag_idx++;
                }
            }

            /* Add it to dest. */
            buf[idx++] = '\0';
            strcpy(&dest[size], buf);
            size += strlen(buf);
        }
    }

    dest[size] = '\0';
    return ++size;
}

/*

=item C<void PDB_disassemble(PARROT_INTERP, const char *command)>

Disassemble the bytecode.

=cut

*/

void
PDB_disassemble(PARROT_INTERP, SHIM(const char *command))
{
    ASSERT_ARGS(PDB_disassemble)
    PDB_t    * const pdb = interp->pdb;
    opcode_t * pc        = interp->code->base.data;

    PDB_file_t  *pfile;
    PDB_line_t  *pline, *newline;
    PDB_label_t *label;
    opcode_t    *code_end;

    const unsigned int default_size = 32768;
    size_t space;  /* How much space do we have? */
    size_t size, alloced, n;

    TRACEDEB_MSG("PDB_disassemble");

    pfile = mem_allocate_zeroed_typed(PDB_file_t);
    pline = mem_allocate_zeroed_typed(PDB_line_t);

    /* If we already got a source, free it */
    if (pdb->file) {
        PDB_free_file(interp, pdb->file);
        pdb->file = NULL;
    }

    pfile->line   = pline;
    pline->number = 1;
    pfile->source = (char *)mem_sys_allocate(default_size);

    alloced       = space = default_size;
    code_end      = pc + interp->code->base.size;

    while (pc != code_end) {
        /* Grow it early */
        if (space < default_size) {
            alloced += default_size;
            space   += default_size;
            pfile->source = (char *)mem_sys_realloc(pfile->source, alloced);
        }

        size = PDB_disassemble_op(interp, pfile->source + pfile->size,
                space, &interp->op_info_table[*pc], pc, pfile, NULL, 1);
        space       -= size;
        pfile->size += size;
        pfile->source[pfile->size - 1] = '\n';

        /* Store the opcode of this line */
        pline->opcode = pc;
        n             = interp->op_info_table[*pc].op_count;

        ADD_OP_VAR_PART(interp, interp->code, pc, n);
        pc += n;

        /* Prepare for next line */
        newline              = mem_allocate_typed(PDB_line_t);
        newline->label       = NULL;
        newline->next        = NULL;
        newline->number      = pline->number + 1;
        pline->next          = newline;
        pline                = newline;
        pline->source_offset = pfile->size;
    }

    /* Add labels to the lines they belong to */
    label = pfile->label;

    while (label) {
        /* Get the line to apply the label */
        pline = pfile->line;

        while (pline && pline->opcode != label->opcode)
            pline = pline->next;

        if (!pline) {
            Parrot_io_eprintf(pdb->debugger,
                        "Label number %li out of bounds.\n", label->number);

            PDB_free_file(interp, pfile);
            return;
        }

        pline->label = label;

        label        = label->next;
    }

    pdb->state |= PDB_SRC_LOADED;
    pdb->file   = pfile;
}

/*

=item C<long PDB_add_label(PDB_file_t *file, const opcode_t *cur_opcode,
opcode_t offset)>

Add a label to the label list.

=cut

*/

long
PDB_add_label(ARGMOD(PDB_file_t *file), ARGIN(const opcode_t *cur_opcode),
        opcode_t offset)
{
    ASSERT_ARGS(PDB_add_label)
    PDB_label_t *_new;
    PDB_label_t *label = file->label;

    /* See if there is already a label at this line */
    while (label) {
        if (label->opcode == cur_opcode + offset)
            return label->number;
        label = label->next;
    }

    /* Allocate a new label */
    label        = file->label;
    _new         = mem_allocate_typed(PDB_label_t);
    _new->opcode = cur_opcode + offset;
    _new->next   = NULL;

    if (label) {
        while (label->next)
            label = label->next;

        _new->number = label->number + 1;
        label->next  = _new;
    }
    else {
        file->label  = _new;
        _new->number = 1;
    }

    return _new->number;
}

/*

=item C<void PDB_free_file(PARROT_INTERP, PDB_file_t *file)>

Frees any allocated source files.

=cut

*/

void
PDB_free_file(SHIM_INTERP, ARGIN_NULLOK(PDB_file_t *file))
{
    ASSERT_ARGS(PDB_free_file)
    while (file) {
        /* Free all of the allocated line structures */
        PDB_line_t  *line = file->line;
        PDB_label_t *label;
        PDB_file_t  *nfile;

        while (line) {
            PDB_line_t * const nline = line->next;
            mem_sys_free(line);
            line = nline;
        }

        /* Free all of the allocated label structures */
        label = file->label;

        while (label) {
            PDB_label_t * const nlabel = label->next;

            mem_sys_free(label);
            label  = nlabel;
        }

        /* Free the remaining allocated portions of the file structure */
        if (file->sourcefilename)
            mem_sys_free(file->sourcefilename);

        if (file->source)
            mem_sys_free(file->source);

        nfile = file->next;
        mem_sys_free(file);
        file  = nfile;
    }
}

/*

=item C<void PDB_load_source(PARROT_INTERP, const char *command)>

Load a source code file.

=cut

*/

PARROT_EXPORT
void
PDB_load_source(PARROT_INTERP, ARGIN(const char *command))
{
    ASSERT_ARGS(PDB_load_source)
    FILE          *file;
    char           f[DEBUG_CMD_BUFFER_LENGTH + 1];
    int            i, j, c;
    PDB_file_t    *pfile;
    PDB_line_t    *pline;
    PDB_t         * const pdb = interp->pdb;
    opcode_t      *pc         = interp->code->base.data;

    unsigned long  size = 0;

    TRACEDEB_MSG("PDB_load_source");

    /* If there was a file already loaded or the bytecode was
       disassembled, free it */
    if (pdb->file) {
        PDB_free_file(interp->pdb->debugee, interp->pdb->debugee->pdb->file);
        interp->pdb->debugee->pdb->file = NULL;
    }

    /* Get the name of the file */
    for (j = 0; command[j] == ' '; ++j)
        continue;
    for (i = 0; command[j]; i++, j++)
        f[i] = command[j];

    f[i] = '\0';

    /* open the file */
    file = fopen(f, "r");

    /* abort if fopen failed */
    if (!file) {
        Parrot_io_eprintf(pdb->debugger, "Unable to load '%s'\n", f);
        return;
    }

    pfile = mem_allocate_zeroed_typed(PDB_file_t);
    pline = mem_allocate_zeroed_typed(PDB_line_t);

    pfile->source = (char *)mem_sys_allocate(1024);
    pfile->line   = pline;
    pline->number = 1;

    PARROT_ASSERT(interp->op_info_table);
    PARROT_ASSERT(pc);

    while ((c = fgetc(file)) != EOF) {
        /* Grow it */
        if (++size == 1024) {
            pfile->source = (char *)mem_sys_realloc(pfile->source,
                                            (size_t)pfile->size + 1024);
            size = 0;
        }
        pfile->source[pfile->size] = (char)c;

        pfile->size++;

        if (c == '\n') {
            /* If the line has an opcode move to the next one,
               otherwise leave it with NULL to skip it. */
            PDB_line_t *newline = mem_allocate_zeroed_typed(PDB_line_t);

            if (PDB_hasinstruction(pfile->source + pline->source_offset)) {
                size_t n      = interp->op_info_table[*pc].op_count;
                pline->opcode = pc;
                ADD_OP_VAR_PART(interp, interp->code, pc, n);
                pc           += n;

                /* don't walk off the end of the program into neverland */
                if (pc >= interp->code->base.data + interp->code->base.size)
                    break;
            }

            newline->number      = pline->number + 1;
            pline->next          = newline;
            pline                = newline;
            pline->source_offset = pfile->size;
            pline->opcode        = NULL;
            pline->label         = NULL;
        }
    }

    fclose(file);

    pdb->state |= PDB_SRC_LOADED;
    pdb->file   = pfile;

    TRACEDEB_MSG("PDB_load_source finished");
}

/*

=item C<char PDB_hasinstruction(const char *c)>

Return true if the line has an instruction.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_PURE_FUNCTION
char
PDB_hasinstruction(ARGIN(const char *c))
{
    ASSERT_ARGS(PDB_hasinstruction)
    char h = 0;

    /* as long as c is not NULL, we're not looking at a comment (#...) or a '\n'... */
    while (*c && *c != '#' && *c != '\n') {
        /* ... and c is alphanumeric or a quoted string then the line contains
         * an instruction. */
        if (isalnum((unsigned char) *c) || *c == '"') {
            h = 1;
        }
        else if (*c == ':') {
            /* probably a label */
            h = 0;
        }

        c++;
    }

    return h;
}

/*

=item C<static void no_such_register(PARROT_INTERP, char register_type, UINTVAL
register_num)>

Auxiliar error message function.

=cut

*/

static void
no_such_register(PARROT_INTERP, char register_type, UINTVAL register_num)
{
    ASSERT_ARGS(no_such_register)

    Parrot_io_eprintf(interp, "%c%u = no such register\n",
        register_type, register_num);
}

/*

=item C<void PDB_assign(PARROT_INTERP, const char *command)>

Assign to registers.

=cut

*/

void
PDB_assign(PARROT_INTERP, ARGIN(const char *command))
{
    ASSERT_ARGS(PDB_assign)
    UINTVAL register_num;
    char reg_type_id;
    int reg_type;
    PDB_t *pdb       = interp->pdb;
    Interp *debugger = pdb ? pdb->debugger : interp;
    Interp *debugee  = pdb ? pdb->debugee : interp;

    /* smallest valid commad length is 4, i.e. "I0 1" */
    if (strlen(command) < 4) {
        Parrot_io_eprintf(debugger, "Must give a register number and value to assign\n");
        return;
    }
    reg_type_id = (unsigned char) toupper((unsigned char) command[0]);
    command++;
    register_num = get_ulong(&command, 0);

    switch (reg_type_id) {
      case 'I':
        reg_type = REGNO_INT;
        break;
      case 'N':
        reg_type = REGNO_NUM;
        break;
      case 'S':
        reg_type = REGNO_STR;
        break;
      case 'P':
        reg_type = REGNO_PMC;
        Parrot_io_eprintf(debugger, "Assigning to PMCs is not currently supported\n");
        return;
      default:
        Parrot_io_eprintf(debugger, "Invalid register type %c\n", reg_type_id);
        return;
    }
    if (register_num >= Parrot_pcc_get_regs_used(debugee,
                                CURRENT_CONTEXT(debugee), reg_type)) {
        no_such_register(debugger, reg_type_id, register_num);
        return;
    }
    switch (reg_type) {
      case REGNO_INT:
        IREG(register_num) = get_ulong(&command, 0);
        break;
      case REGNO_NUM:
        NREG(register_num) = atof(command);
        break;
      case REGNO_STR:
        SREG(register_num) = Parrot_str_new(debugee, command, strlen(command));
        break;
      default:
        ; /* Must never come here */
    }
    Parrot_io_eprintf(debugger, "\n  %c%u = ", reg_type_id, register_num);
    Parrot_io_eprintf(debugger, "%Ss\n", GDB_print_reg(debugee, reg_type, register_num));
}

/*

=item C<void PDB_list(PARROT_INTERP, const char *command)>

Show lines from the source code file.

=cut

*/

void
PDB_list(PARROT_INTERP, ARGIN(const char *command))
{
    ASSERT_ARGS(PDB_list)
    char          *c;
    unsigned long  line_number;
    unsigned long  i;
    PDB_line_t    *line;
    PDB_t         *pdb = interp->pdb;
    unsigned long  n   = 10;

    TRACEDEB_MSG("PDB_list");
    if (!pdb->file || !pdb->file->line) {
        Parrot_io_eprintf(pdb->debugger, "No source file loaded\n");
        return;
    }

    /* set the list line if provided */
    line_number = get_ulong(&command, 0);
    pdb->file->list_line = (unsigned long) line_number;

    /* set the number of lines to print */
    n = get_ulong(&command, 10);

    /* if n is zero, we simply return, as we don't have to print anything */
    if (n == 0)
        return;

    line = pdb->file->line;

    for (i = 0; i < pdb->file->list_line && line->next; i++)
        line = line->next;

    i = 1;
    while (line->next) {
        Parrot_io_eprintf(pdb->debugger, "%li  ", pdb->file->list_line + i);
        /* If it has a label print it */
        if (line->label)
            Parrot_io_eprintf(pdb->debugger, "L%li:\t", line->label->number);

        c = pdb->file->source + line->source_offset;

        while (*c != '\n')
            Parrot_io_eprintf(pdb->debugger, "%c", *(c++));

        Parrot_io_eprintf(pdb->debugger, "\n");

        line = line->next;

        if (i++ == n)
            break;
    }

    if (--i != n)
        pdb->file->list_line = 0;
    else
        pdb->file->list_line += n;
}

/*

=item C<void PDB_eval(PARROT_INTERP, const char *command)>

C<eval>s an instruction.

=cut

*/

void
PDB_eval(PARROT_INTERP, ARGIN(const char *command))
{
    ASSERT_ARGS(PDB_eval)

    PDB_t         *pdb = interp->pdb;
    Interp *warninterp = (interp->pdb && interp->pdb->debugger) ?
        interp->pdb->debugger : interp;
    TRACEDEB_MSG("PDB_eval");
    UNUSED(command);
    Parrot_io_eprintf(warninterp, "The eval command is currently unimplemeneted\n");
}

/*

=item C<opcode_t * PDB_compile(PARROT_INTERP, const char *command)>

Compiles instructions with the PASM compiler.

Appends an C<end> op.

This may be called from C<PDB_eval> above or from the compile opcode
which generates a malloced string.

=cut

*/

PARROT_CAN_RETURN_NULL
opcode_t *
PDB_compile(PARROT_INTERP, ARGIN(const char *command))
{
    ASSERT_ARGS(PDB_compile)

    UNUSED(command);
    Parrot_ex_throw_from_c_args(interp, NULL,
        EXCEPTION_UNIMPLEMENTED,
        "PDB_compile ('PASM1' compiler) has been deprecated");
}

/*

=item C<void PDB_print(PARROT_INTERP, const char *command)>

Print interp registers.

=cut

*/

void
PDB_print(PARROT_INTERP, ARGIN(const char *command))
{
    ASSERT_ARGS(PDB_print)
    const STRING *s = GDB_P(interp->pdb->debugee, command);

    TRACEDEB_MSG("PDB_print");
    Parrot_io_eprintf(interp, "%Ss\n", s);
}


/*

=item C<void PDB_info(PARROT_INTERP)>

Print the interpreter info.

=cut

*/

void
PDB_info(PARROT_INTERP)
{
    ASSERT_ARGS(PDB_info)

    /* If a debugger is created, use it for printing and use the
     * data in his debugee. Otherwise, use current interpreter
     * for both */
    Parrot_Interp itdeb = interp->pdb ? interp->pdb->debugger : interp;
    Parrot_Interp itp = interp->pdb ? interp->pdb->debugee : interp;

    Parrot_io_eprintf(itdeb, "Total memory allocated = %ld\n",
            interpinfo(itp, TOTAL_MEM_ALLOC));
    Parrot_io_eprintf(itdeb, "GC mark runs = %ld\n",
            interpinfo(itp, GC_MARK_RUNS));
    Parrot_io_eprintf(itdeb, "Lazy gc mark runs = %ld\n",
            interpinfo(itp, GC_LAZY_MARK_RUNS));
    Parrot_io_eprintf(itdeb, "GC collect runs = %ld\n",
            interpinfo(itp, GC_COLLECT_RUNS));
    Parrot_io_eprintf(itdeb, "Collect memory = %ld\n",
            interpinfo(itp, TOTAL_COPIED));
    Parrot_io_eprintf(itdeb, "Active PMCs = %ld\n",
            interpinfo(itp, ACTIVE_PMCS));
    Parrot_io_eprintf(itdeb, "Extended PMCs = %ld\n",
            interpinfo(itp, EXTENDED_PMCS));
    Parrot_io_eprintf(itdeb, "Timely GC PMCs = %ld\n",
            interpinfo(itp, IMPATIENT_PMCS));
    Parrot_io_eprintf(itdeb, "Total PMCs = %ld\n",
            interpinfo(itp, TOTAL_PMCS));
    Parrot_io_eprintf(itdeb, "Active buffers = %ld\n",
            interpinfo(itp, ACTIVE_BUFFERS));
    Parrot_io_eprintf(itdeb, "Total buffers = %ld\n",
            interpinfo(itp, TOTAL_BUFFERS));
    Parrot_io_eprintf(itdeb, "Header allocations since last collect = %ld\n",
            interpinfo(itp, HEADER_ALLOCS_SINCE_COLLECT));
    Parrot_io_eprintf(itdeb, "Memory allocations since last collect = %ld\n",
            interpinfo(itp, MEM_ALLOCS_SINCE_COLLECT));
}

/*

=item C<void PDB_help(PARROT_INTERP, const char *command)>

Print the help text. "Help" with no arguments prints a list of commands.
"Help xxx" prints information on command xxx.

=cut

*/

void
PDB_help(PARROT_INTERP, ARGIN(const char *command))
{
    ASSERT_ARGS(PDB_help)
    const DebuggerCmd *cmd;

    const char * cmdline = command;
    cmd = get_cmd(& cmdline);

    if (cmd) {
        Parrot_io_eprintf(interp->pdb->debugger, "%s\n", cmd->help);
    }
    else {
        if (*cmdline == '\0') {
            unsigned int i;
            Parrot_io_eprintf(interp->pdb->debugger, "List of commands:\n");
            for (i= 0; i < sizeof (DebCmdList) / sizeof (DebuggerCmdList); ++i) {
                const DebuggerCmdList *cmdlist = DebCmdList + i;
                Parrot_io_eprintf(interp->pdb->debugger,
                    "    %-12s-- %s\n", cmdlist->name, cmdlist->cmd->shorthelp);
            }
            Parrot_io_eprintf(interp->pdb->debugger, "\n"
"Type \"help\" followed by a command name for full documentation.\n\n");

        }
        else {
            Parrot_io_eprintf(interp->pdb->debugger, "Unknown command: %s\n", command);
        }
    }
}

/*

=item C<void PDB_backtrace(PARROT_INTERP)>

Prints a backtrace of the interp's call chain.

=cut

*/

void
PDB_backtrace(PARROT_INTERP)
{
    ASSERT_ARGS(PDB_backtrace)
    STRING           *str;
    PMC              *old       = PMCNULL;
    int               rec_level = 0;
    int               limit_count = 0;

    /* information about the current sub */
    PMC *sub = interpinfo_p(interp, CURRENT_SUB);
    PMC *ctx = CURRENT_CONTEXT(interp);

    if (!PMC_IS_NULL(sub)) {
        str = Parrot_Context_infostr(interp, ctx);
        if (str) {
            Parrot_io_eprintf(interp, "%Ss", str);
            if (interp->code->annotations) {
                PMC *annot = PackFile_Annotations_lookup(interp, interp->code->annotations,
                        Parrot_pcc_get_pc(interp, ctx) - interp->code->base.data + 1, NULL);
                if (!PMC_IS_NULL(annot)) {
                    PMC *pfile = VTABLE_get_pmc_keyed_str(interp, annot,
                            Parrot_str_new_constant(interp, "file"));
                    PMC *pline = VTABLE_get_pmc_keyed_str(interp, annot,
                            Parrot_str_new_constant(interp, "line"));
                    if ((!PMC_IS_NULL(pfile)) && (!PMC_IS_NULL(pline))) {
                        STRING *file = VTABLE_get_string(interp, pfile);
                        INTVAL line = VTABLE_get_integer(interp, pline);
                        Parrot_io_eprintf(interp, " (%Ss:%li)", file, (long)line);
                    }
                }
            }
            Parrot_io_eprintf(interp, "\n");
        }
    }

    /* backtrace: follow the continuation chain */
    while (1) {
        Parrot_Continuation_attributes *sub_cont;

        /* Limit the levels dumped, no segfault on infinite recursion */
        if (++limit_count > RECURSION_LIMIT)
            break;

        sub = Parrot_pcc_get_continuation(interp, ctx);

        if (PMC_IS_NULL(sub))
            break;


        sub_cont = PARROT_CONTINUATION(sub);

        if (!sub_cont)
            break;


        str = Parrot_Context_infostr(interp, Parrot_pcc_get_caller_ctx(interp, ctx));


        if (!str)
            break;


        /* recursion detection */
        if (ctx == sub_cont->to_ctx) {
            ++rec_level;
        }
        else if (!PMC_IS_NULL(old) && PMC_cont(old) &&
            Parrot_pcc_get_pc(interp, PMC_cont(old)->to_ctx) ==
            Parrot_pcc_get_pc(interp, PMC_cont(sub)->to_ctx) &&
            Parrot_pcc_get_sub(interp, PMC_cont(old)->to_ctx) ==
            Parrot_pcc_get_sub(interp, PMC_cont(sub)->to_ctx)) {
                ++rec_level;
        }
        else if (rec_level != 0) {
            Parrot_io_eprintf(interp, "... call repeated %d times\n", rec_level);
            rec_level = 0;
        }

        /* print the context description */
        if (rec_level == 0) {
            PackFile_ByteCode *seg = sub_cont->seg;
            Parrot_io_eprintf(interp, "%Ss", str);
            if (seg->annotations) {
                PMC *annot = PackFile_Annotations_lookup(interp, seg->annotations,
                        Parrot_pcc_get_pc(interp, sub_cont->to_ctx) - seg->base.data,
                        NULL);

                if (!PMC_IS_NULL(annot)) {
                    PMC *pfile = VTABLE_get_pmc_keyed_str(interp, annot,
                            Parrot_str_new_constant(interp, "file"));
                    PMC *pline = VTABLE_get_pmc_keyed_str(interp, annot,
                            Parrot_str_new_constant(interp, "line"));
                    if ((!PMC_IS_NULL(pfile)) && (!PMC_IS_NULL(pline))) {
                        STRING *file = VTABLE_get_string(interp, pfile);
                        INTVAL line = VTABLE_get_integer(interp, pline);
                        Parrot_io_eprintf(interp, " (%Ss:%li)", file, (long)line);
                    }
                }
            }
            Parrot_io_eprintf(interp, "\n");
        }

        /* get the next Continuation */
        ctx = Parrot_pcc_get_caller_ctx(interp, ctx);
        old = sub;

        if (!ctx)
            break;
    }

    if (rec_level != 0)
        Parrot_io_eprintf(interp, "... call repeated %d times\n", rec_level);
}

/*
 * GDB functions
 *
 * GDB_P  gdb> pp $I0   print register I0 value
 *
 * RT46139 more, more
 */

/*

=item C<static STRING * GDB_print_reg(PARROT_INTERP, int t, int n)>

Used by GDB_P to convert register values for display.  Takes register
type and number as arguments.

Returns a pointer to the start of the string, (except for PMCs, which
print directly and return "").

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PARROT_OBSERVER
static STRING *
GDB_print_reg(PARROT_INTERP, int t, int n)
{
    ASSERT_ARGS(GDB_print_reg)
    char * string;

    if (n >= 0 && (UINTVAL)n < Parrot_pcc_get_regs_used(interp, CURRENT_CONTEXT(interp), t)) {
        switch (t) {
          case REGNO_INT:
            return Parrot_str_from_int(interp, IREG(n));
          case REGNO_NUM:
            return Parrot_str_from_num(interp, NREG(n));
          case REGNO_STR:
            /* This hack is needed because we occasionally are told
               that we have string registers when we actually don't */
            string = (char *) SREG(n);

            if (string == '\0')
                return Parrot_str_new(interp, "", 0);
            else
                return SREG(n);
          case REGNO_PMC:
            /* prints directly */
            trace_pmc_dump(interp, PREG(n));
            return Parrot_str_new(interp, "", 0);
          default:
            break;
        }
    }
    return Parrot_str_new(interp, "no such register", 0);
}

/*

=item C<static STRING * GDB_P(PARROT_INTERP, const char *s)>

Used by PDB_print to print register values.  Takes a pointer to the
register name(s).

Returns "" or error message.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PARROT_OBSERVER
static STRING *
GDB_P(PARROT_INTERP, ARGIN(const char *s))
{
    ASSERT_ARGS(GDB_P)
    int t;
    char reg_type;

    TRACEDEB_MSG("GDB_P");
    /* Skip leading whitespace. */
    while (isspace((unsigned char)*s))
        s++;

    reg_type = (unsigned char) toupper((unsigned char)*s);

    switch (reg_type) {
        case 'I': t = REGNO_INT; break;
        case 'N': t = REGNO_NUM; break;
        case 'S': t = REGNO_STR; break;
        case 'P': t = REGNO_PMC; break;
        default: return Parrot_str_new(interp, "Need a register.", 0);
    }
    if (! s[1]) {
        /* Print all registers of this type. */
        const int max_reg = Parrot_pcc_get_regs_used(interp, CURRENT_CONTEXT(interp), t);
        int n;

        for (n = 0; n < max_reg; n++) {
            /* this must be done in two chunks because PMC's print directly. */
            Parrot_io_eprintf(interp, "\n  %c%d = ", reg_type, n);
            Parrot_io_eprintf(interp, "%Ss", GDB_print_reg(interp, t, n));
        }
        return Parrot_str_new(interp, "", 0);
    }
    else if (s[1] && isdigit((unsigned char)s[1])) {
        const int n = atoi(s + 1);
        return GDB_print_reg(interp, t, n);
    }
    else
        return Parrot_str_new(interp, "no such register", 0);

}

/*

=back

=head1 SEE ALSO

F<include/parrot/debugger.h>, F<src/parrot_debugger.c> and F<ops/debug.ops>.

=head1 HISTORY

=over 4

=item Initial version by Daniel Grunblatt on 2002.5.19.

=item Start of rewrite - leo 2005.02.16

The debugger now uses its own interpreter. User code is run in
Interp *debugee. We have:

  debug_interp->pdb->debugee->debugger
    ^                            |
    |                            v
    +------------- := -----------+

Debug commands are mostly run inside the C<debugger>. User code
runs of course in the C<debugee>.

=back

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
