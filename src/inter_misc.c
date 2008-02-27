/*
Copyright (C) 2001-2007, The Perl Foundation.
$Id$

=head1 NAME

src/inter_misc.c - Parrot Interpreter miscellaneous functions

=head1 DESCRIPTION

NCI function setup, compiler registration, C<interpinfo>, and C<sysinfo> opcodes.

=head2 Functions

=over 4

=cut

*/


#include "parrot/parrot.h"
#include "inter_misc.str"
#include "../compilers/imcc/imc.h"

/* XXX Put me somewhere else */
void Parrot_NCI_nci_make_raw_nci(PARROT_INTERP, PMC *method, void *func);

/* HEADERIZER HFILE: include/parrot/interpreter.h */

/*

=item C<void register_nci_method>

Create an entry in the C<nci_method_table> for the given NCI method of PMC
class C<type>.

=cut

*/

PARROT_API
void
register_nci_method(PARROT_INTERP, const int type, ARGIN(void *func),
                    ARGIN(const char *name), ARGIN(const char *proto))
{
    PMC    * const method      = pmc_new(interp, enum_class_NCI);
    STRING * const method_name = string_make(interp, name, strlen(name),
        NULL, PObj_constant_FLAG|PObj_external_FLAG);

    /* create call func */
    VTABLE_set_pointer_keyed_str(interp, method,
            string_make(interp, proto, strlen(proto), NULL,
                PObj_constant_FLAG|PObj_external_FLAG),
            func);

    /* insert it into namespace */
    VTABLE_set_pmc_keyed_str(interp, interp->vtables[type]->_namespace,
            method_name, method);
}

/*

=item C<void register_raw_nci_method_in_ns>

Create an entry in the C<nci_method_table> for the given raw NCI method
of PMC class C<type>.

=cut

*/

PARROT_API
void
register_raw_nci_method_in_ns(PARROT_INTERP, const int type, ARGIN(void *func),
        ARGIN(const char *name))
{
    PMC    * const method      = pmc_new(interp, enum_class_NCI);
    STRING * const method_name = string_make(interp, name, strlen(name),
        NULL, PObj_constant_FLAG|PObj_external_FLAG);

    /* setup call func */
    Parrot_NCI_nci_make_raw_nci(interp, method, func);

    /* insert it into namespace */
    VTABLE_set_pmc_keyed_str(interp, interp->vtables[type]->_namespace,
            method_name, method);
}

/*

=item C<void Parrot_mark_method_writes>

Mark the method C<name> on PMC type C<type> as one that modifies the PMC.

=cut

*/

PARROT_API
void
Parrot_mark_method_writes(PARROT_INTERP, int type, ARGIN(const char *name))
{
    STRING *const str_name = const_string(interp, name);
    PMC    *const pmc_true = pmc_new(interp, enum_class_Integer);
    PMC    *const method   = VTABLE_get_pmc_keyed_str(
        interp, interp->vtables[type]->_namespace, str_name);
    VTABLE_set_integer_native(interp, pmc_true, 1);
    VTABLE_setprop(interp, method, const_string(interp, "write"),
                   pmc_true);
}

/*

=item C<void Parrot_compreg>

Register a parser/compiler function.

=cut

*/

PARROT_API
void
Parrot_compreg(PARROT_INTERP, ARGIN(STRING *type),
                    NOTNULL(Parrot_compiler_func_t func))
{
    PMC* const iglobals = interp->iglobals;
    PMC        *nci     = pmc_new(interp, enum_class_NCI);
    STRING     *sc      = CONST_STRING(interp, "PJt");
    PMC        *hash    = VTABLE_get_pmc_keyed_int(interp, interp->iglobals,
                              IGLOBALS_COMPREG_HASH);

    if (!hash) {
        hash = pmc_new_noinit(interp, enum_class_Hash);
        VTABLE_init(interp, hash);
        VTABLE_set_pmc_keyed_int(interp, iglobals,
                (INTVAL)IGLOBALS_COMPREG_HASH, hash);
    }

    VTABLE_set_pmc_keyed_str(interp, hash, type, nci);

    /* build native call interface for the C sub in "func" */
    VTABLE_set_pointer_keyed_str(interp, nci, sc, (void*)func);
}

