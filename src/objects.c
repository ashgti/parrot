/* objects.c
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  CVS Info
 *     $Id$
 *  Overview:
 *     Handles class and object manipulation
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#include "parrot/parrot.h"
#include <assert.h>

/* This should be public, but for right now it's internal */
static PMC *
find_global(Parrot_Interp interpreter, STRING *globalname) {
    if (!VTABLE_exists_keyed_str(interpreter,
                interpreter->globals->stash_hash, globalname)) {
        return NULL;
    }

    return VTABLE_get_pmc_keyed_str(interpreter,
            interpreter->globals->stash_hash, globalname);
}

/* Subclass a class. Single parent class, nice and
   straightforward. If child_class is NULL, this is an anonymous
   subclass we're creating, which happens commonly enough to warrant
   an actual single-subclass function
 */
PMC *
Parrot_single_subclass(Parrot_Interp interpreter, PMC *base_class,
                       STRING *child_class_name)
{
    PMC *child_class;
    PMC *child_class_array;
    PMC *classname_pmc;
    PMC *parents, *temp_pmc;

    if (!PObj_is_class_TEST(base_class)) {
        internal_exception(NO_CLASS, "Can't subclass a non-class!");
    }

    child_class = pmc_new(interpreter, enum_class_ParrotClass);
    /* Hang an array off the data pointer */
    child_class_array = PMC_data(child_class) =
        pmc_new(interpreter, enum_class_SArray);
    /* We will have five entries in this array */
    VTABLE_set_integer_native(interpreter, child_class_array, PCD_MAX);

    /* We have the same number of attributes as our parent */
    child_class->cache.int_val = base_class->cache.int_val;

    /* Our parent class array has a single member in it */
    parents = pmc_new(interpreter, enum_class_Array);
    VTABLE_set_integer_native(interpreter, parents, 1);
    VTABLE_set_pmc_keyed_int(interpreter, parents, 0, base_class);
    VTABLE_set_pmc_keyed_int(interpreter, child_class_array, PCD_PARENTS,
            parents);

    /* Set the classname, if we have one */
    classname_pmc = pmc_new(interpreter, enum_class_PerlString);
    if (child_class_name) {
        VTABLE_set_string_native(interpreter, classname_pmc, child_class_name);

        /* Add ourselves to the interpreter's class hash */
        VTABLE_set_pmc_keyed_str(interpreter, interpreter->class_hash,
                child_class_name, child_class);
    }
    else {
        VTABLE_set_string_native(interpreter, classname_pmc,
                string_make(interpreter,
                    "\0\0anonymous",
                    11, NULL, 0, NULL));
    }

    VTABLE_set_pmc_keyed_int(interpreter, child_class_array, PCD_CLASS_NAME,
            classname_pmc);

    /* Our penultimate parent list is a clone of our parent's parent
       list, with our parent unshifted onto the beginning */
    temp_pmc = pmc_new_noinit(interpreter, enum_class_Array);
    VTABLE_clone(interpreter,
            VTABLE_get_pmc_keyed_int(interpreter,
                (PMC *)PMC_data(base_class), PCD_ALL_PARENTS),
            temp_pmc);
    VTABLE_unshift_pmc(interpreter, temp_pmc, base_class);
    VTABLE_set_pmc_keyed_int(interpreter, child_class_array, PCD_ALL_PARENTS,
            temp_pmc);

    /* Our attribute list is our parent's attribute list */
    temp_pmc = pmc_new_noinit(interpreter, enum_class_OrderedHash);
    VTABLE_clone(interpreter,
            VTABLE_get_pmc_keyed_int(interpreter,
                (PMC *)PMC_data(base_class), PCD_ATTRIB_OFFS),
            temp_pmc);
    VTABLE_set_pmc_keyed_int(interpreter, child_class_array, PCD_ATTRIB_OFFS,
            temp_pmc);

    /* And our full keyed attribute list is our parent's */
    temp_pmc = pmc_new_noinit(interpreter, enum_class_OrderedHash);
    VTABLE_clone(interpreter,
            VTABLE_get_pmc_keyed_int(interpreter,
                (PMC *)PMC_data(base_class), PCD_ATTRIBUTES),
            temp_pmc);
    VTABLE_set_pmc_keyed_int(interpreter, child_class_array, PCD_ATTRIBUTES,
            temp_pmc);

    Parrot_class_register(interpreter, child_class_name, child_class);

    return child_class;
}

