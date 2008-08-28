/*
Copyright (C) 2001-2007, The Perl Foundation.
$Id$

=head1 NAME

src/extend.c - Parrot extension interface

=head1 DESCRIPTION

These are the functions that parrot extensions (i.e. parrot subroutines
written in C, or some other compiled language, rather than in parrot
bytecode) may access.

There is a deliberate distancing from the internals here. Don't go
peeking inside -- you've as much access as bytecode does, but no more,
so we can provide backwards compatibility for as long as we possibly
can.

=head2 Functions

=over 4

=cut

*/

/* Some internal notes. Parrot will die a horrible and bizarre death
   if the stack start pointer's not set and a DOD run is
   triggered. The pointer *will* be set by the interpreter if the
   interpreter calls code which calls these functions, so most
   extension code is safe, no problem.

   The problem comes in if these routines are called from *outside*
   an interpreter. This happens when an embedding application calls
   them to do stuff with PMCs, STRINGS, interpreter contents, and
   suchlike things. This is perfectly legal -- in fact it's what
   we've documented should be done -- but the problem is that the
   stack base pointer will be NULL. This is Very Bad.

   To deal with this there are two macros that are defined to handle
   the problem.

   PARROT_CALLIN_START(interp) will figure out if the stack
   anchor needs setting and, if so, will set it. It must *always*
   come immediately after the last variable declared in the block
   making the calls into the interpreter, as it declares a variable
   and has some code.

   PARROT_CALLIN_END(interp) will put the stack anchor back to
   the way it was, and should always be the last statement before a
   return. (If you have multiple returns have it in multiple times)

   Not doing this is a good way to introduce bizarre heisenbugs, so
   just do it. This is the only place they ought to have to be put
   in, and most of the functions are already written, so it's not
   like it's an onerous requirement.

*/

#include "parrot/parrot.h"
#include "parrot/extend.h"

/* HEADERIZER HFILE: include/parrot/extend.h */

/*

=item C<int Parrot_vfprintf>

Writes a C string format with a varargs list to a PIO.

=item C<int Parrot_fprintf>

Writes a C string format with varargs to a PIO.

=item C<int Parrot_printf>

Writes a C string format with varargs to C<stdout>.

=item C<int Parrot_eprintf>

Writes a C string format with varargs to C<stderr>.

=cut

*/

PARROT_API
int
Parrot_vfprintf(PARROT_INTERP, ARGIN(Parrot_PMC pio),
        ARGIN(const char *s), va_list args)
{
    STRING * str;
    INTVAL retval;

    PARROT_CALLIN_START(interp);
    str = Parrot_vsprintf_c(interp, s, args);
    retval = PIO_putps(interp, pio, str);
    PARROT_CALLIN_END(interp);

    return retval;
}

PARROT_API
int
Parrot_fprintf(PARROT_INTERP, ARGIN(Parrot_PMC pio),
        ARGIN(const char *s), ...)
{
    va_list args;
    INTVAL retval;

    va_start(args, s);
    retval = Parrot_vfprintf(interp, pio, s, args);
    va_end(args);

    return retval;
}

PARROT_API
int
Parrot_printf(NULLOK_INTERP, ARGIN(const char *s), ...)
{
    va_list args;
    INTVAL retval;
    va_start(args, s);

    if (interp) {
        retval = Parrot_vfprintf(interp, PIO_STDOUT(interp), s, args);
    }
    else {
        /* Be nice about this...
         **   XXX BD Should this use the default PIO_STDOUT or something?
         */
        retval = vfprintf(stdout, s, args);
    }
    va_end(args);

    return retval;
}

PARROT_API
int
Parrot_eprintf(NULLOK_INTERP, ARGIN(const char *s), ...)
{
    va_list args;
    INTVAL retval;

    va_start(args, s);

    if (interp) {
        retval = Parrot_vfprintf(interp, PIO_STDERR(interp), s, args);
    }
    else {
        /* Be nice about this...
         **   XXX BD Should this use the default PIO_STDOUT or something?
         */
        retval=vfprintf(stderr, s, args);
    }

    va_end(args);

    return retval;
}

/*

=item C<Parrot_String Parrot_PMC_get_string_intkey>

Return the integer keyed string value of the passed-in PMC

=cut

*/

