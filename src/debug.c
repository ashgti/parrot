/*
 * debug.c
 *
 * CVS Info
 *    $Id$
 * Overview:
 *    Parrot debugger
 * History:
 *      Initial version by Daniel Grunblatt on 2002.5.19
 * Notes:
 * References:
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "parrot/parrot.h"
#include "parrot/interp_guts.h"
#include "parrot/oplib.h"
#include "parrot/trace.h"
#include "parrot/debug.h"

/* na(c) [Next Argument (Char pointer)]
 *
 * Moves the pointer to the next argument in the user input.
 */
#define na(c) { \
    while(*c && !isspace((int) *c)) \
        c++; \
    while(*c && isspace((int) *c)) \
        c++; }

/* nextarg
 *
 * Returns the position just past the current argument in a PASM
 * instruction. This is not the same as na(), above, which is intended
 * for debugger commands. This function is used for eval. */
static char*
nextarg(char* command)
{
    while (*command && (isalnum((int) *command) || *command == ',' ||
        *command == ']'))
            command++;
    while (*command && isspace((int) *command))
        command++;
    return command;
}

/* skip_ws
 *
 * Returns the pointer past any whitespace */

static const char*
skip_ws(const char* str)
{
    while (*str && isspace((int) *str))
        str++;
    return str;
}

/* skip_command
 *
 * Returns the pointer past the current debugger command. (This is an
 * alternative to the na() macro above.) */

static const char*
skip_command(const char* str)
{
    while (*str && !isspace((int) *str))
        str++;
    while (*str && isspace((int) *str))
        str++;
    return str;
}

/* parse_int
 *
 * Parse an integer out of a string and return a pointer to just after
 * the int. */

static const char*
parse_int(const char* str, int* intP)
{
    char* end;

    *intP = strtol(str, &end, 0);

    return end;
}

/* parse_string
 *
 * Parse a double-quoted string out of a C string and return a pointer
 * to just after the string. The parsed string is converted to a
 * Parrot STRING. */
static const char*
parse_string(struct Parrot_Interp *interpreter,
    const char* str, STRING** strP)
{
    const char* string;

    if (*str != '"')
        return NULL;
    str++;
    string = str;
    while (*str && *str != '"') {
        if (*str == '\\' && str[1])
            str += 2;
        else
            str++;
    }

    *strP = string_make(interpreter, string, str - string, NULL, 0, NULL);

    if (*str)
        str++;
    return str;
}

/* parse_key
 *
 * Parse an aggregate key out of a string and return a pointer to just
 * after the key. Currently only string and integer keys are
 * allowed. */
static const char*
parse_key(struct Parrot_Interp *interpreter,
    const char* str, PMC** keyP)
{
    *keyP = NULL;
    if (*str != '[')
        return NULL;
    str++; /* Skip [ */
    if (*str == '"') {
        STRING* string;
        str = parse_string(interpreter, str, &string);
        *keyP = key_new_string(interpreter, string);
    }
    else if (isdigit((int) *str)) {
        int value;
        str = parse_int(str, &value);
        *keyP = key_new_integer(interpreter, (INTVAL) value);
    }
    else {
        return NULL;
    }

    if (*str != ']')
        return NULL;
    return ++str;
}

/* parse_command
 *
 * Convert the command at the beginning of a string into a numeric
 * value that can be used as a switch key for fast lookup. */
static const char*
parse_command(const char* command, unsigned long* cmdP)
{
    int i;
    unsigned long c = 0;

    if (*command == '\0') {
        *cmdP = c;
        return 0;
    }

    for (i = 0; *command && isalpha((int) *command); command++, i++)
        c += (tolower((int) *command) + (i + 1)) * ((i + 1) * 255);

    /* Nonempty and did not start with a letter */
    if (c == 0)
        c = -1;

    *cmdP = c;

    return command;
}

/* PDB_get_command(struct Parrot_Interp *interpreter)
 *
 * Get a command from the user input to execute.
 *
 * It saves the last command executed (in pdb->last_command), so it first
 * frees the old one and updates it with the current one.
 *
 * Also prints the next line to run if the program is still active.
 *
 * The user input can't be longer than 255 characters.
 *
 * The input is saved in pdb->cur_command.
 */
void
PDB_get_command(struct Parrot_Interp *interpreter)
{
    unsigned int i;
    char *c;
    PDB_t *pdb = interpreter->pdb;
    PDB_line_t *line;
    int ch;

    /* flush the buffered data */
    fflush(stdout);

    /* not used any more */
    if (pdb->last_command && *pdb->cur_command)
        mem_sys_free(pdb->last_command);

    /* update the last command */
    if (pdb->cur_command && *pdb->cur_command)
        pdb->last_command = pdb->cur_command;

    /* if the program is stopped and running show the next line to run */
    if ((pdb->state & PDB_STOPPED) && (pdb->state & PDB_RUNNING)) {
        line = pdb->file->line;

        while (pdb->cur_opcode != line->opcode)
            line = line->next;

        PIO_eprintf(interpreter, "\n%li  ", line->number);
        c = pdb->file->source + line->source_offset;
        while (*c != '\n'  && c)
            PIO_eprintf(interpreter, "%c", *(c++));
        PIO_eprintf(interpreter, "\n");
    }

    i = 0;

    c = (char *)mem_sys_allocate(255);

    PIO_eprintf(interpreter, "\n(pdb) ");

    /* skip leading whitespace */
    do {
        ch = fgetc(stdin);
    } while (isspace(ch) && ch != '\n');

    /* generate string (no more than 255 chars) */
     while (ch != EOF && ch != '\n' && (i < 255)) {
        c[i++] = ch;
        ch = fgetc(stdin);
    }

    c[i] = '\0';

    if (ch == -1) strcpy(c, "quit");
    pdb->cur_command = c;
}

/* PDB_run_command(struct Parrot_Interp *interpreter, const char *command)
 *
 * Run a command.
 *
 * Hash the command to make a simple switch calling the correct handler.
 */
void
PDB_run_command(struct Parrot_Interp *interpreter, const char *command)
{
    PDB_t *pdb = interpreter->pdb;
    unsigned long c;
    const char* temp;

    /* keep a pointer to the command, in case we need to report an error */
    temp = command;

    /* get a number from what the user typed */
    command = parse_command(command, &c);

    if (command) na(command);

    switch (c) {
        case c_disassemble:
            PDB_disassemble(interpreter, command);
            break;
        case c_load:
            PDB_load_source(interpreter, command);
            break;
        case c_l:
        case c_list:
            PDB_list(interpreter, command);
            break;
        case c_b:
        case c_break:
            PDB_set_break(interpreter, command);
            break;
        case c_w:
        case c_watch:
            PDB_watchpoint(interpreter, command);
            break;
        case c_d:
        case c_delete:
            PDB_delete_breakpoint(interpreter, command);
            break;
        case c_r:
        case c_run:
            PDB_init(interpreter, command);
            PDB_continue(interpreter, NULL);
            break;
        case c_c:
        case c_continue:
            PDB_continue(interpreter, command);
            break;
        case c_p:
        case c_print:
            PDB_print(interpreter, command);
            break;
        case c_s:
        case c_stack:
            PDB_print_stack(interpreter, command);
            break;
        case c_n:
        case c_next:
            PDB_next(interpreter, command);
            break;
        case c_t:
        case c_trace:
            PDB_trace(interpreter, command);
            break;
        case c_e:
        case c_eval:
            PDB_eval(interpreter, command);
            break;
        case c_info:
            PDB_info(interpreter);
            break;
        case c_h:
        case c_help:
            PDB_help(command);
            break;
        case c_q:
        case c_quit:
            pdb->state |= PDB_EXIT;
            break;
        case 0:
            if (pdb->last_command)
                PDB_run_command(interpreter,pdb->last_command);
            break;
        default:
            PIO_eprintf(interpreter,
                        "Undefined command: \"%s\".  Try \"help\".",
                        temp);
            break;
    }
}

