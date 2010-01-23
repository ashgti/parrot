/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 NAME

src/pmc.c

=head1 DESCRIPTION

The base vtable calling functions

=head1 FUNCTIONS

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "pmc.str"
#include "pmc/pmc_class.h"
#include "pmc/pmc_callcontext.h"

/* HEADERIZER HFILE: include/parrot/pmc.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static void check_pmc_reuse_flags(PARROT_INTERP,
    UINTVAL srcflags,
    UINTVAL destflags)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static PMC * create_class_pmc(PARROT_INTERP, INTVAL type)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static PMC * get_new_pmc_header(PARROT_INTERP,
    INTVAL base_type,
    UINTVAL flags)
        __attribute__nonnull__(1);

PARROT_CANNOT_RETURN_NULL
static PMC* pmc_reuse_no_init(PARROT_INTERP,
    ARGIN(PMC *pmc),
    INTVAL new_type,
    SHIM(UINTVAL flags))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

#define ASSERT_ARGS_check_pmc_reuse_flags __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_create_class_pmc __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_get_new_pmc_header __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_pmc_reuse_no_init __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pmc))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */


#if PARROT_CATCH_NULL
PMC * PMCNULL;
#endif

/*

=item C<INTVAL PMC_is_null(PARROT_INTERP, const PMC *pmc)>

Tests if the given pmc is null.

=cut

*/

PARROT_EXPORT
INTVAL
PMC_is_null(SHIM_INTERP, ARGIN_NULLOK(const PMC *pmc))
{
    ASSERT_ARGS(PMC_is_null)
#if PARROT_CATCH_NULL
    return pmc == PMCNULL || pmc == NULL;
#else
    return pmc == NULL;
#endif
}

/*

=item C<void Parrot_pmc_destroy(PARROT_INTERP, PMC *pmc)>

Destroy a PMC. Call his destroy vtable function if needed, and deallocate
his attributes if they are automatically allocated.

For internal usage of the PMC handling functions and garbage collection
subsystem.

=cut

*/

PARROT_EXPORT
void
Parrot_pmc_destroy(PARROT_INTERP, ARGMOD(PMC *pmc))
{
    ASSERT_ARGS(Parrot_pmc_destroy)

    if (PObj_custom_destroy_TEST(pmc))
        VTABLE_destroy(interp, pmc);

    PObj_gc_CLEAR(pmc);

    if (PObj_is_PMC_shared_TEST(pmc) && PMC_sync(pmc))
        Parrot_gc_free_pmc_sync(interp, pmc);

    if (pmc->vtable->attr_size)
        Parrot_gc_free_pmc_attributes(interp, pmc);
    else
        PMC_data(pmc) = NULL;

#ifndef NDEBUG

    pmc->vtable      = (VTABLE  *)0xdeadbeef;

#endif

}

/*

=item C<PMC * pmc_new(PARROT_INTERP, INTVAL base_type)>

Creates a new PMC of type C<base_type> (which is an index into the list of PMC
types declared in C<vtables> in F<include/parrot/pmc.h>). Once the PMC has been
successfully created and its vtable pointer initialized, we call its C<init>
method to perform any other necessary initialization.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
pmc_new(PARROT_INTERP, INTVAL base_type)
{
    ASSERT_ARGS(pmc_new)
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

=item C<PMC * pmc_reuse(PARROT_INTERP, PMC *pmc, INTVAL new_type, UINTVAL
flags)>

Reuse an existing PMC, turning it into an empty PMC of the new type. Any
required internal structure will be put in place (such as the extension area)
and the PMC will be ready to go.

Cannot currently handle converting a non-Object PMC into an Object. Use
C<pmc_reuse_by_class> for that.


=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_IGNORABLE_RESULT
PMC *
pmc_reuse(PARROT_INTERP, ARGIN(PMC *pmc), INTVAL new_type,
          UINTVAL flags)
{
    ASSERT_ARGS(pmc_reuse)
    pmc = pmc_reuse_no_init(interp, pmc, new_type, flags);

    /* Call the base init for the redone pmc. Warning, this should not
       be called on Object PMCs. */
    VTABLE_init(interp, pmc);

    return pmc;
}