PARROT_API
Parrot_String
Parrot_PMC_get_string_intkey(PARROT_INTERP,
        Parrot_PMC pmc, Parrot_Int key)
{
    Parrot_String retval;
    PARROT_CALLIN_START(interp);
    retval = VTABLE_get_string_keyed_int(interp, pmc, key);
    PARROT_CALLIN_END(interp);
    return retval;
}


/*

=item C<void * Parrot_PMC_get_pointer_intkey>

Return a pointer to the PMC indicated by an integer key.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
void *
Parrot_PMC_get_pointer_intkey(PARROT_INTERP,
        Parrot_PMC pmc, Parrot_Int key)
{
    void *retval;
    PARROT_CALLIN_START(interp);
    retval = VTABLE_get_pointer_keyed_int(interp, pmc, key);
    PARROT_CALLIN_END(interp);
    return retval;
}

/*

=item C<Parrot_PMC Parrot_PMC_get_pmc_intkey>

Return the integer keyed PMC value of the passed-in PMC

=cut

*/

PARROT_API
Parrot_PMC
Parrot_PMC_get_pmc_intkey(PARROT_INTERP,
        Parrot_PMC pmc, Parrot_Int key)
{
    Parrot_PMC retval;
    PARROT_CALLIN_START(interp);
    retval = VTABLE_get_pmc_keyed_int(interp, pmc, key);
    PARROT_CALLIN_END(interp);
    return retval;
}

/*

=item C<Parrot_Int Parrot_PMC_get_intval>

Return the signed integer value of the value in the PMC.

=cut

*/

PARROT_API
Parrot_Int
Parrot_PMC_get_intval(PARROT_INTERP, Parrot_PMC pmc)
{
    Parrot_Int retval;
    PARROT_CALLIN_START(interp);
    retval = VTABLE_get_integer(interp, pmc);
    PARROT_CALLIN_END(interp);
    return retval;
}

/*

=item C<Parrot_Int Parrot_PMC_get_intval_intkey>

Return the keyed, signed integer value of the value in the PMC.

=cut

*/

PARROT_API
Parrot_Int
Parrot_PMC_get_intval_intkey(PARROT_INTERP,
        Parrot_PMC pmc, Parrot_Int key)
{
    Parrot_Int retval;
    PARROT_CALLIN_START(interp);
    retval = VTABLE_get_integer_keyed_int(interp, pmc, key);
    PARROT_CALLIN_END(interp);
    return retval;
}

/*

=item C<Parrot_Int Parrot_PMC_get_intval_pmckey>

Return the keyed, signed integer value of the value in the PMC.

=cut

*/

PARROT_API
Parrot_Int
Parrot_PMC_get_intval_pmckey(PARROT_INTERP,
        Parrot_PMC pmc, Parrot_PMC key)
{
    Parrot_Int retval;
    PARROT_CALLIN_START(interp);
    retval = VTABLE_get_integer_keyed(interp, pmc, key);
    PARROT_CALLIN_END(interp);
    return retval;
}

/*

=item C<Parrot_Float Parrot_PMC_get_numval>

Return the floating-point value of the PMC.

=cut

*/

PARROT_API
Parrot_Float
Parrot_PMC_get_numval(PARROT_INTERP, Parrot_PMC pmc)
{
    Parrot_Float retval;
    PARROT_CALLIN_START(interp);
    retval = VTABLE_get_number(interp, pmc);
    PARROT_CALLIN_END(interp);
    return retval;
}

/*

=item C<Parrot_Float Parrot_PMC_get_numval_intkey>

Return the keyed, signed integer value of the value in the PMC.

=cut

*/

PARROT_API
Parrot_Float
Parrot_PMC_get_numval_intkey(PARROT_INTERP,
        Parrot_PMC pmc, Parrot_Int key)
{
    Parrot_Float retval;
    PARROT_CALLIN_START(interp);
    retval = VTABLE_get_number_keyed_int(interp, pmc, key);
    PARROT_CALLIN_END(interp);
    return retval;
}

/*

=item C<char * Parrot_PMC_get_cstring_intkey>

Return a null-terminated string that represents the string value of the PMC.

Note that you must free this string with C<string_cstring_free()>!

=cut

*/