/*

=item C<PMC * Parrot_compile_string>

Compile code string.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC *
Parrot_compile_string(PARROT_INTERP, ARGIN(STRING *type),
        ARGIN(const char *code), ARGOUT(STRING **error))
{
    if (string_compare(interp, const_string(interp, "PIR"), type) == 0)
        return IMCC_compile_pir_s(interp, code, error);

    if (string_compare(interp, const_string(interp, "PASM"), type) == 0)
        return IMCC_compile_pasm_s(interp, code, error);

    *error=const_string(interp, "Invalid interpreter type");
    return NULL;
}

/*

=item C<void * Parrot_compile_file>

Compile code file.

=cut

*/

PARROT_API
PARROT_CANNOT_RETURN_NULL
void *
Parrot_compile_file(PARROT_INTERP, ARGIN(const char *fullname), ARGOUT(STRING **error))
{
    return IMCC_compile_file_s(interp, fullname, error);
}

#ifdef GC_IS_MALLOC
#  if 0
struct mallinfo {
    int arena;                  /* non-mmapped space allocated from system */
    int ordblks;                /* number of free chunks */
    int smblks;                 /* number of fastbin blocks */
    int hblks;                  /* number of mmapped regions */
    int hblkhd;                 /* space in mmapped regions */
    int usmblks;                /* maximum total allocated space */
    int fsmblks;                /* space available in freed fastbin blocks */
    int uordblks;               /* total allocated space */
    int fordblks;               /* total free space */
    int keepcost;               /* top-most, releasable (via malloc_trim)
                                 * space */
};
#  endif
extern struct mallinfo mallinfo(void);
#endif /* GC_IS_MALLOC */

/*

=item C<INTVAL interpinfo>

C<what> specifies the type of information you want about the
interpreter.

=cut

*/

PARROT_API
INTVAL
interpinfo(PARROT_INTERP, INTVAL what)
{
    INTVAL ret = 0;
    int j;
    Arenas *arena_base = interp->arena_base;

    switch (what) {
        case TOTAL_MEM_ALLOC:
#ifdef GC_IS_MALLOC
#  if 0
            interp->memory_allocated = mallinfo().uordblks;
#  endif
#endif
            ret = arena_base->memory_allocated;
            break;
        case DOD_RUNS:
            ret = arena_base->dod_runs;
            break;
        case LAZY_DOD_RUNS:
            ret = arena_base->lazy_dod_runs;
            break;
        case COLLECT_RUNS:
            ret = arena_base->collect_runs;
            break;
        case ACTIVE_PMCS:
            ret = arena_base->pmc_pool->total_objects -
                arena_base->pmc_pool->num_free_objects;
            break;
        case ACTIVE_BUFFERS:
            ret = 0;
            for (j = 0; j < (INTVAL)arena_base->num_sized; j++) {
                Small_Object_Pool * const header_pool =
                    arena_base->sized_header_pools[j];
                if (header_pool)
                    ret += header_pool->total_objects -
                        header_pool->num_free_objects;
            }
            break;
        case TOTAL_PMCS:
            ret = arena_base->pmc_pool->total_objects;
            break;
        case TOTAL_BUFFERS:
            ret = 0;
            for (j = 0; j < (INTVAL)arena_base->num_sized; j++) {
                Small_Object_Pool * const header_pool =
                    arena_base->sized_header_pools[j];
                if (header_pool)
                    ret += header_pool->total_objects;
            }
            break;
        case HEADER_ALLOCS_SINCE_COLLECT:
            ret = arena_base->header_allocs_since_last_collect;
            break;
        case MEM_ALLOCS_SINCE_COLLECT:
            ret = arena_base->mem_allocs_since_last_collect;
            break;
        case TOTAL_COPIED:
            ret = arena_base->memory_collected;
            break;
        case IMPATIENT_PMCS:
            ret = arena_base->num_early_DOD_PMCs;
            break;
        case EXTENDED_PMCS:
            ret = arena_base->num_extended_PMCs;
            break;
        case CURRENT_RUNCORE:
            ret = interp->run_core;
            break;
        default:        /* or a warning only? */
            real_exception(interp, NULL, UNIMPLEMENTED,
                    "illegal argument in interpinfo");
    }
    return ret;
}

