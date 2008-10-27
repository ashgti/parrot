/*
Copyright (C) 2001-2008, The Perl Foundation.
$Id$

=head1 NAME

src/pmc.c - The base vtable calling functions

=head1 DESCRIPTION

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "pmc.str"

/* HEADERIZER HFILE: include/parrot/pmc.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static PMC* create_class_pmc(PARROT_INTERP, INTVAL type)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static PMC* get_new_pmc_header(PARROT_INTERP,
    INTVAL base_type,
    UINTVAL flags)
        __attribute__nonnull__(1);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */


#if PARROT_CATCH_NULL
PMC * PMCNULL;
#endif

/*

=item C<INTVAL PMC_is_null>

Tests if the given pmc is null.

=cut

*/

PARROT_API
INTVAL
PMC_is_null(SHIM_INTERP, NULLOK(const PMC *pmc))
{
#if PARROT_CATCH_NULL
    return pmc == PMCNULL || pmc == NULL;
#else
    return pmc == NULL;
#endif
}

/*

=item C<PMC * pmc_new>

Creates a new PMC of type C<base_type> (which is an index into the list of PMC
types declared in C<vtables> in F<include/parrot/pmc.h>). Once the PMC has been
successfully created and its vtable pointer initialized, we call its C<init>
method to perform any other necessary initialization.

=cut

*/

PARROT_API
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
pmc_new(PARROT_INTERP, INTVAL base_type)
{
    PARROT_ASSERT(interp->vtables[base_type]);
    {
        PMC *const classobj = interp->vtables[base_type]->pmc_class;

        if (!PMC_IS_NULL(classobj) && PObj_is_class_TEST(classobj))
            return VTABLE_instantiate(interp, classobj, PMCNULL);
        else {
            PMC * const pmc = get_new_pmc_header(interp, base_type, 0);
            VTABLE_init(interp, pmc);
            return pmc;
        }
    }
}

/*

=item C<PMC* pmc_reuse>

Reuse an existing PMC, turning it into an empty PMC of the new type. Any
required internal structure will be put in place (such as the extension area)
and the PMC will be ready to go. This will throw an exception if the PMC is
constant or of a singleton type (such as the environment PMC) or is being
turned into a PMC of a singleton type.

=cut

*/

PARROT_API
PARROT_CANNOT_RETURN_NULL
PMC*
pmc_reuse(PARROT_INTERP, ARGIN(PMC *pmc), INTVAL new_type,
          SHIM(UINTVAL flags))
{
    INTVAL has_ext, new_flags;
    VTABLE *new_vtable;

    if (pmc->vtable->base_type == new_type)
        return pmc;

    new_vtable = interp->vtables[new_type];

    /* Singleton/const PMCs/types are not eligible */

    if ((pmc->vtable->flags | new_vtable->flags)
        & (VTABLE_PMC_IS_SINGLETON | VTABLE_IS_CONST_FLAG))
    {
        /* First, is the destination a singleton? No joy for us there */
        if (new_vtable->flags & VTABLE_PMC_IS_SINGLETON)
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_ALLOCATION_ERROR,
                "Parrot VM: Can't turn to a singleton type!\n");

        /* First, is the destination a constant? No joy for us there */
        if (new_vtable->flags & VTABLE_IS_CONST_FLAG)
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_ALLOCATION_ERROR,
                "Parrot VM: Can't turn to a constant type!\n");

        /* Is the source a singleton? */
        if (pmc->vtable->flags & VTABLE_PMC_IS_SINGLETON)
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_ALLOCATION_ERROR,
                "Parrot VM: Can't modify a singleton\n");

        /* Is the source constant? */
        if (pmc->vtable->flags & VTABLE_IS_CONST_FLAG)
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_ALLOCATION_ERROR,
                "Parrot VM: Can't modify a constant\n");
    }

    /* Do we have an extension area? */
    has_ext = (PObj_is_PMC_EXT_TEST(pmc) && pmc->pmc_ext);

    /* Do we need one? */
    if (new_vtable->flags & VTABLE_PMC_NEEDS_EXT) {
        if (!has_ext) {
            /* If we need an ext area, go allocate one */
            add_pmc_ext(interp, pmc);
        }
        new_flags = PObj_is_PMC_EXT_FLAG;
    }
    else {
        if (has_ext)
            Parrot_free_pmc_ext(interp, pmc);

#if ! PMC_DATA_IN_EXT
        PMC_data(pmc) = NULL;
#endif
        new_flags = 0;
    }

    /* we are a PMC + maybe is_PMC_EXT */
    PObj_flags_SETTO(pmc, PObj_is_PMC_FLAG | new_flags);

    /* Set the right vtable */
    pmc->vtable = new_vtable;

    /* Call the base init for the redone pmc */
    VTABLE_init(interp, pmc);

    return pmc;
}

