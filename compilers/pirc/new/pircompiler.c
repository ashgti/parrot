/*
 * $Id$
 * Copyright (C) 2007-2008, The Perl Foundation.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "pircompiler.h"
#include "parrot/parrot.h"
#include "piryy.h"

/* XXX count memory, so we can check out mem. savings of string reuse */
static int totalmem = 0;



/*

=over 4

=item C<static allocated_mem_ptrs *
new_mem_ptrs_block(void)>

Create a new C<allocated_mem_ptrs> block; all pointers to allocated memory
within pirc are stored in such blocks. One block has space for a number of
pointers; if the block is full, a new block must be created and linked into
the list of blocks.

=cut

*/
PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static allocated_mem_ptrs *
new_mem_ptrs_block(void) {
    return mem_allocate_zeroed_typed(allocated_mem_ptrs);
}

/*

=item C<static void
register_ptr(lexer_state *lexer, void *ptr)>

Store the pointer C<ptr> in a datastructure; whenever C<release_resources()>
is invoked, C<ptr> will be freed through C<mem_sys_free()>.

=cut

*/
static void
register_ptr(NOTNULL(lexer_state *lexer), ARGIN(void *ptr)) {
    allocated_mem_ptrs *ptrs = lexer->mem_allocations;

    PARROT_ASSERT(ptrs);

    if (ptrs->allocs_in_this_block == NUM_MEM_ALLOCS_PER_BLOCK) {
        allocated_mem_ptrs *newblock = new_mem_ptrs_block();
        newblock->next               = ptrs;
        lexer->mem_allocations       = newblock;
        ptrs                         = newblock;
    }

    /* store the pointer in the current block */
    ptrs->ptrs[ptrs->allocs_in_this_block++] = ptr;
}

/*

=item C<void *
pir_mem_allocate_zeroed(lexer_state * const lexer, size_t numbytes)>

Memory allocation function for all PIR internal functions. Memory is allocated
through Parrot's allocation functions, but the pointer to the allocated memory
is stored in a data structure; this way, freeing all memory can be done by just
iterating over these pointers and freeing them.

=cut

*/
PARROT_MALLOC
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
void *
pir_mem_allocate_zeroed(NOTNULL(lexer_state * const lexer), size_t numbytes) {
    void *ptr = mem_sys_allocate_zeroed(numbytes);

    totalmem += numbytes;

    register_ptr(lexer, ptr);
    return ptr;
}

/*

=item C<void
init_hashtable(lexer_state * const lexer, hashtable * const table, unsigned size)>

Initialize the hashtable C<table> with space for C<size> buckets.

=cut

*/
void
init_hashtable(NOTNULL(lexer_state * const lexer), NOTNULL(hashtable * const table),
               unsigned size)
{
    table->contents  = (bucket **)pir_mem_allocate_zeroed(lexer, size * sizeof (bucket *));
    table->size      = size;
    table->obj_count = 0;
}

/*

=item C<lexer_state *
new_lexer(char * const filename)>

Constructor for a lexer structure. Initializes all fields, creates
a Parrot interpreter structure.

=cut

*/
PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
lexer_state *
new_lexer(NULLOK(char * const filename), int flags) {
    lexer_state *lexer     = mem_allocate_zeroed_typed(lexer_state);
    lexer->filename        = filename;
    lexer->interp          = Parrot_new(NULL);
    lexer->flags           = flags;
    lexer->mem_allocations = new_mem_ptrs_block();

    if (!lexer->interp)
        panic(lexer, "Failed to create a Parrot interpreter structure.");

    /* create a hashtable to store all strings */
    init_hashtable(lexer, &lexer->strings, HASHTABLE_SIZE_INIT);
    /* create a hashtable for storing global labels */
    init_hashtable(lexer, &lexer->globals, HASHTABLE_SIZE_INIT);
    /* create a hashtable for storing .const declarations */
    init_hashtable(lexer, &lexer->constants, HASHTABLE_SIZE_INIT);



    return lexer;
}



