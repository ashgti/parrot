/*
 * $Id$
 * Copyright (C) 2007, The Perl Foundation.
 */

/*

=head1 NAME

config/gen/platform/generic/stat.c

=head1 DESCRIPTION

File stat stuff

=head2 Functions

=over 4

=cut

*/


/*

=item C<PMC *
Parrot_stat_file(PARROT_INTERP, STRING *filename)>

RT#48260: Not yet documented!!!

=cut

*/

PMC *
Parrot_stat_file(PARROT_INTERP, STRING *filename)
{
    return NULL;
}

/*

=item C<PMC *
Parrot_stat_info_pmc(PARROT_INTERP, STRING *filename, INTVAL thing)>

RT#48260: Not yet documented!!!

=cut

*/

PMC *
Parrot_stat_info_pmc(PARROT_INTERP, STRING *filename, INTVAL thing)
{
    return NULL;
}

/*

=item C<static INTVAL
stat_common(PARROT_INTERP, struct stat *statbuf, INTVAL thing, int status)>

RT#48260: Not yet documented!!!

=cut

*/

static INTVAL
stat_common(PARROT_INTERP, struct stat *statbuf, INTVAL thing, int status)
{
    INTVAL result = -1;

    if (thing == STAT_EXISTS)
        return status == 0;

    if (status == -1) {
        const char *err = strerror(errno);
        real_exception(interp, NULL, E_IOError, "stat failed: %s", err);
    }

    switch (thing) {
        case STAT_FILESIZE:
            result = statbuf->st_size;
            break;
        case STAT_ISDIR:
            result = S_ISDIR(statbuf->st_mode);
            break;
        case STAT_ISDEV:
            result = S_ISCHR(statbuf->st_mode) || S_ISBLK(statbuf->st_mode);
            break;
        case STAT_CREATETIME:
            result = -1;
            break;
        case STAT_ACCESSTIME:
            result = statbuf->st_atime;
            break;
        case STAT_MODIFYTIME:
            result = statbuf->st_mtime;
            break;
        case STAT_CHANGETIME:
            result = statbuf->st_ctime;
            break;
        case STAT_BACKUPTIME:
            result = -1;
            break;
        case STAT_UID:
            result = statbuf->st_uid;
            break;
        case STAT_GID:
            result = statbuf->st_gid;
            break;
        case STAT_PLATFORM_DEV:
            result = statbuf->st_dev;
            break;
        case STAT_PLATFORM_INODE:
            result = statbuf->st_ino;
            break;
        case STAT_PLATFORM_MODE:
            result = statbuf->st_mode;
            break;
        case STAT_PLATFORM_NLINKS:
            result = statbuf->st_nlink;
            break;
        case STAT_PLATFORM_DEVTYPE:
            result = statbuf->st_rdev;
            break;
        case STAT_PLATFORM_BLOCKSIZE:
            result = statbuf->st_blksize;
            break;
        case STAT_PLATFORM_BLOCKS:
            result = statbuf->st_blocks;
            break;
        default:
            break;
    }

    return result;
}

/*

=item C<INTVAL
Parrot_stat_info_intval(PARROT_INTERP, STRING *file, INTVAL thing)>

RT#48260: Not yet documented!!!

=cut

*/

INTVAL
Parrot_stat_info_intval(PARROT_INTERP, STRING *file, INTVAL thing)
{
    struct stat statbuf;
    char *filename;
    int status;

    /* Get the name of the file as something we can use */
    filename = string_to_cstring(interp, file);

    /* Everything needs the result of stat, so just go do it */
    status = stat(filename, &statbuf);
    string_cstring_free(filename);
    return stat_common(interp, &statbuf, thing, status);
}

/*

=item C<INTVAL
Parrot_fstat_info_intval(PARROT_INTERP, INTVAL file, INTVAL thing)>

RT#48260: Not yet documented!!!

=cut

*/

INTVAL
Parrot_fstat_info_intval(PARROT_INTERP, INTVAL file, INTVAL thing)
{
    struct stat statbuf;
    int status;

    /* Everything needs the result of stat, so just go do it */
    status = fstat(file, &statbuf);
    return stat_common(interp, &statbuf, thing, status);
}

/*

=item C<FLOATVAL
Parrot_stat_info_floatval(PARROT_INTERP, STRING *filename, INTVAL thing)>

RT#48260: Not yet documented!!!

=cut

*/

FLOATVAL
Parrot_stat_info_floatval(PARROT_INTERP, STRING *filename, INTVAL thing)
{
    return (FLOATVAL)-1;
}

/*

=item C<STRING *
Parrot_stat_info_string(PARROT_INTERP, STRING *filename, INTVAL thing)>

RT#48260: Not yet documented!!!

=cut

*/

STRING *
Parrot_stat_info_string(PARROT_INTERP, STRING *filename, INTVAL thing)
{
    return NULL;
}

/*

=back

=cut

*/

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
