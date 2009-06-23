/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 NAME

src/io/filehandle.c - FileHandle utility functions

=head1 DESCRIPTION

This file defines a set of utility functions for the FileHandle PMC used by all
operating systems. For the primary public I/O API, see F<src/io/api.c>.

=cut

*/

#include "parrot/parrot.h"
#include "io_private.h"
#include "../pmc/pmc_filehandle.h"
#include "../pmc/pmc_handle.h"

/* HEADERIZER HFILE: include/parrot/io.h */

/*

=head2 Functions

=over 4

=item C<INTVAL Parrot_io_parse_open_flags(PARROT_INTERP, STRING *mode_str)>

Parses a Parrot string for file open mode flags (C<r> for read, C<w> for write,
C<a> for append, and C<p> for pipe) and returns the combined generic bit flags.

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
INTVAL
Parrot_io_parse_open_flags(PARROT_INTERP, ARGIN_NULLOK(STRING *mode_str))
{
    ASSERT_ARGS(Parrot_io_parse_open_flags)
    INTVAL i, mode_len;
    INTVAL flags = 0;

    if (STRING_IS_NULL(mode_str))
        return PIO_F_READ;

    mode_len = Parrot_str_byte_length(interp, mode_str);

    for (i = 0; i < mode_len; ++i) {
        INTVAL s = Parrot_str_indexed(interp, mode_str, i);
        switch (s) {
            case 'r':
                flags |= PIO_F_READ;
                break;
            case 'w':
                flags |= PIO_F_WRITE;
                if (!(flags & PIO_F_APPEND)) /* don't truncate if appending */
                    flags |= PIO_F_TRUNC;
                break;
            case 'a':
                flags |= PIO_F_APPEND;
                flags |= PIO_F_WRITE;
                if ((flags & PIO_F_TRUNC)) /* don't truncate if appending */
                    flags &= ~PIO_F_TRUNC;
                break;
            case 'p':
                flags |= PIO_F_PIPE;
                break;
            default:
                break;
        }
    }

    return flags;
}

/*

=item C<STRING * Parrot_io_make_string(PARROT_INTERP, STRING **buf, size_t len)>

Creates a STRING* suitable for returning results from IO read functions.
The passed in C<buf> parameter can:

=over 4

=item 1

Point to a NULL STRING

=item 2

Point to a real STRING

=item 3

Point to a fake STRING with (strstart, bufused) holding the *buffer
information.

=back

In the third case, the buffer or STRING must be able to hold the required
amount of data. For cases 1 and 2, a NULL C<strstart> tells this function to
allocate the STRING memory.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING *
Parrot_io_make_string(PARROT_INTERP, ARGMOD(STRING **buf), size_t len)
{
    ASSERT_ARGS(Parrot_io_make_string)
    /*
     * when we get a NULL string, we read a default len
     */
    if (*buf == NULL) {
        *buf = Parrot_str_new_noinit(interp, enum_stringrep_one, len);
        return *buf;
    }
    else {
        STRING *s = *buf;
        if (s->bufused < len)
            Parrot_gc_reallocate_string_storage(interp, s, len);
        return s;
    }
}

