/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TK_HLL = 258,
     TK_LOADLIB = 259,
     TK_SUB = 260,
     TK_END = 261,
     TK_PARAM = 262,
     TK_LEX = 263,
     TK_LINE = 264,
     TK_FILE = 265,
     TK_LOCAL = 266,
     TK_NAMESPACE = 267,
     TK_INVOCANT = 268,
     TK_METH_CALL = 269,
     TK_GLOBALCONST = 270,
     TK_CONST = 271,
     TK_RETURN = 272,
     TK_YIELD = 273,
     TK_SET_YIELD = 274,
     TK_SET_RETURN = 275,
     TK_BEGIN_YIELD = 276,
     TK_END_YIELD = 277,
     TK_BEGIN_RETURN = 278,
     TK_END_RETURN = 279,
     TK_BEGIN_CALL = 280,
     TK_END_CALL = 281,
     TK_GET_RESULTS = 282,
     TK_CALL = 283,
     TK_SET_ARG = 284,
     TK_GET_RESULT = 285,
     TK_NCI_CALL = 286,
     TK_TAILCALL = 287,
     TK_ANNOTATE = 288,
     TK_NL = 289,
     TK_LABEL = 290,
     TK_IDENT = 291,
     TK_INT = 292,
     TK_NUM = 293,
     TK_PMC = 294,
     TK_STRING = 295,
     TK_IF = 296,
     TK_UNLESS = 297,
     TK_NULL = 298,
     TK_GOTO = 299,
     TK_STRINGC = 300,
     TK_USTRINGC = 301,
     TK_INTC = 302,
     TK_NUMC = 303,
     TK_PREG = 304,
     TK_NREG = 305,
     TK_SREG = 306,
     TK_IREG = 307,
     TK_ARROW = 308,
     TK_NE = 309,
     TK_EQ = 310,
     TK_LT = 311,
     TK_LE = 312,
     TK_GT = 313,
     TK_GE = 314,
     TK_USHIFT = 315,
     TK_RSHIFT = 316,
     TK_LSHIFT = 317,
     TK_FDIV = 318,
     TK_OR = 319,
     TK_AND = 320,
     TK_XOR = 321,
     TK_CONC = 322,
     TK_ASSIGN_USHIFT = 323,
     TK_ASSIGN_RSHIFT = 324,
     TK_ASSIGN_LSHIFT = 325,
     TK_ASSIGN_INC = 326,
     TK_ASSIGN_DEC = 327,
     TK_ASSIGN_MUL = 328,
     TK_ASSIGN_MOD = 329,
     TK_ASSIGN_POW = 330,
     TK_ASSIGN_DIV = 331,
     TK_ASSIGN_BOR = 332,
     TK_ASSIGN_BAND = 333,
     TK_ASSIGN_FDIV = 334,
     TK_ASSIGN_BNOT = 335,
     TK_ASSIGN_CONC = 336,
     TK_FLAG_INIT = 337,
     TK_FLAG_LOAD = 338,
     TK_FLAG_MAIN = 339,
     TK_FLAG_ANON = 340,
     TK_FLAG_METHOD = 341,
     TK_FLAG_OUTER = 342,
     TK_FLAG_VTABLE = 343,
     TK_FLAG_LEX = 344,
     TK_FLAG_MULTI = 345,
     TK_FLAG_POSTCOMP = 346,
     TK_FLAG_IMMEDIATE = 347,
     TK_FLAG_SUBID = 348,
     TK_FLAG_INSTANCEOF = 349,
     TK_FLAG_NSENTRY = 350,
     TK_FLAG_UNIQUE_REG = 351,
     TK_FLAG_NAMED = 352,
     TK_FLAG_SLURPY = 353,
     TK_FLAG_FLAT = 354,
     TK_FLAG_OPTIONAL = 355,
     TK_FLAG_OPT_FLAG = 356,
     TK_FLAG_INVOCANT = 357,
     TK_FLAG_LOOKAHEAD = 358,
     TK_MACRO = 359,
     TK_ENDM = 360,
     TK_MACRO_LOCAL = 361,
     TK_MACRO_LABEL = 362,
     TK_MACRO_CONST = 363,
     TK_MACRO_LABEL_ID = 364,
     TK_MACRO_LOCAL_ID = 365,
     TK_MACRO_IDENT = 366,
     TK_MACRO_ARG_IDENT = 367,
     TK_MACRO_ARG_OTHER = 368,
     TK_MACRO_CONST_VAL = 369,
     TK_PASM_MARKER_START = 370,
     TK_PIR_MARKER_START = 371,
     TK_PCC_SUB = 372,
     TK_PARROT_OP = 373
   };