/*

=item C<PMC * pmc_reuse_init(PARROT_INTERP, PMC *pmc, INTVAL new_type, PMC
*init, UINTVAL flags)>

Reuse an existing PMC, turning it into an PMC of the new type. Any
required internal structure will be put in place (such as the extension area)
and the PMC will be inited.

Cannot currently handle converting a non-Object PMC into an Object. Use
C<pmc_reuse_by_class> for that.


=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_IGNORABLE_RESULT
PMC *
pmc_reuse_init(PARROT_INTERP, ARGIN(PMC *pmc), INTVAL new_type, ARGIN(PMC *init),
          UINTVAL flags)
{
    ASSERT_ARGS(pmc_reuse_init)
    pmc = pmc_reuse_no_init(interp, pmc, new_type, flags);

    /* Call the base init for the redone pmc. Warning, this should not
       be called on Object PMCs. */
    VTABLE_init_pmc(interp, pmc, init);

    return pmc;
}

/*

=item C<static PMC* pmc_reuse_no_init(PARROT_INTERP, PMC *pmc, INTVAL new_type,
UINTVAL flags)>

Prepare pmc for reuse. Do all scuffolding except initing.

=cut

*/
PARROT_CANNOT_RETURN_NULL
static PMC*
pmc_reuse_no_init(PARROT_INTERP, ARGIN(PMC *pmc), INTVAL new_type,
    SHIM(UINTVAL flags)) {

    ASSERT_ARGS(pmc_reuse_no_init)
    VTABLE *new_vtable;
    INTVAL  has_ext, new_flags = 0;

    if (pmc->vtable->base_type == new_type)
        return pmc;

    new_vtable = interp->vtables[new_type];

    /* Singleton/const PMCs/types are not eligible */
    check_pmc_reuse_flags(interp, pmc->vtable->flags, new_vtable->flags);

    /* Free the old PMC resources. */
    Parrot_pmc_destroy(interp, pmc);

    PObj_flags_SETTO(pmc, PObj_is_PMC_FLAG | new_flags);

    /* Set the right vtable */
    pmc->vtable = new_vtable;

    if (new_vtable->attr_size)
        Parrot_gc_allocate_pmc_attributes(interp, pmc);

    else
        PMC_data(pmc) = NULL;

    return pmc;
}

/*

=item C<PMC * pmc_reuse_by_class(PARROT_INTERP, PMC *pmc, PMC *class_, UINTVAL
flags)>

Reuse an existing PMC. Convert it to the type specified by the given Class
PMC. At the moment, this means we can only use this function to reuse PMCs
into types with Classes (not built-in PMCs). Use C<pmc_reuse> if you need
to convert to a built-in PMC type.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_IGNORABLE_RESULT
PMC *
pmc_reuse_by_class(PARROT_INTERP, ARGMOD(PMC *pmc), ARGIN(PMC *class_),
    UINTVAL flags)
{
    ASSERT_ARGS(pmc_reuse_by_class)
    const INTVAL   new_type   = PARROT_CLASS(class_)->id;
    VTABLE * const new_vtable = interp->vtables[new_type];
    INTVAL         new_flags  = flags;

    if (pmc->vtable->base_type == new_type)
        return pmc;

    /* Singleton/const PMCs/types are not eligible */
    check_pmc_reuse_flags(interp, pmc->vtable->flags, new_vtable->flags);

    Parrot_pmc_destroy(interp, pmc);

    PObj_flags_SETTO(pmc, PObj_is_PMC_FLAG | new_flags);

    /* Set the right vtable */
    pmc->vtable = new_vtable;

    if (new_vtable->attr_size)
        Parrot_gc_allocate_pmc_attributes(interp, pmc);
    else
        PMC_data(pmc) = NULL;

    return pmc;
}


/*

=item C<static void check_pmc_reuse_flags(PARROT_INTERP, UINTVAL srcflags,
UINTVAL destflags)>

We're converting one PMC type to another, either in C<pmc_reuse> or
C<pmc_reuse_by_class>. Check to make sure that neither the existing PMC
or the intended target PMC type are singletons or constants. We throw an
exception if we are attempting an illegal operation.

=cut

*/

