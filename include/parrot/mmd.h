/* mmd.h
 *  Copyright: 2003 The Perl Foundation.  All Rights Reserved.
 *  CVS Info
 *     $Id$
 *  Overview:
 *     This is the api header for the mmd subsystem
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#if !defined(PARROT_MMD_H_GUARD)
#define PARROT_MMD_H_GUARD

/* inplace */
void mmd_dispatch_v_pp(Parrot_Interp, PMC *, PMC *, INTVAL);
void mmd_dispatch_v_pi(Parrot_Interp, PMC *, INTVAL, INTVAL);
void mmd_dispatch_v_pn(Parrot_Interp, PMC *, FLOATVAL, INTVAL);
void mmd_dispatch_v_ps(Parrot_Interp, PMC *, STRING *, INTVAL);

/* return result */
PMC* mmd_dispatch_p_ppp(Parrot_Interp, PMC *, PMC *, PMC *, INTVAL);
PMC* mmd_dispatch_p_pip(Parrot_Interp, PMC *, INTVAL, PMC *, INTVAL);
PMC* mmd_dispatch_p_pnp(Parrot_Interp, PMC *, FLOATVAL, PMC *, INTVAL);
PMC* mmd_dispatch_p_psp(Parrot_Interp, PMC *, STRING *, PMC *, INTVAL);

/* compare */
INTVAL mmd_dispatch_i_pp(Parrot_Interp, PMC *, PMC *, INTVAL);

/* function typedefs */
typedef PMC*    (*mmd_f_p_ppp)(Interp *, PMC *, PMC *, PMC *);
typedef PMC*    (*mmd_f_p_pip)(Interp *, PMC *, INTVAL, PMC *);
typedef PMC*    (*mmd_f_p_pnp)(Interp *, PMC *, FLOATVAL, PMC *);
typedef PMC*    (*mmd_f_p_psp)(Interp *, PMC *, STRING *, PMC *);

typedef void    (*mmd_f_v_pp)(Interp *, PMC *, PMC *);
typedef void    (*mmd_f_v_pi)(Interp *, PMC *, INTVAL);
typedef void    (*mmd_f_v_pn)(Interp *, PMC *, FLOATVAL);
typedef void    (*mmd_f_v_ps)(Interp *, PMC *, STRING *);

typedef INTVAL  (*mmd_f_i_pp) (Interp *, PMC *, PMC *);

void mmd_add_by_class(Parrot_Interp, INTVAL, STRING *, STRING *, funcptr_t);
void mmd_register(Parrot_Interp, INTVAL, INTVAL, INTVAL, funcptr_t);
void mmd_register_sub(Parrot_Interp, INTVAL, INTVAL, INTVAL, PMC*);
void mmd_add_function(Parrot_Interp, INTVAL, funcptr_t);
void mmd_add_function_sub(Parrot_Interp, INTVAL, PMC*);
void mmd_destroy(Parrot_Interp);
PMC *mmd_vtfind(Parrot_Interp, INTVAL, INTVAL, INTVAL);

typedef struct _MMD_init {
        INTVAL func_nr;
        INTVAL left, right;
        funcptr_t func_ptr;
} MMD_init;

void Parrot_mmd_register_table(Interp*, INTVAL, const MMD_init *, INTVAL);
void Parrot_mmd_rebuild_table(Interp*, INTVAL class_enum, INTVAL func_nr);

funcptr_t get_mmd_dispatch_type(Interp *interpreter,
        INTVAL function, INTVAL left_type, INTVAL right_type, int *is_pmc);

typedef struct _MMD_table {
    funcptr_t *mmd_funcs;     /* The functions for the MMD table */
    UINTVAL x;               /* The x coord for each table */
    UINTVAL y;               /* The y coord for each table */
} MMD_table;


PMC *Parrot_MMD_search_default_func(Interp *, STRING *meth, STRING *signature);
PMC *Parrot_MMD_dispatch_func(Interp *, PMC *multi, STRING *meth,
        STRING *signature);
PMC *Parrot_MMD_search_default_infix(Interp *, STRING *meth,
        INTVAL left_type, INTVAL right_type);

int Parrot_run_maybe_mmd_meth(Interp*, PMC *object, STRING *meth, STRING *sig);
int Parrot_run_maybe_mmd_func(Interp*, STRING *meth, STRING *sig);
/*
 * in src/objects.c :
 */
const char* Parrot_MMD_method_name(Interp* i, INTVAL idx);
INTVAL Parrot_MMD_method_idx(Interp* interpreter, char *name);

#endif /* PARROT_MMD_H_GUARD */

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
