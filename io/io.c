/*
Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
$Id$

=head1 NAME

io/io.c - Generic IO

=head1 DESCRIPTION

The Parrot IO subsystem uses a per-interpreter stack to provide a
layer-based approach to IO. Each layer implements a subset of the
C<ParrotIOLayerAPI> vtable. To find an IO function the layer
stack is searched downwards until a non-C<NULL> function pointer
is found for that particular slot.

This file implements the generic functionality. Specific layers are in
separate files: F<io/io_buf.c>, F<io/io_stdio.c>, F<io/io_unix.c>,
F<io/io_win32.c>.

The C<ParrotIO> PMC provides th class-based interface that is used in
Parrot ops. The C<ParrotIO struct> is defined in F<io/io_private.h>.

=head2 Resource Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "io_private.h"

#include <stdarg.h>

/* This is list of valid layers */
ParrotIOLayer *pio_registered_layers;

/* This is the default stack used for IO. Copy this to each new interp */
/*
ParrotIOLayer   * pio_default_stack;
*/

/*
        The standard streams are:

                interpreter->piodata->table[PIO_STD*_FILENO].
*/


PIOOFF_T piooffsetzero;

/*

=item C<PMC *
new_io_pmc(theINTERP, ParrotIO *io)>

Creates and returns a new C<ParrotIO> PMC.

=cut

*/

PMC *
new_io_pmc(theINTERP, ParrotIO *io)
{
    PMC *new_pmc;
    new_pmc = pmc_new(interpreter, enum_class_ParrotIO);
    PMC_data(new_pmc) = io;
    /* io could be NULL */
    if(io)
       new_pmc->cache.struct_val = io->stack;
    else
       new_pmc->cache.struct_val = NULL;
    return new_pmc;
}

/*

=item C<ParrotIOTable
alloc_pio_array(int numhandles)>

Called in C<PIO_init()> to allocate an interpreter's IO handle table
with room for C<numhandles> IO handles.

=cut

*/

ParrotIOTable
alloc_pio_array(int numhandles)
{
    ParrotIOTable newhandles;
    size_t size = numhandles * sizeof(ParrotIO *);
    newhandles = (ParrotIOTable)mem_sys_allocate_zeroed(size);
    return newhandles;
}

/*

=item C<int
realloc_pio_array(ParrotIOTable *table, int numhandles)>

Unimplemented. Simply returns C<-1>.

=cut

*/

int
realloc_pio_array(ParrotIOTable *table, int numhandles)
{
    UNUSED(table);
    UNUSED(numhandles);
    return -1;
}

/*

=item C<ParrotIO *
PIO_new(theINTERP, INTVAL iotype, INTVAL flags, INTVAL mode)>

Create a new IO stream. 

The values of C<flags> and C<mode> are set in the returned C<ParrotIO>.

Currently C<iotype> is unused.

=cut

*/

ParrotIO *
PIO_new(theINTERP, INTVAL iotype, INTVAL flags, INTVAL mode)
{
    ParrotIO *new_io;

    UNUSED(iotype);

    new_io = (ParrotIO *)mem_sys_allocate(sizeof(ParrotIO));
    new_io->fpos = new_io->lpos = piooffsetzero;
    new_io->flags = flags;
    new_io->mode = mode;
    new_io->stack = interpreter->piodata->default_stack;
    new_io->b.flags = 0;
    new_io->b.size = 0;
    new_io->b.startb = NULL;
    new_io->b.endb = NULL;
    new_io->b.next = NULL;
    return new_io;
}

/*

=item C<void
PIO_destroy(theINTERP, PMC *pmc)>

Destroying the IO stream at the moment only frees the memory and removes
the pointers from the PMC.

=cut

*/

void
PIO_destroy(theINTERP, PMC *pmc)
{
    ParrotIO *io = PMC_data(pmc);
    UNUSED(interpreter);

    if(!io)
        return;
    if (io->b.startb && (io->b.flags & PIO_BF_MALLOC)) {
        mem_sys_free(io->b.startb);
        io->b.startb = NULL;
        io->b.flags &= ~PIO_BF_MALLOC;
    }
    mem_sys_free(io);
    PMC_data(pmc) = NULL;
    pmc->cache.struct_val = NULL;
}

/*

=item C<void
PIO_init(theINTERP)>

This is called when creating an interpreter to set up its layer stack
and creates the C<STD*> handles.

=cut

*/

