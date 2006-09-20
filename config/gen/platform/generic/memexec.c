/*
 * Memory protection stuff
 */

#ifdef PARROT_HAS_EXEC_PROTECT
/*
 * Allocate executable memory
 * Round up to page size because the whole page will be marked as
 *   executable
 */
void *
mem_alloc_executable(size_t size)
{
    void *p;
    size_t pagesize = sysconf(_SC_PAGESIZE);
    size = (size + pagesize - 1) & ~(pagesize-1);
    if (posix_memalign(&p, pagesize, size))
        return NULL;
    mprotect(p, size, PROT_READ|PROT_WRITE|PROT_EXEC);
    return p;
}

void
mem_free_executable(void *p)
{
    free(p);
}

/*
 * Reallocate executable memory
 * Round up to page size because the whole page will be marked as
 *   executable
 * The intermediate temp is required because we don't know the old size
 */
void *
mem_realloc_executable(void* oldp, size_t newsize)
{
    void *temp;
    void *newp;
    size_t pagesize = sysconf(_SC_PAGESIZE);
    size_t roundup;
    temp = realloc(oldp, newsize);
    if (temp == NULL)
        return NULL;
    free(oldp);
    roundup = (newsize + pagesize - 1) & ~(pagesize-1);
    if (posix_memalign(&newp, pagesize, roundup))
        newp = NULL;
    if (newp) {
        mprotect(newp, roundup, PROT_READ|PROT_WRITE|PROT_EXEC);
        memcpy(newp, temp, newsize);
    }
    free(temp);
    return newp;
}
#endif

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