static void
check_pmc_reuse_flags(PARROT_INTERP, UINTVAL srcflags, UINTVAL destflags)
{
    ASSERT_ARGS(check_pmc_reuse_flags)
    if ((srcflags | destflags) & (VTABLE_PMC_IS_SINGLETON | VTABLE_IS_CONST_FLAG))
    {
        /* First, is the destination a singleton? No joy for us there */
        if (destflags & VTABLE_PMC_IS_SINGLETON)
            Parrot_ex_throw_from_c_args(interp, NULL,
                EXCEPTION_ALLOCATION_ERROR,
                "Parrot VM: Can't turn to a singleton type!\n");

        /* Is the destination a constant? No joy for us there */
        if (destflags & VTABLE_IS_CONST_FLAG)
            Parrot_ex_throw_from_c_args(interp, NULL,
                EXCEPTION_ALLOCATION_ERROR,
                "Parrot VM: Can't turn to a constant type!\n");

        /* Is the source a singleton? */
        if (srcflags & VTABLE_PMC_IS_SINGLETON)
            Parrot_ex_throw_from_c_args(interp, NULL,
                EXCEPTION_ALLOCATION_ERROR,
                "Parrot VM: Can't modify a singleton\n");

        /* Is the source constant? */
        if (srcflags & VTABLE_IS_CONST_FLAG)
            Parrot_ex_throw_from_c_args(interp, NULL,
                EXCEPTION_ALLOCATION_ERROR,
                "Parrot VM: Can't modify a constant\n");
    }
}

/*

=item C<static PMC * get_new_pmc_header(PARROT_INTERP, INTVAL base_type, UINTVAL
flags)>

Gets a new PMC header of the given integer type. Initialize the pmc if
necessary. In the case of singleton PMC types, get the existing singleton
instead of allocating a new one.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static PMC *
get_new_pmc_header(PARROT_INTERP, INTVAL base_type, UINTVAL flags)
{
    ASSERT_ARGS(get_new_pmc_header)
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
         * singletons (monadic objects) exist only once
         * the interface * with the class is:
         * - get_pointer: return NULL or a pointer to the single instance
         * - set_pointer: set the only instance once
         *
         * - singletons are created in the constant pmc pool
         */
        PMC *pmc = (PMC *)(vtable->get_pointer)(interp, NULL);

        /* LOCK */
        if (!pmc) {
            pmc = Parrot_gc_new_pmc_header(interp, PObj_constant_FLAG);
            PARROT_ASSERT(pmc);

            pmc->vtable    = vtable;
            VTABLE_set_pointer(interp, pmc, pmc);
        }

        return pmc;
    }

    if (vtable_flags & VTABLE_IS_CONST_PMC_FLAG)
        flags |= PObj_constant_FLAG;
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

    if (vtable_flags & VTABLE_IS_SHARED_FLAG)
        flags |= PObj_is_PMC_shared_FLAG;

    pmc            = Parrot_gc_new_pmc_header(interp, flags);
    pmc->vtable    = vtable;

    if (vtable->attr_size)
        Parrot_gc_allocate_pmc_attributes(interp, pmc);

    return pmc;
}


/*

=item C<PMC * pmc_new_noinit(PARROT_INTERP, INTVAL base_type)>

Creates a new PMC of type C<base_type> (which is an index into the list of PMC
types declared in C<vtables> in F<include/parrot/pmc.h>). Unlike C<pmc_new()>,
C<pmc_new_noinit()> does not call its C<init> method.  This allows separate
allocation and initialization for continuations.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PMC *
pmc_new_noinit(PARROT_INTERP, INTVAL base_type)
{
    ASSERT_ARGS(pmc_new_noinit)
    PMC *const classobj = interp->vtables[base_type]->pmc_class;

    if (!PMC_IS_NULL(classobj) && PObj_is_class_TEST(classobj))
        return VTABLE_instantiate(interp, classobj, PMCNULL);

    return get_new_pmc_header(interp, base_type, 0);
}


/*

=item C<PMC * constant_pmc_new_noinit(PARROT_INTERP, INTVAL base_type)>

Creates a new constant PMC of type C<base_type>.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PMC *
constant_pmc_new_noinit(PARROT_INTERP, INTVAL base_type)
{
    ASSERT_ARGS(constant_pmc_new_noinit)
    return get_new_pmc_header(interp, base_type, PObj_constant_FLAG);
}


/*

=item C<PMC * constant_pmc_new(PARROT_INTERP, INTVAL base_type)>

Creates a new constant PMC of type C<base_type>, then calls its C<init>.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PMC *
constant_pmc_new(PARROT_INTERP, INTVAL base_type)
{
    ASSERT_ARGS(constant_pmc_new)
    PMC * const pmc = get_new_pmc_header(interp, base_type, PObj_constant_FLAG);
    VTABLE_init(interp, pmc);
    return pmc;
}


/*

=item C<PMC * pmc_new_init(PARROT_INTERP, INTVAL base_type, PMC *init)>

As C<pmc_new()>, but passes C<init> to the PMC's C<init_pmc()> vtable entry.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PMC *
pmc_new_init(PARROT_INTERP, INTVAL base_type, ARGOUT(PMC *init))
{
    ASSERT_ARGS(pmc_new_init)
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

=item C<PMC * constant_pmc_new_init(PARROT_INTERP, INTVAL base_type, PMC *init)>

As C<constant_pmc_new>, but passes C<init> to the PMC's C<init_pmc> vtable
entry.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PMC *
constant_pmc_new_init(PARROT_INTERP, INTVAL base_type, ARGIN_NULLOK(PMC *init))
{
    ASSERT_ARGS(constant_pmc_new_init)
    PMC * const pmc = get_new_pmc_header(interp, base_type, PObj_constant_FLAG);
    VTABLE_init_pmc(interp, pmc, init);
    return pmc;
}


/*

=item C<PMC * temporary_pmc_new(PARROT_INTERP, INTVAL base_type)>

Creates a new temporary PMC of type C<base_type>, then call C<init>. Cannot
be used to create PMC Objects which have been defined from PIR.

B<You> are responsible for freeing this PMC when it goes out of scope with
C<free_temporary_pmc()>.  B<Do not> store this PMC in any other PMCs, or
allow it to be stored.  B<Do not> store any regular PMC in this PMC, or
allow the storage of any regular PMC in this PMC. Temporary PMCs do not
participate in garbage collection, and mixing them with PMCs that are
garbage-collected will cause bugs.

If you don't know what this means means, or you can't tell if either case
will happen as the result of any call you make on or with this PMC,
B<DO NOT> use this function, lest you cause weird crashes and memory errors.
Use C<pmc_new()> instead.

(Why do these functions even exist?  Used judiciously, they can reduce GC
pressure in hotspots tremendously.  If you haven't audited the code carefully
-- including profiling and benchmarking -- then use C<pmc_new()> instead, and
never B<ever> add C<PARROT_EXPORT> to either function.)

=cut

*/

