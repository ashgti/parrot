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
#include "pmc/pmc_filehandle.h"

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

=item C<void Parrot_io_set_os_handle(PARROT_INTERP, PMC *filehandle, PIOHANDLE
file_descriptor)>

Sets the C<os_handle> attribute of the FileHandle object, which stores the
low-level filehandle for the OS.

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

Possibly, this function should reset some characteristics of the object (like
buffer and file positions) to their default values.

=cut

*/

PARROT_EXPORT
void
Parrot_io_set_os_handle(SHIM_INTERP, ARGIN(PMC *filehandle), PIOHANDLE file_descriptor)
{
    ASSERT_ARGS(Parrot_io_set_os_handle)
    PARROT_FILEHANDLE(filehandle)->os_handle = file_descriptor;
}

/*

=item C<PIOHANDLE Parrot_io_get_os_handle(PARROT_INTERP, PMC *filehandle)>

Retrieve the C<os_handle> attribute of the FileHandle object, which stores the
low-level filehandle for the OS.

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

PARROT_EXPORT
PIOHANDLE
Parrot_io_get_os_handle(SHIM_INTERP, ARGIN(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_get_os_handle)
    return PARROT_FILEHANDLE(filehandle)->os_handle;
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
Parrot_io_set_flags(SHIM_INTERP, ARGIN(PMC *filehandle), INTVAL flags)
{
    ASSERT_ARGS(Parrot_io_set_flags)
    Parrot_FileHandle_attributes *handle_struct = PARROT_FILEHANDLE(filehandle);
    handle_struct->flags = flags;
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
Parrot_io_get_flags(SHIM_INTERP, ARGIN(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_get_flags)
    Parrot_FileHandle_attributes *handle_struct = PARROT_FILEHANDLE(filehandle);
    INTVAL flags = handle_struct->flags;
    return flags;
}

/*

=item C<void Parrot_io_set_file_size(PARROT_INTERP, PMC *filehandle, PIOOFF_T
file_size)>

Set the C<file_size> attribute of the FileHandle object, which stores the
current file size.

Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

PARROT_EXPORT
void
Parrot_io_set_file_size(SHIM_INTERP, ARGIN(PMC *filehandle), PIOOFF_T file_size)
{
    ASSERT_ARGS(Parrot_io_set_file_size)
    PARROT_FILEHANDLE(filehandle)->file_size = file_size;
}


/*

=item C<PIOOFF_T Parrot_io_get_file_size(PARROT_INTERP, PMC *filehandle)>

Get the C<file_size> attribute of the FileHandle object, which stores the
current file size.


Currently, this pokes directly into the C struct of the FileHandle PMC. This
needs to change to a general interface that can be used by all subclasses and
polymorphic equivalents of FileHandle. For now, hiding it behind a function, so
it can be cleanly changed later.

=cut

*/

PARROT_EXPORT
PIOOFF_T
Parrot_io_get_file_size(SHIM_INTERP, ARGIN(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_get_file_size)
    return PARROT_FILEHANDLE(filehandle)->file_size;
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
Parrot_io_set_buffer_start(SHIM_INTERP, ARGIN(PMC *filehandle),
        ARGIN_NULLOK(unsigned char *new_start))
{
    ASSERT_ARGS(Parrot_io_set_buffer_start)
    PARROT_FILEHANDLE(filehandle)->buffer_start = new_start;
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
Parrot_io_get_buffer_start(SHIM_INTERP, ARGIN(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_get_buffer_start)
    return PARROT_FILEHANDLE(filehandle)->buffer_start;
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
Parrot_io_get_buffer_next(SHIM_INTERP, ARGIN(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_get_buffer_next)
    return PARROT_FILEHANDLE(filehandle)->buffer_next;
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
Parrot_io_set_buffer_next(SHIM_INTERP, ARGIN(PMC *filehandle),
        ARGIN_NULLOK(unsigned char *new_next))
{
    ASSERT_ARGS(Parrot_io_set_buffer_next)
    PARROT_FILEHANDLE(filehandle)->buffer_next = new_next;
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
Parrot_io_get_buffer_end(SHIM_INTERP, ARGIN_NULLOK(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_get_buffer_end)
    return PARROT_FILEHANDLE(filehandle)->buffer_end;
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
Parrot_io_set_buffer_end(SHIM_INTERP, ARGIN(PMC *filehandle),
        ARGIN_NULLOK(unsigned char *new_end))
{
    ASSERT_ARGS(Parrot_io_set_buffer_end)
    PARROT_FILEHANDLE(filehandle)->buffer_end = new_end;
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
Parrot_io_get_buffer_flags(SHIM_INTERP, ARGIN(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_get_buffer_flags)
    return PARROT_FILEHANDLE(filehandle)->buffer_flags;
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
Parrot_io_set_buffer_flags(SHIM_INTERP, ARGIN(PMC *filehandle), INTVAL new_flags)
{
    ASSERT_ARGS(Parrot_io_set_buffer_flags)
    PARROT_FILEHANDLE(filehandle)->buffer_flags = new_flags;
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
Parrot_io_get_buffer_size(SHIM_INTERP, ARGIN(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_get_buffer_size)
    return PARROT_FILEHANDLE(filehandle)->buffer_size;
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
Parrot_io_set_buffer_size(SHIM_INTERP, ARGIN(PMC *filehandle), size_t new_size)
{
    ASSERT_ARGS(Parrot_io_set_buffer_size)
    PARROT_FILEHANDLE(filehandle)->buffer_size = new_size;
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
Parrot_io_clear_buffer(SHIM_INTERP, ARGIN(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_clear_buffer)
    Parrot_FileHandle_attributes * const io = PARROT_FILEHANDLE(filehandle);
    if (io->buffer_start && (io->flags & PIO_BF_MALLOC)) {
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
Parrot_io_get_file_position(SHIM_INTERP, ARGIN(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_get_file_position)
    return PARROT_FILEHANDLE(filehandle)->file_pos;
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
Parrot_io_get_last_file_position(SHIM_INTERP, ARGIN(PMC *filehandle))
{
    ASSERT_ARGS(Parrot_io_get_last_file_position)
    return PARROT_FILEHANDLE(filehandle)->last_pos;
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
Parrot_io_set_file_position(SHIM_INTERP, ARGIN(PMC *filehandle), PIOOFF_T file_pos)
{
    ASSERT_ARGS(Parrot_io_set_file_position)
    Parrot_FileHandle_attributes * const handle_struct = PARROT_FILEHANDLE(filehandle);
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
    Parrot_FileHandle_attributes * const handle_struct = PARROT_FILEHANDLE(filehandle);
    if (STRING_IS_NULL(handle_struct->encoding))
        return 0;

    if (Parrot_str_equal(interp, value, handle_struct->encoding))
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

    if (Parrot_io_is_closed_filehandle(interp, pmc))
        return -1;

    Parrot_io_flush_buffer(interp, pmc);
    PIO_FLUSH(interp, pmc);

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
    if (Parrot_io_is_closed(interp, pmc))
        return;

    Parrot_io_flush_buffer(interp, pmc);
    PIO_FLUSH(interp, pmc);
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
