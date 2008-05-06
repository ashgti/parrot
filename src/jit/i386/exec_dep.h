/* Copyright (C) 2003-2008, The Perl Foundation.  */

/*
 * exec_dep.h
 *
 * SVN Info
 *    $Id$
 * Overview:
 *    i386 dependent functions to emit an executable.
 * History:
 *      Initial version by Daniel Grunblatt on 2003.6.9
 * Notes:
 * References:
 */

#ifndef PARROT_I386_EXEC_DEP_H_GUARD
#define PARROT_I386_EXEC_DEP_H_GUARD

/* HEADERIZER BEGIN: src/exec_dep.c */

void offset_fixup(Parrot_exec_objfile_t *obj);
void Parrot_exec_cpcf_op(Parrot_jit_info_t *jit_info, PARROT_INTERP)
        __attribute__nonnull__(2);

void Parrot_exec_normal_op(Parrot_jit_info_t *jit_info, PARROT_INTERP)
        __attribute__nonnull__(2);

void Parrot_exec_normal_op(Parrot_jit_info_t *jit_info, PARROT_INTERP)
        __attribute__nonnull__(2);

void Parrot_exec_restart_op(Parrot_jit_info_t *jit_info, PARROT_INTERP)
        __attribute__nonnull__(2);

/* HEADERIZER END: src/exec_dep.c */

#endif /* PARROT_I386_EXEC_DEP_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