/*

=item C<static bucket *
new_bucket(void)>

Constructor for a bucket object.

=cut

*/
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
bucket *
new_bucket(NOTNULL(lexer_state * const lexer)) {
    return pir_mem_allocate_zeroed_typed(lexer, bucket);
}

/*

=item C<static void
store_string(lexer_state * const lexer, char * const str)>

Store the string C<str> in a hashtable; whenever this string is needed, a pointer
to the same physical string is returned, preventing allocating different buffers
for the same string. This is especially useful for ops, as most ops in a typical
program will be used many times.

=cut

*/
static void
store_string(NOTNULL(lexer_state * const lexer), NOTNULL(char const * const str)) {
    hashtable    *table = &lexer->strings;
    unsigned long hash  = get_hashcode(str, table->size);
    bucket *b           = new_bucket(lexer);
    bucket_string(b)    = str;
    store_bucket(table, b, hash);
}

/*

=item C<static char const *
find_string(lexer_state * const lexer, char * const str)>

Find the string C<str> in the lexer's string hashtable. If the string was found,
then a pointer to that buffer is returned. So, whenever for instance the string
"print" is used, the string will only be stored in memory once, and a pointer to
that buffer will be returned.

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static char const *
find_string(NOTNULL(lexer_state * const lexer), NOTNULL(char const * const str)) {
    hashtable    *table = &lexer->strings;
    unsigned long hash  = get_hashcode(str, table->size);
    bucket *b           = get_bucket(table, hash);

    while (b) {
        /* loop through the buckets to see if this is the string */
        if (STREQ(bucket_string(b), str))
            return bucket_string(b); /* if so, return a pointer to the actual string. */

        b = b->next;
    }

    return NULL;
}

/*

=item C<char *
dupstrn(lexer_state * const lexer, char const * const source, size_t slen)>

See dupstr, except that this version takes the number of characters to be
copied. Easy for copying a string except the quotes, for instance.

XXX maybe make this a runtime (commandline) option? Might be slightly slower.

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
char const *
dupstrn(NOTNULL(lexer_state * const lexer), NOTNULL(char * const source), size_t slen) {
    char const * result = find_string(lexer, source);
    /* make sure the string is terminated in time */
    source[slen] = '\0';

    if (result == NULL) { /* not found */
        char * newbuffer = (char *)pir_mem_allocate_zeroed(lexer, slen + 1 * sizeof (char));
        /* only copy num_chars characters */
        strncpy(newbuffer, source, slen);
        /* cache the string */
        store_string(lexer, newbuffer);

        result = newbuffer;
    }

    return result;
}

/*

=item C<char *
dupstr(lexer_state * const lexer, char const * const source)>

The C89 standard does not define a strdup() in the C library,
so define our own strdup. Function names beginning with "str"
are reserved, so make it dupstr, as that is what it
does: duplicate a string.

=cut

*/
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
char const *
dupstr(NOTNULL(lexer_state * const lexer), NOTNULL(char * const source)) {
    return dupstrn(lexer, source, strlen(source));
}

/*

=item C<void
release_resources(lexer_state *lexer)>

Release all resources pointed to by C<lexer>.
Free all memory that was allocated through C<pir_mem_allocate_zeroed()>.
Call the destructor on the Parrot Interpreter, and free C<lexer> itself.

=cut

*/
void
release_resources(NOTNULL(lexer_state *lexer)) {
    allocated_mem_ptrs *iter;

    if (TEST_FLAG(lexer->flags, LEXER_FLAG_VERBOSE))
        fprintf(stderr, "Total nr of bytes allocated: %d\n", totalmem);

    Parrot_destroy(lexer->interp);

    iter = lexer->mem_allocations;

    while (iter) {
        allocated_mem_ptrs *temp = iter;
        unsigned i;

        /* free all pointers in the ppointer block */
        for (i = 0; i < iter->allocs_in_this_block; i++)
            mem_sys_free(iter->ptrs[i]);

        iter = iter->next;
        /* free the current pointer block itself */
        mem_sys_free(temp);
    }

    /* finally, free the lexer itself */
    mem_sys_free(lexer);

    lexer = NULL;
}






/*

=back

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */

