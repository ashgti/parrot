/*
Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
$Id$

=head1 NAME

io/io_private.h - IO internals

=head1 DESCRIPTION

Internal Details of the Parrot IO subsystem.

=head2 References

Perl6 RFCs (14,30,47,60,186,239,321,345,350).

Some ideas and goals from Perl5.7 and Nick Ing-Simmons' work.

Some ideas from AT&T SFIO.

=cut

*/

#if !defined(PARROT_IO_PRIVATE_H_GUARD)
#define PARROT_IO_PRIVATE_H_GUARD

#define PARROT_IN_IO 1
#define PARROT_ASYNC_DEVEL 0
#define PARROT_NET_DEVEL 0

#include <parrot/io.h>

#if PARROT_NET_DEVEL
/* XXX: Parrot config is currently not probing for all headers so
 * I'm sticking here rather than parrot.h
 */
#include <sys/socket.h>
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
#define PIO_F_LINEBUF   00010000        /* Flushes on newline           */
#define PIO_F_BLKBUF    00020000
#define PIO_F_SHARED    00100000        /* Stream shares a file handle  */
#define PIO_F_ASYNC     10000000        /* In Parrot async is default   */

/* Buffer flags */
#define PIO_BF_MALLOC   00000001        /* Buffer malloced              */
#define PIO_BF_READBUF  00000002        /* Buffer is read-buffer        */
#define PIO_BF_WRITEBUF 00000004        /* Buffer is write-buffer       */

/*
 * Layer flags
 *
 * Terminal layer can't be pushed on top of other layers;
 * vice-versa, non-terminal layers be pushed on an empty io stack
 * An OS layer would be a terminal layer, non-terminals might be
 * buffering, translation, compression or encryption layers.
 */
#define PIO_L_TERMINAL          0x0001
#define PIO_L_FASTGETS          0x0002


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
extern ParrotIOTable alloc_pio_array(int);
extern int realloc_pio_array(ParrotIOTable *, int);

/* redefine PIO_STD* for internal use */
#define PIO_STDIN(i)   (((ParrotIOData*)i->piodata)->table[PIO_STDIN_FILENO])
#define PIO_STDOUT(i)  (((ParrotIOData*)i->piodata)->table[PIO_STDOUT_FILENO])
#define PIO_STDERR(i)  (((ParrotIOData*)i->piodata)->table[PIO_STDERR_FILENO])

/*
 * These function walk down the layerstack starting at l
 * and calling the first non-null function it finds.
 */
ParrotIO *PIO_open_down(theINTERP, ParrotIOLayer * layer, const char * name,
                        INTVAL flags);
ParrotIO *PIO_open_async_down(theINTERP, ParrotIOLayer * layer,
                              const char * name, const char * mode,
                              DummyCodeRef *);
ParrotIO *PIO_fdopen_down(theINTERP, ParrotIOLayer * layer, PIOHANDLE fd,
                          INTVAL flags);
INTVAL    PIO_close_down(theINTERP, ParrotIOLayer * layer, ParrotIO * io);
size_t    PIO_write_down(theINTERP, ParrotIOLayer * layer, ParrotIO * io,
                         const void * buf, size_t len);
size_t    PIO_write_async_down(theINTERP, ParrotIOLayer * layer, ParrotIO * io,
                               void * buf, size_t len, DummyCodeRef *);
size_t    PIO_read_down(theINTERP, ParrotIOLayer * layer, ParrotIO * io,
                        void * buf, size_t len);
size_t    PIO_read_async_down(theINTERP, ParrotIOLayer * layer, ParrotIO * io,
                              void * buf, size_t len, DummyCodeRef *);
size_t    PIO_peek_down(theINTERP, ParrotIOLayer * layer, ParrotIO * io,
                        void * buf);
INTVAL    PIO_flush_down(theINTERP, ParrotIOLayer * layer, ParrotIO * io);
PIOOFF_T  PIO_seek_down(theINTERP, ParrotIOLayer * layer, ParrotIO * io,
                        PIOOFF_T offset, INTVAL whence);
