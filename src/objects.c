/*
Copyright (C) 2001-2003, The Perl Foundation.
$Id$

=head1 NAME

objects.c - Class and object

=head1 DESCRIPTION

Handles class and object manipulation.

=head2 Functions

=over 4

=cut

*/

#define PARROT_IN_OBJECTS_C
#include "parrot/parrot.h"
#include <assert.h>

#include "objects.str"

static void parrot_class_register(Interp * , PMC *name,
        PMC *new_class, PMC *parent, PMC *mro);


STRING*
readable_name(Interp *interpreter, PMC *name)
{
    if (name->vtable->base_type == enum_class_String)
        return VTABLE_get_string(interpreter, name);
    else {
        STRING *j = CONST_STRING(interpreter, ";");
        PMC *ar = pmc_new(interpreter, enum_class_ResizableStringArray);
        assert(name->vtable->base_type == enum_class_Key);
        while (name) {
            VTABLE_push_string(interpreter, ar, key_string(interpreter, name));
            name = key_next(interpreter, name);
        }
        return string_join(interpreter, j, ar);
    }
}

static void
fail_if_exist(Interp *interpreter, PMC *name)
{
    STRING *class_name;
    INTVAL type;

    PMC * const classname_hash = interpreter->class_hash;
    PMC * type_pmc = VTABLE_get_pointer_keyed(interpreter, 
            classname_hash, name); 
    if (PMC_IS_NULL(type_pmc) || 
            type_pmc->vtable->base_type == enum_class_NameSpace)
        type = 0;
    else
        type = VTABLE_get_integer(interpreter, type_pmc);
    /* TODO get printable name */
    class_name = VTABLE_get_string(interpreter, name);
    if (type > enum_type_undef) {
        real_exception(interpreter, NULL, INVALID_OPERATION,
                "Class %Ss already registered!\n", class_name);
    }
    if (type < enum_type_undef) {
        real_exception(interpreter, NULL, INVALID_OPERATION,
                "native type with name '%s' already exists - "
                "can't register Class", data_types[type].name);
    }
}

/*
 * FIXME make array clone shallow
 */
static PMC *
clone_array(Interp* interpreter, PMC *source_array)
{
    PMC * const new_array = pmc_new(interpreter, source_array->vtable->base_type);
    const INTVAL count = VTABLE_elements(interpreter, source_array);
    INTVAL i;

    /*
     * preserve type, we have OrderedHash and Array
     * XXX this doesn't preserve the keys of the ordered hash
     *     (but the keys aren't used -leo)
     */
    VTABLE_set_integer_native(interpreter, new_array, count);
    for (i = 0; i < count; i++) {
        VTABLE_set_pmc_keyed_int(interpreter, new_array, i,
                VTABLE_get_pmc_keyed_int(interpreter, source_array, i));
    }
    return new_array;
}

/* Take the class and completely rebuild the attribute stuff for
   it. Horribly destructive, and definitely not a good thing to do if
   there are instantiated objects for the class */
static void
rebuild_attrib_stuff(Interp* interpreter, PMC *class)
{
    INTVAL cur_offset;
    SLOTTYPE *class_slots;
    PMC *attr_offset_hash;
    PMC *class_offset_hash;
    PMC *mro;
    STRING *classname;
    INTVAL n_class;
    INTVAL n_mro;
    PMC *attribs;
    INTVAL attr_count;
#ifndef NDEBUG
    PMC * const orig_class = class;
#endif

    /* attrib count isn't set yet, a GC causedd by concat could
     * corrupt data under construction
     */
    Parrot_block_DOD(interpreter);

    class_slots = PMC_data(class);
    attr_offset_hash = pmc_new(interpreter, enum_class_Hash);
    set_attrib_num(class, class_slots, PCD_ATTRIBUTES, attr_offset_hash);

    class_offset_hash = pmc_new(interpreter, enum_class_Hash);
    set_attrib_num(class, class_slots, PCD_ATTRIB_OFFS, class_offset_hash);

    mro = class->vtable->mro;
    n_mro = VTABLE_elements(interpreter, mro);

    /*
     * walk from oldest parent downto n_class == 0 which is this class
     */
    cur_offset = 0;
    for (n_class = n_mro - 1; n_class >= 0; --n_class) {
        class = VTABLE_get_pmc_keyed_int(interpreter, mro, n_class);
        if (!PObj_is_class_TEST(class)) {
            /* this Class isa PMC - no attributes there
            */
            continue;
        }

        class_slots = PMC_data(class);
        classname = VTABLE_get_string(interpreter,
                    get_attrib_num(class_slots, PCD_CLASS_NAME));
        attribs = get_attrib_num(class_slots, PCD_CLASS_ATTRIBUTES);
        attr_count = VTABLE_elements(interpreter, attribs);
        /* Note the current offset as where this class'
               attributes start */
        if (attr_count || !n_class) {
            VTABLE_set_integer_keyed_str(interpreter,
                    class_offset_hash, classname,
                    cur_offset + POD_FIRST_ATTRIB);
        }
        if (attr_count) {
            INTVAL offset;

            STRING * const partial_name =
                string_concat(interpreter, classname,
                    string_from_cstring(interpreter, "\0", 1),
                    0);
            for (offset = 0; offset < attr_count; offset++) {
                STRING * const attr_name = VTABLE_get_string_keyed_int(interpreter, attribs,
                        offset);
                STRING * const full_name = string_concat(interpreter, partial_name,
                        attr_name, 0);
                /*
                 * store this attribute with short and full name
                 */
                VTABLE_set_integer_keyed_str(interpreter, attr_offset_hash,
                        attr_name, cur_offset);
                VTABLE_set_integer_keyed_str(interpreter, attr_offset_hash,
                        full_name, cur_offset++);
            }
        }
    }

#ifndef NDEBUG
    assert(class == orig_class);
#endif

    /* And note the totals */
    ATTRIB_COUNT(class) = cur_offset;
    Parrot_unblock_DOD(interpreter);
}

/*

=item C<static void create_deleg_pmc_vtable(Interp *, PMC *class, STRING *name)>

Create a vtable that dispatches either to the contained PMC in the first
attribute (deleg_pmc) or to an overridden method (delegate), depending
on the existence of the method for this class.

=cut

*/