PARROT_API
PARROT_MALLOC
PARROT_CAN_RETURN_NULL
char *
Parrot_PMC_get_cstring_intkey(PARROT_INTERP,
        Parrot_PMC pmc, Parrot_Int key)
{
    STRING *intermediate;
    char   *retval;

    PARROT_CALLIN_START(interp);
    intermediate = VTABLE_get_string_keyed_int(interp, pmc, key);
    retval       = string_to_cstring(interp, intermediate);
    PARROT_CALLIN_END(interp);

    return retval;
}

/*

=item C<char * Parrot_PMC_get_cstring>

Return a null-terminated string that represents the string value of the PMC.

Note that you must free this string with C<string_cstring_free()>!

=cut

*/

PARROT_API
PARROT_MALLOC
PARROT_CAN_RETURN_NULL
char *
Parrot_PMC_get_cstring(PARROT_INTERP, Parrot_PMC pmc)
{
    STRING *intermediate;
    char   *retval;

    PARROT_CALLIN_START(interp);
    intermediate = VTABLE_get_string(interp, pmc);
    retval       = string_to_cstring(interp, intermediate);
    PARROT_CALLIN_END(interp);

    return retval;
}

/*

=item C<char * Parrot_PMC_get_cstringn>

Return a null-terminated string for the PMC, along with the length.

Yes, right now this is a bit of a cheat. It needs fixing, but without
disturbing the interface.

Note that you must free the string with C<string_cstring_free()>.

=cut

*/

PARROT_API
PARROT_MALLOC
PARROT_CAN_RETURN_NULL
char *
Parrot_PMC_get_cstringn(PARROT_INTERP, ARGIN(Parrot_PMC pmc),
        ARGOUT(Parrot_Int *length))
{
    char *retval;

    PARROT_CALLIN_START(interp);
    retval  = string_to_cstring(interp, VTABLE_get_string(interp, pmc));
    *length = strlen(retval);
    PARROT_CALLIN_END(interp);

    return retval;
}

/*

=item C<char * Parrot_PMC_get_cstringn_intkey>

Return a null-terminated string for the PMC, along with the length.

Yes, right now this is a bit of a cheat. It needs fixing, but without
disturbing the interface.

Note that you must free this string with C<string_cstring_free()>.

=cut

*/

PARROT_API
PARROT_MALLOC
PARROT_CAN_RETURN_NULL
char *
Parrot_PMC_get_cstringn_intkey(PARROT_INTERP, ARGIN(Parrot_PMC pmc),
        ARGOUT(Parrot_Int *length), Parrot_Int key)
{
    char *retval;

    PARROT_CALLIN_START(interp);
    retval  = string_to_cstring(interp,
                               VTABLE_get_string_keyed_int(interp, pmc, key));
    *length = strlen(retval);
    PARROT_CALLIN_END(interp);

    return retval;
}

/*

=item C<void Parrot_PMC_set_string>

Assign the passed-in Parrot string to the passed-in PMC.

=cut

*/

PARROT_API
void
Parrot_PMC_set_string(PARROT_INTERP,
        Parrot_PMC pmc, Parrot_String value)
{
    PARROT_CALLIN_START(interp);
    VTABLE_set_string_native(interp, pmc, value);
    PARROT_CALLIN_END(interp);
}

/*

=item C<void Parrot_PMC_set_string_intkey>

Assign the passed-in Parrot string to the passed-in PMC.

=cut

*/

PARROT_API
void
Parrot_PMC_set_string_intkey(PARROT_INTERP,
        Parrot_PMC pmc, Parrot_Int key, Parrot_String value)
{
    PARROT_CALLIN_START(interp);
    VTABLE_set_string_keyed_int(interp, pmc, key, value);
    PARROT_CALLIN_END(interp);
}

/*

=item C<void Parrot_PMC_set_pmc_intkey>

Assign the passed-in pmc to the passed-in slot of the passed-in PMC.

=cut

*/

PARROT_API
void
Parrot_PMC_set_pmc_intkey(PARROT_INTERP,
        Parrot_PMC pmc, Parrot_Int key, Parrot_PMC value)
{
    PARROT_CALLIN_START(interp);
    VTABLE_set_pmc_keyed_int(interp, pmc, key, value);
    PARROT_CALLIN_END(interp);
}

/*

=item C<void Parrot_PMC_set_pmc_pmckey>

Assign the passed-in pmc to the passed-in slot of the passed-in PMC.

=cut

*/

