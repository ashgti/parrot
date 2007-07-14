/*
Copyright (C) 2001-2006, The Perl Foundation.
$Id$

=head1 NAME

src/io/io_private.h - IO internals

=head1 DESCRIPTION

Internal Details of the Parrot IO subsystem.

=head2 References

Perl6 RFCs (14,30,47,60,186,239,321,345,350).

Some ideas and goals from Perl5.7 and Nick Ing-Simmons' work.

Some ideas from AT&T SFIO.

=cut

*/

#ifndef PARROT_IO_PRIVATE_H_GUARD
#define PARROT_IO_PRIVATE_H_GUARD

#define PARROT_IN_IO 1
#define PARROT_ASYNC_DEVEL 0
#define PARROT_NET_DEVEL 1

#include <parrot/io.h>

#if PARROT_NET_DEVEL
/* XXX: Parrot config is currently not probing for all headers so
 * I'm sticking here rather than parrot.h
 */
#  ifdef UNIX
#    include <sys/socket.h>
#  endif

#  ifdef WIN32
#    include <winsock.h>
#  endif

#endif /* PARROT_NET_DEVEL */

/* IO object flags */
#define PIO_F_READ      00000001
#define PIO_F_WRITE     00000002
#define PIO_F_APPEND    00000004
#define PIO_F_TRUNC     00000010
#define PIO_F_EOF       00000020
#define PIO_F_FILE      00000100
#define PIO_F_PIPE      00000200
#define PIO_F_SOCKET    00000400
#define PIO_F_CONSOLE   00001000        /* A terminal                   */
#define PIO_F_READLINE  00002000        /* user interactive readline    */
#define PIO_F_LINEBUF   00010000        /* Flushes on newline           */
#define PIO_F_BLKBUF    00020000
#define PIO_F_SOFT_SP   00040000        /* Python softspace */
#define PIO_F_SHARED    00100000        /* Stream shares a file handle  */
#define PIO_F_ASYNC     01000000        /* In Parrot async is default   */

/* Buffer flags */
#define PIO_BF_MALLOC   00000001        /* Buffer malloced              */
#define PIO_BF_READBUF  00000002        /* Buffer is read-buffer        */
#define PIO_BF_WRITEBUF 00000004        /* Buffer is write-buffer       */
#define PIO_BF_MMAP     00000010        /* Buffer mmap()ed              */

/*
 * Layer flags
 *
 * Terminal layer can't be pushed on top of other layers;
 * vice-versa, non-terminal layers be pushed on an empty io stack
 * An OS layer would be a terminal layer, non-terminals might be
 * buffering, translation, compression or encryption layers.
 */
#define PIO_L_TERMINAL          0x0001
#define PIO_L_FASTGETS          0x0002  /* ??? */
#define PIO_L_LAYER_COPIED      0x0004  /* PMC has private layer */


#define PIO_ACCMODE     0000003
#define PIO_DEFAULTMODE DEFAULT_OPEN_MODE
#define PIO_UNBOUND     (size_t)-1

/* This is list of valid layers */
extern ParrotIOLayer **pio_registered_layers;

/* This is the actual (default) layer stack which is used for IO */
/* extern ParrotIOLayer *pio_default_stack; */

typedef struct _ParrotIOBuf ParrotIOBuf;
typedef PMC **ParrotIOTable;

struct _ParrotIOBuf {
    INTVAL flags;               /* Buffer specific flags        */
    size_t size;
    unsigned char *startb;      /* Start of buffer              */
    unsigned char *endb;        /* End of buffer                */
    unsigned char *next;        /* Current read/write pointer   */
};

struct _ParrotIO {
    PIOHANDLE fd;               /* Low level OS descriptor      */
    PIOHANDLE fd2;              /* For pipes we need 2nd handle */
    INTVAL mode;                /* Read/Write/etc.              */
    INTVAL flags;               /* Da flags                     */
    PIOOFF_T fsize;             /* Current file size            */
    PIOOFF_T fpos;              /* Current real file pointer    */
    PIOOFF_T lpos;              /* Last file position           */
    ParrotIOBuf b;              /* Buffer structure             */
    ParrotIOLayer *stack;
    INTVAL recsep;              /* Record Separator             */
#if PARROT_NET_DEVEL
    struct sockaddr_in local;
    struct sockaddr_in remote;
#endif /* PARROT_NET_DEVEL */
    /* ParrotIOFilter * filters; */
};

struct _ParrotIOData {
    ParrotIOTable table;
    ParrotIOLayer *default_stack;
};

/* functions internal to the subsystem */

/* redefine PIO_STD* for internal use */
#define _PIO_STDIN(i)   (((ParrotIOData*)i->piodata)->table[PIO_STDIN_FILENO])
#define _PIO_STDOUT(i)  (((ParrotIOData*)i->piodata)->table[PIO_STDOUT_FILENO])
#define _PIO_STDERR(i)  (((ParrotIOData*)i->piodata)->table[PIO_STDERR_FILENO])

/*
 * These function walk down the layerstack starting at l
 * and calling the first non-null function it finds.
 */

/* HEADERIZER BEGIN: src/io/io_layers.c */
/* HEADERIZER END: src/io/io_layers.c */

/* HEADERIZER BEGIN: src/io/io_passdown.c */

PARROT_WARN_UNUSED_RESULT
ParrotIO * PIO_accept_down( PARROT_INTERP,
    ParrotIOLayer *layer,
    ParrotIO *io )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
INTVAL PIO_bind_down( PARROT_INTERP,
    ParrotIOLayer *layer,
    ParrotIO *io,
    STRING *address )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
INTVAL PIO_close_down( PARROT_INTERP,
    NULLOK(ParrotIOLayer *layer),
    ParrotIO * io )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
INTVAL PIO_connect_down( PARROT_INTERP,
    ParrotIOLayer *layer,
    ParrotIO *io,
    STRING *address )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
INTVAL PIO_eof_down( PARROT_INTERP,
    NULLOK(ParrotIOLayer *layer),
    ParrotIO * io )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
ParrotIO * PIO_fdopen_down( PARROT_INTERP,
    NULLOK(ParrotIOLayer *layer),
    PIOHANDLE fd,
    INTVAL flags )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
INTVAL PIO_flush_down( PARROT_INTERP,
    NULLOK(ParrotIOLayer *layer),
    ParrotIO * io )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
INTVAL PIO_listen_down( PARROT_INTERP,
    ParrotIOLayer *layer,
    ParrotIO *io,
    INTVAL backlog )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
ParrotIO * PIO_open_async_down( PARROT_INTERP,
    NULLOK(ParrotIOLayer *layer),
    NOTNULL(const char *name),
    NOTNULL(const char *mode),
    DummyCodeRef * dummy )
        __attribute__nonnull__(1)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4);

PARROT_WARN_UNUSED_RESULT
ParrotIO * PIO_open_down( PARROT_INTERP,
    NULLOK(ParrotIOLayer *layer),
    NOTNULL(const char *name),
    INTVAL flags )
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

PARROT_WARN_UNUSED_RESULT
size_t PIO_peek_down( PARROT_INTERP,
    NULLOK(ParrotIOLayer *layer),
    ParrotIO * io,
    STRING ** buf )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
INTVAL PIO_poll_down( PARROT_INTERP,
    ParrotIOLayer *layer,
    ParrotIO *io,
    INTVAL which,
    INTVAL sec,
    INTVAL usec )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
size_t PIO_read_async_down( PARROT_INTERP,
    NULLOK(ParrotIOLayer *layer),
    ParrotIO * io,
    STRING ** buf,
    DummyCodeRef *dummy )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
size_t PIO_read_down( PARROT_INTERP,
    NULLOK(ParrotIOLayer *layer),
    ParrotIO * io,
    STRING ** buf )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
INTVAL PIO_recv_down( PARROT_INTERP,
    ParrotIOLayer *layer,
    ParrotIO *io,
    STRING **buf )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PIOOFF_T PIO_seek_down( PARROT_INTERP,
    NULLOK(ParrotIOLayer *layer),
    ParrotIO * io,
    PIOOFF_T offset,
    INTVAL whence )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
INTVAL PIO_send_down( PARROT_INTERP,
    ParrotIOLayer *layer,
    ParrotIO *io,
    STRING *buf )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
INTVAL PIO_setbuf_down( PARROT_INTERP,
    NULLOK(ParrotIOLayer *layer),
    ParrotIO * io,
    size_t bufsize )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
INTVAL PIO_setlinebuf_down( PARROT_INTERP,
    NULLOK(ParrotIOLayer *layer),
    ParrotIO * io )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
ParrotIO * PIO_socket_down( PARROT_INTERP,
    ParrotIOLayer *layer,
    INTVAL fam,
    INTVAL type,
    INTVAL proto )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PIOOFF_T PIO_tell_down( PARROT_INTERP,
    NULLOK(ParrotIOLayer *layer),
    ParrotIO * io )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
size_t PIO_write_async_down( PARROT_INTERP,
    NULLOK(ParrotIOLayer *layer),
    ParrotIO * io,
    STRING *s,
    DummyCodeRef *dummy )
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
size_t PIO_write_down( PARROT_INTERP,
    NULLOK(ParrotIOLayer *layer),
    ParrotIO * io,
    STRING *s )
        __attribute__nonnull__(1);

/* HEADERIZER END: src/io/io_passdown.c */

/*
 * By default, any layer not implementing an interface (ie. leaving
 * null value for a function) implicitly passes calls to the
 * next layer. To override or shadow an API the layer must implement
 * the specific call.
 */
struct _ParrotIOLayerAPI {
    INTVAL          (*Init)(Interp *, ParrotIOLayer * l);
    ParrotIOLayer * (*New)(ParrotIOLayer * proto);
    void            (*Delete)(ParrotIOLayer * l);
    INTVAL          (*Pushed)(ParrotIOLayer * l, ParrotIO * io);
    INTVAL          (*Popped)(ParrotIOLayer * l, ParrotIO * io);
    ParrotIO *      (*Open)(Interp *, ParrotIOLayer * l,
                            const char * name, INTVAL flags);
    ParrotIO *      (*Open2_Unused)(Interp *);
    ParrotIO *      (*Open3_Unused)(Interp *);
    ParrotIO *      (*Open_ASync)(Interp *, ParrotIOLayer * l,
                                  const char * name, const char * mode,
                                  DummyCodeRef *);
    ParrotIO *      (*FDOpen)(Interp *, ParrotIOLayer * l,
                              PIOHANDLE fd, INTVAL flags);
    INTVAL          (*Close)(Interp *, ParrotIOLayer * l,
                                ParrotIO * io);
    size_t          (*Write)(Interp *, ParrotIOLayer * l,
                             ParrotIO * io, STRING *);
    size_t          (*Write_ASync)(Interp *, ParrotIOLayer * layer,
                                   ParrotIO * io, STRING *, DummyCodeRef *);
    size_t          (*Read)(Interp *, ParrotIOLayer * layer,
                            ParrotIO * io, STRING **);
    size_t          (*Read_ASync)(Interp *, ParrotIOLayer * layer,
                                  ParrotIO * io, STRING **, DummyCodeRef *);
    INTVAL          (*Flush)(Interp *, ParrotIOLayer * layer,
                             ParrotIO * io);
    size_t          (*Peek)(Interp *, ParrotIOLayer * layer,
                            ParrotIO * io, STRING ** buf);
    PIOOFF_T        (*Seek)(Interp *, ParrotIOLayer * layer,
                            ParrotIO * io, PIOOFF_T offset, INTVAL whence);
    PIOOFF_T        (*Tell)(Interp *, ParrotIOLayer * layer,
                            ParrotIO * io);
    INTVAL          (*SetBuf)(Interp *, ParrotIOLayer * layer,
                              ParrotIO * io, size_t bufsize);
    INTVAL          (*SetLineBuf)(Interp *, ParrotIOLayer * layer,
                                  ParrotIO * io);
    INTVAL          (*GetCount)(Interp *, ParrotIOLayer * layer);
    INTVAL          (*Fill)(Interp *, ParrotIOLayer * layer);
    INTVAL          (*Eof)(Interp *, ParrotIOLayer * l,
                           ParrotIO * io);
    /* Network API */
    INTVAL          (*Poll)(Interp *, ParrotIOLayer *l, ParrotIO *io,
                            int which, int sec, int usec);
    ParrotIO *      (*Socket)(Interp *, ParrotIOLayer *,
                            int dom, int type, int proto);
    INTVAL          (*Connect)(Interp *, ParrotIOLayer *, ParrotIO *,
                            STRING *);
    INTVAL          (*Send)(Interp *, ParrotIOLayer *, ParrotIO *, STRING *);
    INTVAL          (*Recv)(Interp *, ParrotIOLayer *, ParrotIO *, STRING **);
    INTVAL          (*Bind)(Interp *, ParrotIOLayer *, ParrotIO *, STRING *);
    INTVAL          (*Listen)(Interp *, ParrotIOLayer *, ParrotIO *, INTVAL);
    ParrotIO *      (*Accept)(Interp *, ParrotIOLayer *, ParrotIO *);
};

