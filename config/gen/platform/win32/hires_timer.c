/*
 * $Id$
 * Copyright (C) 2009, Parrot Foundation.
 */

/*

=head1 NAME

config/gen/platform/win32/hires_timer.c

=head1 DESCRIPTION

High-resolution timer support for win32

=head2 Functions

=over 4

=cut

*/


#define TIME_IN_NS(n) ((n).dwHighDateTime * 2^32 + (n).dwLowDateTime)

/*

=item C<UHUGEINTVAL Parrot_hires_get_time()>

Return a high-resolution number representing how long Parrot has been running.

=cut

*/

UHUGEINTVAL Parrot_hires_get_time()
{
    FILETIME creation, exit, kernel, user;
    GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user);
    return TIME_IN_NS(kernel) + TIME_IN_NS(user);
}

/*

=item C<UINTVAL Parrot_hires_get_resolution()>

Return the number of ns that each time unit from Parrot_hires_get_time represents.

=cut

*/

UINTVAL Parrot_hires_get_resolution()
{
    return 100;
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