/* PDB_next(struct Parrot_Interp *interpreter, const char *command)
 *
 * Execute the next N operation(s).
 *
 * Inits the program if needed, runs the next N >= 1 operations and
 * stops.
 *
 */
void
PDB_next(struct Parrot_Interp *interpreter, const char *command)
{
    PDB_t *pdb = interpreter->pdb;
    unsigned long n = 1;

    /* Init the program if it's not running */
    if (!(pdb->state & PDB_RUNNING))
        PDB_init(interpreter, command);

    /* Get the number of operations to execute if any */
    if (command && isdigit((int) *command))
        n = atol(command);

    /* Erase the stopped flag */
    pdb->state &= ~PDB_STOPPED;

    /* Execute */
    for ( ; n && pdb->cur_opcode; n--)
        DO_OP(pdb->cur_opcode,interpreter);

    /* Set the stopped flag */
    pdb->state |= PDB_STOPPED;

    /* If program ended */
    if (!pdb->cur_opcode)
        (void)PDB_program_end(interpreter);
}

/* PDB_trace
 * execute the next instruction
 */
void
PDB_trace(struct Parrot_Interp *interpreter,
          const char *command)
{
    PDB_t *pdb = interpreter->pdb;
    unsigned long n = 1;

    if (!(pdb->state & PDB_RUNNING))
    {
        PDB_init(interpreter, command);
    }

    if (command && isdigit((int) *command))
        n = atol(command);

    pdb->state &= ~PDB_STOPPED;

    for ( ; n && pdb->cur_opcode; n--) {
        trace_op(interpreter,
                interpreter->code->byte_code,
                interpreter->code->byte_code +
                interpreter->code->cur_cs->base.size,
                interpreter->pdb->cur_opcode);
        DO_OP(pdb->cur_opcode,interpreter);
    }

    pdb->state |= PDB_STOPPED;

    /* If program ended */
    if (!pdb->cur_opcode)
        (void)PDB_program_end(interpreter);
}

/*  PDB_cond
 *
 *  Analyzes a condition from the user input.
 */
PDB_condition_t *
PDB_cond(struct Parrot_Interp *interpreter, const char *command)
{
    PDB_condition_t *condition;
    int i;
    char str[255];

    /* Allocate new condition */
    condition = (PDB_condition_t *)mem_sys_allocate(sizeof(PDB_condition_t));

    /* return if no more arguments */
    if (!(command && *command))
        return NULL;

    switch (*command) {
        case 'i':
        case 'I':
            condition->type = PDB_cond_int;
            break;
        case 'n':
        case 'N':
            condition->type = PDB_cond_num;
            break;
        case 's':
        case 'S':
            condition->type = PDB_cond_str;
            break;
        case 'p':
        case 'P':
            condition->type = PDB_cond_pmc;
            break;
        default:
            PIO_eprintf(interpreter, "First argument must be a register\n");
            return NULL;
    }

    /* get the register number */
    condition->reg = atoi(++command);

    /* the next argument might have no spaces between the register and the
     * condition. */
    command++;

    if (condition->reg > 9)
        command++;

    if (*command == ' ')
        na(command);

    /* Now the condition */
    switch (*command) {
        case '>':
            if (*(command + 1) == '=')
                condition->type |= PDB_cond_ge;
            else if (*(command + 1) == ' ')
                condition->type |= PDB_cond_gt;
            else
                goto INV_COND;
            break;
        case '<':
            if (*(command + 1) == '=')
                condition->type |= PDB_cond_le;
            else if (*(command + 1) == ' ')
                condition->type |= PDB_cond_lt;
            else
                goto INV_COND;
            break;
        case '=':
            if (*(command + 1) == '=')
                condition->type |= PDB_cond_eq;
            else
                goto INV_COND;
            break;
        case '!':
            if (*(command + 1) == '=')
                condition->type |= PDB_cond_ne;
            else
                goto INV_COND;
            break;
        default:
INV_COND:   PIO_eprintf(interpreter, "Invalid condition\n");
            return NULL;
    }

    if (*(command + 1) == '=')
        command += 2;
    else
        command ++;

    if (*command == ' ')
        na(command);

    /* return if no more arguments */
    if (!(command && *command))
        return NULL;

    if (!((isdigit((int)*command)) || (*command == '"'))) {
        condition->value = (void *)mem_sys_allocate(sizeof(int));
        *(int *)condition->value = (int)atoi(++command);
    }
    /* If the first argument was an integer */
    else if (condition->type & PDB_cond_int) {
        /* This must be either an integer constant or register */
        condition->value = (void *)mem_sys_allocate(sizeof(INTVAL));
        *(INTVAL *)condition->value = (INTVAL)atoi(command);
        condition->type |= PDB_cond_const;
    }
    else if (condition->type & PDB_cond_num) {
        condition->value = (void *)mem_sys_allocate(sizeof(FLOATVAL));
        *(FLOATVAL *)condition->value = (FLOATVAL)atof(command);
        condition->type |= PDB_cond_const;
    }
    else if (condition->type & PDB_cond_str) {
        for (i = 1; ((command[i] != '"') && (i < 255)); i++)
            str[i - 1] = command[i];
        str[i - 1] = '\0';
        condition->value = string_make(interpreter,
            str, i - 1, NULL, PObj_external_FLAG, NULL);
        condition->type |= PDB_cond_const;
    }

    return condition;
}

/* PDB_watchpoint
 * set a watchpoint.
 */
void
PDB_watchpoint(struct Parrot_Interp *interpreter, const char *command)
{
    PDB_t *pdb = interpreter->pdb;
    PDB_condition_t *condition;

    if (!(condition = PDB_cond(interpreter, command)))
        return;

    /* Add it to the head of the list */
    if (pdb->watchpoint)
        condition->next = pdb->watchpoint;
    pdb->watchpoint = condition;
}

/* PDB_set_break
 * set a break point, the source code file must be loaded.
 */