PARROT_API
void
Parrot_PMC_set_pmc_pmckey(PARROT_INTERP,
        Parrot_PMC pmc, Parrot_PMC key, Parrot_PMC value)
{
    PARROT_CALLIN_START(interp);
    VTABLE_set_pmc_keyed(interp, pmc, key, value);
    PARROT_CALLIN_END(interp);
}

/*

=item C<void Parrot_PMC_set_pointer_intkey>

Assign the passed-in pointer to the passed-in PMC.

=cut

*/

PARROT_API
void
Parrot_PMC_set_pointer_intkey(PARROT_INTERP,
        ARGIN(Parrot_PMC pmc), Parrot_Int key, ARGIN_NULLOK(void *value))
{
    PARROT_CALLIN_START(interp);
    VTABLE_set_pointer_keyed_int(interp, pmc, key, value);
    PARROT_CALLIN_END(interp);
}

/*

=item C<void Parrot_PMC_set_intval>

Assign the passed-in Parrot integer to the passed-in PMC.

=cut

*/

PARROT_API
void
Parrot_PMC_set_intval(PARROT_INTERP, Parrot_PMC pmc, Parrot_Int value)
{
    PARROT_CALLIN_START(interp);
    VTABLE_set_integer_native(interp, pmc, value);
    PARROT_CALLIN_END(interp);
}

/*

=item C<void Parrot_PMC_set_intval_intkey>

Assign the passed-in Parrot integer to the passed-in PMC.

=cut

*/

PARROT_API
void
Parrot_PMC_set_intval_intkey(PARROT_INTERP,
        Parrot_PMC pmc, Parrot_Int key, Parrot_Int value)
{
    PARROT_CALLIN_START(interp);
    VTABLE_set_integer_keyed_int(interp, pmc, key, value);
    PARROT_CALLIN_END(interp);
}

/*

=item C<void Parrot_PMC_set_numval>

Assign the passed-in Parrot number to the passed-in PMC.

=cut

*/

PARROT_API
void
Parrot_PMC_set_numval(PARROT_INTERP, Parrot_PMC pmc, Parrot_Float value)
{
    PARROT_CALLIN_START(interp);
    VTABLE_set_number_native(interp, pmc, value);
    PARROT_CALLIN_END(interp);
}

/*

=item C<void Parrot_PMC_set_numval_intkey>

Assign the passed-in Parrot number to the passed-in PMC.

=cut

*/

PARROT_API
void
Parrot_PMC_set_numval_intkey(PARROT_INTERP,
        Parrot_PMC pmc, Parrot_Int key, Parrot_Float value)
{
    PARROT_CALLIN_START(interp);
    VTABLE_set_number_keyed_int(interp, pmc, key, value);
    PARROT_CALLIN_END(interp);
}

/*

=item C<void Parrot_PMC_set_cstring>

Assign the passed-in null-terminated C string to the passed-in PMC.

=cut

*/

PARROT_API
void
Parrot_PMC_set_cstring(PARROT_INTERP, Parrot_PMC pmc, ARGIN_NULLOK(const char *value))
{
    PARROT_CALLIN_START(interp);
    VTABLE_set_string_native(interp, pmc,
                             string_from_cstring(interp, value, 0));
    PARROT_CALLIN_END(interp);
}

/*

=item C<void Parrot_PMC_set_cstring_intkey>

Assign the passed-in null-terminated C string to the passed-in PMC.

=cut

*/

PARROT_API
void
Parrot_PMC_set_cstring_intkey(PARROT_INTERP,
        Parrot_PMC pmc, Parrot_Int key, ARGIN_NULLOK(const char *value))
{
    PARROT_CALLIN_START(interp);
    VTABLE_set_string_keyed_int(interp, pmc, key,
                                string_from_cstring(interp, value, 0));
    PARROT_CALLIN_END(interp);
}

/*

=item C<void Parrot_PMC_set_cstringn>

Assign the passed-in length-noted string to the passed-in PMC.

=cut

*/

PARROT_API
void
Parrot_PMC_set_cstringn(PARROT_INTERP,
        Parrot_PMC pmc, ARGIN_NULLOK(const char *value), Parrot_Int length)
{
    PARROT_CALLIN_START(interp);
    VTABLE_set_string_native(interp, pmc,
                             string_from_cstring(interp, value, length));
    PARROT_CALLIN_END(interp);
}