/*

=item C<void Parrot_io_set_flags(PARROT_INTERP, PMC *filehandle, INTVAL flags)>

Set the C<flags> attribute of the FileHandle object, which stores bitwise flags
marking filehandle characteristics.

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

PARROT_EXPORT
void
Parrot_io_set_flags(PARROT_INTERP, ARGIN(PMC *filehandle), INTVAL flags)
{
    ASSERT_ARGS(Parrot_io_set_flags)
    PMC * const handle = Parrot_io_get_os_handle_pmc(interp, filehandle);
    SETATTR_FileHandle_flags(interp, filehandle, flags);
    SETATTR_Handle_flags(interp, handle, flags);
}

/*

=item C<INTVAL Parrot_io_get_flags(PARROT_INTERP, PMC *filehandle)>

Set the C<flags> attribute of the FileHandle object, which stores bitwise flags
marking filehandle characteristics.

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

PARROT_EXPORT
INTVAL
Parrot_io_get_flags(PARROT_INTERP, ARGIN(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_get_flags)
    INTVAL flags;
    GETATTR_FileHandle_flags(interp, filehandle, flags);
    return flags;
}

/*

=item C<void Parrot_io_set_buffer_start(PARROT_INTERP, PMC *filehandle, unsigned
char *new_start)>

Set the C<buffer_start> attribute of the FileHandle object, which stores
the position of the start of the buffer.

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

void
Parrot_io_set_buffer_start(PARROT_INTERP, ARGIN(PMC *filehandle),
        ARGIN_NULLOK(unsigned char *new_start))
{
    ASSERT_ARGS(Parrot_io_set_buffer_start)
    PMC * const handle = Parrot_io_get_os_handle_pmc(interp, filehandle);
    PARROT_HANDLE(handle)->buffer_start = new_start;
}

/*

=item C<unsigned char * Parrot_io_get_buffer_start(PARROT_INTERP, PMC
*filehandle)>

Get the C<buffer_start> attribute of the FileHandle object, which stores
the position of the start of the buffer.

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
unsigned char *
Parrot_io_get_buffer_start(PARROT_INTERP, ARGIN(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_get_buffer_start)
    PMC * handle = Parrot_io_get_os_handle_pmc(interp, filehandle);
    return PARROT_HANDLE(handle)->buffer_start;
}

/*

=item C<unsigned char * Parrot_io_get_buffer_next(PARROT_INTERP, PMC
*filehandle)>

Get the C<buffer_next> attribute of the FileHandle object, which stores
the current position within the buffer.

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
unsigned char *
Parrot_io_get_buffer_next(PARROT_INTERP, ARGIN(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_get_buffer_next)
    PMC * const handle = Parrot_io_get_os_handle_pmc(interp, filehandle);
    return PARROT_HANDLE(handle)->buffer_next;
}

/*

=item C<void Parrot_io_set_buffer_next(PARROT_INTERP, PMC *filehandle, unsigned
char *new_next)>

Set the C<buffer_next> attribute of the FileHandle object, which stores
the current position within the buffer.

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

void
Parrot_io_set_buffer_next(PARROT_INTERP, ARGIN(PMC *filehandle),
        ARGIN_NULLOK(unsigned char *new_next))
{
    ASSERT_ARGS(Parrot_io_set_buffer_next)
    PMC * const handle = Parrot_io_get_os_handle_pmc(interp, filehandle);
    PARROT_HANDLE(handle)->buffer_next = new_next;
}

/*

=item C<unsigned char * Parrot_io_get_buffer_end(PARROT_INTERP, PMC
*filehandle)>

Get the C<buffer_end> attribute of the FileHandle object, which stores
the position of the end of the buffer.

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
unsigned char *
Parrot_io_get_buffer_end(PARROT_INTERP, ARGIN_NULLOK(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_get_buffer_end)
    PMC * const handle = Parrot_io_get_os_handle_pmc(interp, filehandle);
    return PARROT_HANDLE(handle)->buffer_end;
}

/*

=item C<void Parrot_io_set_buffer_end(PARROT_INTERP, PMC *filehandle, unsigned
char *new_end)>

Set the C<buffer_end> attribute of the FileHandle object, which stores
the position of the end of the buffer.

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

void
Parrot_io_set_buffer_end(PARROT_INTERP, ARGIN(PMC *filehandle),
        ARGIN_NULLOK(unsigned char *new_end))
{
    ASSERT_ARGS(Parrot_io_set_buffer_end)
    PMC * const handle = Parrot_io_get_os_handle_pmc(interp, filehandle);
    PARROT_HANDLE(handle)->buffer_end = new_end;
}

/*

=item C<INTVAL Parrot_io_get_buffer_flags(PARROT_INTERP, PMC *filehandle)>

Get the C<buffer_flags> attribute of the FileHandle object, which stores
a collection of flags specific to the buffer.

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

PARROT_CAN_RETURN_NULL
INTVAL
Parrot_io_get_buffer_flags(PARROT_INTERP, ARGIN(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_get_buffer_flags)
    PMC * const handle = Parrot_io_get_os_handle_pmc(interp, filehandle);
    return PARROT_HANDLE(handle)->buffer_flags;
}

/*

=item C<void Parrot_io_set_buffer_flags(PARROT_INTERP, PMC *filehandle, INTVAL
new_flags)>

Set the C<buffer_flags> attribute of the FileHandle object, which stores
a collection of flags specific to the buffer.

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

void
Parrot_io_set_buffer_flags(PARROT_INTERP, ARGIN(PMC *filehandle), INTVAL new_flags)
{
    ASSERT_ARGS(Parrot_io_set_buffer_flags)
    PMC * const handle = Parrot_io_get_os_handle_pmc(interp, filehandle);
    PARROT_HANDLE(handle)->buffer_flags = new_flags;
}

/*

=item C<size_t Parrot_io_get_buffer_size(PARROT_INTERP, PMC *filehandle)>

Get the C<buffer_size> attribute of the FileHandle object, which stores
the size of the buffer (in bytes).

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

PARROT_CAN_RETURN_NULL
size_t
Parrot_io_get_buffer_size(PARROT_INTERP, ARGIN(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_get_buffer_size)
    PMC * const handle = Parrot_io_get_os_handle_pmc(interp, filehandle);
    return PARROT_HANDLE(handle)->buffer_size;
}

/*

=item C<void Parrot_io_set_buffer_size(PARROT_INTERP, PMC *filehandle, size_t
new_size)>

Set the C<buffer_size> attribute of the FileHandle object, which stores
the size of the buffer (in bytes).

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

void
Parrot_io_set_buffer_size(PARROT_INTERP, ARGIN(PMC *filehandle), size_t new_size)
{
    ASSERT_ARGS(Parrot_io_set_buffer_size)
    PMC * const handle = Parrot_io_get_os_handle_pmc(interp, filehandle);
    PARROT_HANDLE(handle)->buffer_size = new_size;
}

/*

=item C<void Parrot_io_clear_buffer(PARROT_INTERP, PMC *filehandle)>

Clear the filehandle buffer and free the associated memory.

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

PARROT_CAN_RETURN_NULL
void
Parrot_io_clear_buffer(PARROT_INTERP, ARGIN(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_clear_buffer)
    PMC * const handle = Parrot_io_get_os_handle_pmc(interp, filehandle);
    const INTVAL flags = PARROT_FILEHANDLE(filehandle)->flags;
    Parrot_Handle_attributes * io = PARROT_HANDLE(handle);
    if (io->buffer_start && (flags & PIO_BF_MALLOC)) {
        mem_sys_free(io->buffer_start);
        io->buffer_start = NULL;
    }
}

/*

=item C<PIOOFF_T Parrot_io_get_file_position(PARROT_INTERP, PMC *filehandle)>

Get the C<file_pos> attribute of the FileHandle object, which stores
the current file position of the filehandle.

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

PARROT_EXPORT
PIOOFF_T
Parrot_io_get_file_position(PARROT_INTERP, ARGIN(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_get_file_position)
    PMC * const handle = Parrot_io_get_os_handle_pmc(interp, filehandle);
    return PARROT_HANDLE(handle)->file_pos;
}

/*

=item C<PIOOFF_T Parrot_io_get_last_file_position(PARROT_INTERP, PMC
*filehandle)>

Get the C<file_pos> attribute of the FileHandle object, which stores
the current file position of the filehandle.

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

PARROT_EXPORT
PIOOFF_T
Parrot_io_get_last_file_position(PARROT_INTERP, ARGIN(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_get_last_file_position)
    PMC * const handle = Parrot_io_get_os_handle_pmc(interp, filehandle);
    return PARROT_HANDLE(handle)->last_pos;
}

/*

=item C<void Parrot_io_set_file_position(PARROT_INTERP, PMC *filehandle,
PIOOFF_T file_pos)>

Get the C<file_pos> attribute of the FileHandle object, which stores the
current file position of the filehandle. Also set the C<last_pos> attribute to
the previous value of C<file_pos>.

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

PARROT_EXPORT
void
Parrot_io_set_file_position(PARROT_INTERP, ARGIN(PMC *filehandle), PIOOFF_T file_pos)
{
    ASSERT_ARGS(Parrot_io_set_file_position)
    PMC * const handle = Parrot_io_get_os_handle_pmc(interp, filehandle);
    Parrot_Handle_attributes *handle_struct = PARROT_HANDLE(handle);
    handle_struct->last_pos = handle_struct->file_pos;
    handle_struct->file_pos = file_pos;
}

/*

=item C<INTVAL Parrot_io_is_encoding(PARROT_INTERP, PMC *filehandle, STRING
*value)>

Check whether the encoding attribute of the filehandle matches a passed in
string.

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
INTVAL
Parrot_io_is_encoding(PARROT_INTERP, ARGIN(PMC *filehandle), ARGIN(STRING *value))
{
    ASSERT_ARGS(Parrot_io_is_encoding)
    STRING * encoding;
    GETATTR_FileHandle_encoding(interp, filehandle, encoding);
    if (STRING_IS_NULL(encoding))
        return 0;

    if (Parrot_str_equal(interp, value, encoding))
        return 1;

    return 0;
}

/*

=item C<INTVAL Parrot_io_close_filehandle(PARROT_INTERP, PMC *pmc)>

Flushes and closes the C<FileHandle> PMC C<*pmc>, but leaves the object intact
to be reused or collected.

=cut

*/