void
PDB_set_break(struct Parrot_Interp *interpreter, const char *command)
{
    PDB_t *pdb = interpreter->pdb;
    PDB_breakpoint_t *newbreak,*sbreak;
    PDB_line_t *line;
    long ln,i;

    /* If no line number was specified set it at the current line */
    if (command && *command) {
        ln = atol(command);

        /* Move to the line where we will set the break point */
        line = pdb->file->line;
        for (i = 1; ((i < ln) && (line->next)); i++)
            line = line->next;

        /* Abort if the line number provided doesn't exists */
        if (!line->next) {
            PIO_eprintf(interpreter,
                        "Can't set a breakpoint at line number %li\n",ln);
            return;
        }
    }
    else {
        /* Get the line to set it */
        line = pdb->file->line;
        while (line->opcode != pdb->cur_opcode)
            line = line->next;
    }

    /* Skip lines that are not related to an opcode */
    while (!line->opcode)
        line = line->next;

    /* Search if we got a breakpoint at that line  */
    i = 0;
    sbreak = pdb->breakpoint;
    while (sbreak && sbreak->pc != line->opcode) {
        sbreak = sbreak->next;
        i++;
    }

    /* Revive the breakpoint if was deleted */
    if (sbreak && sbreak->skip == -1) {
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),
                    "Breakpoint %li at line %li\n",i,line->number);
        sbreak->skip = 0;
        return;
    }
    /* Don't do anything if there is already a breakpoint at this line */
    if (sbreak) {
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),
                    "Breakpoint %li already at line %li\n",i,line->number);
        return;
    }

    /* Allocate the new break point */
    newbreak = (PDB_breakpoint_t *)mem_sys_allocate(sizeof(PDB_breakpoint_t));

    na(command);

    /* if there is another argument to break, besides the line number,
     * it should be an 'if', so we call another handler. */
    if (command && *command) {
        na(command);
        if (!(newbreak->condition = PDB_cond(interpreter, command)))
            return;
    }
    else {
        newbreak->condition = NULL;
    }

    /* Set the address where to stop */
    newbreak->pc = line->opcode;
    /* No next breakpoint */
    newbreak->next = NULL;
    /* Don't skip (at least initially) */
    newbreak->skip = 0;

    /* Add the breakpoint to the end of the list */
    i = 0;
    sbreak = pdb->breakpoint;
    if (sbreak) {
        i++;
        while (sbreak->next) {
            sbreak = sbreak->next;
            i++;
        }
        sbreak->next = newbreak;
    }
    else {
        pdb->breakpoint = newbreak;
    }

    PIO_fprintf(interpreter, PIO_STDERR(interpreter),"Breakpoint %li at line %li\n",i,line->number);
}

/* PDB_init
 * init the program
 */
void
PDB_init(struct Parrot_Interp *interpreter, const char *command)
{
    PDB_t *pdb = interpreter->pdb;
    PMC *userargv;
    STRING *arg;
    struct PackFile *code;
    unsigned long i;
    char c[256];
    void* stacktop = interpreter->lo_var_ptr;

    /* The bytecode is readonly, right? */
    code = interpreter->code;
    /* Destroy the old interpreter FIXME */
    free(interpreter);
    /* Get a new interpreter */
    interpreter = make_interpreter(NO_FLAGS);
    interpreter->code = code;
    interpreter->pdb = pdb;
    interpreter->lo_var_ptr = stacktop;

    /* set the user arguments */
    userargv = pmc_new(interpreter, enum_class_PerlArray);
    interpreter->ctx.pmc_reg.registers[0] = userargv;

    while (command && *command) {
        i = 0;
        while (command[i] && !isspace((int) command[i])) {
            c[i] = command[i];
            i++;
        }
        c[i] = '\0';
        na(command);

        arg = string_make(interpreter, c, i, NULL, 0, NULL);
        VTABLE_push_string(interpreter, userargv, arg);
    }

    /* Restart if we are already running */
    if (pdb->state & PDB_RUNNING)
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"Restarting\n");

    /* Get the bytecode start */
    pdb->cur_opcode = interpreter->code->byte_code;
    /* Add the RUNNING state */
    pdb->state |= PDB_RUNNING;
}

/* PDB_continue
 * continue running the program
 */
void
PDB_continue(struct Parrot_Interp *interpreter,
             const char *command)
{
    PDB_t *pdb = interpreter->pdb;
    long ln;

    /* Skip any breakpoint? */
    if (command && *command) {
        if (!pdb->breakpoint) {
            PIO_fprintf(interpreter, PIO_STDERR(interpreter),"No breakpoints to skip\n");
            return;
        }
        ln = atol(command);
        PDB_skip_breakpoint(interpreter,ln);
    }
    /* Run while no break point is reached */
    while (!PDB_break(interpreter))
        DO_OP(pdb->cur_opcode,interpreter);
}

/* PDB_delete_breakpoint
 * delete a breakpoint
 */
void
PDB_delete_breakpoint(struct Parrot_Interp *interpreter,
                      const char *command)
{
    PDB_breakpoint_t *breakpoint;
    PDB_line_t *line;
    long m,n;

    if (isdigit((int) *command)) {
        m = n = atol(command);
        breakpoint = interpreter->pdb->breakpoint;
        while (breakpoint && n--)
            breakpoint = breakpoint->next;
        if (!breakpoint) {
            PIO_eprintf(interpreter, "No breakpoint number %ld", m);
            return;
        }

        line = interpreter->pdb->file->line;
        while (line->opcode != breakpoint->pc)
            line = line->next;

        breakpoint->skip = -1;
    }
}

/* PDB_skip_breakpoint
 * skip i times all breakpoints.
 */
void
PDB_skip_breakpoint(struct Parrot_Interp *interpreter, long i)
{
    interpreter->pdb->breakpoint_skip = i - 1;
}

/* PDB_program_end
 * end the program
 */
char
PDB_program_end(struct Parrot_Interp *interpreter)
{
    PDB_t *pdb = interpreter->pdb;

    /* Remove the RUNNING state */
    pdb->state &= ~PDB_RUNNING;
    PIO_fprintf(interpreter, PIO_STDERR(interpreter),"Program exited.\n");
    return 1;
}

/* PDB_check_condition
 *
 * Returns TRUE if the condition was met.
 */
char
PDB_check_condition(struct Parrot_Interp *interpreter,
    PDB_condition_t *condition)
{
    INTVAL i,j;
    FLOATVAL k, l;
    STRING *m, *n;

    if (condition->type & PDB_cond_int) {
        i = interpreter->ctx.int_reg.registers[condition->reg];
        if (condition->type & PDB_cond_const)
            j = *(INTVAL *)condition->value;
        else
            j = interpreter->ctx.int_reg.registers[*(int *)condition->value];
        if (((condition->type & PDB_cond_gt) && (i > j)) ||
            ((condition->type & PDB_cond_ge) && (i >= j)) ||
            ((condition->type & PDB_cond_eq) && (i == j)) ||
            ((condition->type & PDB_cond_ne) && (i != j)) ||
            ((condition->type & PDB_cond_le) && (i <= j)) ||
            ((condition->type & PDB_cond_lt) && (i < j)))
                return 1;
        return 0;
    }
    else if (condition->type & PDB_cond_num) {
        k = interpreter->ctx.num_reg.registers[condition->reg];
        if (condition->type & PDB_cond_const)
            l = *(FLOATVAL *)condition->value;
        else
            l = interpreter->ctx.num_reg.registers[*(int *)condition->value];
        if (((condition->type & PDB_cond_gt) && (k > l)) ||
            ((condition->type & PDB_cond_ge) && (k >= l)) ||
            ((condition->type & PDB_cond_eq) && (k == l)) ||
            ((condition->type & PDB_cond_ne) && (k != l)) ||
            ((condition->type & PDB_cond_le) && (k <= l)) ||
            ((condition->type & PDB_cond_lt) && (k < l)))
                return 1;
        return 0;
    }
    else if (condition->type & PDB_cond_str) {
        m = interpreter->ctx.string_reg.registers[condition->reg];
        if (condition->type & PDB_cond_const)
            n = (STRING *)condition->value;
        else
            n = interpreter->ctx.string_reg.registers[*(int *)condition->value];
        if (((condition->type & PDB_cond_gt) &&
                (string_compare(interpreter, m, n) > 0)) ||
            ((condition->type & PDB_cond_ge) &&
                (string_compare(interpreter, m, n) >= 0)) ||
            ((condition->type & PDB_cond_eq) &&
                (string_compare(interpreter, m, n) == 0)) ||
            ((condition->type & PDB_cond_ne) &&
                (string_compare(interpreter, m, n) != 0)) ||
            ((condition->type & PDB_cond_le) &&
                (string_compare(interpreter, m, n) <= 0)) ||
            ((condition->type & PDB_cond_lt) &&
                (string_compare(interpreter, m, n) < 0)))
                    return 1;
        return 0;
    }
    return 0;
}