/*

=item C<void Parrot_PMC_push_intval>

Push the passed-in Parrot integer onto the passed in PMC

=cut

*/

PARROT_API
void
Parrot_PMC_push_intval(PARROT_INTERP,
        Parrot_PMC pmc, Parrot_Int value)
{
    PARROT_CALLIN_START(interp);
    VTABLE_push_integer(interp, pmc, value);
    PARROT_CALLIN_END(interp);
}

/*

=item C<void Parrot_PMC_push_numval>

Push the passed-in Parrot number onto the passed in PMC

=cut

*/

PARROT_API
void
Parrot_PMC_push_numval(PARROT_INTERP, Parrot_PMC pmc, Parrot_Float value)
{
    PARROT_CALLIN_START(interp);
    VTABLE_push_float(interp, pmc, value);
    PARROT_CALLIN_END(interp);
}

/*

=item C<void Parrot_PMC_delete_pmckey>

Deletes the value associated with the passed-in PMC from the PMC.

=cut

*/

PARROT_API
void
Parrot_PMC_delete_pmckey(PARROT_INTERP, Parrot_PMC pmc, Parrot_PMC key)
{
    PARROT_CALLIN_START(interp);
    VTABLE_delete_keyed(interp, pmc, key);
    PARROT_CALLIN_END(interp);
}

/*

=item C<void Parrot_PMC_set_cstringn_intkey>

Assign the passed-in length-noted string to the passed-in PMC.

=cut

*/

PARROT_API
void
Parrot_PMC_set_cstringn_intkey(PARROT_INTERP,
        Parrot_PMC pmc, Parrot_Int key, ARGIN_NULLOK(const char *value), Parrot_Int length)
{
    PARROT_CALLIN_START(interp);
    VTABLE_set_string_keyed_int(interp, pmc, key,
                                string_from_cstring(interp, value, length));
    PARROT_CALLIN_END(interp);
}

/*

=item C<Parrot_PMC Parrot_PMC_new>

Create and return a new PMC.

=cut

*/

PARROT_API
Parrot_PMC
Parrot_PMC_new(PARROT_INTERP, Parrot_Int type)
{
    Parrot_PMC newpmc;
    PARROT_CALLIN_START(interp);
    newpmc = pmc_new_noinit(interp, type);
    VTABLE_init(interp, newpmc);
    PARROT_CALLIN_END(interp);
    return newpmc;
}

/*

=item C<Parrot_Int Parrot_PMC_typenum>

Returns the internal identifier that represents the named class.

=cut

*/

PARROT_API
Parrot_Int
Parrot_PMC_typenum(PARROT_INTERP, ARGIN_NULLOK(const char *_class))
{
    Parrot_Int retval;
    PARROT_CALLIN_START(interp);
    retval = pmc_type(interp, string_from_cstring(interp, _class, 0));
    PARROT_CALLIN_END(interp);
    return retval;
}

/*

=item C<Parrot_PMC Parrot_PMC_null>

Returns the special C<NULL> PMC.

=cut

*/

PARROT_API
Parrot_PMC
Parrot_PMC_null(void)
{
    return PMCNULL;
}

/*

=item C<void Parrot_free_cstring>

Deallocate a C string that the interpreter has handed to you.

=cut

*/

PARROT_API
void
Parrot_free_cstring(ARGIN_NULLOK(char *string))
{
    string_cstring_free(string);
}

/*

=item C<void* Parrot_call_sub>

Call a parrot subroutine with the given function signature. The first char in
C<signature> denotes the return value. Next chars are arguments.

The return value of this function can be void or a pointer type.

Signature chars are:

    v ... void return
    I ... Parrot_Int
    N ... Parrot_Float
    S ... Parrot_String
    P ... Parrot_PMC

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
void*
Parrot_call_sub(PARROT_INTERP, Parrot_PMC sub,
                 ARGIN(const char *signature), ...)
{
    va_list ap;
    void *result;

    PARROT_CALLIN_START(interp);

    va_start(ap, signature);
    CONTEXT(interp)->constants =
        PMC_sub(sub)->seg->const_table->constants;
    result = Parrot_runops_fromc_arglist(interp, sub, signature, ap);
    va_end(ap);

    PARROT_CALLIN_END(interp);
    return result;
}

/*

=item C<Parrot_Int Parrot_call_sub_ret_int>

Like C<Parrot_call_sub>, with Parrot_Int return result.

=cut

*/

