/* exit.h
 *  Copyright: (When this is determined...it will go here)
 *  CVS Info
 *     $Id$
 *  Overview:
 *     
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 *      exit.c
 */

#if !defined(PARROT_EXIT_H_GUARD)
#define PARROT_EXIT_H_GUARD

int Parrot_on_exit(void (*function)(int , void *), void *arg);
void Parrot_exit(int status);

#endif

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
