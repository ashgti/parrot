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

/*
 * GC_DEFAULT_TYPE selection
 * MS  -- stop-the-world mark & sweep
 * INF -- infinite memory "collector"
 */
#define PARROT_GC_DEFAULT_TYPE MS


/* Set to 1 if we want to use the fixed-size allocator. Set to 0 if we want
   to allocate these things using mem_sys_allocate instead.
   When PARROT_GC_DEFAULT_TYPE set to BOEHM_GC it's better to set it to 0.
   Otherwise Boehm will not collect memory from FixedSizeAllocator which leads
   to "memory leaks" */
#define PARROT_GC_USE_FIXED_SIZE_ALLOCATOR 1

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
