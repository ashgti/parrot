/* io.h
 *  Copyright (C) 2001-2003, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *      Parrot IO subsystem
 *  Data Structure and Algorithms:
 *  History:
 *      Originally written by Melvin Smith
 *      Refactored by Juergen Boemmels
 *      2003-08-18: Internal structures moved to io/io_private.h
 *  Notes:
 *  References:
 *      Perl6 RFCs (14,30,47,60,186,239,321,345,350)
 *      Some ideas and goals from Perl5.7 and Nick Ing-Simmons' work
 *      Some ideas from AT&T SFIO
 */

#if !defined(PARROT_IO_H_GUARD)
#define PARROT_IO_H_GUARD

#include <stdio.h>

/* which OS are we on? (this should be moved into Configure-land) */
#ifdef MINIPARROT
#  define PIO_OS_STDIO
#else
#  ifdef _WIN32
#    define PIO_OS_WIN32
#  else
#    ifdef PARROT_HAS_HEADER_UNISTD
#      define PIO_OS_UNIX
#    else
#      define PIO_OS_STDIO
#    endif
#  endif
#endif

#ifdef PIO_OS_WIN32
#  include <io.h>
#endif

#ifndef STDIN_FILENO
#  define STDIN_FILENO 0
#endif

#ifndef STDOUT_FILENO
#  define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
#  define STDERR_FILENO 2
#endif

/* &gen_from_def(stdio.pasm) */

#define PIO_STDIN_FILENO 0
#define PIO_STDOUT_FILENO 1
#define PIO_STDERR_FILENO 2

/* &end_gen */

#ifndef O_ACCMODE
#  define O_ACCMODE 0003
#endif

#ifdef BLKSIZE
#  define PIO_BLKSIZE BLKSIZE
#else
#  define PIO_BLKSIZE  8192
#endif
#define PIO_LINEBUFSIZE 256     /* Default linebuffer size */
#define PIO_GRAIN 2048          /* Smallest size for a block buffer */
#define PIO_BUFSIZE (PIO_GRAIN * 2)

#define PIO_NR_OPEN 256         /* Size of an "IO handle table" */

/* &gen_from_enum(iotypes.pasm) */
enum {
    PIO_TYPE_FILE,
    PIO_TYPE_PIPE,
    PIO_TYPE_SOCKET,
    PIO_TYPE_MAX
};
/* &end_gen */

/* This is temporary until subs/code refs are done..*/
typedef void *DummyCodeRef;


#ifdef PIO_OS_WIN32
typedef Parrot_WIN32_HANDLE PIOHANDLE;
typedef Parrot_OFF_T PIOOFF_T;
#endif
#ifdef PIO_OS_UNIX
/* Hopefully INTVAL_SIZE is enough for PTR_SIZE so that
 * the FILE* of pio_stdio_layers fit into a PIOHANDLE. */
typedef INTVAL PIOHANDLE;
typedef off_t PIOOFF_T;
#endif
#ifdef PIO_OS_STDIO
typedef FILE* PIOHANDLE;
typedef long PIOOFF_T;
#endif

extern PIOOFF_T piooffsetzero;

typedef struct _ParrotIOLayerAPI ParrotIOLayerAPI;
typedef struct _ParrotIOLayer ParrotIOLayer;
typedef struct _ParrotIOFilter ParrotIOFilter;
typedef struct _ParrotIO ParrotIO;
typedef struct _ParrotIOData ParrotIOData;

struct _ParrotIOLayer {
    void *self;                 /* Instance specific data       */
    const char *name;
    INTVAL flags;
    const ParrotIOLayerAPI *api;
    ParrotIOLayer *up;
    ParrotIOLayer *down;
};

#define PIO_DOWNLAYER(x)   x->down
#define PIO_UPLAYER(x)     x->up


/* Others to come */
#ifdef PIO_OS_UNIX
extern ParrotIOLayer pio_unix_layer;
#endif
#ifdef PIO_OS_WIN32
extern ParrotIOLayer pio_win32_layer;
#endif
#ifdef PIO_OS_STDIO
extern ParrotIOLayer pio_stdio_layer;
#endif
extern ParrotIOLayer pio_buf_layer;
extern ParrotIOLayer pio_mmap_layer;
extern ParrotIOLayer pio_string_layer;




extern INTVAL pio_errno;

/* io.c - If you add new layers, register them in init_layers() */
PARROT_API extern void PIO_init(Interp *);
PARROT_API extern void PIO_finish(Interp *);
PARROT_API void PIO_internal_shutdown(Interp *);
PARROT_API extern INTVAL PIO_init_stacks(Interp *);
PARROT_API extern void PIO_atexit(Interp *);
PARROT_API extern INTVAL PIO_push_layer(Interp *, PMC *, ParrotIOLayer *);
PARROT_API extern ParrotIOLayer *PIO_pop_layer(Interp *, PMC *);
PARROT_API extern ParrotIOLayer *PIO_copy_stack(ParrotIOLayer *);
PARROT_API ParrotIOLayer *PIO_get_layer(Interp *, const char *name);


PARROT_API extern struct PMC *new_io_pmc(Interp *, ParrotIO *);
PARROT_API extern void free_io_header(ParrotIO *);
PARROT_API extern ParrotIO *PIO_new(Interp *, INTVAL, INTVAL, INTVAL);
PARROT_API extern void PIO_destroy(Interp *, PMC *io);