/*=for api objects Parrot_new_class
 *
 * Create a brand new class, named what we pass in.
 */
void
Parrot_new_class(Parrot_Interp interpreter, PMC *class, STRING *class_name)
{
    PMC *class_array;
    PMC *classname_pmc;

    /* Hang an array off the data pointer, empty of course */
    class_array = PMC_data(class) = pmc_new(interpreter, enum_class_SArray);
    /* We will have five entries in this array */
    VTABLE_set_integer_native(interpreter, class_array, PCD_MAX);
    /* Our parent class array has nothing in it */
    VTABLE_set_pmc_keyed_int(interpreter, class_array, PCD_PARENTS,
            pmc_new(interpreter, enum_class_Array));
    VTABLE_set_pmc_keyed_int(interpreter, class_array, PCD_ALL_PARENTS,
            pmc_new(interpreter, enum_class_Array));
    VTABLE_set_pmc_keyed_int(interpreter, class_array, PCD_ATTRIB_OFFS,
            pmc_new(interpreter, enum_class_OrderedHash));
    VTABLE_set_pmc_keyed_int(interpreter, class_array, PCD_ATTRIBUTES,
            pmc_new(interpreter, enum_class_OrderedHash));

    /* Set the classname, if we have one */
    classname_pmc = pmc_new(interpreter, enum_class_PerlString);
    VTABLE_set_string_native(interpreter, classname_pmc, class_name);
    VTABLE_set_pmc_keyed_int(interpreter, class_array, PCD_CLASS_NAME,
            classname_pmc);

    Parrot_class_register(interpreter, class_name, class);
}


PMC *
Parrot_class_lookup(Parrot_Interp interpreter, STRING *class_name)
{
    if (VTABLE_exists_keyed_str(interpreter, interpreter->class_hash,
                class_name))
        return VTABLE_get_pmc_keyed_str(interpreter, interpreter->class_hash,
                class_name);
    return PMCNULL;
}

/* This is the method to register a new Parrot class as an
   instantiatable type. Doing this involves putting it in the class
   hash, setting its vtable so that the init method inits objects of
   the class rather than the class itself, and adding it to the
   interpreter's base type table so you can "new Px, foo" the things.
*/
void
Parrot_class_register(Parrot_Interp interpreter, STRING *class_name,
        PMC *new_class)
{
    INTVAL new_type;
    VTABLE *new_vtable;

    /*
     * register the class in the PMCs name hash and in the
     * class_name hash
     */
    if ((new_type = pmc_type(interpreter, class_name)) > enum_type_undef) {
        internal_exception(1, "Class %s already registered!\n",
                string_to_cstring(interpreter, class_name));
    }
    new_type = pmc_register(interpreter, class_name);
    /* Build a new vtable for this class
     * The child class PMC gets a ParrotClass vtable, which is a
     * good base to work from
     * XXX we are leaking ths vtable
     */
    new_vtable = Parrot_clone_vtable(interpreter, new_class->vtable);

    /* register the class */
    VTABLE_set_pmc_keyed_str(interpreter, interpreter->class_hash,
            class_name, new_class);

    /* Set the vtable's type to the newly allocated type */
    Parrot_vtable_set_type(interpreter, new_vtable, new_type);

    /* And cache our class PMC in the vtable so we can find it later
    */
    Parrot_vtable_set_data(interpreter, new_vtable, new_class);

    /* Reset the init method to our instantiation method */
    new_vtable->init = Parrot_instantiate_object;
    new_class->vtable = new_vtable;

    /* Put our new vtable in the global table */
    Parrot_base_vtables[new_type] = new_vtable;
}


/*=for api objects Parrot_instantiate_object
 *
 * Create a  parrot object. Takes a passed-in class PMC that has
 * sufficient information to describe the layout of the object and,
 * well, makes the darned object.
 *
 */
