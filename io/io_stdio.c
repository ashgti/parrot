/* io_stdio.c
 *  Copyright: (When this is determined...it will go here)
 *  CVS Info
 *  Overview:
 *      The "STDIO" layer of Parrot IO. Buffering and all the fun stuff.
 *
 *  Data Structure and Algorithms:
 *  History:
 *      Initially written by Melvin Smith
 *  Notes:
 *  References:
 *      Some ideas from AT&T SFIO
 */

#include "parrot/parrot.h"

/* Defined at bottom */
extern ParrotIOLayerAPI        pio_stdio_layer_api;

ParrotIOLayer           pio_stdio_layer = {
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

ParrotIO *      PIO_stdio_open(theINTERP, ParrotIOLayer * layer,
			const char * spath, const char * smode);
INTVAL          PIO_stdio_setbuf(theINTERP, ParrotIOLayer * layer,
                        ParrotIO * io, size_t bufsize);
ParrotIO *      PIO_stdio_fdopen(theINTERP, ParrotIOLayer * layer,
		        INTVAL fd, const char * smode);
INTVAL          PIO_stdio_close(theINTERP, ParrotIOLayer * layer,
                        ParrotIO * io);
void            PIO_stdio_flush(theINTERP, ParrotIOLayer * layer,
                        ParrotIO * io);
size_t          PIO_stdio_read(theINTERP, ParrotIOLayer * layer,
                        ParrotIO * io, void * buffer, size_t len);
size_t          PIO_stdio_write(theINTERP, ParrotIOLayer * layer,
                        ParrotIO * io, const void * buffer, size_t len);
INTVAL          PIO_stdio_puts(theINTERP, ParrotIOLayer * l, ParrotIO * io,
                        const char * s);




ParrotIO * PIO_stdio_open(theINTERP, ParrotIOLayer * layer,
			const char * spath, const char * smode) {
        ParrotIO * io;
        ParrotIOLayer * l = layer;
        while(l) {
                if(l->api->Open) {
                        io = (*l->api->Open)(interpreter, l, spath,
                                                smode);
                        /*
                         * We have an IO stream now setup stuff
                         * for our layer before returning it.
                         */
                        PIO_stdio_setbuf(interpreter, l, io, PIO_BUFSIZE);
                        return io;
                }
                l = PIO_DOWNLAYER(l);
        }
        return NULL;
}


/*
 * Don't pass setbuf() calls down the stack, top layer wins.
 * This doesn't mean other layers can't buffer, I just to
 * think about the mechanism for buffer control or if it even
 * makes sense this way. Most layers will not implement setbuf()...
 */
INTVAL PIO_stdio_setbuf(theINTERP, ParrotIOLayer * layer, ParrotIO * io,
                                        size_t bufsize) {
        size_t size;
        ParrotIOLayer * l = layer;
        ParrotIOBuf * b = &io->b;
        /* If there is a buffer, make sure we flush before
         * dinking around with the buffer.
         */
        if( b->startb )
                PIO_stdio_flush(interpreter, l, io);

        /* Choose an appropriate buffer size for caller */
        if( bufsize == PIO_UNBOUND ) {
                b->size = PIO_BUFSIZE;
        }
        else {
                b->size = bufsize;
        }

        if( b->startb && (io->flags & PIO_F_MALLOC) ) {
                free(b->startb);
                b->startb = NULL;
        }

        if( b->size > 0 ) {
                b->startb = b->next = b->endw = b->endr = malloc(bufsize);
                io->flags |= PIO_F_MALLOC;
        }

        if( bufsize != 0 )
                /* FIXME: Line buffering not supported yet */
                io->flags |= PIO_F_BUF;
        else
                io->flags &= ~(PIO_F_BUF|PIO_F_LINEBUF);

#if 0
        if((interpreter->flags & PARROT_DEBUG_FLAG) != 0) {
                fprintf(stderr,
                "PIO_setbuf: Alloced %d byte buffer for stream (fd %d)\n",
                        bufsize, io->fd );

        }
#endif

        return 1;
}


ParrotIO * PIO_stdio_fdopen(theINTERP, ParrotIOLayer * layer,
		        INTVAL fd, const char * smode) {
        ParrotIO * io;
        ParrotIOLayer * l = PIO_DOWNLAYER(layer);
        while(l) {
                if(l->api->FDOpen) {
                        io = (*l->api->FDOpen)(interpreter, l, fd,
                                                smode);
                        PIO_stdio_setbuf(interpreter, l, io, PIO_BUFSIZE);
                        return io;
                }
                l = PIO_DOWNLAYER(l);
        }
        return NULL;
}


INTVAL PIO_stdio_close(theINTERP, ParrotIOLayer * layer, ParrotIO * io) {
        ParrotIOLayer * l = PIO_DOWNLAYER(layer);
        PIO_stdio_flush(interpreter, layer, io);
        while(l) {
                if(l->api->Close) {
                        return (*l->api->Close)(interpreter,l,io);
                }
                l = PIO_DOWNLAYER(l);
        }
        return 0;
}


void PIO_stdio_flush(theINTERP, ParrotIOLayer * layer, ParrotIO * io) {
#if 0
        size_t err;
        size_t to_write;
        if( !io->out.buf || io->out.buf == io->out.head )
                return;
        if( io->out.head )
                to_write = io->out.head - io->out.buf;
        else to_write = io->out.bufsize;

        /* Flush to next layer */
        write_buffer:
        err = write(io->fd, io->out.buf, to_write);
#if 0
        if((interpreter->flags & PARROT_DEBUG_FLAG) != 0) {
                fprintf(stderr, "PIO_flush: Flushed %d bytes\n", to_write);
        }
#endif
        if( err >= 0 ) {
                io->out.head = io->out.buf;
                return;
        }
        else {
                switch(errno) {
                        case EAGAIN:    return;
#ifdef EINTR
                        case EINTR:     goto write_buffer;
#endif
                        default:        return;
                }
        }
#endif
}


size_t PIO_stdio_read(theINTERP, ParrotIOLayer * layer, ParrotIO * io,
		                void * buffer, size_t len) {
        return 0;
}


size_t PIO_stdio_write(theINTERP, ParrotIOLayer * layer, ParrotIO * io,
			const void * buffer, size_t len) {
        return 0;
}


INTVAL PIO_stdio_puts(theINTERP, ParrotIOLayer * layer, ParrotIO * io,
                                const char * s) {
        ParrotIOLayer * l = layer;
        while((l = PIO_DOWNLAYER(l)) != NULL) {
                if( l->api->PutS ) {
                        return (*l->api->PutS)(interpreter, l, io, s);
                }
        }
        return 0;
}



ParrotIOLayerAPI        pio_stdio_layer_api = {
        PIO_base_init,
        PIO_base_new_layer,
        PIO_base_delete_layer,
        NULL,
        NULL,
        PIO_stdio_open,
        NULL,
        NULL,
        NULL,
        PIO_stdio_fdopen,
        PIO_stdio_close,
        PIO_stdio_write,
        NULL,
        PIO_stdio_read,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        PIO_stdio_puts,
        NULL 
};



/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/