/* PDB_break
 * return true if we have to stop running
 */
char
PDB_break(struct Parrot_Interp *interpreter)
{
    PDB_t *pdb = interpreter->pdb;
    PDB_breakpoint_t *breakpoint = pdb->breakpoint;
    PDB_condition_t *watchpoint = pdb->watchpoint;

    /* Check the watchpoints first. */
    while (watchpoint) {
        if (PDB_check_condition(interpreter, watchpoint)) {
            pdb->state |= PDB_STOPPED;
            return 1;
        }
        watchpoint = watchpoint->next;
    }

    /* If program ended */
    if (!pdb->cur_opcode)
        return PDB_program_end(interpreter);

    /* If the program is STOPPED allow it to continue */
    if (pdb->state & PDB_STOPPED) {
        pdb->state &= ~PDB_STOPPED;
        return 0;
    }

    /* If we have to skip breakpoints, do so. */
    if (pdb->breakpoint_skip) {
        pdb->breakpoint_skip--;
        return 0;
    }

    while (breakpoint) {
        /* if we are in a break point */
        if (pdb->cur_opcode == breakpoint->pc) {
            if (breakpoint->skip < 0)
                return 0;

            /* Check if there is a condition for this breakpoint */
            if ((breakpoint->condition) &&
                (!PDB_check_condition(interpreter, breakpoint->condition)))
                    return 0;

            /* Add the STOPPED state and stop */
            pdb->state |= PDB_STOPPED;
            return 1;
        }
        breakpoint = breakpoint->next;
    }

    return 0;
}

/* PDB_escape
 * escapes " \r \n \t \a and \\
 */