PARROT_API
Parrot_Int
Parrot_call_sub_ret_int(PARROT_INTERP, Parrot_PMC sub,
                 ARGIN(const char *signature), ...)
{
    va_list ap;
    Parrot_Int result;

    PARROT_CALLIN_START(interp);

    va_start(ap, signature);
    CONTEXT(interp)->constants =
        PMC_sub(sub)->seg->const_table->constants;
    result = Parrot_runops_fromc_arglist_reti(interp, sub, signature, ap);
    va_end(ap);

    PARROT_CALLIN_END(interp);
    return result;
}

/*

=item C<Parrot_Float Parrot_call_sub_ret_float>

Like C<Parrot_call_sub>, with Parrot_Float return result.

=cut

*/

PARROT_API
Parrot_Float
Parrot_call_sub_ret_float(PARROT_INTERP, Parrot_PMC sub,
                 ARGIN(const char *signature), ...)
{
    va_list      ap;
    Parrot_Float result;

    PARROT_CALLIN_START(interp);

    va_start(ap, signature);
    CONTEXT(interp)->constants =
        PMC_sub(sub)->seg->const_table->constants;
    result = Parrot_runops_fromc_arglist_retf(interp, sub, signature, ap);
    va_end(ap);

    PARROT_CALLIN_END(interp);
    return result;
}

/*

=item C<void * Parrot_call_method>

Call the parrot subroutine C<sub> as a method on PMC object C<obj>. The method
should have the name C<method> as a Parrot_string, and should have a function
signature C<signature>. Any arguments to the method can be passed at the end
as a variadic argument list.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
void *
Parrot_call_method(PARROT_INTERP, Parrot_PMC sub, Parrot_PMC obj,
                        Parrot_String method, ARGIN(const char *signature), ...)
{
    void    *result;
    va_list ap;

    PARROT_CALLIN_START(interp);
    va_start(ap, signature);
    result = Parrot_run_meth_fromc_arglist(interp, sub,
            obj, method, signature, ap);
    va_end(ap);
    PARROT_CALLIN_END(interp);
    return result;
}

/*

=item C<Parrot_Int Parrot_call_method_ret_int>

Call a parrot method for the given object.

=cut

*/

PARROT_API
Parrot_Int
Parrot_call_method_ret_int(PARROT_INTERP, Parrot_PMC sub,
        Parrot_PMC obj, Parrot_String method, ARGIN(const char *signature), ...)
{
    Parrot_Int result;
    va_list    ap;

    PARROT_CALLIN_START(interp);
    va_start(ap, signature);
    result = Parrot_run_meth_fromc_arglist_reti(interp, sub,
            obj, method, signature, ap);
    va_end(ap);
    PARROT_CALLIN_END(interp);
    return result;
}

/*

=item C<Parrot_Float Parrot_call_method_ret_float>

Call a parrot method for the given object.

=cut

*/

PARROT_API
Parrot_Float
Parrot_call_method_ret_float(PARROT_INTERP, Parrot_PMC sub,
        Parrot_PMC obj, Parrot_String method, ARGIN(const char *signature), ...)
{
    Parrot_Float result;
    va_list      ap;

    PARROT_CALLIN_START(interp);
    va_start(ap, signature);
    result = Parrot_run_meth_fromc_arglist_retf(interp, sub,
            obj, method, signature, ap);
    va_end(ap);
    PARROT_CALLIN_END(interp);
    return result;
}

/*

=item C<Parrot_Int Parrot_get_intreg>

Return the value of an integer register.

=cut

*/

PARROT_API
Parrot_Int
Parrot_get_intreg(PARROT_INTERP, Parrot_Int regnum)
{
    return REG_INT(interp, regnum);
}

/*

=item C<Parrot_Float Parrot_get_numreg>

Return the value of a numeric register.

=cut

*/

PARROT_API
Parrot_Float
Parrot_get_numreg(PARROT_INTERP, Parrot_Int regnum)
{
    return REG_NUM(interp, regnum);
}

/*

=item C<Parrot_String Parrot_get_strreg>

Return the value of a string register.

=cut

*/

