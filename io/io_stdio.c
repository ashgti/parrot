/* io_stdio.c
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  CVS Info
 *      $Id$
 *  Overview:
 *      This is the Parrot IO STDIO layer. This may provide a subset of
 *      full functionality, but must compile on any system with the
 *      ANSI C standard library.  Also note that unlike the other low-level
 *      IO layers (UNIX, Win32), this is _buffered_ IO, out of necessity.
 * 
 *  Data Structure and Algorithms:
 *  History:
 *      Adapted from io_unix.c by Josh WIlmes (josh@hitchhiker.org)
 *  Notes:
 *  References:
 */

#include <stdio.h>
#include "parrot/parrot.h"
#include "io_private.h"

#ifdef PIO_OS_STDIO

/* Defined at bottom */
extern ParrotIOLayerAPI pio_stdio_layer_api;

ParrotIOLayer pio_stdio_layer = {
    NULL,
    "stdio",
    PIO_L_TERMINAL,
    &pio_stdio_layer_api,
    0, 0
};


/*
 * Currently keeping layer prototypes local to each layer
 * file.
 */
static const char * flags_to_stdio(INTVAL flags);

static INTVAL    PIO_stdio_init(theINTERP, ParrotIOLayer *layer);
static ParrotIO *PIO_stdio_open(theINTERP, ParrotIOLayer *layer,
                                const char *spath, INTVAL flags);
static ParrotIO *PIO_stdio_fdopen(theINTERP, ParrotIOLayer *layer,
                                  PIOHANDLE fd, INTVAL flags);
static INTVAL PIO_stdio_close(theINTERP, ParrotIOLayer *layer, ParrotIO *io);
static INTVAL PIO_stdio_flush(theINTERP, ParrotIOLayer *layer, ParrotIO *io);
static size_t    PIO_stdio_read(theINTERP, ParrotIOLayer *layer,
                                ParrotIO *io, void *buffer, size_t len);
static size_t    PIO_stdio_write(theINTERP, ParrotIOLayer *layer,
                                 ParrotIO *io, const void *buffer, size_t len);
static INTVAL    PIO_stdio_puts(theINTERP, ParrotIOLayer *l, ParrotIO *io,
                                const char *s);
static INTVAL    PIO_stdio_seek(theINTERP, ParrotIOLayer *l, ParrotIO *io,
                                PIOOFF_T offset, INTVAL whence);
static PIOOFF_T  PIO_stdio_tell(theINTERP, ParrotIOLayer *l, ParrotIO *io);




static const char *
flags_to_stdio(INTVAL flags)
{
    if ((flags & (PIO_F_WRITE | PIO_F_READ | PIO_F_APPEND)) ==
        (PIO_F_WRITE | PIO_F_READ | PIO_F_APPEND)) {
        return "a+b";
    }
    else if ((flags & (PIO_F_WRITE | PIO_F_READ | PIO_F_TRUNC)) == 
             (PIO_F_WRITE | PIO_F_READ | PIO_F_TRUNC)) {
        return "w+b";
    }
    else if ((flags & (PIO_F_WRITE | PIO_F_READ)) == 
             (PIO_F_WRITE | PIO_F_READ)) {
        return "r+b";
    }
    else if (flags & PIO_F_APPEND) {
        return "ab";
    }    
    else if (flags & PIO_F_WRITE) {
        return "wb";
    }
    else {
        /* PIO_F_READ, hopefully */
        return "rb";
    }
}


/*
 * Setup standard streams, etc.
 */
static INTVAL
PIO_stdio_init(theINTERP, ParrotIOLayer *layer)
{
    PIO_STDIN(interpreter)
        = new_io_pmc(interpreter,
                     PIO_stdio_fdopen(interpreter, layer, stdin, PIO_F_READ));

    PIO_STDOUT(interpreter)
        = new_io_pmc(interpreter,
                     PIO_stdio_fdopen(interpreter, layer, stdout, PIO_F_WRITE));

    PIO_STDERR(interpreter)
        = new_io_pmc(interpreter,
                     PIO_stdio_fdopen(interpreter, layer, stderr, PIO_F_WRITE));

    return 0;
}


/*
 * Open modes (read, write, append, etc.) are done in pseudo-Perl
 * style using <, >, etc.
 */