char *
PDB_escape(const char *string, INTVAL length)
{
    const char *end;
    char *new,*fill;

    length = length > 20 ? 20 : length;
    end = string + length;

    /* Return if there is no string to escape*/
    if (!string || !*string)
        return NULL;

    fill = new = (char *)mem_sys_allocate(length * 2 + 1);

    for ( ; string < end; string++) {
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
    return new;
}

/* PDB_unescape
 * do inplace unescape of \r \n \t \a and \\
 */
int
PDB_unescape(char *string)
{
    char *fill;
    int i, l = 0;

    for ( ; *string; string++) {
        l++;
        if (*string == '\\') {
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

size_t
PDB_disassemble_op(struct Parrot_Interp *interpreter, char* dest, int space,
                   op_info_t* info, opcode_t* op,
                   PDB_file_t *file, opcode_t* code_start, int full_name)
{
    int size = 0;
    int j;
    char *p;

    /* Write the opcode name */
    p = file ? &dest[file->size] : dest;
    strcpy(p, full_name ? info->full_name : info->name);
    size += strlen(dest);

    dest[size++] = ' ';

    /* Concat the arguments */
    for (j = 1; j < info->arg_count; j++) {
        char buf[256];
        INTVAL i = 0;
        FLOATVAL f;
        PMC* k;

        switch (info->types[j]) {
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
            /* If the opcode jumps and this is the last argument
               means this is a label */
            if ((j == info->arg_count - 1) &&
                (info->jump & PARROT_JUMP_RELATIVE))
            {
                if (file) {
                    dest[size++] = 'L';
                    i = PDB_add_label(file, op, op[j]);
                }
                else if (code_start) {
                    dest[size++] = 'O';
                    dest[size++] = 'P';
                    i = op[j] + (op - code_start);
                }
                else {
                    if (op[j] > 0) dest[size++] = '+';
                    i = op[j];
                }
            }

            /* Convert the integer to a string */
            INTEGER:
            if (i == 0) i = (INTVAL) op[j];
            assert(size + 20 < space);
            sprintf(&dest[size], INTVAL_FMT, i);
            size += strlen(&dest[size]);
            break;
        case PARROT_ARG_NC:
            /* Convert the float to a string */
            f=interpreter->code->const_table->constants[op[j]]->u.number;
#ifdef HAS_SNPRINTF
            snprintf(buf, sizeof(buf), FLOATVAL_FMT, f);
#else
            sprintf(buf, FLOATVAL_FMT, f);  /* XXX buffer overflow! */
#endif
            strcpy(&dest[size], buf);
            size += strlen(buf);
            break;
        case PARROT_ARG_SC:
            dest[size++] = '"';
            if (interpreter->code->const_table->constants[op[j]]->
                    u.string->strlen)
            {
                char* escaped = PDB_escape(interpreter->code->const_table->
                           constants[op[j]]->u.string->strstart,
                           interpreter->code->const_table->
                           constants[op[j]]->u.string->strlen);
                if (escaped) {
                    strcpy(&dest[size],escaped);
                    size += strlen(escaped);
                    mem_sys_free(escaped);
                }
            }
            dest[size++] = '"';
            break;
        case PARROT_ARG_K:
            dest[size-1] = '[';
#ifdef HAS_SNPRINTF
            snprintf(buf, sizeof(buf), "P" INTVAL_FMT, op[j]);
#else
            /* XXX buffer overflow! */
            sprintf(buf, "P" INTVAL_FMT, op[j]);
#endif
            strcpy(&dest[size], buf);
            size += strlen(buf);
            dest[size++] = ']';
            break;
        case PARROT_ARG_KC:
            dest[size-1] = '[';
            k = interpreter->code->const_table->constants[op[j]]->u.key;
            while (k) {
                switch (PObj_get_FLAGS(k)) {
                case 0:
                    break;
                case KEY_integer_FLAG:
#ifdef HAS_SNPRINTF
                    snprintf(buf, sizeof(buf), INTVAL_FMT, k->cache.int_val);
#else
                    /* XXX buffer overflow! */
                    sprintf(buf, INTVAL_FMT, k->cache.int_val);
#endif
                    strcpy(&dest[size], buf);
                    size += strlen(buf);
                    break;
                case KEY_number_FLAG:
#ifdef HAS_SNPRINTF
                    snprintf(buf, sizeof(buf), FLOATVAL_FMT, k->cache.num_val);
#else
                    /* XXX buffer overflow! */
                    sprintf(buf, FLOATVAL_FMT, k->cache.num_val);
#endif
                    strcpy(&dest[size], buf);
                    size += strlen(buf);
                    break;
                case KEY_string_FLAG:
                    dest[size++] = '"';
                    {
                    	char *temp = string_to_cstring(
                    		interpreter,k->cache.string_val);
	                    strcpy(&dest[size], temp);
	                    free(temp);
                    }
                    size += string_length(k->cache.string_val);
                    dest[size++] = '"';
                    break;
                case KEY_integer_FLAG|KEY_register_FLAG:
#ifdef HAS_SNPRINTF
                    snprintf(buf, sizeof(buf),"I" INTVAL_FMT,k->cache.int_val);
#else
                    /* XXX buffer overflow! */
                    sprintf(buf, "I" INTVAL_FMT, k->cache.int_val);
#endif
                    strcpy(&dest[size], buf);
                    size += strlen(buf);
                    break;
                case KEY_number_FLAG|KEY_register_FLAG:
#ifdef HAS_SNPRINTF
                    snprintf(buf,sizeof(buf), "N" INTVAL_FMT,k->cache.int_val);
#else
                    /* XXX buffer overflow! */
                    sprintf(buf, "N" INTVAL_FMT, k->cache.int_val);
#endif
                    strcpy(&dest[size], buf);
                    size += strlen(buf);
                    break;
                case KEY_string_FLAG|KEY_register_FLAG:
#ifdef HAS_SNPRINTF
                    snprintf(buf,sizeof(buf), "S" INTVAL_FMT,k->cache.int_val);
#else
                    /* XXX buffer overflow! */
                    sprintf(buf, "S" INTVAL_FMT, k->cache.int_val);
#endif
                    strcpy(&dest[size], buf);
                    size += strlen(buf);
                    break;
                case KEY_pmc_FLAG|KEY_register_FLAG:
#ifdef HAS_SNPRINTF
                    snprintf(buf,sizeof(buf), "P" INTVAL_FMT,k->cache.int_val);
#else
                    /* XXX buffer overflow! */
                    sprintf(buf, "P" INTVAL_FMT, k->cache.int_val);
#endif
                    strcpy(&dest[size], buf);
                    size += strlen(buf);
                    break;
                default:
                    dest[size++] = '?';
                    break;
                }
                k = k->data;
                if (k) dest[size++] = ';';
            }
            dest[size++] = ']';
            break;
        case PARROT_ARG_KI:
            dest[size - 1] = '[';
#ifdef HAS_SNPRINTF
            snprintf(buf, sizeof(buf), "I" INTVAL_FMT, op[j]);
#else
            /* XXX buffer overflow! */
            sprintf(buf, "I" INTVAL_FMT, op[j]);
#endif
            strcpy(&dest[size], buf);
            size += strlen(buf);
            dest[size++] = ']';
            break;
        case PARROT_ARG_KIC:
            dest[size - 1] = '[';
#ifdef HAS_SNPRINTF
            snprintf(buf, sizeof(buf), INTVAL_FMT, op[j]);
#else
            sprintf(buf, INTVAL_FMT, op[j]); /* XXX buffer overflow! */
#endif
            strcpy(&dest[size], buf);
            size += strlen(buf);
            dest[size++] = ']';
            break;
        default:
            break;
        }

        if (j != info->arg_count - 1)
            dest[size++] = ',';
    }

    dest[size] = '\0';
    return ++size;
}

/* PDB_disassemble
 * Disassemble the bytecode
 */
void
PDB_disassemble(struct Parrot_Interp *interpreter, const char *command)
{
    PDB_t *pdb = interpreter->pdb;
    PDB_file_t *pfile;
    PDB_line_t *pline,*newline;
    PDB_label_t *label;
    opcode_t *code_end,*pc = interpreter->code->byte_code;

    int space = 0;  /* How much space do we have? */

    pfile = (PDB_file_t *)mem_sys_allocate(sizeof(PDB_file_t));
    pline = (PDB_line_t *)mem_sys_allocate(sizeof(PDB_line_t));

    /* If we already got a source, free it */
    if (pdb->file)
        PDB_free_file(interpreter);

    pfile->source = (char *)mem_sys_allocate(32768);
    pfile->line = pline;
    pline->number = 1;

    code_end = pc + interpreter->code->cur_cs->base.size;
    while (pc != code_end) {
        /* Grow it early*/
        if (pfile->size % 32768 < 32668 ) {
            pfile->source = mem_sys_realloc(pfile->source,
                                            (size_t)pfile->size + 32768);

            space += 32768;
        }

        pfile->size =
            PDB_disassemble_op(interpreter, pfile->source, space,
                               &interpreter->op_info_table[*pc], pc,
                               pfile, NULL, 1);
        pfile->source[pfile->size - 1] = '\n';

        /* Store the opcode of this line */
        pline->opcode = pc;
        pc += interpreter->op_info_table[*pc].arg_count;
        /* Prepare for next line */
        newline = (PDB_line_t *)mem_sys_allocate(sizeof(PDB_line_t));
        newline->number = pline->number + 1;
        newline->label  = NULL;
        newline->next   = NULL;
        pline->next = newline;
        pline = newline;
        pline->source_offset = pfile->size;
    }

    /* Add labels to the lines they belong to */
    label = pfile->label;
    while (label) {
        /* Get the line to apply the label */
        pline = pfile->line;
        while (pline && pline->opcode != label->opcode)
            pline = pline->next;

        if (!(pline)) {
            PIO_fprintf(interpreter, PIO_STDERR(interpreter),"Label number %li out of bounds.\n",label->number);
            return;
        }

        pline->label = label;

        label = label->next;
    }

    pdb->state |= PDB_SRC_LOADED;
    pdb->file = pfile;
}

/* PDB_add_label
 * Add a label to the label list
 */
long
PDB_add_label(PDB_file_t *file, opcode_t *cur_opcode, opcode_t offset)
{
    PDB_label_t *new, *label = file->label;

    /* See if there is already a label at this line */
    if (label) {
        while (label) {
            if (label->opcode == cur_opcode + offset)
                return label->number;
            label = label->next;
        }
    }
    /* Allocate a new label */
    label = file->label;
    new = (PDB_label_t *)mem_sys_allocate(sizeof(PDB_label_t));
    new->opcode = cur_opcode + offset;
    new->next   = NULL;
    if (label) {
        while (label->next)
            label = label->next;
        new->number = label->number + 1;
        label->next = new;
    }
    else {
        file->label = new;
        new->number = 1;
    }
    return new->number;
}

/* PDB_free_file
 * Frees an allocated file.
 */
void
PDB_free_file(struct Parrot_Interp *interpreter)
{
    PDB_line_t *nline, *line = interpreter->pdb->file->line;
    PDB_label_t *nlabel, *label = interpreter->pdb->file->label;

    /* while there is a line structure allocated, free it */
    while (line) {
        nline = line->next;
        mem_sys_free(line);
        line = nline;
    }

    /* while there is a label structure allocated, free it */
    while (label) {
        nlabel = label->next;
        mem_sys_free(label);
        label = nlabel;
    }

    /* XXX change me when there is more than one file */
    mem_sys_free(interpreter->pdb->file->source);
    /* Free the file structure */
    mem_sys_free(interpreter->pdb->file);

}

/* PDB_load_source
 * load a source code file
 */
void
PDB_load_source(struct Parrot_Interp *interpreter, const char *command)
{
    FILE *file;
    char f[255], c;
    int i;
    unsigned long size = 0;
    PDB_t *pdb = interpreter->pdb;
    PDB_file_t *pfile;
    PDB_line_t *pline,*newline;
    opcode_t *pc = pdb->cur_opcode;

    /* If there was a file already loaded or the bytecode was
       disassembled, free it */
    if (pdb->file)
        PDB_free_file(interpreter);
    /* Get the name of the file */
    for (i = 0; command[i]; i++)
        f[i] = command[i];

    f[i] = '\0';
    /* open the file */
    file = fopen(f,"r");

    /* abort if fopen failed */
    if (!file) {
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"Unable to load %s\n", f);
        return;
    }

    pfile = (PDB_file_t *)mem_sys_allocate_zeroed(sizeof(PDB_file_t));
    pline = (PDB_line_t *)mem_sys_allocate_zeroed(sizeof(PDB_line_t));

    pfile->source = (char *)mem_sys_allocate(1024);
    pfile->line = pline;
    pline->number = 1;

    while (!feof(file)) {
        c = (char)fgetc(file);
        /* Grow it */
        if (++size == 1024) {
            pfile->source = mem_sys_realloc(pfile->source,
                                            (size_t)pfile->size + 1024);
            size = 0;
        }
        pfile->source[pfile->size] = c;

        pfile->size++;

        if (c == '\n') {
            /* If the line has an opcode move to the next one,
               otherwise leave it with NULL to skip it. */
            if (PDB_hasinstruction(pfile->source + pline->source_offset)) {
                pline->opcode = pc;
                pc += interpreter->op_info_table[*pc].arg_count;
            }
            newline = 
                    (PDB_line_t *)mem_sys_allocate_zeroed(sizeof(PDB_line_t));
            newline->number = pline->number + 1;
            pline->next = newline;
            pline = newline;
            pline->source_offset = pfile->size;
            pline->opcode = NULL;
            pline->label = NULL;
        }
    }

    pdb->state |= PDB_SRC_LOADED;
    pdb->file = pfile;
}

/* PDB_hasinstruction
 * return true if the line has an instruction
 * XXX TODO:
 *  1- This should take the line get an instruction, get
 *     the opcode for that instruction and check that is
 *     the correct one.
 *  2- Decide what to do with macros if anything.
 */
char
PDB_hasinstruction(char *c)
{
    char h = 0;

    while (*c && *c != '#' && *c != '\n') {
        if (isalnum((int) *c) || *c == '"')
            h = 1;
        else if (*c == ':')
            h = 0;
        c++;
    }

    return h;
}

/* PDB_list
 * show lines from the source code file
 */
void
PDB_list(struct Parrot_Interp *interpreter, const char *command)
{
    char *c;
    long line_number;
    unsigned long i,n = 10;
    PDB_t *pdb = interpreter->pdb;
    PDB_line_t *line;

    /* set the list line if provided */
    if (isdigit((int) *command)) {
        line_number = atol(command) - 1;
        if (line_number < 0) {
            pdb->file->list_line = 0;
        }
        else {
            pdb->file->list_line = (unsigned long) line_number;
        }
        na(command);
    }
    else {
        pdb->file->list_line = 0;
    }

    /* set the number of lines to print */
    if (isdigit((int) *command)) {
        n = atol(command);
        na(command);
    }

    /* if n is zero, we simply return, as we don't have to print anything */
    if (n == 0) return;

    line = pdb->file->line;

    for (i = 0; i < pdb->file->list_line && line->next; i++)
        line = line->next;

    i = 1;
    while (line->next) {
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"%li  ",pdb->file->list_line + i);
        /* If it has a label print it */
        if (line->label)
            PIO_fprintf(interpreter, PIO_STDERR(interpreter),"L%li:\t",line->label->number);
        c = pdb->file->source + line->source_offset;
        while (*c != '\n')
            PIO_fprintf(interpreter, PIO_STDERR(interpreter),"%c",*(c++));
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"\n");
        line = line->next;
        if (i++ == n)
            break;
    }

    if (--i != n)
        pdb->file->list_line = 0;
    else
        pdb->file->list_line += n;
}

/* PDB_eval
 * evals an instruction with fully qualified opcode name
 * and valid arguments, NO error checking.
 */
void
PDB_eval(struct Parrot_Interp *interpreter, const char *command)
{
    opcode_t *run;
    char *c;
    struct PackFile_ByteCode *eval_cs;

    c = mem_sys_allocate(strlen(command) + 1);
    strcpy(c, command);
    eval_cs = PDB_compile(interpreter, c);

    if (eval_cs) {
        Parrot_switch_to_cs(interpreter, eval_cs);
        run = eval_cs->base.data;
        DO_OP(run,interpreter);
        Parrot_pop_cs(interpreter);
    }
}

/* PDB_compile
 * compiles one instruction with fully qualified opcode name
 * and valid arguments, NO error checking.
 *
 * this may be called from PDB_eval above or from the compile opcode
 * which generates a malloced string
 */
struct PackFile_ByteCode *
PDB_compile(struct Parrot_Interp *interpreter, char *command)
{
    char buf[256];
    char s[1], *c = buf;
    char *orig = command;
    op_info_t *op_info;
    opcode_t *eval;
    int op_number,i,k,l,j = 0;
    struct PackFile_ByteCode * eval_cs = Parrot_new_eval_cs(interpreter);
    /* Opcodes can't have more that 10 arguments, +1 for end */
    eval = eval_cs->base.data = mem_sys_allocate(sizeof(opcode_t) * 11);

    /* find_op needs a string with only the opcode name */
    while (*command && !(isspace((int) *command)))
        *(c++) = *(command++);
    *c = '\0';
    /* Find the opcode number */
    op_number = interpreter->op_lib->op_code(buf, 1);
    if (op_number < 0) {
        PIO_eprintf(interpreter, "Invalid opcode '%s'\n", buf);
        return NULL;
    }
    /* Start generating the bytecode */
    eval[j++] = (opcode_t)op_number;
    /* Get the info for that opcode */
    op_info = &interpreter->op_info_table[op_number];

    /* handle the arguments */
    for (i = 1; i < op_info->arg_count; i++) {
        command = nextarg(command);
        switch (op_info->types[i]) {
            /* If it's a register skip the letter that
               presides the register number */
            case PARROT_ARG_I:
            case PARROT_ARG_N:
            case PARROT_ARG_S:
            case PARROT_ARG_P:
                command++;
            case PARROT_ARG_IC:
                eval[j++] = (opcode_t)atoi(command);
                break;
            case PARROT_ARG_NC:
                k = PDB_extend_const_table(interpreter);

                interpreter->code->const_table->constants[k]->type =PFC_NUMBER;
                interpreter->code->const_table->constants[k]->u.number =
                    (FLOATVAL)atof(command);
                eval[j++] = (opcode_t)k;
                break;
            case PARROT_ARG_SC:
                /* Separate the string */
                *s = *command++;
                c = buf;
                while (*command != *s)
                    *(c++) = *(command++);
                *c = '\0';
                l = PDB_unescape(buf);

                k = PDB_extend_const_table(interpreter);

                interpreter->code->const_table->constants[k]->type =PFC_STRING;
                interpreter->code->const_table->constants[k]->u.string =
                    string_make(interpreter, buf, (UINTVAL)l,
                            NULL, PObj_constant_FLAG, NULL);

                /* Add it to the bytecode */
                eval[j++] = (opcode_t)k;
                break;
            case PARROT_ARG_KIC:
                command++; /* Skip opening [ */
                eval[j++] = (opcode_t)atoi(command);
                break;
            default:
                PIO_eprintf(interpreter, "unknown operand at '%s'\n", command);
                return NULL;
                break;
        }
    }
    eval[j++] = 0;      /* append end op */
    eval_cs->base.size = j;
    mem_sys_free(orig);
    return eval_cs;
}

/* PDB_extend_const_table
 * extend the constant table
 */
int
PDB_extend_const_table(struct Parrot_Interp *interpreter)
{
    int k;

    /* Update the constant count and reallocate */
    k = ++interpreter->code->const_table->const_count;
    interpreter->code->const_table->constants =
        mem_sys_realloc(interpreter->code->const_table->constants,
            k * sizeof(struct PackFile_Constant *));

    /* Allocate a new constant */
    interpreter->code->const_table->constants[--k] = PackFile_Constant_new();
    return k;
}

/* PDB_print_stack
 * print entries in the stack
 */
void
PDB_print_stack(struct Parrot_Interp *interpreter, const char *command)
{
    unsigned long c = 0;

    /* Print from the user stack? */
    if (!*command || isdigit((int) *command))
        PDB_print_user_stack(interpreter, command);
    else {
        parse_command(command, &c);

        switch (c) {
            case c_i:
            case c_int:
                na(command);
                PDB_print_stack_int(interpreter, command);
                break;
            case c_n:
            case c_next:
                na(command);
                PDB_print_stack_num(interpreter, command);
                break;
            case c_s:
            case c_str:
                na(command);
                PDB_print_stack_string(interpreter, command);
                break;
            case c_p:
            case c_pmc:
                na(command);
                PDB_print_stack_pmc(interpreter, command);
                break;
            default:
                PIO_fprintf(interpreter, PIO_STDERR(interpreter),"Unknown argument \"%s\" to 'stack'\n", command);
                break;
        }
    }
}

/* PDB_print_stack_int
 * print the integer register stuck
 */
void
PDB_print_stack_int(struct Parrot_Interp *interpreter, const char *command)
{
    unsigned long depth = 0, i = 0;
    struct IRegChunk *chunk = interpreter->ctx.int_reg_top;

    valid_chunk(chunk, command, depth, FRAMES_PER_INT_REG_CHUNK, i);

    if (!chunk) {
        i = depth / FRAMES_PER_INT_REG_CHUNK;
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"There are only %li frames\n",i);
        return;
    }

    PIO_fprintf(interpreter, PIO_STDERR(interpreter),"Integer stack, frame %li, depth %li\n", i, depth);

    na(command);
    PDB_print_int(interpreter, &chunk->IReg[depth], atoi(command));
}