PIOOFF_T  PIO_tell_down(theINTERP, ParrotIOLayer * layer, ParrotIO * io);
INTVAL    PIO_setbuf_down(theINTERP, ParrotIOLayer * layer, ParrotIO * io,
                          size_t bufsize);
INTVAL    PIO_setlinebuf_down(theINTERP, ParrotIOLayer * layer, ParrotIO * io);
INTVAL    PIO_eof_down(theINTERP, ParrotIOLayer * layer, ParrotIO * io);
INTVAL    PIO_poll_down(theINTERP, ParrotIOLayer * layer, ParrotIO *io,
                        INTVAL which, INTVAL sec, INTVAL usec);
ParrotIO *PIO_socket_down(theINTERP, ParrotIOLayer *layer, INTVAL fam,
                          INTVAL type, INTVAL proto);
INTVAL    PIO_recv_down(theINTERP, ParrotIOLayer *layer, ParrotIO *io, STRING **buf);
INTVAL    PIO_send_down(theINTERP, ParrotIOLayer *layer, ParrotIO *io, STRING *buf);
INTVAL    PIO_connect_down(theINTERP, ParrotIOLayer *layer, ParrotIO *io, STRING *address);

/*
 * By default, any layer not implementing an interface (ie. leaving
 * null value for a function) implicitly passes calls to the
 * next layer. To override or shadow an API the layer must implement
 * the specific call.
 */
struct _ParrotIOLayerAPI {
    INTVAL          (*Init)(theINTERP, ParrotIOLayer * l);
    ParrotIOLayer * (*New)(ParrotIOLayer * proto);
    void            (*Delete)(ParrotIOLayer * l);
    INTVAL          (*Pushed)(ParrotIOLayer * l, ParrotIO * io);
    INTVAL          (*Popped)(ParrotIOLayer * l, ParrotIO * io);
    ParrotIO *      (*Open)(theINTERP, ParrotIOLayer * l,
                            const char * name, INTVAL flags);
    ParrotIO *      (*Open2_Unused)(theINTERP);
    ParrotIO *      (*Open3_Unused)(theINTERP);
    ParrotIO *      (*Open_ASync)(theINTERP, ParrotIOLayer * l,
                                  const char * name, const char * mode,
                                  DummyCodeRef *);
    ParrotIO *      (*FDOpen)(theINTERP, ParrotIOLayer * l,
                              PIOHANDLE fd, INTVAL flags);
    INTVAL          (*Close)(theINTERP, ParrotIOLayer * l,
                                ParrotIO * io);
    size_t          (*Write)(theINTERP, ParrotIOLayer * l,
                             ParrotIO * io, const void * buf,
                             size_t len);
    size_t          (*Write_ASync)(theINTERP, ParrotIOLayer * layer,
                                   ParrotIO * io, void * buf, size_t len,
                                   DummyCodeRef *);
    size_t          (*Read)(theINTERP, ParrotIOLayer * layer,
                            ParrotIO * io, void * buf, size_t len);
    size_t          (*Read_ASync)(theINTERP, ParrotIOLayer * layer,
                                  ParrotIO * io, void * buf, size_t len,
                                  DummyCodeRef *);
    INTVAL          (*Flush)(theINTERP, ParrotIOLayer * layer,
                             ParrotIO * io);
    size_t          (*Peek)(theINTERP, ParrotIOLayer * layer,
                            ParrotIO * io, void * buf);
    PIOOFF_T        (*Seek)(theINTERP, ParrotIOLayer * layer,
                            ParrotIO * io, PIOOFF_T offset, INTVAL whence);
    PIOOFF_T        (*Tell)(theINTERP, ParrotIOLayer * layer,
                            ParrotIO * io);
    INTVAL          (*SetBuf)(theINTERP, ParrotIOLayer * layer,
                              ParrotIO * io, size_t bufsize);
    INTVAL          (*SetLineBuf)(theINTERP, ParrotIOLayer * layer,
                                  ParrotIO * io);
    INTVAL          (*GetCount)(theINTERP, ParrotIOLayer * layer);
    INTVAL          (*Fill)(theINTERP, ParrotIOLayer * layer);
    INTVAL          (*Eof)(theINTERP, ParrotIOLayer * l,
                           ParrotIO * io);
    /* Network API */
    INTVAL          (*Poll)(theINTERP, ParrotIOLayer *l, ParrotIO *io,
                            int which, int sec, int usec);
    ParrotIO *      (*Socket)(theINTERP, ParrotIOLayer *,
                            int dom, int type, int proto);
    INTVAL          (*Connect)(theINTERP, ParrotIOLayer *, ParrotIO *,
                            STRING *);
    INTVAL          (*Send)(theINTERP, ParrotIOLayer *, ParrotIO *, STRING *);
    INTVAL          (*Recv)(theINTERP, ParrotIOLayer *, ParrotIO *, STRING **);
};