void
PIO_init(theINTERP)
{
    /* Has interp been initialized already? */
    if (interpreter->piodata) {
        /* memsub system is up and running: */
        /* Init IO stacks and handles for interp instance.  */
        if (PIO_init_stacks(interpreter) != 0) {
            internal_exception(PIO_ERROR, "PIO init stacks failed.");
        }

        if (!PIO_STDIN(interpreter) || !PIO_STDOUT(interpreter)
            || !PIO_STDERR(interpreter)) {
            internal_exception(PIO_ERROR, "PIO init std handles failed.");
        }

        if (Interp_flags_TEST(interpreter, PARROT_DEBUG_FLAG)) {
            PIO_eprintf(NULL, "PIO: IO system initialized.\n");
        }

        return;
    }

    interpreter->piodata = mem_sys_allocate(sizeof(ParrotIOData));
    if (interpreter->piodata == NULL)
        internal_exception(PIO_ERROR, "PIO alloc piodata failure.");
    interpreter->piodata->default_stack = NULL;
    interpreter->piodata->table = alloc_pio_array(PIO_NR_OPEN);
    if (interpreter->piodata->table == NULL)
        internal_exception(PIO_ERROR, "PIO alloc table failure.");

}

/*

=item C<void
PIO_finish(theINTERP)>

Called when the interpreter is being destroyed to close down its IO
resourses.

=cut

*/

void
PIO_finish(theINTERP)
{
    ParrotIOLayer *p, *down;
#if 0
    ParrotIO *io;
    int i;
#endif

    /* XXX is this all correct? */

    fflush(stdout);
    fflush(stderr);

#if 0
    /* new_io_pmc isn't possible aynmore  - mem subsystem is down already */

    /* TODO: close std descriptors */

    for (i = 0 ; i < PIO_NR_OPEN; i++) {
        if ( (io = interpreter->piodata->table[i]) ) {
            PIO_close(interpreter, new_io_pmc(interpreter, io));
        }
    }
#endif
    for (p = interpreter->piodata->default_stack; p; ) {
        down = p->down;
        if (p->api->Delete)
            (*p->api->Delete) (p);
        /* mem_sys_free(p); */ /* XXX ??? */
        p = down;
    }
    mem_sys_free(interpreter->piodata->table);
    mem_sys_free(interpreter->piodata);
}

/*

=item C<void
PIO_atexit(theINTERP)>

IO system destructor, flush streams, free structures, etc.

This is unimplemented and unused.

=cut

*/

void
PIO_atexit(theINTERP)
{
    UNUSED(interpreter);
#if 0
    PIO_flush(interpreter, pio_stdout);
#endif
}

/*

=item C<INTVAL
PIO_init_stacks(theINTERP)>

Initialize the interpreter's default IO stack by pushing on the IO
layers (OS-specific first).

=cut

*/

INTVAL
PIO_init_stacks(theINTERP)
{
    ParrotIOLayer *p;

    /* First push the platform specific OS layer */
    /* Optimize this to keep a default stack and just
     * call copy stack.
     */
#ifdef PIO_OS_UNIX
    PIO_push_layer(interpreter, PIO_base_new_layer(&pio_unix_layer), PMCNULL);
#endif
#ifdef PIO_OS_WIN32
    PIO_push_layer(interpreter, PIO_base_new_layer(&pio_win32_layer), PMCNULL);
#endif
#ifdef PIO_OS_STDIO
    PIO_push_layer(interpreter, PIO_base_new_layer(&pio_stdio_layer), PMCNULL);
#endif
    PIO_push_layer(interpreter, PIO_base_new_layer(&pio_buf_layer), PMCNULL);

    /* Note: All layer pushes should be done before init calls */
    for (p = interpreter->piodata->default_stack; p; p = p->down) {
        if (p->api->Init) {
            if ((*p->api->Init) (interpreter, p) != 0) {
                char buf[1024];
                sprintf(buf, "Parrot IO: Failed init layer(%s).\n", p->name);
                internal_exception(PIO_ERROR, buf);
            }
        }
    }

    return 0;
}

/*

=item C<INTVAL
PIO_base_init(theINTERP, ParrotIOLayer *l)>

Init routine called once for each layer at interpreter creation time.
This is similar to a Perl module C<INIT {}> block.

This default implementation does nothing. Returns C<0>.

=cut

*/