/* these are defined rather than using NULL because strictly-speaking, ANSI C
 * doesn't like conversions between function and non-function pointers. */
#define PIO_null_init (INTVAL (*)(Interp *, ParrotIOLayer *))0
#define PIO_null_push_layer (INTVAL (*)(ParrotIOLayer *, ParrotIO *))0
#define PIO_null_pop_layer (INTVAL (*)(ParrotIOLayer *, ParrotIO *))0
#define PIO_null_open (ParrotIO * (*)(Interp *, ParrotIOLayer *, const char*, INTVAL))0
#define PIO_null_open2 (ParrotIO * (*)(Interp *))0
#define PIO_null_open3 (ParrotIO * (*)(Interp *))0
#define PIO_null_open_async (ParrotIO * (*)(Interp *, ParrotIOLayer *, const char *, const char *, DummyCodeRef *))0
#define PIO_null_fdopen (ParrotIO * (*)(Interp *, ParrotIOLayer *, PIOHANDLE, INTVAL))0
#define PIO_null_close (INTVAL (*)(Interp *, ParrotIOLayer *, ParrotIO *))0
#define PIO_null_write (size_t (*)(Interp *, ParrotIOLayer *, ParrotIO *, STRING*))0
#define PIO_null_write_async (size_t (*)(Interp *, ParrotIOLayer *, ParrotIO *, STRING *,DummyCodeRef *))0
#define PIO_null_read (size_t (*)(Interp *, ParrotIOLayer *, ParrotIO *, STRING**))0
#define PIO_null_read_async (size_t (*)(Interp *, ParrotIOLayer *, ParrotIO *, STRING **, DummyCodeRef *))0
#define PIO_null_flush (INTVAL (*)(Interp *, ParrotIOLayer *, ParrotIO *))0
#define PIO_null_peek (size_t (*)(Interp *, ParrotIOLayer *, ParrotIO *, STRING**))0
#define PIO_null_seek (PIOOFF_T (*)(Interp *, ParrotIOLayer *, ParrotIO *, PIOOFF_T, INTVAL))0
#define PIO_null_tell (PIOOFF_T (*)(Interp *, ParrotIOLayer *, ParrotIO *))0
#define PIO_null_setbuf (INTVAL (*)(Interp *, ParrotIOLayer *, ParrotIO *, size_t))0
#define PIO_null_setlinebuf (INTVAL (*)(Interp *, ParrotIOLayer *, ParrotIO *))0
#define PIO_null_getcount (INTVAL (*)(Interp *, ParrotIOLayer *))0
#define PIO_null_fill (INTVAL (*)(Interp *, ParrotIOLayer *))0
#define PIO_null_eof (INTVAL (*)(Interp *, ParrotIOLayer *, ParrotIO *))0
#define PIO_null_socket (ParrotIO * (*)(Interp *, ParrotIOLayer *, int, int, int))0

/*
 * more API XXX should be in io.h when things settle
 */

ParrotIOLayer * PIO_utf8_register_layer(void);
ParrotIOLayer * PIO_mmap_register_layer(void);
ParrotIOLayer * PIO_string_register_layer(void);

/* Parrot_Socklen_t is used in POSIX accept call */
#if PARROT_HAS_SOCKLEN_T
typedef socklen_t Parrot_Socklen_t;
#else
typedef int Parrot_Socklen_t;
#endif

#endif /* PARROT_IO_PRIVATE_H_GUARD */

/*

=head1 SEE ALSO

F<src/io/io_buf.c>,
F<src/io/io_layers.c>,
F<src/io/io_passdown.c>,
F<src/io/io_stdio.c>,
F<src/io/io_unix.c>,
F<src/io/io_utf8.c>,
F<src/io/io_win32.c>,
F<src/io/io.c>.

=head1 HISTORY

Originally written by Melvin Smith.

Refactored by Juergen Boemmels.

Internal Definitions moved from F<include/parrot/io.h>.

=head1 TODO

Move the Layer structure to here also.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