/* PDB_print_stack_num
 * print the float register stack
 */
void
PDB_print_stack_num(struct Parrot_Interp *interpreter, const char *command)
{
    unsigned long depth = 0, i = 0;
    struct NRegChunk *chunk = interpreter->ctx.num_reg_top;

    valid_chunk(chunk, command, depth, FRAMES_PER_NUM_REG_CHUNK, i);

    if (!chunk) {
        i = depth / FRAMES_PER_NUM_REG_CHUNK;
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"There are only %li frames\n",i);
        return;
    }

    PIO_fprintf(interpreter, PIO_STDERR(interpreter),"Float stack, frame %li, depth %li\n", i, depth);

    na(command);
    PDB_print_num(interpreter, &chunk->NReg[depth], atoi(command));
}

/* PDB_print_stack_string
 * print the string register stack
 */
void
PDB_print_stack_string(struct Parrot_Interp *interpreter, const char *command)
{
    unsigned long depth = 0, i = 0;
    struct SRegChunk *chunk = interpreter->ctx.string_reg_top;

    valid_chunk(chunk, command, depth, FRAMES_PER_STR_REG_CHUNK, i);

    if (!chunk) {
        i = depth / FRAMES_PER_STR_REG_CHUNK;
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"There are only %li frames\n",i);
        return;
    }

    PIO_fprintf(interpreter, PIO_STDERR(interpreter),"String stack, frame %li, depth %li\n", i, depth);

    na(command);
    PDB_print_string(interpreter,&chunk->SReg[depth], atoi(command));
}