#endif
/* Tokens.  */
#define TK_HLL 258
#define TK_LOADLIB 259
#define TK_SUB 260
#define TK_END 261
#define TK_PARAM 262
#define TK_LEX 263
#define TK_LINE 264
#define TK_FILE 265
#define TK_LOCAL 266
#define TK_NAMESPACE 267
#define TK_INVOCANT 268
#define TK_METH_CALL 269
#define TK_GLOBALCONST 270
#define TK_CONST 271
#define TK_RETURN 272
#define TK_YIELD 273
#define TK_SET_YIELD 274
#define TK_SET_RETURN 275
#define TK_BEGIN_YIELD 276
#define TK_END_YIELD 277
#define TK_BEGIN_RETURN 278
#define TK_END_RETURN 279
#define TK_BEGIN_CALL 280
#define TK_END_CALL 281
#define TK_GET_RESULTS 282
#define TK_CALL 283
#define TK_SET_ARG 284
#define TK_GET_RESULT 285
#define TK_NCI_CALL 286
#define TK_TAILCALL 287
#define TK_ANNOTATE 288
#define TK_NL 289
#define TK_LABEL 290
#define TK_IDENT 291
#define TK_INT 292
#define TK_NUM 293
#define TK_PMC 294
#define TK_STRING 295
#define TK_IF 296
#define TK_UNLESS 297
#define TK_NULL 298
#define TK_GOTO 299
#define TK_STRINGC 300
#define TK_USTRINGC 301
#define TK_INTC 302
#define TK_NUMC 303
#define TK_PREG 304
#define TK_NREG 305
#define TK_SREG 306
#define TK_IREG 307
#define TK_ARROW 308
#define TK_NE 309
#define TK_EQ 310
#define TK_LT 311
#define TK_LE 312
#define TK_GT 313
#define TK_GE 314
#define TK_USHIFT 315
#define TK_RSHIFT 316
#define TK_LSHIFT 317
#define TK_FDIV 318
#define TK_OR 319
#define TK_AND 320
#define TK_XOR 321
#define TK_CONC 322
#define TK_ASSIGN_USHIFT 323
#define TK_ASSIGN_RSHIFT 324
#define TK_ASSIGN_LSHIFT 325
#define TK_ASSIGN_INC 326
#define TK_ASSIGN_DEC 327
#define TK_ASSIGN_MUL 328
#define TK_ASSIGN_MOD 329
#define TK_ASSIGN_POW 330
#define TK_ASSIGN_DIV 331
#define TK_ASSIGN_BOR 332
#define TK_ASSIGN_BAND 333
#define TK_ASSIGN_FDIV 334
#define TK_ASSIGN_BNOT 335
#define TK_ASSIGN_CONC 336
#define TK_FLAG_INIT 337
#define TK_FLAG_LOAD 338
#define TK_FLAG_MAIN 339
#define TK_FLAG_ANON 340
#define TK_FLAG_METHOD 341
#define TK_FLAG_OUTER 342
#define TK_FLAG_VTABLE 343
#define TK_FLAG_LEX 344
#define TK_FLAG_MULTI 345
#define TK_FLAG_POSTCOMP 346
#define TK_FLAG_IMMEDIATE 347
#define TK_FLAG_SUBID 348
#define TK_FLAG_INSTANCEOF 349
#define TK_FLAG_NSENTRY 350
#define TK_FLAG_UNIQUE_REG 351
#define TK_FLAG_NAMED 352
#define TK_FLAG_SLURPY 353
#define TK_FLAG_FLAT 354
#define TK_FLAG_OPTIONAL 355
#define TK_FLAG_OPT_FLAG 356
#define TK_FLAG_INVOCANT 357
#define TK_FLAG_LOOKAHEAD 358
#define TK_MACRO 359
#define TK_ENDM 360
#define TK_MACRO_LOCAL 361
#define TK_MACRO_LABEL 362
#define TK_MACRO_CONST 363
#define TK_MACRO_LABEL_ID 364
#define TK_MACRO_LOCAL_ID 365
#define TK_MACRO_IDENT 366
#define TK_MACRO_ARG_IDENT 367
#define TK_MACRO_ARG_OTHER 368
#define TK_MACRO_CONST_VAL 369
#define TK_PASM_MARKER_START 370
#define TK_PIR_MARKER_START 371
#define TK_PCC_SUB 372
#define TK_PARROT_OP 373




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 236 "pir.y"
{
    double              dval;
    int                 ival;
    unsigned            uval;
    char   const       *sval;
    struct constant    *cval;
    struct constdecl   *cdec;
    struct ucstring    *ustr;
    struct instruction *instr;
    struct expression  *expr;
    struct target      *targ;
    struct argument    *argm;
    struct invocation  *invo;
    struct key         *key;
    struct symbol      *symb;
    struct macro_def   *mval;
    struct macro_param *pval;
}
/* Line 1489 of yacc.c.  */
#line 304 "pirparser.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



