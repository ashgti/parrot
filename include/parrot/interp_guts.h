/*
** interp_guts.h
*/

#ifndef INTERP_GUTS_H
#define INTERP_GUTS_H

#define DO_OP(PC,INTERP) (PC = ((INTERP->op_func_table)[*PC])(PC,INTERP))

#endif /* INTERP_GUTS_H */