static void
create_deleg_pmc_vtable(Interp *interpreter, PMC *class, 
        PMC *class_name, int full)
{
    int i;
    const char *meth;
    STRING meth_str;
    union {
        const void * __c_ptr;
        void * __ptr;
    } __ptr_u;                  /* FIXME - THIS SHOULD NOT BE HERE */

    PMC * const vtable_pmc = get_attrib_num((SLOTTYPE*)PMC_data(class), PCD_OBJECT_VTABLE);
    VTABLE * const vtable           = PMC_struct_val(vtable_pmc);
    VTABLE * const deleg_pmc_vtable = interpreter->vtables[enum_class_deleg_pmc];
    VTABLE * const object_vtable    = interpreter->vtables[enum_class_ParrotObject];
    VTABLE * const delegate_vtable  = interpreter->vtables[enum_class_delegate];

    memset(&meth_str, 0, sizeof(meth_str));
    meth_str.encoding = Parrot_fixed_8_encoding_ptr;
    meth_str.charset = Parrot_default_charset_ptr;
    for (i = 0; (meth = Parrot_vtable_slot_names[i]); ++i) {
        if (!*meth)
            continue;
        meth_str.strstart = const_cast(meth);
        meth_str.strlen = meth_str.bufused = strlen(meth);
        meth_str.hashval = 0;
        if (Parrot_find_global_k(interpreter, class_name, &meth_str)) {
            /*
             * the method exists; keep the ParrotObject aka delegate vtable slot
             */
            ((void **)vtable)[i] = ((void**)object_vtable)[i];
#if 0
            PIO_eprintf(interpreter, "deleg_pmc class '%Ss' found '%s'\n",
                    class_name, meth);
#endif
        }
        else if (full) {
            /*
             * the method doesn't exist; put in the deleg_pmc vtable,
             * but only if ParrotObject hasn't overridden the method
             */
            if (((void **)delegate_vtable)[i] == ((void**)object_vtable)[i])
                ((void **)vtable)[i] = ((void**)deleg_pmc_vtable)[i];
            else
                ((void **)vtable)[i] = ((void**)object_vtable)[i];
        }
    }
}

/*

=item C<const char* Parrot_MMD_method_name(Interp*, INTVAL)>

Return the method name for the given MMD enum.

=item C<INTVAL Parrot_MMD_method_idx(Interp*, STRING *)>

Return the MMD function number for method name or -1 on failure.

TODO allow dynamic expansion at runtime.

=cut

*/
const char*
Parrot_MMD_method_name(Interp* interpreter, INTVAL idx)
{
    assert(idx >= 0);

    if (idx >= MMD_USER_FIRST)
        return NULL;
    return Parrot_mmd_func_names[idx];
}

INTVAL
Parrot_MMD_method_idx(Interp* interpreter, char *name)
{
    INTVAL i;

    for (i = 0; i < MMD_USER_FIRST; ++i) {
        if (!strcmp(Parrot_mmd_func_names[i], name))
            return i;
    }
    return -1;
}


/*

=item C<PMC *
Parrot_single_subclass(Interp* ointerpreter, PMC *base_class,
                       PMC *child_class_name)>

Subclass a class. Single parent class, nice and straightforward. If
C<child_class> is C<NULL>, this is an anonymous subclass we're creating,
which happens commonly enough to warrant an actual single-subclass
function.

=cut

*/

PMC *
Parrot_single_subclass(Interp* interpreter, PMC *base_class,
                       PMC *name)
{
    PMC *child_class;
    SLOTTYPE *child_class_array;
    PMC *parents, *temp_pmc, *mro;
    int parent_is_class;

    /* Set the classname, if we have one */
    if (!PMC_IS_NULL(name)) {
        fail_if_exist(interpreter, name);
    }
    else {
        /* XXX not really threadsafe but good enough for now */
        static int anon_count;

        STRING *child_class_name = Parrot_sprintf_c(interpreter, 
                "%c%canon_%d", 0, 0, ++anon_count);
        name = pmc_new(interpreter, enum_class_String); 
        VTABLE_set_string_native(interpreter, name, child_class_name );
    }
    /*
     * ParrotClass is the baseclass anyway, so build just a new class
     */
    if (base_class == interpreter->vtables[enum_class_ParrotClass]->class) {
        return pmc_new_init(interpreter, enum_class_ParrotClass, name);
    }
    parent_is_class = PObj_is_class_TEST(base_class);

    child_class = pmc_new(interpreter, enum_class_ParrotClass);
    /* Hang an array off the data pointer */
    set_attrib_array_size(child_class, PCD_MAX);
    child_class_array = PMC_data(child_class);
    set_attrib_flags(child_class);
    /* We will have five entries in this array */

    /* We have the same number of attributes as our parent */
    ATTRIB_COUNT(child_class) = parent_is_class ? ATTRIB_COUNT(base_class) : 0;

    /* Our parent class array has a single member in it */
    parents = pmc_new(interpreter, enum_class_ResizablePMCArray);
    VTABLE_set_integer_native(interpreter, parents, 1);
    VTABLE_set_pmc_keyed_int(interpreter, parents, 0, base_class);
    set_attrib_num(child_class, child_class_array, PCD_PARENTS, parents);


    set_attrib_num(child_class, child_class_array, PCD_CLASS_NAME, name);

    /* Our mro list is a clone of our parent's mro
     * list, with our self unshifted onto the beginning
     */
    mro = VTABLE_clone(interpreter, base_class->vtable->mro);
    VTABLE_unshift_pmc(interpreter, mro, child_class);

    /* But we have no attributes of our own. Yet */
    temp_pmc = pmc_new(interpreter, enum_class_ResizablePMCArray);
    set_attrib_num(child_class, child_class_array, PCD_CLASS_ATTRIBUTES,
            temp_pmc);

    parrot_class_register(interpreter, name, child_class,
            base_class, mro);

    rebuild_attrib_stuff(interpreter, child_class);

    if (!parent_is_class) {
        /* we append one attribute to hold the PMC */
        Parrot_add_attribute(interpreter, child_class,
                CONST_STRING(interpreter, "__value"));
        /*
         * then create a vtable derived from ParrotObject and
         * deleg_pmc - the ParrotObject vtable is already built
         */
        create_deleg_pmc_vtable(interpreter, child_class, name, 1);
    }
    else {
        /*
         * if any parent isa PMC, then still individual vtables might
         * be overridden in this subclass
         */
        const PMC* parent;
        int i, n, any_pmc_parent;

        n = VTABLE_elements(interpreter, mro);
        any_pmc_parent = 0;

        /* 0 = this, 1 = parent (handled above), 2 = grandpa */
        for (i = 2; i < n; ++i) {
            parent = VTABLE_get_pmc_keyed_int(interpreter, mro, i);
            if (!PObj_is_class_TEST(parent)) {
                any_pmc_parent = 1;
                break;
            }
        }
        if (any_pmc_parent)
            create_deleg_pmc_vtable(interpreter, child_class, name, 0);
    }
    return child_class;
}

