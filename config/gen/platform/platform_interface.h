#ifndef PLATFORM_INTERFACE_H_GUARD
#define PLATFORM_INTERFACE_H_GUARD
/*
** platform_interface.h
*/
#include "feature.h"

/*
** I/O:
*/

/*
** Memory:
*/

void *Parrot_memalign(size_t align, size_t size);
void *Parrot_memalign_if_possible(size_t align, size_t size);
void Parrot_free_memalign(void *);

#if !defined(HAS_MEMALIGN) && !defined(HAS_POSIX_MEMALIGN)
#  define Parrot_memalign_if_possible(a, s) malloc(a)
#endif

/*
** Miscellaneous:
*/

void Parrot_sleep(unsigned int seconds);
INTVAL Parrot_intval_time(void);
FLOATVAL Parrot_floatval_time(void);
void Parrot_setenv(const char *name, const char *value);

/*
** Dynamic Loading:
*/

void *Parrot_dlopen(const char *filename);
const char *Parrot_dlerror(void);
void *Parrot_dlsym(void *handle, const char *symbol);
int Parrot_dlclose(void *handle);

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
#endif
