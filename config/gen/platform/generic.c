/*
** platform.c [generic version]
*/

#include <time.h>
#include <sys/time.h>

#include "parrot/parrot.h"

#ifdef HAS_HEADER_DLFCN
#  include <dlfcn.h>
#endif

#define PARROT_DLOPEN_FLAGS RTLD_LAZY


/*
** Parrot_intval_time()
*/

INTVAL
Parrot_intval_time(void)
{
    return time(NULL);
}


/*
** Parrot_floatval_time()
*/

FLOATVAL
Parrot_floatval_time(void)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (FLOATVAL)t.tv_sec + ((FLOATVAL)t.tv_usec / 1000000.0);
}


/*
** Parrot_sleep()
*/

void
Parrot_sleep(unsigned int seconds)
{
    sleep(seconds);
}


/*
** Parrot_setenv()
*/

#ifdef HAS_SETENV
void
Parrot_setenv(const char *name, const char *value)
{
    setenv(name, value, 1);
}
#else
/* putenv-based version might go here, but see perl5's util.c for
   warnings and workarounds.
*/
#endif


/*
** Parrot_dlopen()
*/

void *
Parrot_dlopen(const char *filename)
{
#ifdef HAS_HEADER_DLFCN
    return dlopen(filename, PARROT_DLOPEN_FLAGS);
#else
    return 0;
#endif
}


/*
** Parrot_dlerror()
*/

const char *
Parrot_dlerror(void)
{
#ifdef HAS_HEADER_DLFCN
    return dlerror();
#else
    return 0;
#endif
}


/*
** Parrot_dlsym()
*/

void *
Parrot_dlsym(void *handle, const char *symbol)
{
#ifdef HAS_HEADER_DLFCN
    return dlsym(handle, symbol);
#else
    return 0;
#endif
}


/*
** Parrot_dlclose()
*/

int
Parrot_dlclose(void *handle)
{
#ifdef HAS_HEADER_DLFCN
    return dlclose(handle);
#else
    return -1;
#endif
}

/*
 * memalign related stuff
 */

#if defined(HAS_POSIX_MEMALIGN)
#include <stdlib.h>

void *
Parrot_memalign(size_t align, size_t size)
{
    void *p;
    int i = posix_memalign(&p, align, size);
    return i == 0 ? p : NULL;
}

void *
Parrot_memalign_if_possible(size_t align, size_t size)
{
    void *p;
    int i = posix_memalign(&p, align, size);
    return i == 0 ? p : NULL;
}

#elif defined(HAS_MEMALIGN)
#include <malloc.h>

void *
Parrot_memalign(size_t align, size_t size)
{
    return memalign(align, size);
}

void *
Parrot_memalign_if_possible(size_t align, size_t size)
{
    return memalign(align, size);
}

#else
#error "generic platform without memalign"
#endif

void
Parrot_free_memalign(void *p)
{
    free(p);
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