PARROT_EXPORT
INTVAL
Parrot_io_close_filehandle(PARROT_INTERP, ARGMOD(PMC *pmc))
{
    ASSERT_ARGS(Parrot_io_close_filehandle)
    INTVAL result;
    PMC * handle = Parrot_io_get_os_handle_pmc(interp, pmc);

    if (Parrot_io_is_closed_filehandle(interp, pmc))
        return -1;

    Parrot_io_flush_buffer(interp, handle);
    PIO_FLUSH_HANDLE(interp, handle);

    result = PIO_CLOSE(interp, pmc);
    Parrot_io_clear_buffer(interp, pmc);

    return result;
}

/*

=item C<INTVAL Parrot_io_is_closed_filehandle(PARROT_INTERP, PMC *pmc)>

Test whether a filehandle is closed.

=cut

*/

PARROT_EXPORT
INTVAL
Parrot_io_is_closed_filehandle(PARROT_INTERP, ARGMOD(PMC *pmc))
{
    ASSERT_ARGS(Parrot_io_is_closed_filehandle)
    return PIO_IS_CLOSED(interp, pmc);
}

/*

=item C<void Parrot_io_flush_filehandle(PARROT_INTERP, PMC *pmc)>

Flushes the C<FileHandle> PMC C<*pmc>.

=cut

*/

PARROT_EXPORT
void
Parrot_io_flush_filehandle(PARROT_INTERP, ARGMOD(PMC *pmc))
{
    ASSERT_ARGS(Parrot_io_flush_filehandle)
    PMC * handle = Parrot_io_get_os_handle_pmc(interp, pmc);
    if (Parrot_io_is_closed(interp, pmc))
        return;

    Parrot_io_flush_buffer(interp, handle);
    PIO_FLUSH_HANDLE(interp, handle);
}

/*

=back

=head1 SEE ALSO

F<src/io/unix.c>,
F<src/io/win32.c>,
F<src/io/stdio.c>,
F<src/io/io_private.h>.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