void
Parrot_instantiate_object(Parrot_Interp interpreter, PMC *object) {
    PMC *new_object_array;
    INTVAL attrib_count;
    PMC *class_array;
    PMC *class;
    INTVAL class_enum;
    PMC *class_name;

    class = object->vtable->data;
    /* * remember PMC type */
    class_enum = object->vtable->base_type;
    /* put in the real vtable
     * XXX we are leaking ths vtable
     */
    object->vtable = Parrot_clone_vtable(interpreter,
                Parrot_base_vtables[enum_class_ParrotObject]);
    /* and set type of class */
    object->vtable->base_type = class_enum;

    /* Grab the attribute count from the parent */
    attrib_count = class->cache.int_val;

    class_array = PMC_data(class);
    class_name = VTABLE_get_pmc_keyed_int(interpreter, class_array,
            PCD_CLASS_NAME);

    /* Build the array that hangs off the new object */
    new_object_array = pmc_new(interpreter, enum_class_Array);
    /* Presize it */
    VTABLE_set_integer_native(interpreter, new_object_array,
            attrib_count + POD_FIRST_ATTRIB);
    /* 0 - class PMC, 1 - class name */
    VTABLE_set_pmc_keyed_int(interpreter, new_object_array, POD_CLASS, class);
    VTABLE_set_pmc_keyed_int(interpreter, new_object_array, POD_CLASS_NAME,
            class_name);

    /* Note the number of used slots */
    object->cache.int_val = POD_FIRST_ATTRIB;

    PMC_data(object) = new_object_array;
    PObj_flag_SET(is_PMC_ptr, object);
    /* We are an object now */
    PObj_is_object_SET(object);

    /* TODO We really ought to call the class init routines here... */
}

PMC *
Parrot_add_parent(Parrot_Interp interpreter, PMC *new_base_class,
           PMC *existing_class) {
    return NULL;
}

PMC *
Parrot_remove_parent(Parrot_Interp interpreter, PMC *removed_class,
                     PMC *existing_class) {
    return NULL;
}

PMC *
Parrot_multi_subclass(Parrot_Interp interpreter, PMC *base_class_array,
                      STRING *child_class_name) {
    return NULL;
}

/*=for api objects Parrot_object_is
 *
 * Is the object an instance of class.
 */
INTVAL
Parrot_object_isa(Parrot_Interp interpreter, PMC *pmc, PMC *cl) {
    PMC * t;
    PMC * object_array = PMC_data(pmc);
    PMC* classsearch_array; /* The array of classes we're searching */
    INTVAL i, classcount;

    /* if this is a class */
    if (PObj_is_class_TEST(pmc)) {
        t = pmc;
        /* check if this is self */
        if (pmc == cl)
            return 1;
    }
    else {
        /* else get the objects class and the data array */
        t = VTABLE_get_pmc_keyed_int(interpreter, object_array, POD_CLASS);
        object_array = PMC_data(t);
    }
    if (t == cl)
        return 1;
    /* If not, time to walk through the parent class array. Wheee */
    classsearch_array =
        VTABLE_get_pmc_keyed_int(interpreter, object_array, PCD_ALL_PARENTS);
    classcount = VTABLE_get_integer(interpreter, classsearch_array);
    for (i = 0; i < classcount; ++i) {
        if (VTABLE_get_pmc_keyed_int(interpreter, classsearch_array, i) == cl)
            return 1;
    }
    return 0;
}

/*=for api objects Parrot_new_method_cache
 *
 * Create a new method cache PMC.
 */
PMC *
Parrot_new_method_cache(Parrot_Interp interpreter) {
    return NULL;
}

/*=for api objects Parrot_find_method
 *
 * Find a method PMC for a named method, given the class PMC, current
 * interpreter, and name of the method.
 *
 * This routine should use the current scope's method cache, if there
 * is one. If not, it creates a new method cache. Or, rather, it will
 * when we've got that bit working. For now it unconditionally goes
 * and looks up the name in the global stash.
 *
 */