/*

=item C<void
Parrot_new_class(Interp* interpreter, PMC *class, PMC *class_name)>

Creates a new class, named C<class_name>.

=cut

*/

void
Parrot_new_class(Interp* interpreter, PMC *class, PMC *name)
{
    SLOTTYPE *class_array;
    PMC *mro;

    /* check against duplicate newclasses */
    fail_if_exist(interpreter, name);

    /* Hang an array off the data pointer, empty of course */
    set_attrib_array_size(class, PCD_MAX);
    class_array = PMC_data(class);
    /* set_attrib_flags(class); init does it */

    /* Our parent class array has nothing in it */
    set_attrib_num(class, class_array, PCD_PARENTS,
                   pmc_new(interpreter, enum_class_ResizablePMCArray));
    /* TODO create all class structures in constant PMC pool
     */

    /*
     * create MRO (method resolution order) array
     * first entry is this class itself
     */
    mro = pmc_new(interpreter, enum_class_ResizablePMCArray);
    VTABLE_push_pmc(interpreter, mro, class);

    /* no attributes yet
     */
    set_attrib_num(class, class_array, PCD_CLASS_ATTRIBUTES,
            pmc_new(interpreter, enum_class_ResizablePMCArray));

    /* Set the classname */
    set_attrib_num(class, class_array, PCD_CLASS_NAME, name);

    parrot_class_register(interpreter, name, class, NULL, mro);

    rebuild_attrib_stuff(interpreter, class);
}

/*

=item C<PMC *
Parrot_class_lookup(Interp* interpreter, STRING *class_name)>

=item C<PMC *
Parrot_class_lookup_p(Interp* interpreter, PMC *class_name)>

Looks for the class named C<class_name> and returns it if it exists.
Otherwise it returns C<PMCNULL>.

=cut

*/

PMC *
Parrot_class_lookup(Interp* interpreter, STRING *class_name)
{
    const INTVAL type = pmc_type(interpreter, class_name);
    if (type > 0) {
        PMC * const pmc = interpreter->vtables[type]->class;
        assert(pmc);
        return pmc;
    }
    return PMCNULL;
}

PMC *
Parrot_class_lookup_p(Interp* interpreter, PMC *class_name)
{
    const INTVAL type = pmc_type_p(interpreter, class_name);
    if (type > 0) {
        PMC * const pmc = interpreter->vtables[type]->class;
        assert(pmc);
        return pmc;
    }
    return PMCNULL;
}

static INTVAL
register_type(Interp *interpreter, PMC *name)
{
    INTVAL type;
    PMC * classname_hash, *item;

    classname_hash = interpreter->class_hash;

    type = interpreter->n_vtable_max++;
    /* Have we overflowed the table? */
    if (type >= interpreter->n_vtable_alloced) {
        parrot_realloc_vtables(interpreter);
    }
    /* set entry in name->type hash */
    item = pmc_new(interpreter, enum_class_Integer);
    PMC_int_val(item) = type;
    VTABLE_set_pmc_keyed(interpreter, classname_hash, name, item);
    return type;
}

/*

=item C<static void
parrot_class_register(Interp* interpreter, PMC *class_name,
        PMC *new_class, PMC *mro)>

This is the way to register a new Parrot class as an instantiable
type. Doing this involves putting it in the class hash, setting its
vtable so that the C<init> method initializes objects of the class rather than
the class itself, and adding it to the interpreter's base type table so
you can create a new C<foo> in PASM like this: C<new Px, foo>.

=cut

*/

static void
parrot_class_register(Interp* interpreter, PMC *name,
        PMC *new_class, PMC *parent, PMC *mro)
{
    VTABLE *new_vtable, *parent_vtable;
    PMC *vtable_pmc;
    PMC *ns, *top;
    INTVAL new_type;

    new_type = register_type(interpreter, name);
    /* Build a new vtable for this class
     * The child class PMC gets the vtable of its parent class or
     * a ParrotClass vtable
     */
    parent_vtable = new_class->vtable;
    if (parent && PObj_is_class_TEST(parent))
        parent_vtable = parent->vtable;
    else
        parent_vtable = new_class->vtable;
    new_vtable = Parrot_clone_vtable(interpreter, parent_vtable);

    /* Set the vtable's type to the newly allocated type */
    new_vtable->base_type = new_type;
    /* And cache our class PMC in the vtable so we can find it later */
    new_vtable->class =  new_class;
    new_vtable->mro = mro;

    /* Reset the init method to our instantiation method */
    new_vtable->init = Parrot_instantiate_object;
    new_vtable->init_pmc = Parrot_instantiate_object_init;
    new_class->vtable = new_vtable;

    /* Put our new vtable in the global table */
    interpreter->vtables[new_type] = new_vtable;

    /* check if we already have a NameSpace */
    top = CONTEXT(interpreter->ctx)->current_namespace;
    ns = VTABLE_get_pmc_keyed(interpreter, top, name);
    /* XXX nested, use current as base ? */
    if (PMC_IS_NULL(ns)) {
        /* XXX try HLL namespace too XXX */
        top = Parrot_get_ctx_HLL_namespace(interpreter);
        ns = VTABLE_get_pmc_keyed(interpreter, top, name);
    }
    if (PMC_IS_NULL(ns)) {
        ns = pmc_new(interpreter, enum_class_NameSpace);
        VTABLE_set_pmc_keyed(interpreter, top, name, ns);
    }
    /* attach namspace to vtable */
    new_vtable->_namespace = ns;

    /*
     * prepare object vtable - again that of the parent or
     * a ParrotObject vtable
     */
    if (parent && PObj_is_class_TEST(parent)) {
        vtable_pmc =
            get_attrib_num((SLOTTYPE*)PMC_data(parent), PCD_OBJECT_VTABLE);
        parent_vtable = PMC_struct_val(vtable_pmc);
    }
    else
        parent_vtable = interpreter->vtables[enum_class_ParrotObject];

    new_vtable = Parrot_clone_vtable(interpreter, parent_vtable);
    new_vtable->base_type = new_type;
    new_vtable->mro = mro;
    new_vtable->class =  new_class;
    set_attrib_num(new_class, (SLOTTYPE*)PMC_data(new_class), PCD_OBJECT_VTABLE,
            vtable_pmc = constant_pmc_new(interpreter, enum_class_VtableCache));
    PMC_struct_val(vtable_pmc) = new_vtable;
    /* attach namspace to object vtable too */
    new_vtable->_namespace = ns;
}

