#if !defined(PARROT_SMALLOBJECT_H_GUARD)
#define PARROT_SMALLOBJECT_H_GUARD

#include "parrot/parrot.h"

struct Small_Object_Arena {
    size_t used;
    size_t total_objects;
    struct Small_Object_Arena *prev;
    struct Small_Object_Arena *next;
    void *start_objects;
};

/* Tracked resource pool */
struct Small_Object_Pool {
    struct Small_Object_Arena *last_Arena;
    size_t object_size;     /* size in bytes of an individual pool item */
    size_t objects_per_alloc; 
    size_t total_objects;
    size_t num_free_objects;    /* number of resources in the free pool */
	size_t replenish_level;
    void *free_list;
    UINTVAL align_1;    /* alignment (must be power of 2) minus one */
    /* adds a free object to the pool's free list  */
    void  (*add_free_object)(struct Parrot_Interp *, struct Small_Object_Pool *, void *);
    /* gets and removes a free object from the pool's free list */
    void *(*get_free_object)(struct Parrot_Interp *, struct Small_Object_Pool *);
    /* frees up objects. can perform a DOD. Can optionally be set to NULL */
    void  (*free_objects)   (struct Parrot_Interp *, struct Small_Object_Pool *);
    /* allocates more objects */
    void  (*more_objects)   (struct Parrot_Interp *, struct Small_Object_Pool *);
    void *mem_pool;
    size_t start_arena_memory;
    size_t end_arena_memory;
};

INTVAL contained_in_pool(struct Parrot_Interp *,struct Small_Object_Pool *, void *);
size_t get_max_pool_address(struct Parrot_Interp *interpreter, struct Small_Object_Pool *pool);
size_t get_min_pool_address(struct Parrot_Interp *interpreter, struct Small_Object_Pool *pool);

void add_free_object(struct Parrot_Interp *, struct Small_Object_Pool *, void *);
void *get_free_object(struct Parrot_Interp *, struct Small_Object_Pool *);
void alloc_more_objects(struct Parrot_Interp *,struct Small_Object_Pool *);
struct Small_Object_Pool * new_small_object_pool(struct Parrot_Interp *, size_t, size_t);

struct Small_Object_Pool * get_sized_small_object_pool(struct Parrot_Interp *, size_t);
void set_sized_small_object_pool(struct Parrot_Interp *, size_t, struct Small_Object_Pool *);

#endif /* PARROT_SMALLOBJECT_H */

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil 
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