static ParrotIO *
PIO_stdio_open(theINTERP, ParrotIOLayer *layer,
              const char *spath, INTVAL flags)
{
    ParrotIO *io;
    const char *oflags;
    INTVAL type;
    FILE *fptr;
    type = PIO_TYPE_FILE;

    if ((flags & (PIO_F_WRITE | PIO_F_READ)) == 0)
        return NULL;

    oflags = flags_to_stdio(flags);

    /* Only files for now */
    flags |= PIO_F_FILE;

    /* Try opening the file- note that this can't really handle O_EXCL, etc. */
    fptr = fopen(spath, oflags);

    if (fptr == NULL && errno == ENOENT && (flags & PIO_F_WRITE)) {
        fptr = fopen(spath, "w+b");
    }

    /* File open */
    if (fptr != NULL) {
        if (PIO_isatty(fptr))
            flags |= PIO_F_CONSOLE;
        io = PIO_new(interpreter, type, flags, 0);
        io->fd = fptr;
        return io;
    }
    return NULL;
}


static ParrotIO *
PIO_stdio_fdopen(theINTERP, ParrotIOLayer *layer, PIOHANDLE fptr, INTVAL flags)
{
    ParrotIO *io;
    INTVAL mode;
    mode = 0;

    if (PIO_isatty(fptr))
        flags |= PIO_F_CONSOLE;

    /* fdopened files are always shared */
    flags |= PIO_F_SHARED;

    io = PIO_new(interpreter, PIO_F_FILE, flags, mode);
    io->fd = fptr;
    return io;
}


static INTVAL
PIO_stdio_close(theINTERP, ParrotIOLayer *layer, ParrotIO *io)
{
    if (io->fd != NULL)
        fclose(io->fd);
    io->fd = NULL;
    return 0;
}


INTVAL
PIO_stdio_isatty(PIOHANDLE fptr)
{
    /* no obvious way to check for this with STDIO */
    return 0;
}


INTVAL
PIO_stdio_getblksize(PIOHANDLE fptr)
{
    /* Hard coded for now */
    return PIO_BLKSIZE;
}


static INTVAL
PIO_stdio_flush(theINTERP, ParrotIOLayer *layer, ParrotIO *io)
{
    return fflush(io->fd);
}


static size_t
PIO_stdio_read(theINTERP, ParrotIOLayer *layer, ParrotIO *io,
              void *buffer, size_t len)
{
    size_t bytes;

    UNUSED(interpreter);
    UNUSED(layer);

    bytes = fread(buffer, 1, len, io->fd);

    if (bytes != len) {
        if (feof(io->fd)) {
            io->flags |= PIO_F_EOF;
        } 
    }
    
    return bytes;
}


static size_t
PIO_stdio_write(theINTERP, ParrotIOLayer *layer, ParrotIO *io,
               const void *buffer, size_t len)
{
    UNUSED(interpreter);
    UNUSED(layer);

    return(fwrite(buffer, 1, len, io->fd));
}


static INTVAL
PIO_stdio_puts(theINTERP, ParrotIOLayer *l, ParrotIO *io, const char *s)
{
    return(fputs(s, io->fd));
}


/*
 * Hard seek
 */
static INTVAL
PIO_stdio_seek(theINTERP, ParrotIOLayer *l, ParrotIO *io,
              PIOOFF_T offset, INTVAL whence)
{
    PIOOFF_T pos;
    errno = 0;

    if ((pos = fseek(io->fd, offset, whence)) >= 0) {
        io->lpos = io->fpos;
        io->fpos = pos;
    }

    /* Seek clears EOF */
    io->flags &= ~PIO_F_EOF;
    return (((INTVAL)pos != -1) ? 0 : -1);
}


static PIOOFF_T
PIO_stdio_tell(theINTERP, ParrotIOLayer *l, ParrotIO *io)
{
    return(ftell(io->fd));
}



ParrotIOLayerAPI pio_stdio_layer_api = {
    PIO_stdio_init,
    PIO_base_new_layer,
    PIO_base_delete_layer,
    PIO_null_push_layer,
    PIO_null_pop_layer,
    PIO_stdio_open,
    PIO_null_open2,
    PIO_null_open3,
    PIO_null_open_async,
    PIO_stdio_fdopen,
    PIO_stdio_close,
    PIO_stdio_write,
    PIO_null_write_async,
    PIO_stdio_read,
    PIO_null_read_async,
    PIO_null_flush,
    PIO_stdio_seek,
    PIO_stdio_tell,
    PIO_null_setbuf,
    PIO_null_setlinebuf,
    PIO_null_getcount,
    PIO_null_fill,
    PIO_stdio_puts,
    PIO_null_gets,
    PIO_null_eof
};

#endif /* PIO_OS_STDIO */

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