INTVAL
PIO_base_init(theINTERP, ParrotIOLayer *l)
{
    UNUSED(interpreter);
    UNUSED(l);
    return 0;
}

/*

=back

=head2 Layer and Stack Functions

=over 4

=item C<ParrotIOLayer *
PIO_base_new_layer(ParrotIOLayer *proto)>

The default IO layer constructor. Creates and returns a new
C<ParrotIOLayer>. If a prototype C<*proto> is supplied then it's values
will be copied to the new instance.

=cut

*/

ParrotIOLayer *
PIO_base_new_layer(ParrotIOLayer *proto)
{
    ParrotIOLayer *new_layer;

    new_layer = mem_sys_allocate(sizeof(ParrotIOLayer));
    if (proto) {
        /* FIXME: Flag here to indicate whether to free strings */
        new_layer->name = proto->name;
        new_layer->flags = proto->flags;
        new_layer->api = proto->api;
    }
    else {
        new_layer->name = NULL;
        new_layer->flags = 0;
        new_layer->api = NULL;
    }
    new_layer->self = 0;
    new_layer->up = NULL;
    new_layer->down = NULL;
    return new_layer;
}

/*

=item C<void
PIO_base_delete_layer(ParrotIOLayer *layer)>

The default IO layer destructor. Frees the memory associated with
C<*layer>.

=cut

*/

void
PIO_base_delete_layer(ParrotIOLayer *layer)
{
    if (layer != NULL)
        mem_sys_free(layer);
}

/*

=item C<INTVAL
PIO_push_layer(theINTERP, ParrotIOLayer *layer, PMC *pmc)>

Push a layer onto an IO object (C<*pmc>) or the default stack.

=cut

*/

INTVAL
PIO_push_layer(theINTERP, ParrotIOLayer *layer, PMC *pmc)
{
    ParrotIOLayer *t;

    if (layer == NULL)
        return -1;
    if (!PMC_IS_NULL(pmc)) {
        ParrotIO *io = PMC_data(pmc);
        if (!io)
            return -1;
        if (io->stack == NULL && (layer->flags & PIO_L_TERMINAL) == 0) {
            /* Error( 1st layer must be terminal) */
            return -1;
        }
        /* Check and see if this layer already is on stack
         * This is a internals sanity check not a user level
         * check, at least until I fix copy-on-write stacks.
         * -Melvin
         */
        for (t = io->stack; t; t = t->down) {
            if (t == layer)
                return -1;
        }

        layer->down = io->stack;
        if (io->stack)
            io->stack->up = layer;
        io->stack = layer;
        if (layer->api->Pushed)
            (*layer->api->Pushed) (layer, io);
    }
    else {
        ParrotIOData *d = interpreter->piodata;
        if (d->default_stack == NULL && (layer->flags & PIO_L_TERMINAL) == 0) {
            /* Error( 1st layer must be terminal) */
            return -1;
        }
        /* Sanity check */
        for (t = d->default_stack; t; t = t->down) {
            if (t == layer)
                return -1;
        }

        layer->down = d->default_stack;
        if (d->default_stack)
            d->default_stack->up = layer;
        d->default_stack = layer;
        return 0;
    }
    return -1;
}

/*

=item C<ParrotIOLayer *
PIO_pop_layer(theINTERP, PMC *pmc)>

Pop a layer from an IO object (C<*pmc>) or the default stack.

=cut

*/

ParrotIOLayer *
PIO_pop_layer(theINTERP, PMC *pmc)
{
    ParrotIOLayer *layer;
    ParrotIO *io = PMC_data(pmc);

    if (!PMC_IS_NULL(pmc)) {
        if (!io)
            return 0;
        layer = io->stack;
        if (layer) {
            io->stack = layer->down;
            io->stack->up = 0;
            layer->up = 0;
            layer->down = 0;
            if (layer->api->Popped)
                (*layer->api->Popped) (layer, io);
            return layer;
        }
        return layer;
    }
    /* Null io object - use default stack */
    else {
        ParrotIOData *d;
        d = interpreter->piodata;
        layer = d->default_stack;
        if (layer) {
            d->default_stack = layer->down;
            d->default_stack->up = NULL;
            layer->up = 0;
            layer->down = 0;
            return layer;
        }
    }

    return 0;
}