static PMC*
get_init_meth(Interp* interpreter, PMC *class,
          STRING *prop_str , STRING **meth_str)
{
    STRING *meth;
    HashBucket *b;
    PMC *props, *ns, *method;

    *meth_str = NULL;
#if 0
    PMC *prop;
    prop = VTABLE_getprop(interpreter, class, prop_str);
    if (!VTABLE_defined(interpreter, prop))
        return NULL;
    meth = VTABLE_get_string(interpreter, prop);
#else
    if ( !(props = PMC_metadata(class)))
        return NULL;
    b = parrot_hash_get_bucket(interpreter,
                (Hash*) PMC_struct_val(props), prop_str);
    if (!b)
        return NULL;
    meth = PMC_str_val((PMC*) b->value);
#endif
    *meth_str = meth;

    ns = VTABLE_namespace(interpreter, class);
    method = VTABLE_get_pmc_keyed_str(interpreter, ns, meth);
    return PMC_IS_NULL(method) ? NULL : method;
}


static void
do_initcall(Interp* interpreter, PMC* class, PMC *object, PMC *init)
{
    PMC * const classsearch_array = class->vtable->mro;
    PMC *parent_class;
    INTVAL i, nparents;
    /*
     * 1) if class has a CONSTRUCT property run it on the object
     *    no redispatch
     *
     * XXX isn't CONSTRUCT for creating new objects?
     */
    STRING *meth_str;
    PMC *meth = get_init_meth(interpreter, class,
            CONST_STRING(interpreter, "CONSTRUCT"), &meth_str);
    int default_meth;

    if (meth) {
        if (init)
            Parrot_run_meth_fromc_args(interpreter, meth,
                    object, meth_str, "vP", init);
        else
            Parrot_run_meth_fromc_args(interpreter, meth,
                    object, meth_str, "v");
    }
    /*
     * 2. if class has a BUILD property call it for all classes
     *    in reverse search order - this class last.
     *
     *    Note: mro contains this class as first element
     */
    nparents = VTABLE_elements(interpreter, classsearch_array);
    for (i = nparents - 1; i >= 0; --i) {
        parent_class = VTABLE_get_pmc_keyed_int(interpreter,
                classsearch_array, i);
        /* if it's a PMC, we put one PMC of that type into
         * the attribute slot #0 and call init() on that PMC
         */
        if (!PObj_is_class_TEST(parent_class)) {
            PMC *attr, *next_parent;
            SLOTTYPE *obj_data;

            /*
             * but only if init isn't inherited
             * or rather just on the last non-class parent
             */
            assert(i >= 1);
            next_parent = VTABLE_get_pmc_keyed_int(interpreter,
                    classsearch_array, i - 1);
            if (!PObj_is_class_TEST(next_parent)) {
                continue;
            }
            attr = pmc_new_noinit(interpreter,
                    parent_class->vtable->base_type);
            obj_data = PMC_data(object);
            set_attrib_num(object, obj_data, POD_FIRST_ATTRIB, attr);
            VTABLE_init(interpreter, attr);
            continue;
        }
        meth = get_init_meth(interpreter, parent_class,
                CONST_STRING(interpreter, "BUILD"), &meth_str);
        /* no method found and no BUILD property set? */
        if (!meth && meth_str == NULL) {
            PMC *ns;
            /* use __init as fallback constructor method, if it exists */
            meth_str = CONST_STRING(interpreter, "__init");
            ns = VTABLE_namespace(interpreter, parent_class);
            /* can't use find_method, it walks mro */
            meth = VTABLE_get_pmc_keyed_str(interpreter, ns, meth_str);
            if (meth == PMCNULL)
                meth = NULL;
            default_meth = 1;
        }
        else
            default_meth = 0;
        if (meth) {
            if (init)
                Parrot_run_meth_fromc_args(interpreter, meth,
                        object, meth_str, "vP", init);
            else
                Parrot_run_meth_fromc_args(interpreter, meth,
                        object, meth_str, "v");
        }
        else if (meth_str != NULL &&
                string_length(interpreter, meth_str) != 0 && !default_meth) {
            real_exception(interpreter, NULL, METH_NOT_FOUND,
                    "Class BUILD method ('%Ss') not found", meth_str);
        }
    }
}

/*

=item C<void
Parrot_instantiate_object(Interp* interpreter, PMC *object, PMC *init)>

Creates a Parrot object. Takes a passed-in class PMC that has sufficient
information to describe the layout of the object and, well, makes the
darned object.

=cut

*/

static void instantiate_object(Interp*, PMC *object, PMC *init);

void
Parrot_instantiate_object_init(Interp* interpreter,
        PMC *object, PMC *init)
{
    instantiate_object(interpreter, object, init);
}

void
Parrot_instantiate_object(Interp* interpreter, PMC *object)
{
    instantiate_object(interpreter, object, NULL);
}

