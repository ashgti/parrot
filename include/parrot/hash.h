/* hash.h
 *  Copyright (C) 2001-2003, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     Hashtable implementation
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#ifndef PARROT_HASH_H_GUARD
#define PARROT_HASH_H_GUARD

/* Prototypes */

/* Hash is really a hashtable, but 'hash' is standard perl nomenclature. */

typedef enum {
    enum_hash_undef,
    enum_hash_int = enum_type_INTVAL,
    enum_hash_num = enum_type_FLOATVAL,
    enum_hash_string = enum_type_STRING,
    enum_hash_pmc = enum_type_PMC,
    enum_hash_ptr = enum_type_ptr
} HashEntryType;

/*
 * hash_entry is currently unused in the hash structure
 */
typedef struct _hash_entry {
    HashEntryType type;
    UnionVal val;
} HashEntry;

/* A BucketIndex is an index into the pool of available buckets. */
typedef UINTVAL BucketIndex;
#define INITBucketIndex ((BucketIndex)-2)

typedef int (*hash_comp_fn)(PARROT_INTERP, const void*const, const void*const);
typedef void (*hash_mark_key_fn)(PARROT_INTERP, PObj *);
typedef size_t (*hash_hash_key_fn)(PARROT_INTERP, NOTNULL(void*), size_t seed);

typedef enum {
    Hash_key_type_int,
    Hash_key_type_cstring,
    Hash_key_type_STRING,
    Hash_key_type_PMC,
    Hash_key_type_ptr
} Hash_key_type;

typedef struct _hashbucket {
    struct _hashbucket *next;
    void *key;
    void *value;
} HashBucket;

typedef struct _hash {
    HashBucket *bs;             /* store of buckets */
    HashBucket **bi;            /* list of Bucket pointers */
    HashBucket *free_list;      /* empty buckets */
    UINTVAL entries;            /* Number of values stored in hashtable */
    UINTVAL mask;               /* alloced - 1 */
    PMC *container;             /* e.g. the PerlHash PMC */
    Hash_key_type key_type;     /* cstring, ascii-string, utf8-string */
    PARROT_DATA_TYPE entry_type;/* type of value */
    size_t seed;                /* randomizes the hash_key generation
                                   updated for each new hash */
    hash_comp_fn   compare;     /* compare two keys, 0 = equal */
    hash_hash_key_fn hash_val;  /* generate a hash value for key */
} Hash;

/* HEADERIZER BEGIN: src/hash.c */

PARROT_API
void parrot_dump_hash(SHIM_INTERP, ARGIN(const Hash *hash))
        __attribute__nonnull__(2);

PARROT_API
void parrot_hash_clone(PARROT_INTERP,
    ARGIN(const Hash *hash),
    ARGOUT(Hash *dest))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*dest);

PARROT_API
void parrot_hash_delete(PARROT_INTERP, ARGMOD(Hash *hash), ARGIN(void *key))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*hash);

PARROT_API
void parrot_hash_destroy(SHIM_INTERP, ARGMOD(Hash *hash))
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*hash);

PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL parrot_hash_exists(PARROT_INTERP,
    ARGIN(Hash *hash),
    ARGIN(void *key))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
void * parrot_hash_get(PARROT_INTERP, ARGIN(Hash *hash), ARGIN(void *key))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
HashBucket * parrot_hash_get_bucket(PARROT_INTERP,
    ARGIN(const Hash *hash),
    ARGIN(void *key))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
void * parrot_hash_get_idx(SHIM_INTERP,
    ARGIN(const Hash *hash),
    ARGMOD(PMC *key))
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*key);

PARROT_API
PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
HashBucket* parrot_hash_put(PARROT_INTERP,
    ARGMOD(Hash *hash),
    ARGIN(void *key),
    ARGIN_NULLOK(void *value))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*hash);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_PURE_FUNCTION
INTVAL parrot_hash_size(PARROT_INTERP, ARGIN(const Hash *hash))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void parrot_hash_visit(PARROT_INTERP,
    ARGMOD(Hash *hash),
    ARGMOD(void *pinfo))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*hash)
        FUNC_MODIFIES(*pinfo);

PARROT_API
void parrot_mark_hash(PARROT_INTERP, ARGIN(Hash *hash))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void parrot_new_cstring_hash(SHIM_INTERP, ARGOUT(Hash **hptr))
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*hptr);

PARROT_API
void parrot_new_hash(SHIM_INTERP, ARGOUT(Hash **hptr))
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*hptr);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC* Parrot_new_INTVAL_hash(PARROT_INTERP, UINTVAL flags)
        __attribute__nonnull__(1);

PARROT_API
void parrot_new_pmc_hash(SHIM_INTERP, ARGOUT(PMC *container))
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*container);

PARROT_API
void parrot_new_pointer_hash(SHIM_INTERP, ARGOUT(Hash **hptr))
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*hptr);

PARROT_WARN_UNUSED_RESULT
PARROT_PURE_FUNCTION
int int_compare(SHIM_INTERP,
    ARGIN_NULLOK(const void *a),
    ARGIN_NULLOK(const void *b));

PARROT_WARN_UNUSED_RESULT
PARROT_PURE_FUNCTION
size_t key_hash_int(SHIM_INTERP, ARGIN(void *value), size_t seed)
        __attribute__nonnull__(2);

void parrot_chash_destroy(PARROT_INTERP, ARGMOD(Hash *hash))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*hash);

void parrot_new_hash_x(
    ARGOUT(Hash **hptr),
    PARROT_DATA_TYPE val_type,
    Hash_key_type hkey_type,
    ARGIN(hash_comp_fn compare),
    ARGIN(hash_hash_key_fn keyhash))
        __attribute__nonnull__(1)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*hptr);

void parrot_new_pmc_hash_x(
    ARGMOD(PMC *container),
    PARROT_DATA_TYPE val_type,
    Hash_key_type hkey_type,
    ARGIN(hash_comp_fn compare),
    ARGIN(hash_hash_key_fn keyhash))
        __attribute__nonnull__(1)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*container);

/* HEADERIZER END: src/hash.c */

#endif /* PARROT_HASH_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