/*

=item C<static PMC* get_new_pmc_header>

Gets a new PMC header.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static PMC*
get_new_pmc_header(PARROT_INTERP, INTVAL base_type, UINTVAL flags)
{
    PMC    *pmc;
    VTABLE *vtable = interp->vtables[base_type];
    UINTVAL vtable_flags;

    /* This is usually because you either didn't call init_world early enough,
     * you added a new PMC class without adding Parrot_(classname)_class_init
     * to init_world, or you forgot to run 'make realclean' after adding a new
     * PMC class.  */
    if (!vtable)
        PANIC(interp, "Null vtable used; did you add a new PMC?");

    vtable_flags = vtable->flags;

    /* we only have one global Env object, living in the interp */
    if (vtable_flags & VTABLE_PMC_IS_SINGLETON) {
        /*
         * singletons (monadic objects) exist only once, the interface
         * with the class is:
         * - get_pointer: return NULL or a pointer to the single instance
         * - set_pointer: set the only instance once
         *
         * - singletons are created in the constant pmc pool
         */
        PMC *pmc = (PMC *)(vtable->get_pointer)(interp, NULL);

        /* LOCK */
        if (!pmc) {
            pmc            = new_pmc_header(interp, PObj_constant_FLAG);
            PARROT_ASSERT(pmc);

            pmc->vtable    = vtable;
            pmc->real_self = pmc;
            VTABLE_set_pointer(interp, pmc, pmc);
        }

        return pmc;
    }

    if (vtable_flags & VTABLE_IS_CONST_PMC_FLAG) {
        flags |= PObj_constant_FLAG;
    }
    else if (vtable_flags & VTABLE_IS_CONST_FLAG) {
        /* put the normal vtable in, so that the pmc can be initialized first
         * parrot or user code has to set the _ro property then,
         * to morph the PMC to the const variant
         * This assumes that a constant PMC enum is one bigger then
         * the normal one.
         */

        /*
         * XXX not yet we can't assure that all contents in the
         * const PMC is const too
         * see e.g. t/pmc/sarray_13.pir
         */
#if 0
        flags |= PObj_constant_FLAG;
#endif
        --base_type;
        vtable = interp->vtables[base_type];
    }

    if (vtable_flags & VTABLE_PMC_NEEDS_EXT) {
        flags |= PObj_is_PMC_EXT_FLAG;
        if (vtable_flags & VTABLE_IS_SHARED_FLAG)
            flags |= PObj_is_PMC_shared_FLAG;
    }

    pmc            = new_pmc_header(interp, flags);
    pmc->vtable    = vtable;
    pmc->real_self = pmc;

#ifdef GC_VERBOSE
    if (Interp_flags_TEST(interp, PARROT_TRACE_FLAG)) {
        /* XXX make a more verbose trace flag */
        fprintf(stderr, "\t=> new %p type %d\n", pmc, (int)base_type);
    }
#endif

    return pmc;
}


/*

=item C<PMC * pmc_new_noinit>

Creates a new PMC of type C<base_type> (which is an index into the list of PMC
types declared in C<vtables> in F<include/parrot/pmc.h>). Unlike C<pmc_new()>,
C<pmc_new_noinit()> does not call its C<init> method.  This allows separate
allocation and initialization for continuations.

=cut

*/

PARROT_API
PARROT_CANNOT_RETURN_NULL
PMC *
pmc_new_noinit(PARROT_INTERP, INTVAL base_type)
{
    PMC *const classobj = interp->vtables[base_type]->pmc_class;

    if (!PMC_IS_NULL(classobj) && PObj_is_class_TEST(classobj))
        return VTABLE_instantiate(interp, classobj, PMCNULL);

    return get_new_pmc_header(interp, base_type, 0);
}