PARROT_CANNOT_RETURN_NULL
PMC *
temporary_pmc_new(PARROT_INTERP, INTVAL base_type)
{
    ASSERT_ARGS(temporary_pmc_new)
    PMC * const pmc = get_new_pmc_header(interp, base_type, PObj_constant_FLAG);
    VTABLE_init(interp, pmc);
    return pmc;
}


/*

=item C<void temporary_pmc_free(PARROT_INTERP, PMC *pmc)>

Frees a new temporary PMC created by C<temporary_pmc_new()>.  Do not call
this with any other type of PMC.  Do not forget to call this (or you'll leak
PMCs). Read and I<understand> the warnings for C<temporary_pmc_new()> before
you're tempted to use this.

=cut

*/

void
temporary_pmc_free(PARROT_INTERP, ARGMOD(PMC *pmc))
{
    ASSERT_ARGS(temporary_pmc_free)
    Parrot_gc_free_pmc_header(interp, pmc);
}


/*

=item C<INTVAL get_new_vtable_index(PARROT_INTERP)>

Get a new unique identifier number and allocate a new vtable structure for a
new PMC type.

=cut

*/

INTVAL
get_new_vtable_index(PARROT_INTERP)
{
    ASSERT_ARGS(get_new_vtable_index)
    const INTVAL type_id = interp->n_vtable_max++;

    /* Have we overflowed the table? */
    if (type_id >= interp->n_vtable_alloced)
        parrot_realloc_vtables(interp);

    return type_id;
}

/*

=item C<INTVAL pmc_register(PARROT_INTERP, STRING *name)>

Registers the name of a new PMC type with Parrot, returning the INTVAL
representing that type.

=cut

*/

PARROT_EXPORT
INTVAL
pmc_register(PARROT_INTERP, ARGIN(STRING *name))
{
    ASSERT_ARGS(pmc_register)
    /* If they're looking to register an existing class, return that
       class' type number */
    INTVAL type = pmc_type(interp, name);

    if (type > enum_type_undef)
        return type;

    if (type < enum_type_undef)
        Parrot_ex_throw_from_c_args(interp, NULL, 1,
            "undefined type already exists - can't register PMC");

    type = get_new_vtable_index(interp);

    /* set entry in name->type hash */
    VTABLE_set_integer_keyed_str(interp, interp->class_hash, name, type);

    return type;
}


