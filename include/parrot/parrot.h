/* parrot.h
 *  Copyright: (When this is determined...it will go here)
 *  CVS Info
 *     $Id$
 *  Overview:
 *     General header file includes for the parrot interpreter
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

/* Only parrot core files should include this file.  
   Extensions should include <parrot/extend.h>.
   Programs embedding parrot should include <parrot/embed.h>.
*/

#if !defined(PARROT_PARROT_H_GUARD)
#define PARROT_PARROT_H_GUARD

#if defined(INSIDE_GLOBAL_SETUP)
#  define VAR_SCOPE
#else
#  define VAR_SCOPE extern
#endif

#define PARROT_IN_CORE

#include "parrot/config.h"

/* ANSI C89 headers */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

/* Other headers, where available */

#ifdef HAS_HEADER_UNISTD
#  include <unistd.h>
#endif

#ifdef HAS_HEADER_SYSMMAN
#  include <sys/mman.h>
#endif

#ifdef HAS_HEADER_SYSTIME
#  include <sys/time.h>
#endif

#ifdef HAS_HEADER_MATH
#  include <math.h>
#endif

#ifdef HAS_HEADER_FCNTL
#  include <fcntl.h>
#endif

#ifdef HAS_HEADER_SYSTYPES
#  include <sys/types.h>
#endif

#ifdef HAS_HEADER_SYSSTAT
#  include <sys/stat.h>
#endif


#define NUM_REGISTERS 32
#define PARROT_MAGIC 0x13155a1

#define OPCODE_TYPE_PERL 0x5045524c
#define OPCODE_TYPE_PYTHON 0x5045524b
#define OPCODE_TYPE_JAVA 4871757
#define OPCODE_TYPE_MSNET 2e4e4554

typedef struct PMC PMC;
typedef void STRING_FUNCS;
typedef void BIGNUM;

/*  Casting between pointers and integers:  If pointers and integers
    are the same size, then direct casting is fine.  If pointers and
    integers are not the same size, then the compiler might complain.
    Also, there's a possible loss of information in going from (for
    example) a 64-bit integer to a 32-bit pointer.

    These casts silence the warnings but do no limits checks.
    Perhaps a different set should be defined (and only compiled if
    explicitly Configured in) which do limits checks?
    A. D. Aug. 6, 2002.
*/
#if (INTVAL_SIZE == PTR_SIZE) && (UINTVAL_SIZE == PTR_SIZE)
#  define INTVAL2PTR(any,d)	(any)(d)
#  define UINTVAL2PTR(any,d)	(any)(d)
#else
#  if PTR_SIZE == LONG_SIZE
#    define INTVAL2PTR(any,d)	(any)(unsigned long)(d)
#    define UINTVAL2PTR(any,d)	(any)(unsigned long)(d)
#  else
#    define INTVAL2PTR(any,d)	(any)(unsigned int)(d)
#    define UINTVAL2PTR(any,d)	(any)(unsigned int)(d)
#  endif
#endif
#define PTR2INTVAL(p)	INTVAL2PTR(INTVAL,p)
#define PTR2UINTVAL(p)	UINTVAL2PTR(UINTVAL,p)

/* Use similar macros for casting between pointers and opcode_t.
   (We can't assume that sizeof(opcode_t) == sizeof(intval).
*/
#if (OPCODE_T_SIZE == PTR_SIZE)
#  define OPCODE_T2PTR(any,d)	(any)(d)
#else
#  if PTR_SIZE == LONG_SIZE
#    define OPCODE_T2PTR(any,d)	(any)(unsigned long)(d)
#  else
#    define OPCODE_T2PTR(any,d)	(any)(unsigned int)(d)
#  endif
#endif
#define PTR2OPCODE_T(p)	OPCODE_T2PTR(opcode_t,p)

/* define some shortcuts for dealing with function pointers */
/* according to ANSI C, casting between function and non-function pointers is
 * no good.  So we should use "funcptr_t" in place of void* when dealing with
 * function pointers and NULLfunc in place of NULL */
typedef void (*funcptr_t)(void);
#define NULLfunc (funcptr_t)0

/* define macros for converting between data and function pointers.  As it
 * turns out, ANSI C does appear to permit you to do this conversion if you
 * convert the value to an integer (well, a value type large enough to hold
 * a pointer) in between.  Believe it or not, this even works on TenDRA (tcc).
 * 
 * NOTE!  UINTVAL is incorrect below.  It should be UINTPTR or something like
 * that. The equivalent of C99's uintptr_t- a non-pointer data type that can 
 * hold a pointer.
 */
#define D2FPTR(x) UINTVAL2PTR(funcptr_t, PTR2UINTVAL(x))
#define F2DPTR(x) UINTVAL2PTR(void *, PTR2UINTVAL((funcptr_t) x))

/* On Win32 we need the constant O_BINARY for open() (at least for Borland C), 
   but on UNIX it doesn't exist, so set it to 0 if it's not defined
 */
#ifndef O_BINARY
#  define O_BINARY 0
#endif

/* define a macro to acknowledge an unused argument, and silence a "helpful"
   compiler warning. gcc will emit a warning on an empty if body unless {} is
   used to make an empty block.  */
#define UNUSED(a) if (a) {}

#include "parrot/platform.h"
#include "parrot/global_setup.h"
#include "parrot/interpreter.h"
#include "parrot/encoding.h"
#include "parrot/chartype.h"
#include "parrot/string.h"
#include "parrot/key.h"
#include "parrot/hash.h"
#include "parrot/vtable.h"
#include "parrot/register.h"
#include "parrot/regfuncs.h"
#include "parrot/exceptions.h"
#include "parrot/warnings.h"
#include "parrot/memory.h"
#include "parrot/packfile.h"
#include "parrot/io.h"
#include "parrot/op.h"
#include "parrot/pmc.h"
#include "parrot/events.h"
#include "parrot/stacks.h"
#include "parrot/smallobject.h"
#include "parrot/headers.h"
#include "parrot/dod.h"
#include "parrot/resources.h"
#include "parrot/string_funcs.h"
#include "parrot/misc.h"
#include "parrot/debug.h"
#include "parrot/sub.h"
#endif

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