static void
instantiate_object(Interp* interpreter, PMC *object, PMC *init)
{
    SLOTTYPE *new_object_array;
    INTVAL attrib_count, i;
    SLOTTYPE *class_array;
    PMC *class_name;

    PMC * const class = object->vtable->class;
    /*
     * put in the real vtable
     */
    PMC * const vtable_pmc = get_attrib_num((SLOTTYPE *)PMC_data(class), PCD_OBJECT_VTABLE);
    object->vtable = PMC_struct_val(vtable_pmc);

    /* Grab the attribute count from the parent */
    attrib_count = ATTRIB_COUNT(class);

    class_array = PMC_data(class);
    class_name = get_attrib_num(class_array, PCD_CLASS_NAME);

    /* Build the array that hangs off the new object */
    /* First presize it */
    set_attrib_array_size(object,
                          attrib_count + POD_FIRST_ATTRIB);
    new_object_array = PMC_data(object);

    /* fill with PMCNULL, so that access doesn't segfault */
    for (i = POD_FIRST_ATTRIB; i < attrib_count + POD_FIRST_ATTRIB; ++i)
        set_attrib_num(object, new_object_array, i, PMCNULL);

    /* turn marking on */
    set_attrib_flags(object);
    /* 0 - class PMC, 1 - class name */
    SET_CLASS(new_object_array, object, class);
    set_attrib_num(object, new_object_array, POD_CLASS_NAME, class_name);

    /* We are an object now */
    PObj_is_object_SET(object);

    /* We really ought to call the class init routines here...
     * this assumes that an object isa delegate
     */
    do_initcall(interpreter, class, object, init);
}

/*

=item C<PMC *
Parrot_add_parent(Interp* interpreter, PMC *class,
           PMC *parent)>

Add the parent class to the current class' parent list. This also
involved adding all the parent's parents, as well as all attributes of
the parent classes that we're adding in.

The MRO (method resolution order) is the C3 algorithm used by Perl6
and Python (>= 2.3). See also:
L<http://pugs.blogs.com/pugs/2005/07/day_165_r5671_j.html>

=cut

*/


/* create a list if non-empty lists */
static PMC*
not_empty(Interp* interpreter, PMC *seqs)
{
    INTVAL i;
    PMC * const nseqs = pmc_new(interpreter, enum_class_ResizablePMCArray);

    for (i = 0; i < VTABLE_elements(interpreter, seqs); ++i) {
        PMC * const list = VTABLE_get_pmc_keyed_int(interpreter, seqs, i);
        if (VTABLE_elements(interpreter, list))
            VTABLE_push_pmc(interpreter, nseqs, list);
    }
    return nseqs;
}

/* merge the list if lists */
static PMC*
class_mro_merge(Interp* interpreter, PMC *seqs)
{
    PMC *res, *seq, *cand, *nseqs, *s;
    INTVAL i, j, k;
    cand = NULL; /* silence compiler uninit warning */

    res = pmc_new(interpreter, enum_class_ResizablePMCArray);
    while (1) {
        nseqs = not_empty(interpreter, seqs);
        if (!VTABLE_elements(interpreter, nseqs))
            break;
        for (i = 0; i < VTABLE_elements(interpreter, nseqs); ++i) {
            seq = VTABLE_get_pmc_keyed_int(interpreter, nseqs, i);
            cand = VTABLE_get_pmc_keyed_int(interpreter, seq, 0);
            /* check if candidate is valid */
            for (j = 0; j < VTABLE_elements(interpreter, nseqs); ++j) {
                s = VTABLE_get_pmc_keyed_int(interpreter, nseqs, j);
                for (k = 1; k < VTABLE_elements(interpreter, s); ++k)
                    if (VTABLE_get_pmc_keyed_int(interpreter, s, k) == cand) {
                        cand = NULL;
                        break;
                    }
            }
            if (cand)
                break;
        }
        if (!cand)
            real_exception(interpreter, NULL, E_TypeError,
                    "inconsisten class hierarchy");
        /* push candidate onto mro result */
        VTABLE_push_pmc(interpreter, res, cand);
        /* remove candidate from head of lists */
        for (i = 0; i < VTABLE_elements(interpreter, nseqs); ++i) {
            seq = VTABLE_get_pmc_keyed_int(interpreter, nseqs, i);
            if (VTABLE_get_pmc_keyed_int(interpreter, seq, 0) == cand) {
                VTABLE_shift_pmc(interpreter, seq);
            }
        }
    }
    return res;
}

/* create C3 MRO */
static PMC*
create_class_mro(Interp* interpreter, PMC *class)
{
    PMC *lparents, *bases;
    INTVAL i;

    /* list of lists
     * [ [class] [mro of bases] [bases] ]
     */
    PMC * const lall = pmc_new(interpreter, enum_class_ResizablePMCArray);
    PMC * const lc = pmc_new(interpreter, enum_class_ResizablePMCArray);

    VTABLE_push_pmc(interpreter, lc, class);
    VTABLE_push_pmc(interpreter, lall, lc);

    bases = get_attrib_num(PMC_data(class), PCD_PARENTS);
    for (i = 0; i < VTABLE_elements(interpreter, bases); ++i) {
        PMC * const base = VTABLE_get_pmc_keyed_int(interpreter, bases, i);
        PMC * const lmap = create_class_mro(interpreter, base);
        VTABLE_push_pmc(interpreter, lall, lmap);
    }
    lparents = VTABLE_clone(interpreter, bases);
    VTABLE_push_pmc(interpreter, lall, lparents);
    return class_mro_merge(interpreter, lall);
}

PMC *
Parrot_add_parent(Interp* interpreter, PMC *class, PMC *parent)
{
    PMC *current_parent_array;

    if (!PObj_is_class_TEST(class))
        internal_exception(1, "Class isn't a ParrotClass");
    if (!PObj_is_class_TEST(parent))
        internal_exception(1, "Parent isn't a ParrotClass");

    current_parent_array = get_attrib_num(PMC_data(class), PCD_PARENTS);
    VTABLE_push_pmc(interpreter, current_parent_array, parent);

    class->vtable->mro = create_class_mro(interpreter, class);

    rebuild_attrib_stuff(interpreter, class);
    return NULL;
}

/*

=item C<PMC *
Parrot_remove_parent(Interp* interpreter, PMC *removed_class,
                     PMC *existing_class)>

This currently does nothing but return C<NULL>.

=cut

*/

PMC *
Parrot_remove_parent(Interp* interpreter, PMC *removed_class,
                     PMC *existing_class) {
    return NULL;
}

/*

=item C<PMC *
Parrot_multi_subclass(Interp* interpreter, PMC *base_class_array,
                      STRING *child_class_name)>

This currently does nothing but return C<NULL>.

=cut

*/