PMC *
Parrot_find_method_with_cache(Parrot_Interp interpreter, PMC *class,
                              STRING *method_name) {
    PMC* method = NULL;  /* The method we ultimately return */
    PMC* classname;      /* The classname PMC for the currently
                            searched class */
    PMC* curclass;          /* PMC for the current search class */
    PMC* classsearch_array; /* The array of classes we're searching
                               for the method in */
    INTVAL searchoffset = 0; /* Where in that array we are */
    INTVAL classcount = 0;   /* The number of classes we need to
                                search */
    STRING *FQ_method;   /* The fully qualified name of the method
                            that we're going to look for, rebuilt for
                            each class we search */
    STRING *fallback_name; /* The name of the fallback method for
                              this class */
    STRING *shortcut_name; /* The method name with the separator
                              prepended */

    /* if its a non-classes, just return the sub */
    if (!PObj_is_class_TEST(class)) {
        return find_global(interpreter, method_name);
    }

    /* We're going to make this over and over, so get it once and
       skip the repeated string makes */
    shortcut_name = string_concat(interpreter,
            string_from_cstring(interpreter, PARROT_NAMESPACE_SEPARATOR,
                PARROT_NAMESPACE_SEPARATOR_LENGTH),
            method_name, 0);

    /* The order of operations:
     *
     * - Look for the method in the class we were passed
     * - If that doesn't exist, grab the parent class array
     * -  For each element in the parent class array, look for the
     *    method
     * - If none of that works, try again looking for the fallback method
     */

    /* See if we get lucky and its in the class of the PMC */
    FQ_method = string_concat(interpreter,
            VTABLE_get_string(interpreter,
                VTABLE_get_pmc_keyed_int(interpreter,
                    (PMC *)PMC_data(class), PCD_CLASS_NAME)),
            shortcut_name, 0);

    method = find_global(interpreter, FQ_method);

    /* Bail immediately if we got something */
    if (NULL != method) {
        return method;
    }

    /* If not, time to walk through the parent class array. Wheee */
    classsearch_array =
        VTABLE_get_pmc_keyed_int(interpreter, (PMC *)PMC_data(class),
                PCD_ALL_PARENTS);
    classcount = VTABLE_get_integer(interpreter, classsearch_array);

    for (searchoffset = 0; NULL == method && searchoffset < classcount;
            searchoffset++) {
        curclass = VTABLE_get_pmc_keyed_int(interpreter,
                classsearch_array, searchoffset);

        FQ_method = string_concat(interpreter,
                VTABLE_get_string(interpreter,
                    VTABLE_get_pmc_keyed_int(interpreter,
                        (PMC *)PMC_data(curclass), PCD_CLASS_NAME)),
                shortcut_name, 0);
        method = find_global(interpreter, FQ_method);
    }


    /* Ultimately, if we've failed, pitch an exception */
    if (NULL == method) {
        real_exception(interpreter, NULL, METH_NOT_FOUND,
                "Method '%s' not found\n",
                string_to_cstring(interpreter, method_name));
    }
    return method;
}

INTVAL
Parrot_add_attribute(Parrot_Interp interpreter, PMC* class, STRING* attr)
{
    PMC *class_array;
    STRING *class_name, *full_attr_name;
    INTVAL idx;
    PMC *offs_hash;
    PMC *attr_hash;

    class_array = (PMC*) PMC_data(class);
    class_name = VTABLE_get_string(interpreter,
            VTABLE_get_pmc_keyed_int(interpreter,
            class_array, PCD_CLASS_NAME));
    /*
     * our attributes start at offset found in hash at PCD_ATTRIB_OFFS
     */
    offs_hash = VTABLE_get_pmc_keyed_int(interpreter,
            class_array, PCD_ATTRIB_OFFS);
    if (VTABLE_exists_keyed_str(interpreter, offs_hash, class_name))
        idx = VTABLE_get_integer_keyed_str(interpreter, offs_hash, class_name);
    else {
        PMC* parent_array = VTABLE_get_pmc_keyed_int(interpreter,
                class_array, PCD_ALL_PARENTS);
        if (VTABLE_elements(interpreter, parent_array)) {
            PMC *parent = VTABLE_get_pmc_keyed_int(interpreter,
                    parent_array, 0);
            PMC *parent_attr_hash = VTABLE_get_pmc_keyed_int(interpreter,
                    (PMC*) PMC_data(parent), PCD_ATTRIBUTES);
            idx = VTABLE_elements(interpreter, parent_attr_hash);
        }
        else
            idx = 0;
        VTABLE_set_integer_keyed_str(interpreter, offs_hash, class_name, idx);
    }
    attr_hash = VTABLE_get_pmc_keyed_int(interpreter,
            class_array, PCD_ATTRIBUTES);
    full_attr_name = Parrot_sprintf_c(interpreter, "%S%s%S",
            class_name, PARROT_NAMESPACE_SEPARATOR, attr),
    idx = VTABLE_elements(interpreter, attr_hash);
    assert(class->cache.int_val == idx);
    VTABLE_set_integer_keyed_str(interpreter, attr_hash,
            full_attr_name, idx);
    assert(idx + 1 == VTABLE_elements(interpreter, attr_hash));
    class->cache.int_val = idx + 1;
    return idx;
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
