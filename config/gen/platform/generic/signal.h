#ifndef PARROT_PLATFORM_GENERIC_SIGNAL_H_GUARD
#define PARROT_PLATFORM_GENERIC_SIGNAL_H_GUARD
/*
 * Signal handling stuff
 */

#if defined(PARROT_HAS_HEADER_SIGNAL) && defined(PARROT_HAS_HEADER_SYSTYPES)
#  include <signal.h>
#  include <sys/types.h>
#  define dumpcore() raise(SIGQUIT)
#endif

#ifdef PARROT_HAS_HEADER_SIGNAL
#  undef Parrot_set_sighandler
#  ifdef PARROT_HAS___SIGHANDLER_T
    typedef __sighandler_t Parrot_sighandler_t;
#  else
    typedef void (*Parrot_sighandler_t) (int);
#  endif

    Parrot_sighandler_t Parrot_set_sighandler(int s, Parrot_sighandler_t f);
#endif

#endif /* PARROT_PLATFORM_GENERIC_SIGNAL_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