/*

=item C<ParrotIOLayer *
PIO_copy_stack(ParrotIOLayer *stack)>

Primarily used to copy the default IO stack for a new IO object. Later
we will do some funky copy-on-write stuff.

=cut

*/

ParrotIOLayer *
PIO_copy_stack(ParrotIOLayer *stack)
{
    ParrotIOLayer *ptr_new;
    ParrotIOLayer **ptr_ptr_new;
    ParrotIOLayer *ptr_proto;
    ParrotIOLayer *ptr_last = NULL;
    ptr_ptr_new = &ptr_new;
    ptr_proto = stack;
    while (ptr_proto) {
        *ptr_ptr_new = PIO_base_new_layer(ptr_proto);
        (*ptr_ptr_new)->up = ptr_last;
        ptr_proto = ptr_proto->down;
        ptr_last = *ptr_ptr_new;
        ptr_ptr_new = &((*ptr_ptr_new)->down);
    }

    return ptr_new;
}

/*

=back

=head2 Generic top-level C<ParrotIO> interface

=over 4

=item C<INTVAL
PIO_parse_open_flags(const char *flagstr)>

Parses C<*flagstr> for Perl-style file open mode flags (C<<<>>, C<<>>>,
C<<<>>>>>, C<<+<>>, C<<+>>>) and returns the combined generic bit flags.

The low level OS layers may then interpret the generic bits differently
depending on platform.

XXX BD Should this be static?

=cut

*/

INTVAL
PIO_parse_open_flags(const char *flagstr)
{
    INTVAL flags;
    const char *s;

    if (!flagstr || !(*flagstr))
        return 0;
    flags = 0;
    s = flagstr;
    /* Set mode flags - <, >, >>, +<, +> */
    switch (*s++) {
    case '+':
        flags |= (PIO_F_WRITE | PIO_F_READ);
        switch (*s++) {
        case '<':
            break;
        case '>':
            flags |= PIO_F_TRUNC;
            break;
        default:
            return 0;
        }
        break;
    case '<':
        flags |= PIO_F_READ;
        break;
    case '>':
        flags |= PIO_F_WRITE;
        if (*s == '>') {
            flags |= PIO_F_APPEND;
            s++;
        }
        else {
            flags |= PIO_F_TRUNC;
        }
        break;
    default:
        return 0;
    }

    /* TODO: add ? and ! for block/non-block */
    switch (*s++) {
    case '\0':
        /* No extra arguments */
        break;
    default:
        return 0;
    }

    return flags;
}

/*

=item C<INTVAL
PIO_pioctl(theINTERP, PMC *pmc, INTVAL cmd, INTVAL arg)>

General purpose interface for manipulation of IO objects and
layer attributes.

Refer to the C<PIOCTL*> values in F<include/parrot/io.h>.

All "set" operations return C<0> on success and a negative value on
error. "get" operations will use the return value as the value
requested, but should always be C<<>= 0>>. A negative value indicates an
error. This may be too limited but we will see. --Melvin

=cut

*/

INTVAL
PIO_pioctl(theINTERP, PMC *pmc, INTVAL cmd, INTVAL arg)
{

    ParrotIO * io = PMC_data(pmc);
    ParrotIOBuf * b = &io->b;
    if(!io) return -1;

    switch(cmd) {
       case PIOCTL_CMDSETRECSEP: io->recsep = arg;

       case PIOCTL_CMDGETRECSEP: return io->recsep;

       case PIOCTL_CMDSETBUFTYPE:
            if(arg == PIOCTL_NONBUF)
               return PIO_setbuf(interpreter, pmc, 0);
            else if(arg == PIOCTL_LINEBUF)
               return PIO_setlinebuf(interpreter, pmc);
            else if(arg == PIOCTL_BLKBUF)
               return PIO_setbuf(interpreter, pmc, PIO_UNBOUND);
            else return -1;

       case PIOCTL_CMDGETBUFTYPE:
            if(io->flags & PIO_F_LINEBUF) return PIOCTL_LINEBUF;
            if(io->flags & PIO_F_BLKBUF) return PIOCTL_BLKBUF;
            return PIOCTL_NONBUF;

       case PIOCTL_CMDSETBUFSIZE:
            return PIO_setbuf(interpreter, pmc, arg);

       case PIOCTL_CMDGETBUFSIZE:
             if(b) return b->size;
             else return -1;
       default: return -1;
    }

    return 0;
}