/* PDB_print_stack_pmc
 * print the pmc register stack
 */
void
PDB_print_stack_pmc(struct Parrot_Interp *interpreter, const char *command)
{
    unsigned long depth = 0, i = 0;
    struct PRegChunk *chunk = interpreter->ctx.pmc_reg_top;

    valid_chunk(chunk, command, depth, FRAMES_PER_PMC_REG_CHUNK, i);

    if (!chunk) {
        i = depth / FRAMES_PER_PMC_REG_CHUNK;
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"There are only %li frames\n",i);
        return;
    }

    PIO_fprintf(interpreter, PIO_STDERR(interpreter),"PMC stack, frame %li, depth %li\n", i, depth);

    na(command);
    PDB_print_pmc(interpreter,&chunk->PReg[depth], atoi(command), NULL);
}

static void
dump_string(struct Parrot_Interp *interpreter, STRING* s)
{
    if (s) {
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"\tBuflen  =\t%12ld\n",s->buflen);
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),
                "\tFlags   =\t%12ld\n", PObj_get_FLAGS(s));
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"\tBufused =\t%12ld\n",s->bufused);
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"\tStrlen  =\t%12ld\n",s->strlen);
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"\tOffset  =\t%12d\n",
                (char*) s->strstart - (char*) s->bufstart);
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"\tString  =\t%S\n", s);
    }
}

/* PDB_print_user_stack
 * print an entry from the user stack
 */
void
PDB_print_user_stack(struct Parrot_Interp *interpreter, const char *command)
{
    unsigned long depth = 0,i;
    Stack_Chunk_t *chunk = interpreter->ctx.user_stack;
    Stack_Entry_t *entry;

    valid_chunk(chunk, command, depth, STACK_CHUNK_DEPTH, i);

    entry = (Stack_Entry_t *)(chunk->buffer->bufstart) + depth;

    switch (entry->entry_type) {
        case STACK_ENTRY_INT:
            PIO_fprintf(interpreter, PIO_STDERR(interpreter),"Integer\t=\t%8vi\n",entry->entry.int_val);
            break;
        case STACK_ENTRY_FLOAT:
            PIO_fprintf(interpreter, PIO_STDERR(interpreter),"Float\t=\t%8.4vf\n",entry->entry.num_val);
            break;
        case STACK_ENTRY_STRING:
            PIO_fprintf(interpreter, PIO_STDERR(interpreter),"String =\n");
            dump_string(interpreter, entry->entry.string_val);
            break;
        case STACK_ENTRY_PMC:
            PIO_fprintf(interpreter, PIO_STDERR(interpreter),"PMC =\n%PS\n", entry->entry.pmc_val);
            break;
        case STACK_ENTRY_POINTER:
            PIO_fprintf(interpreter, PIO_STDERR(interpreter),"POINTER\n");
            break;
        case STACK_ENTRY_DESTINATION:
            PIO_fprintf(interpreter, PIO_STDERR(interpreter),"DESTINATION\n");
            break;
        default:
            PIO_fprintf(interpreter, PIO_STDERR(interpreter),"Invalid stack_entry_type!\n");
            break;
    }
}

static STRING* cstring_to_string(struct Parrot_Interp *interpreter, char* str)
{
    return string_make(interpreter, str, strlen(str), NULL, 0, NULL);
}

/* PDB_print
 * print interpreter registers
 */