PARROT_API
Parrot_String
Parrot_get_strreg(PARROT_INTERP, Parrot_Int regnum)
{
    return REG_STR(interp, regnum);
}

/*

=item C<Parrot_PMC Parrot_get_pmcreg>

Return the value of a PMC register.

=cut

*/

PARROT_API
Parrot_PMC
Parrot_get_pmcreg(PARROT_INTERP, Parrot_Int regnum)
{
    return REG_PMC(interp, regnum);
}

/*

=item C<void Parrot_set_intreg>

Set the value of an I register.

=cut

*/

PARROT_API
void
Parrot_set_intreg(PARROT_INTERP, Parrot_Int regnum,
                  Parrot_Int value)
{
    REG_INT(interp, regnum) = value;
}

/*

=item C<void Parrot_set_numreg>

Set the value of an N register.

=cut

*/

PARROT_API
void
Parrot_set_numreg(PARROT_INTERP, Parrot_Int regnum,
                  Parrot_Float value)
{
    REG_NUM(interp, regnum) = value;
}

/*

=item C<void Parrot_set_strreg>

Set the value of an S register.

*/

PARROT_API
void
Parrot_set_strreg(PARROT_INTERP, Parrot_Int regnum,
                  Parrot_String value)
{
    REG_STR(interp, regnum) = value;
}

/*

=item C<void Parrot_set_pmcreg>

Set the value of a P register.

=cut

*/

PARROT_API
void
Parrot_set_pmcreg(PARROT_INTERP, Parrot_Int regnum,
                  Parrot_PMC value)
{
    REG_PMC(interp, regnum) = value;
}

/*=for api extend Parrot_new_string
 *
 */
/*

=item C<Parrot_String Parrot_new_string>

Create a new Parrot string from a passed-in buffer. Pass in a 0 for
flags for right now.

A copy of the buffer is made.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
Parrot_String
Parrot_new_string(PARROT_INTERP, ARGIN_NULLOK(const char *buffer), int length,
        ARGIN_NULLOK(const char * const encoding_name), Parrot_Int flags)
{
    Parrot_String retval;
    PARROT_CALLIN_START(interp);
    retval = string_make(interp, buffer, length, encoding_name, flags);
    PARROT_CALLIN_END(interp);
    return retval;
}

/*

=item C<Parrot_Language Parrot_find_language>

Find the magic language token for a language, by language name.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
Parrot_Language
Parrot_find_language(SHIM_INTERP, SHIM(char *language))
{
    return 0;
}

/*

=item C<void Parrot_register_pmc>

Add a reference of the PMC to the interpreters DOD registry. This
prevents PMCs only known to extension from getting destroyed during DOD
runs.

=cut

*/

PARROT_API
void
Parrot_register_pmc(PARROT_INTERP, Parrot_PMC pmc)
{
    PARROT_CALLIN_START(interp);
    dod_register_pmc(interp, pmc);
    PARROT_CALLIN_END(interp);
}

/*

=item C<void Parrot_unregister_pmc>

Remove a reference of the PMC from the interpreters DOD registry. If the
reference count reaches zero, the PMC will be destroyed during the next
DOD run.

=cut

*/

PARROT_API
void
Parrot_unregister_pmc(PARROT_INTERP, Parrot_PMC pmc)
{
    PARROT_CALLIN_START(interp);
    dod_unregister_pmc(interp, pmc);
    PARROT_CALLIN_END(interp);
}

/*

=item C<void Parrot_PMC_set_vtable>

Replaces the vtable of the PMC.

=cut

*/

PARROT_API
void
Parrot_PMC_set_vtable(SHIM_INTERP, Parrot_PMC pmc, Parrot_VTABLE vtable)
{
    pmc->vtable = vtable;
}

/*

=item C<Parrot_VTABLE Parrot_get_vtable>

Returns the vtable corresponding to the given PMC ID.

=cut

*/

PARROT_API
PARROT_PURE_FUNCTION
Parrot_VTABLE
Parrot_get_vtable(PARROT_INTERP, Parrot_Int id)
{
    return interp->vtables[id];
}

/*

=back

=head1 SEE ALSO

See F<include/parrot/extend.h> and F<docs/pdds/pdd11_extending.pod>.

=head1 HISTORY

Initial version by Dan Sugalski.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