/*

=item C<INTVAL
PIO_setbuf(theINTERP, PMC *pmc, size_t bufsize)>

Sets the buffer size for C<*pmc> to C<bufsize>. Returns C<0> if the
buffering was enabled.

=cut

*/

INTVAL
PIO_setbuf(theINTERP, PMC *pmc, size_t bufsize)
{
    ParrotIOLayer *layer = pmc->cache.struct_val;
    ParrotIO *io = PMC_data(pmc);
    if(!io)
        return -1;
    PIO_flush(interpreter, pmc);
    return PIO_setbuf_down(interpreter, layer, io, bufsize);
}

/*

=item C<INTVAL
PIO_setlinebuf(theINTERP, PMC *pmc)>

Enables line buffering for C<*pmc>. Returns C<0> if line buffering was
successfully set, or already enabled.

=cut

*/

INTVAL
PIO_setlinebuf(theINTERP, PMC *pmc)
{
    ParrotIOLayer *l = pmc->cache.struct_val;
    ParrotIO *io = PMC_data(pmc);
    if(!io)
        return -1;

    return PIO_setlinebuf_down(interpreter, l, io);
}

/*

=item C<PMC *
PIO_open(theINTERP, ParrotIOLayer *layer, const char *spath, 
         const char *sflags)>

Creates and returns a C<ParrotIO> PMC for C<*spath>.

=cut

*/

PMC *
PIO_open(theINTERP, ParrotIOLayer *layer, const char *spath, 
         const char *sflags)
{
    ParrotIO *io;
    INTVAL flags = PIO_parse_open_flags(sflags);

    if (!layer) {
        layer = interpreter->piodata->default_stack;
    }

    io = PIO_open_down(interpreter, layer, spath, flags);
    /* io could be null here but we still have to
     * to create a PMC for the caller, no PMCNULL here
     * as that would cause an exception upon access.
     */
    if (io) {
        io->stack = layer;
    }

    return new_io_pmc(interpreter, io);
}

/*

=item C<PMC *
PIO_fdopen(theINTERP, ParrotIOLayer *layer, PIOHANDLE fd, const char *sflags)>

Creates and returns a C<ParrotIO> PMC for C<*spath> on an existing, open
file descriptor.

This is used particularly to initialize the C<STD*> IO handles onto the
OS IO handles (0,1,2).

=cut

*/

PMC *
PIO_fdopen(theINTERP, ParrotIOLayer *layer, PIOHANDLE fd, const char *sflags)
{
    ParrotIO *io;
    INTVAL flags;
    
    if (!layer) {
        layer = interpreter->piodata->default_stack;
    }

    flags = PIO_parse_open_flags(sflags);
    io = PIO_fdopen_down(interpreter, layer, fd, flags);
    /* io could be null here but we still have to
     * to create a PMC for the caller, no PMCNULL here
     * as that would cause an exception upon access.
     */
    if (io) {
        io->stack = layer;
    }

    return new_io_pmc(interpreter, io);
}

/*

=item C<INTVAL
PIO_close(theINTERP, PMC *pmc)>

Flushes, closes and destroys the C<ParrotIO> PMC C<*pmc>.

=cut

*/

INTVAL
PIO_close(theINTERP, PMC *pmc)
{
    INTVAL res;
    ParrotIOLayer *l = pmc->cache.struct_val;
    ParrotIO *io = PMC_data(pmc);
    if(!io)
        return -1;
    PIO_flush(interpreter, pmc); /* XXX boe: is this neccessary here? */
    res =  PIO_close_down(interpreter, l, io);
    PIO_destroy(interpreter, pmc);

    return res;
}

/*

=item C<void
PIO_flush(theINTERP, PMC *pmc)>

Flushes the C<ParrotIO> PMC C<*pmc>.

=cut

*/

void
PIO_flush(theINTERP, PMC *pmc)
{
    ParrotIOLayer *l = pmc->cache.struct_val;
    ParrotIO *io = PMC_data(pmc);
    if(!io)
        return;

    PIO_flush_down(interpreter, l, io);
}

/*

=item C<INTVAL
PIO_read(theINTERP, PMC *pmc, void *buffer, size_t len)>

Reads up to C<len> bytes from C<*pmc> and copys them into C<*buffer>.

=cut

*/