/* these are defined rather than using NULL because strictly-speaking, ANSI C
 * doesn't like conversions between function and non-function pointers. */
#define PIO_null_push_layer (INTVAL (*)(ParrotIOLayer *, ParrotIO *))0
#define PIO_null_pop_layer (INTVAL (*)(ParrotIOLayer *, ParrotIO *))0
#define PIO_null_open (ParrotIO * (*)(theINTERP, ParrotIOLayer *, const char*, INTVAL))0
#define PIO_null_open2 (ParrotIO * (*)(theINTERP))0
#define PIO_null_open3 (ParrotIO * (*)(theINTERP))0
#define PIO_null_open_async (ParrotIO * (*)(theINTERP, ParrotIOLayer *, const char *, const char *, DummyCodeRef *))0
#define PIO_null_fdopen (ParrotIO * (*)(theINTERP, ParrotIOLayer *, PIOHANDLE, INTVAL))0
#define PIO_null_close (INTVAL (*)(theINTERP, ParrotIOLayer *, ParrotIO *))0
#define PIO_null_write (INTVAL (*)(theINTERP, ParrotIOLayer *, ParrotIO *, const void *, size_t))0
#define PIO_null_write_async (size_t (*)(theINTERP, ParrotIOLayer *, ParrotIO *, void *, size_t, DummyCodeRef *))0
#define PIO_null_read (INTVAL (*)(theINTERP, ParrotIOLayer *, ParrotIO *, const void *, size_t))0
#define PIO_null_read_async (size_t (*)(theINTERP, ParrotIOLayer *, ParrotIO *, void *, size_t, DummyCodeRef *))0
#define PIO_null_flush (INTVAL (*)(theINTERP, ParrotIOLayer *, ParrotIO *))0
#define PIO_null_seek (PIOOFF_T (*)(theINTERP, ParrotIOLayer *, ParrotIO *, PIOOFF_T, INTVAL))0
#define PIO_null_tell (PIOOFF_T (*)(theINTERP, ParrotIOLayer *, ParrotIO *))0
#define PIO_null_setbuf (INTVAL (*)(theINTERP, ParrotIOLayer *, ParrotIO *, size_t))0
#define PIO_null_setlinebuf (INTVAL (*)(theINTERP, ParrotIOLayer *, ParrotIO *))0
#define PIO_null_getcount (INTVAL (*)(theINTERP, ParrotIOLayer *))0
#define PIO_null_fill (INTVAL (*)(theINTERP, ParrotIOLayer *))0
#define PIO_null_eof (INTVAL (*)(theINTERP, ParrotIOLayer *, ParrotIO *))0
#define PIO_null_socket (ParrotIO * (*)(theINTERP, ParrotIOLayer *, int, int, int))0



#endif /* PARROT_IO_PRIVATE_H_GUARD */

/*

=head1 SEE ALSO

F<io/io_buf.c>,
F<io/io_passdown.c>,
F<io/io_stdio.c>,
F<io/io_unix.c>,
F<io/io_win32.c>,
F<io/io.c>.

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
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