PMC *
Parrot_multi_subclass(Interp* interpreter, PMC *base_class_array,
                      STRING *child_class_name) {
    return NULL;
}

/*

=item C<INTVAL
Parrot_object_isa(Interp* interpreter, PMC *pmc, PMC *cl)>

Return whether the object C<pmc> is an instance of class C<cl>.

=cut

*/

INTVAL
Parrot_object_isa(Interp* interpreter, PMC *pmc, PMC *cl)
{
    PMC *mro;
    INTVAL i, classcount;

    /* if this is not a class */
    if (!PObj_is_class_TEST(pmc)) {
        pmc = VTABLE_get_class(interpreter, pmc);
    }
    mro = pmc->vtable->mro;
    classcount = VTABLE_elements(interpreter, mro);
    for (i = 0; i < classcount; ++i) {
        if (VTABLE_get_pmc_keyed_int(interpreter, mro, i) == cl)
            return 1;
    }
    return 0;
}

/*

=item C<PMC *
Parrot_new_method_cache(Interp* interpreter)>

This should create and return a new method cache PMC.

Currently it does nothing but return C<NULL>.

=cut

*/

PMC *
Parrot_new_method_cache(Interp* interpreter) {
    return NULL;
}

/*

=item C<PMC *
Parrot_find_method_with_cache(Interp* interpreter, PMC *class,
                              STRING *method_name)>

Find a method PMC for a named method, given the class PMC, current
interpreter, and name of the method.

This routine should use the current scope's method cache, if there is
one. If not, it creates a new method cache. Or, rather, it will when
we've got that bit working. For now it unconditionally goes and looks up
the name in the global stash.

=item C<PMC *
Parrot_find_method_direct(Interp* interpreter, PMC *class,
                              STRING *method_name)>

Find a method PMC for a named method, given the class PMC, current
interpreter, and name of the method. Don't use a possible method cache.

=item void Parrot_invalidate_method_cache(Interp *, STRING *class)

Clear method cache for the given class. If class is NULL caches for
all classes are invalidated.

=cut

*/

static PMC* find_method_direct(Interp*, PMC *, STRING*);

void
mark_object_cache(Interp* interpreter)
{
}

void
init_object_cache(Interp* interpreter)
{
    Caches * const mc = interpreter->caches = mem_sys_allocate_zeroed(sizeof(*mc));
    SET_NULL(mc->idx);
}

#define TBL_SIZE_MASK 0x1ff   /* x bits 2..10 */
#define TBL_SIZE (1 + TBL_SIZE_MASK)

static void
invalidate_type_caches(Interp *interpreter, UINTVAL type)
{
    Caches * const mc = interpreter->caches;
    INTVAL i;

    if (!mc)
        return;
    /* is it a valid entry */
    if (type >= mc->mc_size || !mc->idx[type])
        return;
    for (i = 0; i < TBL_SIZE; ++i) {
        Meth_cache_entry *e;
        for (e = mc->idx[type][i]; e; ) {
            Meth_cache_entry * const next = e->next;
            mem_sys_free(e);
            e = next;
        }
    }
    mem_sys_free(mc->idx[type]);
    mc->idx[type] = NULL;
}

static void
invalidate_all_caches(Interp *interpreter)
{
    UINTVAL i;
    for (i = 1; i < (UINTVAL)interpreter->n_vtable_max; ++i)
        invalidate_type_caches(interpreter, i);
}

void
Parrot_invalidate_method_cache(Interp *interpreter, STRING *class, STRING *meth)
{
    INTVAL type;

    /* during interpreter creation and NCI registration the class_hash
     * isn't yet up */
    if (!interpreter->class_hash)
        return;
    if (interpreter->resume_flag & RESUME_INITIAL)
        return;
    if (!class) {
        invalidate_all_caches(interpreter);
        return;
    }
    type = pmc_type(interpreter, class);
    if (type < 0)
        return;
    if (type == 0) {
        invalidate_all_caches(interpreter);
        return;
    }
    invalidate_type_caches(interpreter, (UINTVAL)type);
}

/*
 * quick'n'dirty method cache
 * TODO: use a hash if method_name is not constant
 *       i.e. from obj.$Sreg(args)
 *       If this hash is implemented mark it during DOD
 */
PMC *
Parrot_find_method_direct(Interp* interpreter, PMC *class,
                              STRING *method_name)
{
    return find_method_direct(interpreter, class, method_name);
}

PMC *
Parrot_find_method_with_cache(Interp* interpreter, PMC *class,
                              STRING *method_name)
{

    UINTVAL type;
    Caches *mc;
    int is_const;
    UINTVAL bits, i;
    Meth_cache_entry *e, *old;

    assert(method_name != 0);

#if DISABLE_METH_CACHE
    return find_method_direct(interpreter, class, method_name);
#endif

    is_const = PObj_constant_TEST(method_name);
    if (!is_const) {
        return find_method_direct(interpreter, class, method_name);
    }
    mc = interpreter->caches;
    type = class->vtable->base_type;
    bits = (((UINTVAL) method_name->strstart ) >> 2) & TBL_SIZE_MASK;
    if (type >= mc->mc_size) {
        if (mc->idx) {
            mc->idx = mem_sys_realloc(mc->idx, sizeof(UINTVAL*) * (type + 1));
        } else {
            mc->idx = mem_sys_allocate(sizeof(UINTVAL*) * (type + 1));
        }
        for (i = mc->mc_size; i <= type; ++i)
            mc->idx[i] = NULL;
        mc->mc_size = type + 1;
    }
    if (!mc->idx[type]) {
        mc->idx[type] = mem_sys_allocate(sizeof(Meth_cache_entry*) * TBL_SIZE);
        for (i = 0; i < TBL_SIZE; ++i)
            mc->idx[type][i] = NULL;
    }
    e = mc->idx[type][bits];
    old = NULL;
    while (e && e->strstart != method_name->strstart) {
        old = e;
        e = e->next;
    }
    if (!e) {
        PMC * const found = find_method_direct(interpreter, class, method_name);
        /* when here no or no correct entry was at [bits] */
        e = mem_sys_allocate(sizeof(Meth_cache_entry));
        if (old)
            old->next = e;
        else
            mc->idx[type][bits] = e;
        e->pmc = found;
        e->next = NULL;
        e->strstart = method_name->strstart;
    }

    return e->pmc;
}