INTVAL
PIO_read(theINTERP, PMC *pmc, void *buffer, size_t len)
{
    ParrotIOLayer *l = pmc->cache.struct_val;
    ParrotIO *io = PMC_data(pmc);
    if(!io)
        return -1;

    return PIO_read_down(interpreter, l, io, buffer, len);
}

/*

=item C<INTVAL
PIO_write(theINTERP, PMC *pmc, const void *buffer, size_t len)>

Writes C<len> bytes from C<*buffer> to C<*pmc>.

=cut

*/

INTVAL
PIO_write(theINTERP, PMC *pmc, const void *buffer, size_t len)
{
    ParrotIOLayer *l = pmc->cache.struct_val;
    ParrotIO *io = PMC_data(pmc);
    if(!io)
        return -1;

    if (io->flags & PIO_F_WRITE)
        return PIO_write_down(interpreter, l, io, buffer, len);
    else
        return 0;
}

/*

=item C<PIOOFF_T
PIO_seek(theINTERP, PMC *pmc, PIOOFF_T offset, INTVAL w)>

Moves the read/write position of C<*pmc> to offset C<bytes> from the
position indicated by C<w>. Typically C<w> will be C<0> for the start of
the file, C<1> for the current position, and C<2> for the end.

=cut

*/

PIOOFF_T
PIO_seek(theINTERP, PMC *pmc, PIOOFF_T offset, INTVAL w)
{
    ParrotIOLayer *l = pmc->cache.struct_val;
    ParrotIO *io = PMC_data(pmc);
    if(!io)
        return -1;

    return PIO_seek_down(interpreter, l, io, offset, w);
}

/*

=item C<PIOOFF_T
PIO_tell(theINTERP, PMC *pmc)>

Returns the current read/write position of C<*pmc>.

=cut

*/

PIOOFF_T
PIO_tell(theINTERP, PMC *pmc)
{
    ParrotIOLayer *l = pmc->cache.struct_val;
    ParrotIO *io = PMC_data(pmc);
    if(!io)
        return -1;

    return PIO_tell_down(interpreter, l, io);
}

/*

=item C<INTVAL
PIO_eof(theINTERP, PMC *pmc)>

Returns a boolean value indication whether C<*pmc>'s current read/write
position is C<EOF>.

=cut

*/

INTVAL
PIO_eof(theINTERP, PMC *pmc)
{
    ParrotIO *io = PMC_data(pmc);

    UNUSED(interpreter);

    /* io could be null here, but rather than return a negative error
     * we just fake EOF since eof test is usually in a boolean context.
     */
    if (io) {
        return (io->flags & (PIO_F_EOF)) ? 1 : 0;
    }
    return 1;
}

/*

=item C<INTVAL
PIO_puts(theINTERP, PMC *pmc, const char *s)>

Writes C<*s> tp C<*pmc>. C string version.

=cut

*/

INTVAL
PIO_puts(theINTERP, PMC *pmc, const char *s)
{
    return PIO_write(interpreter, pmc, s, strlen(s));
}

/*

=item C<INTVAL
PIO_putps(theINTERP, PMC *pmc, STRING *s)>

Writes C<*s> tp C<*pmc>. Parrot string version.

=cut

*/

INTVAL
PIO_putps(theINTERP, PMC *pmc, STRING *s)
{
    return PIO_write(interpreter, pmc, s->strstart, s->strlen);
}

/*

=item C<INTVAL
PIO_fprintf(theINTERP, PMC *pmc, const char *s, ...)>

Writes a C string format with varargs to C<*pmc>.

=cut

*/

INTVAL
PIO_fprintf(theINTERP, PMC *pmc, const char *s, ...)
{
    va_list args;
    INTVAL ret=-1;

    va_start(args, s);

    ret=PIO_putps(interpreter, pmc, Parrot_vsprintf_c(interpreter, s, args));

    va_end(args);

    return ret;
}

/*

=item C<INTVAL
PIO_printf(theINTERP, const char *s, ...)>

Writes a C string format with varargs to C<stdout>.

=cut

*/