/*

=item C<PMC * constant_pmc_new_noinit>

Creates a new constant PMC of type C<base_type>.

=cut

*/

PARROT_API
PARROT_CANNOT_RETURN_NULL
PMC *
constant_pmc_new_noinit(PARROT_INTERP, INTVAL base_type)
{
    return get_new_pmc_header(interp, base_type, PObj_constant_FLAG);
}

/*

=item C<PMC * constant_pmc_new>

Creates a new constant PMC of type C<base_type>, the call C<init>.

=cut

*/

PARROT_API
PARROT_CANNOT_RETURN_NULL
PMC *
constant_pmc_new(PARROT_INTERP, INTVAL base_type)
{
    PMC * const pmc = get_new_pmc_header(interp, base_type,
            PObj_constant_FLAG);
    VTABLE_init(interp, pmc);
    return pmc;
}

/*

=item C<PMC * pmc_new_init>

As C<pmc_new()>, but passes C<init> to the PMC's C<init_pmc()> method.

=cut

*/

PARROT_API
PARROT_CANNOT_RETURN_NULL
PMC *
pmc_new_init(PARROT_INTERP, INTVAL base_type, ARGOUT(PMC *init))
{
    PMC *const classobj = interp->vtables[base_type]->pmc_class;

    if (!PMC_IS_NULL(classobj) && PObj_is_class_TEST(classobj))
        return VTABLE_instantiate(interp, classobj, init);
    else {
        PMC * const pmc = get_new_pmc_header(interp, base_type, 0);
        VTABLE_init_pmc(interp, pmc, init);
        return pmc;
    }
}

/*

=item C<PMC * constant_pmc_new_init>

As C<constant_pmc_new>, but passes C<init> to the PMC's C<init_pmc> method.

=cut

*/

PARROT_API
PARROT_CANNOT_RETURN_NULL
PMC *
constant_pmc_new_init(PARROT_INTERP, INTVAL base_type, ARGIN_NULLOK(PMC *init))
{
    PMC * const pmc = get_new_pmc_header(interp, base_type, PObj_constant_FLAG);
    VTABLE_init_pmc(interp, pmc, init);
    return pmc;
}

/*

=item C<INTVAL pmc_register>

This segment handles PMC registration and such.

=cut

*/

PARROT_API
INTVAL
pmc_register(PARROT_INTERP, ARGIN(STRING *name))
{
    PMC *classname_hash;

    /* If they're looking to register an existing class, return that
       class' type number */
    INTVAL type = pmc_type(interp, name);

    if (type > enum_type_undef)
        return type;

    if (type < enum_type_undef)
        Parrot_ex_throw_from_c_args(interp, NULL, 1,
            "undefined type already exists - can't register PMC");

    classname_hash = interp->class_hash;
    type           = interp->n_vtable_max++;

    /* Have we overflowed the table? */
    if (type >= interp->n_vtable_alloced)
        parrot_realloc_vtables(interp);

    /* set entry in name->type hash */
    VTABLE_set_integer_keyed_str(interp, classname_hash, name, type);

    return type;
}

/*

=item C<INTVAL pmc_type>

Returns the PMC type for C<name>.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL
pmc_type(PARROT_INTERP, ARGIN_NULLOK(STRING *name))
{
    if (!name)
        return enum_type_undef;
    else {
        PMC * const classname_hash = interp->class_hash;
        PMC * const item           =
            (PMC *)VTABLE_get_pointer_keyed_str(interp, classname_hash, name);

        /* nested namespace with same name */
        if (item->vtable->base_type == enum_class_NameSpace)
            return enum_type_undef;

        if (!PMC_IS_NULL(item))
            return VTABLE_get_integer(interp, item);

        return Parrot_get_datatype_enum(interp, name);
    }
}

/*

=item C<INTVAL pmc_type_p>

Returns the PMC type for C<name>.

=cut

*/

PARROT_API
INTVAL
pmc_type_p(PARROT_INTERP, ARGIN(PMC *name))
{
    PMC * const classname_hash = interp->class_hash;
    PMC * const item           =
        (PMC *)VTABLE_get_pointer_keyed(interp, classname_hash, name);

    if (!PMC_IS_NULL(item))
        return VTABLE_get_integer(interp, item);

    return 0;
}