PARROT_API extern INTVAL PIO_base_init(Interp *, ParrotIOLayer *proto);
PARROT_API extern ParrotIOLayer *PIO_base_new_layer(ParrotIOLayer *proto);
PARROT_API extern void PIO_base_delete_layer(ParrotIOLayer *proto);

PARROT_API extern INTVAL PIO_parse_open_flags(const char *flagstr);
PARROT_API extern PMC *PIO_open(Interp *, ParrotIOLayer *, const char *, const char *);
PARROT_API extern PMC *PIO_fdopen(Interp *, ParrotIOLayer *, PIOHANDLE, const char *);
PARROT_API extern INTVAL PIO_close(Interp *, PMC *);
PARROT_API extern void PIO_flush(Interp *, PMC *);
PARROT_API extern STRING *PIO_reads(Interp *, PMC *, size_t);
PARROT_API extern INTVAL PIO_read(Interp *, PMC *, void *, size_t);
PARROT_API extern INTVAL PIO_write(Interp *, PMC *, const void *, size_t);
PARROT_API extern INTVAL PIO_setbuf(Interp *, PMC *, size_t);
PARROT_API extern INTVAL PIO_setlinebuf(Interp *, PMC *);
PARROT_API extern INTVAL PIO_puts(Interp *, PMC *, const char *);
PARROT_API extern INTVAL PIO_peek(Interp *, PMC *, STRING **);
PARROT_API extern PIOOFF_T PIO_seek(Interp *, PMC *, PIOOFF_T offset, INTVAL whence);
PARROT_API extern INTVAL PIO_eof(Interp *, PMC *);
PARROT_API extern INTVAL PIO_pioctl(Interp *, PMC *, INTVAL cmd, INTVAL arg);
PARROT_API extern INTVAL PIO_poll(Interp *, PMC *pmc, INTVAL which, INTVAL sec, INTVAL usec);
PARROT_API extern PMC *PIO_socket(Interp *, INTVAL fam, INTVAL type, INTVAL proto);
PARROT_API extern INTVAL PIO_recv(Interp *, PMC *pmc, STRING **buf);
PARROT_API extern INTVAL PIO_send(Interp *, PMC *pmc, STRING *buf);
PARROT_API extern INTVAL PIO_connect(Interp *, PMC *pmc, STRING *address);
PARROT_API extern INTVAL PIO_bind(Interp *, PMC *pmc, STRING *address);
PARROT_API extern INTVAL PIO_listen(Interp *, PMC *pmc, INTVAL backlog);
PARROT_API extern PMC *PIO_accept(Interp *, PMC *pmc);


PARROT_API extern INTVAL PIO_putps(Interp *, PMC *io, STRING *s);
PARROT_API extern INTVAL PIO_fprintf(Interp *, PMC *io, const char *s, ...);
PARROT_API extern INTVAL PIO_printf(Interp *, const char *s, ...);
PARROT_API extern INTVAL PIO_eprintf(Interp *, const char *s, ...);
PARROT_API extern PIOHANDLE PIO_getfd(Interp *, PMC *io);
PARROT_API extern PIOOFF_T PIO_tell(Interp *, PMC *io);

PARROT_API extern void Parrot_IOData_mark(Interp *, ParrotIOData *piodata);

PARROT_API extern INTVAL PIO_isatty(Interp *, PMC *io);

/* Put platform specific macros here if you must */
#ifdef PIO_OS_WIN32
extern STRING          *PIO_sockaddr_in(Interp *, unsigned short, STRING *);
extern INTVAL           PIO_win32_getblksize(PIOHANDLE fd);
#  define PIO_getblksize(x)   PIO_win32_getblksize(x)
#endif

#ifdef PIO_OS_UNIX
extern STRING          *PIO_sockaddr_in(Interp *, unsigned short, STRING *);
extern INTVAL           PIO_unix_getblksize(PIOHANDLE fd);
#  define PIO_getblksize(x)   PIO_unix_getblksize(x)
#endif

#ifdef PIO_OS_STDIO
extern INTVAL           PIO_stdio_getblksize(PIOHANDLE fd);
#  define PIO_sockaddr_in(i,p,a)
#  define PIO_getblksize(x)   PIO_stdio_getblksize(x)
#endif

PIOOFF_T PIO_make_offset(INTVAL offset);
PIOOFF_T PIO_make_offset32(INTVAL hi, INTVAL lo);
PIOOFF_T PIO_make_offset_pmc(Interp *, PMC *pmc);

/* the internal system redefines them as macros */
extern PMC *PIO_STDIN(Interp *);
extern PMC *PIO_STDOUT(Interp *);
extern PMC *PIO_STDERR(Interp *);

/*
 * pioctl definitions -- These are mostly for reference
 * or C-API writers.
 */
/* pioctl command constants */
#define PIOCTL_CMDRESERVED         0
#define PIOCTL_CMDSETRECSEP        1
#define PIOCTL_CMDGETRECSEP        2
#define PIOCTL_CMDSETBUFTYPE       3
#define PIOCTL_CMDGETBUFTYPE       4
#define PIOCTL_CMDSETBUFSIZE       5
#define PIOCTL_CMDGETBUFSIZE       6

/*
 * pioctl argument constants. These don't have to
 * be unique across io commands.
 */
#define PIOCTL_NONBUF              0
#define PIOCTL_LINEBUF             1
#define PIOCTL_BLKBUF              2



#endif /* PARROT_IO_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