INTVAL
PIO_printf(theINTERP, const char *s, ...) {
    va_list args;
    STRING *str;
    INTVAL ret=-1;

    va_start(args, s);

    if(interpreter) {
        str=Parrot_vsprintf_c(interpreter, s, args);
        ret=PIO_putps(interpreter, PIO_STDOUT(interpreter), str);
    }
    else {
        /* Be nice about this...
         **   XXX BD Should this use the default PIO_STDOUT or something?
         */
        ret=vfprintf(stdout, s, args);
    }

    va_end(args);

    return ret;
}

/*

=item C<INTVAL
PIO_eprintf(theINTERP, const char *s, ...)>

Writes a C string format with varargs to C<stderr>.

=cut

*/

INTVAL
PIO_eprintf(theINTERP, const char *s, ...) {
    va_list args;
    STRING *str;
    INTVAL ret=-1;

    va_start(args, s);

    if(interpreter) {
        str=Parrot_vsprintf_c(interpreter, s, args);

        ret=PIO_putps(interpreter, PIO_STDERR(interpreter), str);
    }
    else {
        /* Be nice about this...
        **   XXX BD Should this use the default PIO_STDERR or something?
        */
        ret=vfprintf(stderr, s, args);
    }

    va_end(args);

    return ret;
}

/*

=item C<PIOHANDLE
PIO_getfd(theINTERP, PMC *pmc)>

Returns C<*pmc>'s file descriptor, or C<0> if it is not defined.

=cut

*/

PIOHANDLE
PIO_getfd(theINTERP, PMC *pmc)
{
    ParrotIO *io = PMC_data(pmc);

    UNUSED(interpreter);

    if (io) {
        return io->fd;
    }

    return (PIOHANDLE)0;
}

/*

=back

=head2 C<PIO_STD*> Functions

The C<PIO_STD*> functions are defined using the C<theINTERP> macro. The
prototype is protected with an identity macro.

=cut

*/

#define ID(a) a

/*

=over 4

=item C<PMC *
PIO_STDIN ID((theINTERP))>

Returns the C<ParrotIO> PMC for C<stdin>.

=cut

*/

PMC *
PIO_STDIN ID((theINTERP))
{
    return PIO_STDIN(interpreter);
}

/*

=item C<PMC *
PIO_STDOUT ID((theINTERP))>

Returns the C<ParrotIO> PMC for C<stdout>.

=cut

*/

PMC *
PIO_STDOUT ID((theINTERP))
{
    return PIO_STDOUT(interpreter);
}

/*

=item C<PMC *
PIO_STDERR ID((theINTERP))>

Returns the C<ParrotIO> PMC for C<stderr>.

=cut

*/

PMC *
PIO_STDERR ID((theINTERP))
{
    return PIO_STDERR(interpreter);
}

/*

=back

=head2 DOD-related Functions

=over 4

=item C<void
Parrot_IOData_mark(theINTERP, ParrotIOData *piodata)>

Called from C<trace_active_PMCs()> to mark the IO data live.

=cut

*/

void
Parrot_IOData_mark(theINTERP, ParrotIOData *piodata)
{
    INTVAL i;
    ParrotIOTable table = piodata->table;

    /* XXX boe: Parrot_really_destroy might call us with dod_mark_ptr not
     *          set. This is neccessary until destruction ordering prevents
     *          the premature destruction of the standardhandles
     */
    if (!interpreter->dod_mark_ptr)
        interpreter->dod_mark_ptr = table[0];

    for (i = 0; i < PIO_NR_OPEN; i++) {
        if (table[i]) {
            pobject_lives(interpreter, (PObj *)table[i]);
        }
    }

    /* XXX boe: If piodata has children which needs marking too,
     *          we have to call
     * trace_children(interpreter, interpreter->piodata->table[0]);
     */
}

/*

=back

=head2 Offset Functions

These are used to create offsets for the C<seek> op.

=over 4

=item C<PIOOFF_T
PIO_make_offset(INTVAL offset)>

Returns C<offset>.

=cut

*/

PIOOFF_T
PIO_make_offset(INTVAL offset)
{
    return offset;
}

/*

=item C<PIOOFF_T
PIO_make_offset32(INTVAL hi, INTVAL lo)>

C<hi> is shifted 32 bytes to the left and C<or>ed together with C<lo>.
This allows 64-bit seeks with only 32-bit C<INTVALS>.

=cut

*/

PIOOFF_T
PIO_make_offset32(INTVAL hi, INTVAL lo)
{
    return ((PIOOFF_T)hi << 32) | lo;
}