void
PDB_print(struct Parrot_Interp *interpreter, const char *command)
{
    unsigned long c = 0;
    PMC* key = NULL;
    int regnum = -1;

    command = skip_ws(command);
    command = parse_command(command, &c);
    if (command == NULL) {
        PDB_print_int(interpreter, &interpreter->ctx.int_reg, -1);
        PDB_print_num(interpreter, &interpreter->ctx.num_reg, -1);
        PDB_print_string(interpreter, &interpreter->ctx.string_reg, -1);
        PDB_print_pmc(interpreter, &interpreter->ctx.pmc_reg, -1, NULL);
        return;
    }

    command = skip_ws(command);

    if (isdigit((int) *command)) {
        command = parse_int(command, &regnum);
        command = skip_ws(command);
    }

    Parrot_block_DOD(interpreter);

    if (*command == '[') {
        command = parse_key(interpreter, command, &key);
    }

    switch (c) {
        case c_i:
        case c_int:
            PDB_print_int(interpreter, &interpreter->ctx.int_reg, regnum);
            break;
        case c_n:
        case c_num:
            PDB_print_num(interpreter, &interpreter->ctx.num_reg, regnum);
            break;
        case c_s:
        case c_str:
            PDB_print_string(interpreter,&interpreter->ctx.string_reg, regnum);
            break;
        case c_p:
        case c_pmc:
            PDB_print_pmc(interpreter,&interpreter->ctx.pmc_reg, regnum, key);
            break;
        default:
            PIO_eprintf(interpreter, "Unrecognized print option: must be 'int', 'num', 'str', 'pmc', or a register\n");
    }

    Parrot_unblock_DOD(interpreter);
}

/* PDB_print_int
 * print the whole or a specific value of a integer register structure.
 */
void
PDB_print_int(struct Parrot_Interp *interpreter, struct IReg *int_reg, int regnum)
{
    int i,j = 0, k = NUM_REGISTERS;

    if (regnum >= NUM_REGISTERS || regnum < -1) {
        PIO_eprintf(interpreter, "No such register I%d", regnum);
        return;
    }
    else if (regnum != -1) {
        j = regnum;
        k = regnum + 1;
    }
    else {
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"Integer Registers:\n");
    }

    for (i = j; i < k; i++) {
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"I%i =\t",i);
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"%11vi\n",int_reg->registers[i]);
    }
}

/* PDB_print_num
 * print the whole or a specific value of a float register structure.
 */
void
PDB_print_num(struct Parrot_Interp *interpreter, struct NReg *num_reg, int regnum)
{
    int i,j = 0, k = NUM_REGISTERS;

    if (regnum >= NUM_REGISTERS || regnum < -1) {
        PIO_eprintf(interpreter, "No such register N%d", regnum);
        return;
    }
    else if (regnum != -1) {
        j = regnum;
        k = regnum + 1;
    }
    else {
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"Float Registers:\n");
    }

    for (i = j; i < k; i++) {
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"N%i =\t",i);
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"%20.4f\n",num_reg->registers[i]);
    }
}

/* PDB_print_string
 * print the whole or a specific value of a string register structure.
 */
void
PDB_print_string(struct Parrot_Interp *interpreter, struct SReg *string_reg,
                 int regnum)
{
    int i,j = 0, k = NUM_REGISTERS;

    if (regnum >= NUM_REGISTERS || regnum < -1) {
        PIO_eprintf(interpreter, "No such register S%d", regnum);
        return;
    }
    else if (regnum != -1) {
        j = regnum;
        k = regnum + 1;
    }
    else {
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"String Registers:\n");
    }

    for (i = j; i < k; i++) {
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"S%i =\n",i);
        dump_string(interpreter, string_reg->registers[i]);
    }
}

static void
print_pmc(struct Parrot_Interp *interpreter, PMC* pmc)
{
    if (pmc && pmc->vtable) {
        STRING* s = VTABLE_name(interpreter, pmc);
        if (s) {
            PIO_eprintf(interpreter, " [%S]\n", s);
        }
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"Stringified: %PS\n", pmc);
    }
    else {
        PIO_eprintf(interpreter, "<null pmc>\n");
    }
}

void
PDB_print_pmc(struct Parrot_Interp *interpreter, struct PReg *pmc_reg,
              int regnum, PMC* key)
{
    int i,j = 0, k = NUM_REGISTERS;

    if (regnum >= NUM_REGISTERS || regnum < -1) {
        PIO_eprintf(interpreter, "No such register P%d", regnum);
        return;
    }
    else if (regnum != -1) {
        j = regnum;
        k = regnum + 1;
    }
    else {
        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"PMC Registers:\n");
    }

    for (i = j; i < k; i++) {
        PMC* pmc = pmc_reg->registers[i];

        PIO_fprintf(interpreter, PIO_STDERR(interpreter),"P%i", i);
        if (key) trace_key_dump(interpreter, key);
        PIO_fprintf(interpreter, PIO_STDERR(interpreter)," =");

        if (key) pmc = VTABLE_get_pmc_keyed(interpreter, pmc, key);
        print_pmc(interpreter, pmc);
    }
}

/* PDB_info
 * print the interpreter info.
 */
void
PDB_info(struct Parrot_Interp *interpreter)
{
    PIO_eprintf(interpreter, "Total memory allocated = %d\n",
            interpinfo(interpreter, TOTAL_MEM_ALLOC));
    PIO_eprintf(interpreter, "DOD runs = %d\n",
            interpinfo(interpreter, DOD_RUNS));
    PIO_eprintf(interpreter, "Collect runs = %d\n",
            interpinfo(interpreter, COLLECT_RUNS));
    PIO_eprintf(interpreter, "Collect memory = %d\n",
            interpinfo(interpreter, TOTAL_COPIED));
    PIO_eprintf(interpreter, "Active PMCs = %d\n",
            interpinfo(interpreter, ACTIVE_PMCS));
    PIO_eprintf(interpreter, "Active buffers = %d\n",
            interpinfo(interpreter, ACTIVE_BUFFERS));
    PIO_eprintf(interpreter, "Total PMCs = %d\n",
            interpinfo(interpreter, TOTAL_PMCS));
    PIO_eprintf(interpreter, "Total buffers = %d\n",
            interpinfo(interpreter, TOTAL_BUFFERS));
    PIO_eprintf(interpreter, "Header allocations since last collect = %d\n",
            interpinfo(interpreter, HEADER_ALLOCS_SINCE_COLLECT));
    PIO_eprintf(interpreter, "Memory allocations since last collect = %d\n",
            interpinfo(interpreter, MEM_ALLOCS_SINCE_COLLECT));
}

/* PDB_help
 * print the help
 * TODO complete me.
 */
void
PDB_help(const char *command)
{
    PIO_eprintf(NULL, "\
List of commands:\n\
    disassemble -- disassemble the bytecode\n\
    load -- load a source code file\n\
    list (l) -- list the source code file\n\
    run (r) -- run the program\n\
    break (b) -- add a breakpoint\n\
    watch (w) -- add a watchpoint\n\
    delete (d) -- delete a breakpoint\n\
    continue (c) -- continue the program execution\n\
    next (n) -- run the next instruction\n\
    eval (e) -- run an instruction\n\
    trace (t) -- trace the next instruction\n\
    print (p) -- print the interpreter registers\n\
    stack (s) -- examine the stack\n\
    info -- print interpreter information\n\
    quit (q) -- exit the debugger\n\
    help (h) -- print this help\n\n");
}

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