/*

=item C<static PMC* create_class_pmc>

Create a class object for this interpreter.  Takes an interpreter
name and type as arguments.  Returns a pointer to the class object.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static PMC*
create_class_pmc(PARROT_INTERP, INTVAL type)
{
    /*
     * class interface - a PMC is its own class
     * put an instance of this PMC into class
     *
     * create a constant PMC
     */
    PMC * const _class = get_new_pmc_header(interp, type,
                                           PObj_constant_FLAG);

    /* If we are a second thread, we may get the same object as the
     * original because we have a singleton. Just set the singleton to
     * be our class object, but don't mess with its vtable.
     */
    if ((interp->vtables[type]->flags & VTABLE_PMC_IS_SINGLETON)
        && (_class == _class->vtable->pmc_class)) {
        interp->vtables[type]->pmc_class = _class;
    }
    else {
        if (PObj_is_PMC_EXT_TEST(_class))
            Parrot_free_pmc_ext(interp, _class);

        DOD_flag_CLEAR(is_special_PMC, _class);

        PMC_pmc_val(_class)    = (PMC  *)0xdeadbeef;
        PMC_struct_val(_class) = (void *)0xdeadbeef;

        PObj_is_PMC_shared_CLEAR(_class);

        interp->vtables[type]->pmc_class = _class;
    }

    return _class;
}

/*

=item C<void Parrot_create_mro>

Create the MRO (method resolution order) array for this type.

=cut

*/

PARROT_API
void
Parrot_create_mro(PARROT_INTERP, INTVAL type)
{
    PMC    *_class, *mro;
    INTVAL i, count;

    VTABLE *vtable   = interp->vtables[type];
    PMC    *mro_list = vtable->mro;

    /* multithreaded: has already mro */
    if (mro_list && mro_list->vtable->base_type != enum_class_ResizableStringArray)
        return;

    mro         = pmc_new(interp, enum_class_ResizablePMCArray);
    vtable->mro = mro;

    if (vtable->ro_variant_vtable)
        vtable->ro_variant_vtable->mro = mro;

    count = VTABLE_elements(interp, mro_list);

    for (i = 0; i < count; ++i) {
        STRING *class_name  = VTABLE_get_string_keyed_int(interp, mro_list, i);
        INTVAL  parent_type = pmc_type(interp, class_name);

        /* abstract classes don't have a vtable */
        if (!parent_type)
            break;

        vtable = interp->vtables[parent_type];

        if (!vtable->_namespace) {
            /* need a namespace Hash, anchor at parent, name it */
            PMC * const ns = pmc_new(interp,
                    Parrot_get_ctx_HLL_type(interp, enum_class_NameSpace));
            vtable->_namespace = ns;

            /* anchor at parent, aka current_namespace, that is 'parrot' */
            VTABLE_set_pmc_keyed_str(interp,
                    CONTEXT(interp)->current_namespace, class_name, ns);
        }

        _class = vtable->pmc_class;
        if (!_class)
            _class = create_class_pmc(interp, parent_type);

        VTABLE_push_pmc(interp, mro, _class);
    }
}

/*

=back

=head2 DOD registry interface

=over 4

=item C<void dod_register_pmc>

Registers the PMC with the interpreter's DOD registry.

=cut

*/

PARROT_API
void
dod_register_pmc(PARROT_INTERP, ARGIN(PMC* pmc))
{
    /* Better not trigger a DOD run with a potentially unanchored PMC */
    Parrot_block_GC_mark(interp);

    PARROT_ASSERT(interp->DOD_registry);

    VTABLE_set_pmc_keyed(interp, interp->DOD_registry, pmc, PMCNULL);
    Parrot_unblock_GC_mark(interp);
}

/*

=item C<void dod_unregister_pmc>

Unregisters the PMC from the interpreter's DOD registry.

=cut

*/

void
dod_unregister_pmc(PARROT_INTERP, ARGIN(PMC* pmc))
{
    PARROT_ASSERT(interp->DOD_registry);

    VTABLE_delete_keyed(interp, interp->DOD_registry, pmc);
}



/*

=back

=head1 SEE ALSO

F<include/parrot/vtable.h>.

C<5.1.0.14.2.20011008152120.02158148@pop.sidhe.org>
(http://www.nntp.perl.org/group/perl.perl6.internals/5516).

=head1 HISTORY

Initial version by Simon on 2001.10.20.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