/*

=item C<PIOOFF_T
PIO_make_offset_pmc(theINTERP, PMC *pmc)>

Returns the return value of the C<get_integer> vtable method on C<*pmc>. 

=cut

*/

PIOOFF_T
PIO_make_offset_pmc(theINTERP, PMC *pmc)
{
    /* XXX: Maybe use bignums here */
    return VTABLE_get_integer(interpreter, pmc);
}

/*

=back

=head2 Networking Functions

=over 4

=item C<INTVAL
PIO_poll(theINTERP, PMC *pmc, INTVAL which, INTVAL sec, INTVAL usec)>

Polls C<*pmc> for the events in C<which> every C<sec> seconds + C<usec>
microseconds.

=cut

*/

INTVAL
PIO_poll(theINTERP, PMC *pmc, INTVAL which, INTVAL sec, INTVAL usec)
{
    ParrotIOLayer *l = pmc->cache.struct_val;
    ParrotIO *io = PMC_data(pmc);
    return PIO_poll_down(interpreter, l, io, which, sec, usec);
}

/*

=item C<PMC *
PIO_socket(theINTERP, INTVAL fam, INTVAL type, INTVAL proto)>

Creates and returns a socket using the specified address family, socket
type and protocol number. The returned PMC has to be checked with a
boolean test to see whether the socket was successfully created.

=cut

*/

PMC *
PIO_socket(theINTERP, INTVAL fam, INTVAL type, INTVAL proto)
{
    ParrotIO *io;
    ParrotIOLayer *l = interpreter->piodata->default_stack;
    io = PIO_socket_down(interpreter, l, fam, type, proto);
    /* We have to create a PMC here even if the IO handle
     * didn't create because caller has to be able to
     * check with a bool test. Can't use a NULL PMC in a bool
     * test as that will cause an exception.
     */
    return new_io_pmc(interpreter, io);
}

/*

=item C<INTVAL
PIO_recv(theINTERP, PMC *pmc, STRING **buf)>

Receives a message from the connected socket C<*pmc> in C<*buf>.

=cut

*/

INTVAL
PIO_recv(theINTERP, PMC *pmc, STRING **buf)
{
    ParrotIOLayer *l = pmc->cache.struct_val;
    ParrotIO *io = PMC_data(pmc);
    if(!io)
        return -1;

    return PIO_recv_down(interpreter, l, io, buf);
}

/*

=item C<INTVAL
PIO_send(theINTERP, PMC *pmc, STRING *buf)>

Send the message C<*buf> to the connected socket C<*pmc>.

=cut

*/

INTVAL
PIO_send(theINTERP, PMC *pmc, STRING *buf)
{
    ParrotIOLayer *l = pmc->cache.struct_val;
    ParrotIO *io = PMC_data(pmc);
    if(!io)
        return -1;

    return PIO_send_down(interpreter, l, io, buf);
}

/*

=item C<INTVAL
PIO_connect(theINTERP, PMC *pmc, STRING *address)>

Connect C<*pmc> to C<*address>.

=cut

*/

INTVAL
PIO_connect(theINTERP, PMC *pmc, STRING *address)
{
    ParrotIOLayer *l = pmc->cache.struct_val;
    ParrotIO *io = PMC_data(pmc);
    if(!io)
        return -1;

    return PIO_connect_down(interpreter, l, io, address);
}

/*

=item C<INTVAL
PIO_isatty(theINTERP, PMC *pmc)>

Returns a boolean value indicating whether C<*pmc> is a console/tty.

=cut

*/

INTVAL
PIO_isatty(theINTERP, PMC *pmc)
{
    ParrotIO *io = PMC_data(pmc);

    UNUSED(interpreter);

    if (!io)
        return 0;

    return (io->flags & PIO_F_CONSOLE) ? 1 : 0;
}
/*

=back

=head1 SEE ALSO

F<io/io_buf.c>,
F<io/io_passdown.c>,
F<io/io_stdio.c>,
F<io/io_unix.c>,
F<io/io_win32.c>,
F<io/io_private.h>.

=head1 HISTORY

Initially written by Melvin Smith.

Some ideas and goals from Perl5.7 and Nick Ing-Simmons' work.

=head1 TODO

In future rework to use copy-on-write IO stacks rather than creating a
new stack for each IO stream.

Add support for loadable layers in Parrot bytecode.

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