#ifdef NDEBUG
#  define TRACE_FM(i, c, m, sub)
#else
static void
debug_trace_find_meth(Interp* interpreter, PMC *class, STRING *name, PMC *sub)
{
    STRING *class_name;
    const char *result;
    Interp *tracer;

    if (!Interp_trace_TEST(interpreter, PARROT_TRACE_FIND_METH_FLAG))
        return;
    if (PObj_is_class_TEST(class)) {
        SLOTTYPE * const class_array = PMC_data(class);
        PMC * const class_name_pmc = get_attrib_num(class_array, PCD_CLASS_NAME);
        class_name = PMC_str_val(class_name_pmc);
    }
    else
        class_name = class->vtable->whoami;
    if (sub) {
        if (sub->vtable->base_type == enum_class_NCI)
            result = "NCI";
        else
            result = "Sub";
    }
    else
        result = "no";
    tracer = interpreter->debugger ? 
        interpreter->debugger : interpreter;
    PIO_eprintf(tracer,
            "# find_method class '%Ss' method '%Ss': %s\n",
            class_name, name, result);
}

#  define TRACE_FM(i, c, m, sub) \
    debug_trace_find_meth(i, c, m, sub)
#endif

static PMC *
find_method_direct_1(Interp* interpreter, PMC *class,
                              STRING *method_name)
{
    PMC* method, *ns;
    INTVAL i;

    PMC * const mro = class->vtable->mro;
    const INTVAL n = VTABLE_elements(interpreter, mro);
    for (i = 0; i < n; ++i) {
        class = VTABLE_get_pmc_keyed_int(interpreter, mro, i);
        ns = VTABLE_namespace(interpreter, class);
        method = VTABLE_get_pmc_keyed_str(interpreter, ns, method_name);
        TRACE_FM(interpreter, class, method_name, method);
        if (!PMC_IS_NULL(method)) {
            return method;
        }
    }
    TRACE_FM(interpreter, class, method_name, NULL);
    return NULL;
}

static PMC *
find_method_direct(Interp* interpreter, PMC *class,
                              STRING *method_name)
{
    PMC * const found = find_method_direct_1(interpreter, class, method_name);
    STRING * s1, *s2;
    if (found)
        return found;
    s1 = CONST_STRING(interpreter, "__get_string");
    s2 = CONST_STRING(interpreter, "__get_repr");
    if (string_equal(interpreter, method_name, s1) == 0)
        return find_method_direct_1(interpreter, class, s2);
    return NULL;
}

/*
=item C<void
Parrot_note_method_offset(Interp* interpreter, UINTVAL offset, PMC *method)>

Notes where in the hierarchy we just found a method. Used so that we
can do a next and continue the search through the hierarchy for the
next instance of this method.

*/
void
Parrot_note_method_offset(Interp* interpreter, UINTVAL offset, PMC *method)
{
}

/*

=item C<INTVAL
Parrot_add_attribute(Interp* interpreter, PMC* class, STRING* attr)>

Adds the attribute C<attr> to the class.

=cut

*/

/* Life is ever so much easier if a class keeps its attributes at the
   end of the attribute array, since we don't have to insert and
   reorder attributes. Inserting's no big deal, especially since we're
   going to break horribly if you insert into a class that's been
   subclassed, but it'll do for now */

INTVAL
Parrot_add_attribute(Interp* interpreter, PMC* class, STRING* attr)
{
    STRING *full_attr_name;
    char *c_error;

    SLOTTYPE * const class_array = (SLOTTYPE *)PMC_data(class);
    STRING * const class_name = VTABLE_get_string(interpreter,
            get_attrib_num(class_array, PCD_CLASS_NAME));
    PMC * const attr_array = get_attrib_num(class_array, PCD_CLASS_ATTRIBUTES);
    PMC * const attr_hash = get_attrib_num(class_array, PCD_ATTRIBUTES);
    INTVAL idx = VTABLE_elements(interpreter, attr_array);

    VTABLE_set_integer_native(interpreter, attr_array, idx + 1);
    VTABLE_set_string_keyed_int(interpreter, attr_array, idx, attr);
    full_attr_name = string_concat(interpreter, class_name,
            string_from_cstring(interpreter, "\0", 1), 0);
    full_attr_name = string_concat(interpreter, full_attr_name, attr, 0);
    /* TODO escape NUL char */
    if (VTABLE_exists_keyed_str(interpreter, attr_hash, full_attr_name))
    {
        c_error = string_to_cstring(interpreter, full_attr_name);
        internal_exception(1, "Attribute '%s' already exists", c_error);
        string_cstring_free(c_error);
    }
#if 0
    if (VTABLE_exists_keyed_str(interpreter, attr_hash, attr)) {
        /* make old short name invisible */
        static int anon_count;
        STRING *mangled;
        INTVAL old_idx = VTABLE_get_integer_keyed_str(interpreter,
                attr_hash, attr);
        VTABLE_delete_keyed_str(interpreter, attr_hash, attr);

        mangled = Parrot_sprintf_c(interpreter, "%Ss%c%canon_%d",
                attr, 0, 0, ++anon_count);
        VTABLE_set_integer_keyed_str(interpreter, attr_hash,
                mangled, old_idx);
    }
#endif
    /*
     * TODO check if someone is trying to add attributes to a parent class
     * while there are already child class attrs
     */
    idx = VTABLE_elements(interpreter, attr_hash);
    idx /= 2;
    VTABLE_set_integer_keyed_str(interpreter, attr_hash,
            attr, idx);
    VTABLE_set_integer_keyed_str(interpreter, attr_hash,
            full_attr_name, idx);
    ATTRIB_COUNT(class)++;
    return idx;
}

/*

=item C<PMC *
Parrot_get_attrib_by_num(Interp* interpreter, PMC *object, INTVAL attrib)>

Returns attribute number C<attrib> from C<object>. Presumably the code
is asking for the correct attribute number.

=item C<PMC *
Parrot_get_attrib_by_str(Interp* interpreter, PMC *object, STRING *attr)>

Returns attribute with full qualified name C<attr> from C<object>.

=cut

*/