/*

=item C<INTVAL pmc_type(PARROT_INTERP, STRING *name)>

Returns the PMC type for C<name>.

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
INTVAL
pmc_type(PARROT_INTERP, ARGIN_NULLOK(STRING *name))
{
    ASSERT_ARGS(pmc_type)
    if (!name)
        return enum_type_undef;
    else {
        PMC * const classname_hash = interp->class_hash;
        PMC * const item           =
            (PMC *)VTABLE_get_pointer_keyed_str(interp, classname_hash, name);

        if (!PMC_IS_NULL(item)) {
            /* nested namespace with same name */
            if (item->vtable->base_type == enum_class_NameSpace)
                return enum_type_undef;
            else
                return VTABLE_get_integer(interp, item);
        }
        else
            return Parrot_get_datatype_enum(interp, name);
    }
}


/*

=item C<INTVAL pmc_type_p(PARROT_INTERP, PMC *name)>

Returns the PMC type for C<name>.

=cut

*/

PARROT_EXPORT
INTVAL
pmc_type_p(PARROT_INTERP, ARGIN(PMC *name))
{
    ASSERT_ARGS(pmc_type_p)
    PMC * const classname_hash = interp->class_hash;
    PMC * item;

    item = (PMC *)VTABLE_get_pointer_keyed(interp, classname_hash, name);

    if (!PMC_IS_NULL(item))
        return VTABLE_get_integer(interp, item);

    return 0;
}


/*

=item C<static PMC * create_class_pmc(PARROT_INTERP, INTVAL type)>

Create a class object for this interpreter.  Takes an interpreter name and type
as arguments.  Returns a pointer to the class object.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static PMC *
create_class_pmc(PARROT_INTERP, INTVAL type)
{
    ASSERT_ARGS(create_class_pmc)
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
     * be our class object, but don't mess with its vtable.  */
    if ((interp->vtables[type]->flags & VTABLE_PMC_IS_SINGLETON)
    &&  (_class == _class->vtable->pmc_class))
        interp->vtables[type]->pmc_class = _class;
    else {
        Parrot_gc_free_pmc_sync(interp, _class);
        gc_flag_CLEAR(is_special_PMC, _class);
        PObj_is_PMC_shared_CLEAR(_class);
        interp->vtables[type]->pmc_class = _class;
    }

    return _class;
}


/*

=item C<void Parrot_create_mro(PARROT_INTERP, INTVAL type)>

Create the MRO (method resolution order) array for this type.

=cut

*/

PARROT_EXPORT
void
Parrot_create_mro(PARROT_INTERP, INTVAL type)
{
    ASSERT_ARGS(Parrot_create_mro)
    PMC    *_class, *mro;
    VTABLE *vtable   = interp->vtables[type];
    PMC    *mro_list = vtable->mro;
    INTVAL  i, count;

    /* this should never be PMCNULL */
    PARROT_ASSERT(!PMC_IS_NULL(mro_list));

    /* multithreaded: has already mro */
    if (mro_list->vtable->base_type != enum_class_ResizableStringArray)
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
            PMC * const ns     = pmc_new(interp,
                    Parrot_get_ctx_HLL_type(interp, enum_class_NameSpace));
            vtable->_namespace = ns;

            /* anchor at parent, aka current_namespace, that is 'parrot' */
            VTABLE_set_pmc_keyed_str(interp,
                    Parrot_pcc_get_namespace(interp, CURRENT_CONTEXT(interp)), class_name, ns);
        }

        _class = vtable->pmc_class;
        if (!_class)
            _class = create_class_pmc(interp, parent_type);

        VTABLE_push_pmc(interp, mro, _class);
    }
}


/*

=back

=head2 GC registry interface

=over 4

=item C<void gc_register_pmc(PARROT_INTERP, PMC *pmc)>

Registers the PMC with the interpreter's GC registry.

=cut

*/

PARROT_EXPORT
void
gc_register_pmc(PARROT_INTERP, ARGIN(PMC *pmc))
{
    ASSERT_ARGS(gc_register_pmc)
    /* Better not trigger a GC run with a potentially unanchored PMC */
    Parrot_block_GC_mark(interp);

    PARROT_ASSERT(interp->gc_registry);

    VTABLE_set_pmc_keyed(interp, interp->gc_registry, pmc, PMCNULL);
    Parrot_unblock_GC_mark(interp);
}


/*

=item C<void gc_unregister_pmc(PARROT_INTERP, PMC *pmc)>

Unregisters the PMC from the interpreter's GC registry.

=cut

*/

PARROT_EXPORT
void
gc_unregister_pmc(PARROT_INTERP, ARGIN(PMC *pmc))
{
    ASSERT_ARGS(gc_unregister_pmc)
    PARROT_ASSERT(interp->gc_registry);

    VTABLE_delete_keyed(interp, interp->gc_registry, pmc);
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
