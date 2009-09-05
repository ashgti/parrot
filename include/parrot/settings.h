/* settings.h
 *  Copyright (C) 2001-2004, Parrot Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     Overall settings for Parrot
 *  Data Structure and Algorithms:
 *  History: initial version 2004.08.13
 *  Notes:
 *  References:
 */

#ifndef PARROT_SETTINGS_H_GUARD
#define PARROT_SETTINGS_H_GUARD

/*
 * GC switches
 */

/* If you are trying to debug GC problems which only occur on large test cases,
 * turning on GC_DEBUG should help make the problem appear with smaller data
 * samples by reducing various numbers, and causing GC runs to occur more
 * frequently. It does significantly reduce performance. */
#ifndef DISABLE_GC_DEBUG
#  define DISABLE_GC_DEBUG 0
#endif /* DISABLE_GC_DEBUG */

/* Helpful internal macro for testing whether we are currently
 * debugging garbage collection and memory management. See also the
 * definition of GC_VERBOSE in include/parrot/gc_api.h. */
#if DISABLE_GC_DEBUG
#  define GC_DEBUG(interp) 0
#else
#  define GC_DEBUG(interp) Interp_flags_TEST((interp), PARROT_GC_DEBUG_FLAG)
#endif /* DISABLE_GC_DEBUG */

/*
 * GC_SUBSYSTEM selection
 * 0 ... MS  stop-the-world mark & sweep
 * 1 ... IMS incremental mark & sweep
 * 2 ... GMS generational mark & sweep
 * 3 ... INF infinite memory "collector"
 *
 * Please note that only 0 and 3 currently work (and INF doesn't really
 * "work").
 */
#define PARROT_GC_DEFAULT_TYPE MS
#define PARROT_GC_SUBSYSTEM 0

#if PARROT_GC_SUBSYSTEM == 0
#  define PARROT_GC_MS      1
#  define PARROT_GC_IMS     0
#  define PARROT_GC_GMS     0
#  define PARROT_GC_INF     0
#endif
#if PARROT_GC_SUBSYSTEM == 1
#  define PARROT_GC_MS      0
#  define PARROT_GC_IMS     1
#  define PARROT_GC_GMS     0
#  define PARROT_GC_INF     0
#endif
#if PARROT_GC_SUBSYSTEM == 2
#  define PARROT_GC_MS      0
#  define PARROT_GC_IMS     0
#  define PARROT_GC_GMS     1
#  define PARROT_GC_INF     0
#endif
#if PARROT_GC_SUBSYSTEM == 3
#  define PARROT_GC_MS      0
#  define PARROT_GC_IMS     0
#  define PARROT_GC_GMS     0
#  define PARROT_GC_INF     1
#endif

/*
 * JIT/i386 can use the CGP run core for external functions instead
 * of calling the function version of the opcode
 *
 * This is for some reason currently slower, so it's turned off
 */

#define PARROT_I386_JIT_CGP 0

#endif /* PARROT_SETTINGS_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
