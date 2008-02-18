/* objects.h
 *  Copyright (C) 2007-2008, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     Parrot class and object header stuff
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#ifndef PARROT_OO_H_GUARD
#define PARROT_OO_H_GUARD

#include "parrot/parrot.h"

typedef enum {
    PCD_PARENTS,            /* An array of immediate parents */
    PCD_CLASS_NAME,         /* A String PMC */
    PCD_ATTRIBUTES,         /* class::attrib => offset hash */
    PCD_CLASS_ATTRIBUTES,   /* Class attribute array */
    PCD_OBJECT_VTABLE,      /* Vtable PMC that holds the vtable for objects of this class */
    PCD_MAX
} PARROT_CLASS_DATA_ENUM;


/* Objects, classes and PMCarrays all use the same data scheme:
 * PMC_data() holds a malloced array, PMC_int_val() is the size of it
 * this simplifies DOD mark a lot
 *
 * The active destroy flag is necessary to free the malloced array.
 */
#define SLOTTYPE PMC*
#define get_attrib_num(x, y)    ((PMC **)x)[y]
#define set_attrib_num(o, x, y, z) \
    do { \
        DOD_WRITE_BARRIER(interp, o, ((PMC **)x)[y], z); \
        ((PMC **)x)[y] = z; \
    } while (0)
#define set_attrib_flags(x) do { \
        PObj_data_is_PMC_array_SET(x); \
        PObj_active_destroy_SET(x); \
    } while (0)
#define set_attrib_array_size(o, y) do { \
    PMC_data(o) = mem_sys_allocate_zeroed((sizeof (PMC *)*(y))); \
    PMC_int_val(o) = y; \
} while (0)

/*
 * class macros
 */

#define CLASS_ATTRIB_COUNT(cl)  PMC_int_val2(cl)
#define GET_CLASS(obj)          (obj)->vtable->pmc_class

/* HEADERIZER BEGIN: src/oo.c */

PARROT_API
INTVAL Parrot_add_attribute(PARROT_INTERP,
    ARGIN(PMC *_class),
    ARGIN(STRING *attr))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * Parrot_class_lookup(PARROT_INTERP, ARGIN(STRING *class_name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void Parrot_ComposeRole(PARROT_INTERP,
    ARGIN(PMC *role),
    ARGIN(PMC *exclude),
    int got_exclude,
    ARGIN(PMC *alias),
    int got_alias,
    ARGIN(PMC *methods_hash),
    ARGIN(PMC *roles_list))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(5)
        __attribute__nonnull__(7)
        __attribute__nonnull__(8);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC* Parrot_ComputeMRO_C3(PARROT_INTERP, ARGIN(PMC *_class))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * Parrot_find_method_direct(PARROT_INTERP,
    ARGIN(PMC *_class),
    ARGIN(STRING *method_name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * Parrot_find_method_with_cache(PARROT_INTERP,
    ARGIN(PMC *_class),
    ARGIN(STRING *method_name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
PARROT_CAN_RETURN_NULL
PMC* Parrot_find_vtable_meth(PARROT_INTERP,
    ARGIN(PMC *pmc),
    ARGIN(STRING *meth))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
INTVAL Parrot_get_vtable_index(PARROT_INTERP, ARGIN(const STRING *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void Parrot_instantiate_object(PARROT_INTERP, ARGMOD(PMC *object))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*object);

PARROT_API
void Parrot_instantiate_object_init(PARROT_INTERP,
    ARGIN(PMC *object),
    ARGIN(PMC *init))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
void Parrot_invalidate_method_cache(PARROT_INTERP,
    ARGIN_NULLOK(STRING *_class),
    ARGIN(STRING *meth))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

PARROT_API
PARROT_PURE_FUNCTION
INTVAL Parrot_MMD_method_idx(SHIM_INTERP, ARGIN(const char *name))
        __attribute__nonnull__(2);

PARROT_API
PARROT_PURE_FUNCTION
PARROT_CAN_RETURN_NULL
const char* Parrot_MMD_method_name(SHIM_INTERP, INTVAL idx);

PARROT_API
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * Parrot_oo_get_class_str(PARROT_INTERP, ARGIN(STRING *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_IGNORABLE_RESULT
PARROT_CAN_RETURN_NULL
PMC * Parrot_remove_parent(PARROT_INTERP,
    ARGIN(PMC *removed_class),
    ARGIN(PMC *existing_class))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC * Parrot_single_subclass(PARROT_INTERP,
    ARGIN(PMC *base_class),
    ARGIN_NULLOK(PMC *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING* readable_name(PARROT_INTERP, ARGIN(PMC *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void destroy_object_cache(PARROT_INTERP)
        __attribute__nonnull__(1);

void init_object_cache(PARROT_INTERP)
        __attribute__nonnull__(1);

void mark_object_cache(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * Parrot_class_lookup_p(PARROT_INTERP, ARGIN(PMC *class_name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void Parrot_oo_extract_methods_from_namespace(PARROT_INTERP,
    ARGIN(PMC *self),
    ARGIN(PMC *ns))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * Parrot_oo_find_vtable_override(PARROT_INTERP,
    ARGIN(PMC *classobj),
    ARGIN(STRING *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * Parrot_oo_find_vtable_override_for_class(PARROT_INTERP,
    ARGIN(PMC *classobj),
    ARGIN(STRING *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * Parrot_oo_get_class(PARROT_INTERP, ARGIN(PMC *key))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * Parrot_oo_get_namespace(SHIM_INTERP, ARGIN(const PMC *classobj))
        __attribute__nonnull__(2);

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * Parrot_oo_newclass_from_str(PARROT_INTERP, ARGIN(STRING *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
INTVAL Parrot_oo_register_type(PARROT_INTERP, ARGIN(PMC *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

/* HEADERIZER END: src/oo.c */

#endif /* PARROT_OO_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
