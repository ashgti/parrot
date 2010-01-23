/*
 * list.h
 *  Copyright (C) 2002-2008, Parrot Foundation.
 *  License:  Artistic 2.0, see README and LICENSE for details
 *  SVN Info
 *     $Id$
 *  Overview:
 *     list aka array routines for Parrot
 *     s. list.c for more
 */

#include "parrot/parrot.h"

#ifndef PARROT_LIST_H_GUARD
#define PARROT_LIST_H_GUARD

typedef struct List_chunk {
    Buffer             data;        /* item store, Buffer must be first element in struct*/
    struct List_chunk *next;
    struct List_chunk *prev;
    UINTVAL            flags;       /* chunk flags */
    UINTVAL            items;       /* items in this chunk */
    UINTVAL            n_chunks;    /* # of chunks with grow policy in flags */
    UINTVAL            n_items;     /* # of items  with grow policy in flags */
} List_chunk;

#define sparse      PObj_private0_FLAG
#define no_power_2  PObj_private1_FLAG
#define fixed_items PObj_private2_FLAG
#define grow_items  PObj_private3_FLAG

typedef struct List {
    Buffer           chunk_list;      /* pointers to chunks */
    PMC             *container;       /* the Array PMC */
    List_chunk      *first;           /* first chunk holding data */
    List_chunk      *last;            /* last chunk */
    UINTVAL          length;          /* number of items in list */
    UINTVAL          start;           /* offset, where array[0] is */
    PARROT_DATA_TYPE item_type;       /* item type */
    UINTVAL          cap;             /* list capacity in items */
    UINTVAL          collect_runs;    /* counter, when chunklist was built */
    UINTVAL          n_chunks;        /* number of chunks */
    int              grow_policy;     /* fixed / variable len */
    int              items_per_chunk; /* override defaults */
    int              item_size;       /* item size */
} List;

typedef enum {
    enum_grow_unknown,          /* at beginning, or after emptying list */
    enum_grow_mixed = 1,        /* other */
    enum_grow_fixed = fixed_items,        /* fixed maximum size */
    enum_grow_growing = grow_items        /* growing at begin of list */
} ARRAY_GROW_TYPE;

typedef enum {
    enum_add_at_start,          /* don't swap these */
    enum_add_at_end
} ARRAY_ADD_POS;


#ifdef LIST_TEST
#  define MIN_ITEMS 4           /* smallest chunk can hold */
#  define LD_MAX 4              /* log2(MAX_ITEMS) */
#  define MAX_ITEMS 16          /* biggest chunk can hold */
#else
#  define MIN_ITEMS 16          /* smallest chunk can hold */
#  define LD_MAX 10             /* log2(MAX_ITEMS) */
#  define MAX_ITEMS 1024        /* biggest chunk can hold */
#endif /* LIST_TEST */
#define MAX_MASK (MAX_ITEMS-1)

/*
 * bigger MAX_ITEMS didn't improve much in my tests
 * 10^6 list_get, MAX_ITEMS 1024: 0.34s, 2048: 0.33s
 */


/* HEADERIZER BEGIN: src/list.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_EXPORT
PARROT_CONST_FUNCTION
PARROT_WARN_UNUSED_RESULT
UINTVAL ld(UINTVAL x);

PARROT_EXPORT
void Parrot_pmc_array_assign(PARROT_INTERP,
    ARGMOD(List *list),
    INTVAL idx,
    ARGIN_NULLOK(void *item),
    int type)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
List * Parrot_pmc_array_clone(PARROT_INTERP, ARGIN(const List *other))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_EXPORT
void Parrot_pmc_array_delete(PARROT_INTERP,
    ARGMOD(List *list),
    INTVAL idx,
    INTVAL n_items)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
void * Parrot_pmc_array_get(PARROT_INTERP,
    ARGMOD(List *list),
    INTVAL idx,
    int type)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_EXPORT
void Parrot_pmc_array_insert(PARROT_INTERP,
    ARGMOD(List *list),
    INTVAL idx,
    INTVAL n_items)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_EXPORT
void Parrot_pmc_array_mark(PARROT_INTERP, ARGMOD(List *list))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
List * Parrot_pmc_array_new(PARROT_INTERP, PARROT_DATA_TYPE type)
        __attribute__nonnull__(1);

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
List * Parrot_pmc_array_new_init(PARROT_INTERP,
    PARROT_DATA_TYPE type,
    ARGIN(PMC *init))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

PARROT_EXPORT
void Parrot_pmc_array_pmc_new(PARROT_INTERP, ARGMOD(PMC *container))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*container);

PARROT_EXPORT
void Parrot_pmc_array_pmc_new_init(PARROT_INTERP,
    ARGMOD(PMC *container),
    ARGIN(PMC *init))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*container);

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
void * Parrot_pmc_array_pop(PARROT_INTERP, ARGMOD(List *list), int type)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_EXPORT
void Parrot_pmc_array_push(PARROT_INTERP,
    ARGMOD(List *list),
    ARGIN_NULLOK(void *item),
    int type)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_EXPORT
void Parrot_pmc_array_set_length(PARROT_INTERP,
    ARGMOD(List *list),
    INTVAL len)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
void * Parrot_pmc_array_shift(PARROT_INTERP, ARGMOD(List *list), int type)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_EXPORT
void Parrot_pmc_array_splice(PARROT_INTERP,
    ARGMOD(List *list),
    ARGMOD_NULLOK(List *value_list),
    INTVAL offset,
    INTVAL count)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list)
        FUNC_MODIFIES(*value_list);

PARROT_EXPORT
void Parrot_pmc_array_unshift(PARROT_INTERP,
    ARGMOD(List *list),
    ARGIN(void *item),
    int type)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*list);

PARROT_EXPORT
void Parrot_pmc_array_visit(PARROT_INTERP,
    ARGIN(List *list),
    ARGMOD(visit_info *info))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*info);

PARROT_WARN_UNUSED_RESULT
PARROT_PURE_FUNCTION
INTVAL Parrot_pmc_array_length(SHIM_INTERP, ARGIN(const List *list))
        __attribute__nonnull__(2);

#define ASSERT_ARGS_ld __attribute__unused__ int _ASSERT_ARGS_CHECK = (0)
#define ASSERT_ARGS_Parrot_pmc_array_assign __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_Parrot_pmc_array_clone __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(other))
#define ASSERT_ARGS_Parrot_pmc_array_delete __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_Parrot_pmc_array_get __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_Parrot_pmc_array_insert __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_Parrot_pmc_array_mark __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_Parrot_pmc_array_new __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_pmc_array_new_init __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(init))
#define ASSERT_ARGS_Parrot_pmc_array_pmc_new __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(container))
#define ASSERT_ARGS_Parrot_pmc_array_pmc_new_init __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(container) \
    , PARROT_ASSERT_ARG(init))
#define ASSERT_ARGS_Parrot_pmc_array_pop __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_Parrot_pmc_array_push __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_Parrot_pmc_array_set_length __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_Parrot_pmc_array_shift __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_Parrot_pmc_array_splice __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_Parrot_pmc_array_unshift __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list) \
    , PARROT_ASSERT_ARG(item))
#define ASSERT_ARGS_Parrot_pmc_array_visit __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list) \
    , PARROT_ASSERT_ARG(info))
#define ASSERT_ARGS_Parrot_pmc_array_length __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(list))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/list.c */

#endif /* PARROT_LIST_H_GUARD */


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
