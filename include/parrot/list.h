/*
 * list.h
 *  Copyright (C) 2002, The Perl Foundation.
 *  License:  Artistic 2.0, see README and LICENSES for details
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
    Buffer data;                /* item store */
    UINTVAL flags;              /* chunk flags */
    UINTVAL items;              /* items in this chunk */
    UINTVAL n_chunks;           /* # of chunks with grow policy in flags */
    UINTVAL n_items;            /* # of items with grow policy in flags */
    struct List_chunk *next;
    struct List_chunk *prev;
} List_chunk;

#define sparse PObj_private0_FLAG
#define no_power_2  PObj_private1_FLAG
#define fixed_items PObj_private2_FLAG
#define grow_items PObj_private3_FLAG

typedef struct List {
    Buffer chunk_list;          /* pointers to chunks */
    UINTVAL length;             /* number of items in list */
    UINTVAL start;              /* offset, where array[0] is */
    PMC * container;            /* the Array PMC */
    PARROT_DATA_TYPE item_type; /* item type */
    int item_size;              /* item size */
    int items_per_chunk;        /* override defaults */
    UINTVAL cap;                /* list capacity in items */
    int grow_policy;            /* fixed / variable len */
    UINTVAL collect_runs;       /* counter, when chunklist was built */
    UINTVAL n_chunks;           /* number of chunks */
    PMC * user_data;            /* e.g. multiarray dimensions */
    List_chunk *first;          /* first chunk holding data */
    List_chunk *last;           /* last chunk */
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

PARROT_API
PARROT_CONST_FUNCTION
PARROT_WARN_UNUSED_RESULT
UINTVAL ld(UINTVAL x);

PARROT_API
void list_assign(PARROT_INTERP,
    ARGINOUT(List *list),
    INTVAL idx,
    NULLOK(void *item),
    int type)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_API
PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
List * list_clone(PARROT_INTERP, ARGIN(const List *other))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void list_delete(PARROT_INTERP,
    ARGINOUT(List *list),
    INTVAL idx,
    INTVAL n_items)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_API
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
void * list_get(PARROT_INTERP, ARGINOUT(List *list), INTVAL idx, int type)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_API
void list_insert(PARROT_INTERP,
    ARGINOUT(List *list),
    INTVAL idx,
    INTVAL n_items)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_PURE_FUNCTION
INTVAL list_length(SHIM_INTERP, ARGIN(const List *list))
        __attribute__nonnull__(2);

PARROT_API
void list_mark(PARROT_INTERP, ARGINOUT(List *list))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_API
PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
List * list_new(PARROT_INTERP, PARROT_DATA_TYPE type)
        __attribute__nonnull__(1);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
List * list_new_init(PARROT_INTERP, PARROT_DATA_TYPE type, ARGIN(PMC *init))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

PARROT_API
void list_pmc_new(PARROT_INTERP, ARGINOUT(PMC *container))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*container);

PARROT_API
void list_pmc_new_init(PARROT_INTERP,
    ARGINOUT(PMC *container),
    ARGIN(PMC *init))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*container);

PARROT_API
PARROT_CAN_RETURN_NULL
void * list_pop(PARROT_INTERP, ARGINOUT(List *list), int type)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_API
void list_push(PARROT_INTERP,
    ARGINOUT(List *list),
    ARGIN_NULLOK(void *item),
    int type)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_API
void list_set_length(PARROT_INTERP, ARGINOUT(List *list), INTVAL len)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_API
PARROT_CAN_RETURN_NULL
void * list_shift(PARROT_INTERP, ARGINOUT(List *list), int type)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_API
void list_splice(PARROT_INTERP,
    ARGINOUT(List *list),
    ARGIN_NULLOK(List *value_list),
    INTVAL offset,
    INTVAL count)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_API
void list_unshift(PARROT_INTERP,
    ARGINOUT(List *list),
    ARGIN_NULLOK(void *item),
    int type)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_API
void list_visit(PARROT_INTERP, ARGIN(List *list), ARGINOUT(void *pinfo))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*pinfo);

/* HEADERIZER END: src/list.c */

#endif /* PARROT_LIST_H_GUARD */


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
