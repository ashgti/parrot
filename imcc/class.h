/* 
 * parrot/imcc/class.h
 *
 * Intermediate Code Compiler for Parrot.
 * 
 * Copyright (C) 2002 Melvin Smith
 *
 * Class management
 */

#ifndef __IMCC_CLASS_H
#define __IMCC_CLASS_H

typedef struct _Field {

  Symbol * sym;             /* Circular reference */

} Field;


typedef struct _Method {

  Symbol * sym;             /* Circular reference */

} Method;


typedef struct _Class {

  Symbol * sym;             /* Circular reference */
  SymbolTable * members;

} Class;


Class * new_class(Symbol *);
void store_field_symbol(Class * cl, Symbol * sym);
void store_method_symbol(Class * cl, Symbol * sym);
Symbol * lookup_field_symbol(Class *cl, const char * name);
Symbol * lookup_method_symbol(Class *cl, const char * name);


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