/*

=item C<PMC* interpinfo_p>

C<what> specifies the type of information you want about the
interpreter.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC*
interpinfo_p(PARROT_INTERP, INTVAL what)
{
    switch (what) {
        case CURRENT_SUB:
            return CONTEXT(interp->ctx)->current_sub;
        case CURRENT_CONT:
            {
            PMC * const cont = CONTEXT(interp->ctx)->current_cont;
            if (!PMC_IS_NULL(cont) && cont->vtable->base_type ==
                    enum_class_RetContinuation)
                return VTABLE_clone(interp, cont);
            return cont;
            }
        case CURRENT_OBJECT:
            return CONTEXT(interp->ctx)->current_object;
        case CURRENT_LEXPAD:
            return CONTEXT(interp->ctx)->lex_pad;
        default:        /* or a warning only? */
            real_exception(interp, NULL, UNIMPLEMENTED,
                    "illegal argument in interpinfo");
    }
}

/*

=item C<STRING* interpinfo_s>

Takes an interpreter name and an information type as arguments.
Returns corresponding information strings about the interpreter:
the full pathname, executable name, or the file stem,
(or throws an error exception, if the type is not recognised).
Valid types are EXECUTABLE_FULLNAME, EXECUTABLE_BASENAME,
and RUNTIME_PREFIX.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING*
interpinfo_s(PARROT_INTERP, INTVAL what)
{
    switch (what) {
        case EXECUTABLE_FULLNAME:
            return VTABLE_get_string(interp,
                VTABLE_get_pmc_keyed_int(interp, interp->iglobals,
                    IGLOBALS_EXECUTABLE));
        case EXECUTABLE_BASENAME:
            {
            char   *fullname_c;
            STRING *fullname;
            STRING *basename;
            int     pos;

            /* Need to strip back to what follows the final / or \. */
            fullname = VTABLE_get_string(interp,
                VTABLE_get_pmc_keyed_int(interp, interp->iglobals,
                    IGLOBALS_EXECUTABLE));
            fullname_c = string_to_cstring(interp, fullname);
            pos = strlen(fullname_c) - 1;
            while (pos > 0 && fullname_c[pos] != '/' && fullname_c[pos] != '\\')
                pos--;
            if (pos > 0)
                pos++;
            basename = string_from_cstring(interp, fullname_c + pos, 0);
            mem_sys_free(fullname_c);
            return basename;
            }

        case RUNTIME_PREFIX:
            {
            char   * const fullname_c = Parrot_get_runtime_prefix(interp);
            STRING * const fullname   = string_from_cstring(interp, fullname_c, 0);

            mem_sys_free(fullname_c);
            return fullname;
            }

        default:
            real_exception(interp, NULL, UNIMPLEMENTED,
                "illegal argument in interpinfo");
    } /* switch */
}

/*

=item C<INTVAL sysinfo_i>

Returns the system info.

C<info_wanted> is one of:

    PARROT_INTSIZE
    PARROT_FLOATSIZE
    PARROT_POINTERSIZE

In unknown info is requested then -1 is returned.

=cut

*/

PARROT_WARN_UNUSED_RESULT
INTVAL
sysinfo_i(SHIM_INTERP, INTVAL info_wanted)
{
    switch (info_wanted) {
        case PARROT_INTSIZE:
            return sizeof (INTVAL);
        case PARROT_FLOATSIZE:
            return sizeof (FLOATVAL);
        case PARROT_POINTERSIZE:
            return sizeof (void *);
        default:
            return -1;
    }
}

/*

=item C<STRING * sysinfo_s>

Returns the system info string.

C<info_wanted> is one of:

    PARROT_OS
    PARROT_OS_VERSION
    PARROT_OS_VERSION_NUMBER
    CPU_ARCH
    CPU_TYPE

If unknown info is requested then and empty string is returned.

=cut

*/

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
STRING *
sysinfo_s(PARROT_INTERP, INTVAL info_wanted)
{
    switch (info_wanted) {
        case PARROT_OS:
            /*  XXX Can't use CONST_STRING here yet, as it needs literal strings*/
            return string_from_literal(interp, BUILD_OS_NAME);
        case PARROT_OS_VERSION:
        case PARROT_OS_VERSION_NUMBER:
        case CPU_ARCH:
        case CPU_TYPE:
        default:
            return CONST_STRING(interp, "");
    }
}

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