PMC *
Parrot_get_attrib_by_num(Interp* interpreter, PMC *object, INTVAL attrib)
{
    /*
     * this is called from ParrotObject's vtable now, so
     * their is no need for checking object being a valid
     * object PMC
     */
    SLOTTYPE * const attrib_array = PMC_data(object);
    const INTVAL attrib_count = PMC_int_val(object);

    if (attrib >= attrib_count || attrib < POD_FIRST_ATTRIB) {
        real_exception(interpreter, NULL, ATTRIB_NOT_FOUND,
                "No such attribute #%d", (int)attrib);
    }
    return get_attrib_num(attrib_array, attrib);
}

static INTVAL
attr_str_2_num(Interp* interpreter, PMC *object, STRING *attr)
{
    PMC *class;
    PMC *attr_hash;
    SLOTTYPE *class_array;
    HashBucket *b;
    STRING *delimit;
    STRING *attr_name;
    STRING *obj_name;
    int idx, length;

    if (!PObj_is_object_TEST(object))
        internal_exception(INTERNAL_NOT_IMPLEMENTED,
                "Can't set non-core object attribs yet");

    class = GET_CLASS((SLOTTYPE *)PMC_data(object), object);
    class_array = (SLOTTYPE *)PMC_data(class);
    attr_hash = get_attrib_num(class_array, PCD_ATTRIBUTES);
    b = parrot_hash_get_bucket(interpreter,
                (Hash*) PMC_struct_val(attr_hash), attr);
    if (b)
        return PMC_int_val((PMC*)b->value);

    /* Create a delimiter for splitting up the Class\0attribute syntax. */
    delimit = string_from_cstring(interpreter, "\0", 1);

    /* Calculate the offset and the length of the attribute string. */
    idx  = string_str_index(interpreter, attr, delimit, 0) + 1;
    if (!idx) {
        real_exception(interpreter, NULL, ATTRIB_NOT_FOUND,
                "No such attribute '%Ss'", attr);
        return 0;
    }
    length = string_length(interpreter, attr) - idx;

    /* Extract the attribute and object name. */
    attr_name = string_substr(interpreter, attr, idx, length, 0, 0);
    obj_name = string_substr(interpreter, attr, 0, idx-1, 0, 0);

    real_exception(interpreter, NULL, ATTRIB_NOT_FOUND,
	    "No such attribute '%Ss\\0%Ss'",
	    obj_name, attr_name);

    return 0;
}

PMC *
Parrot_get_attrib_by_str(Interp* interpreter, PMC *object, STRING *attr)
{
    return Parrot_get_attrib_by_num(interpreter, object,
                POD_FIRST_ATTRIB +
                attr_str_2_num(interpreter, object, attr));
}

/*

=item C<PMC *
Parrot_set_attrib_by_num(Interp* interpreter, PMC *object,
  INTVAL attrib, PMC *value)>

Set attribute number C<attrib> from C<object> to C<value>. Presumably the code
is asking for the correct attribute number.

=item C<PMC *
Parrot_set_attrib_by_str(Interp* interpreter, PMC *object,
  STRING *attr, PMC *value)>

Sets attribute with full qualified name C<attr> from C<object> to C<value>.

=cut

*/

void
Parrot_set_attrib_by_num(Interp* interpreter, PMC *object,
        INTVAL attrib, PMC *value)
{
    SLOTTYPE * const attrib_array = PMC_data(object);
    const INTVAL attrib_count = PMC_int_val(object);

    if (attrib >= attrib_count || attrib < POD_FIRST_ATTRIB) {
        real_exception(interpreter, NULL, ATTRIB_NOT_FOUND,
                "No such attribute #%d", (int)attrib);
    }
    set_attrib_num(object, attrib_array, attrib, value);
}

void
Parrot_set_attrib_by_str(Interp* interpreter, PMC *object,
        STRING *attr, PMC *value)
{

    Parrot_set_attrib_by_num(interpreter, object,
                POD_FIRST_ATTRIB +
                attr_str_2_num(interpreter, object, attr),
                value);
}

INTVAL
Parrot_class_offset(Interp* interpreter, PMC *object, STRING *class) {
    PMC *offset_hash;
    PMC *class_pmc;
    INTVAL offset;
    HashBucket *b;

    if (!PObj_is_object_TEST(object))
        internal_exception(1, "Not an object");
    class_pmc = GET_CLASS((SLOTTYPE *)PMC_data(object), object);
    offset_hash = get_attrib_num((SLOTTYPE *)PMC_data(class_pmc),
                                 PCD_ATTRIB_OFFS);
#if 0
    if (VTABLE_exists_keyed_str(interpreter, offset_hash, class)) {
        offset = VTABLE_get_integer_keyed_str(interpreter, offset_hash, class);
    }
    else {
        offset = -1;
    }
#else
    /*
     * cheat a bit--the offset_hash is a Hash PMC
     */
    b = parrot_hash_get_bucket(interpreter,
                (Hash*) PMC_struct_val(offset_hash), class);
    if (!b)
        offset = -1;
    else {
        return PMC_int_val((PMC*)b->value);
    }
#endif
    return offset;
}

/*

=item C<PMC *
Parrot_find_class_constructor(Interp* interpreter, STRING *class, INTVAL classtoken)>

Find and return the constructor method PMC for the named sub. The
classtoken is an identifier for the class used for fast lookup, or 0
if you don't have an identifier token. Which, as they're currently
undefined, is pretty likely

=cut

*/

PMC *
Parrot_find_class_constructor(Interp* interpreter, STRING *class, INTVAL classtoken)
{
    return NULL;
}

PMC *
Parrot_find_class_destructor(Interp* interpreter, STRING *class, INTVAL classtoken)
{
    return NULL;
}

PMC *
Parrot_find_class_fallback(Interp* interpreter, STRING *class, INTVAL classtoken)
{
    return NULL;
}

void
Parrot_set_class_constructor(Interp* interpreter, STRING *class, INTVAL classtoken, STRING *method)
{
}

void
Parrot_set_class_destructor(Interp* interpreter, STRING *class, INTVAL classtoken, STRING *method)
{
}

void
Parrot_set_class_fallback(Interp* interpreter, STRING *class, INTVAL classtoken, STRING *method)
{
}

/*

=back

=head1 SEE ALSO

F<include/parrot/objects.h>, F<docs/pdds/pdd15_objects.pod>.

=cut

*/

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
