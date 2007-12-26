/*
 * Copyright (C) 2001-2007, The Perl Foundation
 */

/* packfile.h
*
* $Id$
*
* History:
*  Rework by Melvin; new bytecode format, make bytecode portable.
*   (Do endian conversion and wordsize transforms on the fly.)
*/

#ifndef PARROT_PACKFILE_H_GUARD
#define PARROT_PACKFILE_H_GUARD

#include "parrot/parrot.h"

#define PF_NCONST(pf)  ((pf)->const_table->const_count)
#define PF_CONST(pf, i) ((pf)->const_table->constants[(i)])

#define DIRECTORY_SEGMENT_NAME   "DIRECTORY"
#define FIXUP_TABLE_SEGMENT_NAME "FIXUP"
#define CONSTANT_SEGMENT_NAME    "CONSTANT"
#define BYTE_CODE_SEGMENT_NAME   "BYTECODE"

/*
** Structure Definitions:
*/


/*
** The number of bytes in the packfile header that we can read/write
** directly. This excludes the final element in the struct, which holds a
** pointer to the UUID data, if there is any.
*/
#define PACKFILE_HEADER_BYTES 18

typedef struct PackFile_Header {
    /* Magic string to identify the PBC file. */
    unsigned char magic[8];

    /* Word size, byte ordering and floating point number format. */
    unsigned char wordsize;
    unsigned char byteorder;
    unsigned char floattype;

    /* Version of Parrot that wrote the bytecode file. */
    unsigned char major;
    unsigned char minor;
    unsigned char patch;

    /* Bytecode format version. */
    unsigned char bc_major;
    unsigned char bc_minor;

    /* UUID type and length. */
    unsigned char uuid_type;
    unsigned char uuid_size;

    /* The UUID data. */
    unsigned char *uuid_data;

    /* Directory format. */
    opcode_t dir_format;
} PackFile_Header;

typedef struct PackFile_Constant {
    opcode_t type;
    union {
        opcode_t integer;
        FLOATVAL number;
        STRING *string;
        PMC *key;
    } u;
} PackFile_Constant;

typedef struct PackFile_DebugMapping {
    opcode_t offset;
    opcode_t mapping_type;
    union {
        opcode_t filename;
        opcode_t source_seg; /* XXX Source segments currently unimplemented. */
    } u;
} PackFile_DebugMapping;

/*
** PackFile Segment:
*    The base type of every section
*    include it as first element of every derivated Segment
*/

typedef struct PackFile_Segment * (*PackFile_Segment_new_func_t)
    (PARROT_INTERP, struct PackFile *, const char *, int add);
typedef void (*PackFile_Segment_destroy_func_t)
    (PARROT_INTERP, struct PackFile_Segment *);
typedef size_t (*PackFile_Segment_packed_size_func_t)
    (PARROT_INTERP, const struct PackFile_Segment *);
typedef opcode_t * (*PackFile_Segment_pack_func_t)
    (PARROT_INTERP, struct PackFile_Segment *, opcode_t *dest);
typedef opcode_t * (*PackFile_Segment_unpack_func_t)
    (PARROT_INTERP, struct PackFile_Segment *, opcode_t *packed);
typedef void (*PackFile_Segment_dump_func_t)
    (PARROT_INTERP, struct PackFile_Segment *);

typedef struct PackFile_funcs {
    PackFile_Segment_new_func_t         new_seg;
    PackFile_Segment_destroy_func_t     destroy;
    PackFile_Segment_packed_size_func_t packed_size;
    PackFile_Segment_pack_func_t        pack;
    PackFile_Segment_unpack_func_t      unpack;
    PackFile_Segment_dump_func_t        dump;
} PackFile_funcs;

typedef enum {
    PF_DIR_SEG,
    PF_UNKNOWN_SEG,
    PF_FIXUP_SEG,
    PF_CONST_SEG,
    PF_BYTEC_SEG,
    PF_DEBUG_SEG,

    PF_MAX_SEG
} pack_file_types;

#define PF_DIR_FORMAT 1

typedef struct PackFile_Segment {
    struct PackFile           * pf;
    struct PackFile_Directory * dir;

    /* directory information */
    UINTVAL             type;           /* one of above defined types */
    char                *name;
    size_t              op_count;       /* external size in ops */
    size_t              file_offset;    /* offset in ops */
    /* common payload of all bytecode chunks
     * with the size above these four items are aligned to 16 byte */
    opcode_t            itype;          /* internal type/version */
    opcode_t            id;             /* internal id */
    size_t              size;           /* internal oparray size */
    opcode_t            *data;         /* oparray e.g. bytecode */
} PackFile_Segment;

typedef INTVAL (*PackFile_map_segments_func_t)
    (PARROT_INTERP, PackFile_Segment *seg, void *user_data);

/*
** PackFile_FixupTable:
*/
typedef struct PackFile_FixupEntry {
    opcode_t                  type;     /* who knows what fixups we need? */
    const char               *name;     /* name of the label */
    opcode_t                  offset;   /* location of the item */
    struct PackFile_ByteCode *seg;
} PackFile_FixupEntry;

typedef enum {
    enum_fixup_none,
    enum_fixup_label,
    enum_fixup_sub
} enum_fixup_t;

typedef struct PackFile_FixupTable {
    PackFile_Segment             base;
    opcode_t                     fixup_count;
    PackFile_FixupEntry        **fixups;
    struct PackFile_ByteCode    *code;   /* where this segment belongs to */
} PackFile_FixupTable;

#define PFC_NONE    '\0'
/* no ascii chars use numbers: for n, s, k, p */
#define PFC_NUMBER  '\156'
#define PFC_STRING  '\163'
#define PFC_KEY     '\153'
#define PFC_PMC     '\160'

enum PF_VARTYPE {                  /* s. also imcc/symreg.h */
    PF_VT_START_SLICE = 1 << 10,   /* x .. y slice range */
    PF_VT_END_SLICE   = 1 << 11,
    PF_VT_START_ZERO  = 1 << 12,   /* .. y 0..start */
    PF_VT_END_INF     = 1 << 13,   /* x..  start..inf */
    PF_VT_SLICE_BITS  = PF_VT_START_SLICE | PF_VT_END_SLICE |
                        PF_VT_START_ZERO | PF_VT_END_INF
};

typedef struct PackFile_ConstTable {
    PackFile_Segment           base;
    opcode_t                   const_count;
    PackFile_Constant        **constants;
    struct PackFile_ByteCode  *code;  /* where this segment belongs to */
} PackFile_ConstTable;

typedef struct PackFile_ByteCode {
    PackFile_Segment       base;
    Prederef               prederef;    /* The predereferenced code and info */
    struct Parrot_jit_info_t     *jit_info;    /* JITs data */
    Parrot_PIC_store      *pic_store;   /* PIC storage */
    PackFile_Segment      *pic_index;   /* segment of indices into store */
    struct PackFile_Debug *debugs;
    PackFile_ConstTable   *const_table;
    PackFile_FixupTable   *fixups;
} PackFile_ByteCode;

enum PF_DEBUGMAPPINGTYPE {
    PF_DEBUGMAPPINGTYPE_NONE = 0,
    PF_DEBUGMAPPINGTYPE_FILENAME,
    PF_DEBUGMAPPINGTYPE_SOURCESEG
};

typedef struct PackFile_Debug {
    PackFile_Segment        base;
    opcode_t                num_mappings;
    PackFile_DebugMapping **mappings;
    PackFile_ByteCode      *code;   /* where this segment belongs to */
} PackFile_Debug;

typedef struct PackFile_Directory {
    PackFile_Segment   base;
    size_t             num_segments;
    PackFile_Segment **segments;
} PackFile_Directory;

typedef struct PackFile {
    /* the packfile is its own directory */
    PackFile_Directory   directory;
    PackFile_Directory   *dirp;  /* for freeing */
    opcode_t *src;              /* the possible mmap()ed start of the PF */
    size_t   size;              /* size in bytes */
    INTVAL is_mmap_ped;         /* don't free it, munmap it at destroy */

    PackFile_Header     * header;

    /* directory hold all Segments */
    /* TODO make this reallocatable */
    PackFile_funcs      PackFuncs[PF_MAX_SEG];

    PackFile_ByteCode  * cur_cs;   /* used during PF loading */

    INTVAL    need_wordsize;
    INTVAL    need_endianize;
    opcode_t  (*fetch_op)(ARGIN(const unsigned char *));
    INTVAL    (*fetch_iv)(ARGIN(const unsigned char *));
    void      (*fetch_nv)(ARGOUT(unsigned char *), ARGIN(const unsigned char *));
} PackFile;


typedef enum {
    PBC_MAIN   = 1,
    PBC_LOADED = 2,
    PBC_PBC    = 4,
    PBC_IMMEDIATE = 8,
    PBC_POSTCOMP  = 16,
    PBC_INIT  = 32
} pbc_action_enum_t;

/* HEADERIZER BEGIN: src/packout.c */

PARROT_API
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
opcode_t * PackFile_Constant_pack(PARROT_INTERP,
    ARGIN(const PackFile_ConstTable *const_table),
    ARGIN(const PackFile_Constant *self),
    ARGINOUT(opcode_t *cursor))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
opcode_t * PackFile_ConstTable_pack(PARROT_INTERP,
    ARGIN(const PackFile_Segment *seg),
    ARGINOUT(opcode_t *cursor))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
size_t PackFile_ConstTable_pack_size(PARROT_INTERP,
    ARGIN(const PackFile_Segment *seg))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
int PackFile_find_in_const(PARROT_INTERP,
    ARGIN(const PackFile_ConstTable *ct),
    ARGIN(const PMC *key),
    int type)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
void PackFile_pack(PARROT_INTERP,
    ARGINOUT(PackFile *self),
    ARGINOUT(opcode_t *cursor))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
opcode_t PackFile_pack_size(PARROT_INTERP, ARGIN(const PackFile *self))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

/* HEADERIZER END: src/packout.c */

/* HEADERIZER BEGIN: src/packfile.c */

PARROT_API
void do_sub_pragmas(PARROT_INTERP,
    ARGIN(PackFile_ByteCode *self),
    int action,
    ARGIN_NULLOK(PMC *eval_pmc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
INTVAL PackFile_add_segment(SHIM_INTERP,
    ARGINOUT(PackFile_Directory *dir),
    ARGIN(PackFile_Segment *seg))
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
void PackFile_Constant_destroy(SHIM_INTERP, NULLOK(PackFile_Constant *self));

PARROT_API
PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
PackFile_Constant * PackFile_Constant_new(SHIM_INTERP);

PARROT_API
PARROT_WARN_UNUSED_RESULT
size_t PackFile_Constant_pack_size(PARROT_INTERP,
    NOTNULL(PackFile_Constant *self))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
opcode_t * PackFile_Constant_unpack(PARROT_INTERP,
    ARGIN(PackFile_ConstTable *constt),
    ARGOUT(PackFile_Constant *self),
    ARGIN(opcode_t *cursor))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
opcode_t * PackFile_Constant_unpack_key(PARROT_INTERP,
    NOTNULL(PackFile_ConstTable *constt),
    NOTNULL(PackFile_Constant *self),
    NOTNULL(opcode_t *cursor))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
opcode_t * PackFile_Constant_unpack_pmc(PARROT_INTERP,
    NOTNULL(PackFile_ConstTable *constt),
    NOTNULL(PackFile_Constant *self),
    NOTNULL(opcode_t *cursor))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4);

PARROT_API
void PackFile_ConstTable_clear(PARROT_INTERP,
    ARGINOUT(PackFile_ConstTable *self))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
opcode_t * PackFile_ConstTable_unpack(PARROT_INTERP,
    NOTNULL(PackFile_Segment *seg),
    NOTNULL(opcode_t *cursor))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
void PackFile_destroy(PARROT_INTERP, ARGINOUT_NULLOK(PackFile *pf))
        __attribute__nonnull__(1);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PackFile_FixupEntry * PackFile_find_fixup_entry(PARROT_INTERP,
    INTVAL type,
    ARGIN(const char *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PackFile_Segment * PackFile_find_segment(PARROT_INTERP,
    NOTNULL(PackFile_Directory *dir),
    ARGIN(const char *name),
    int sub_dir)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
void PackFile_fixup_subs(PARROT_INTERP,
    pbc_action_enum_t what,
    ARGIN_NULLOK(PMC *eval))
        __attribute__nonnull__(1);

PARROT_API
void PackFile_FixupTable_clear(PARROT_INTERP,
    ARGINOUT(PackFile_FixupTable *self))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void PackFile_FixupTable_new_entry(PARROT_INTERP,
    ARGIN(const char *label),
    INTVAL type,
    opcode_t offs)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
INTVAL PackFile_funcs_register(SHIM_INTERP,
    ARGOUT(PackFile *pf),
    UINTVAL type,
    PackFile_funcs funcs)
        __attribute__nonnull__(2);

PARROT_API
INTVAL PackFile_map_segments(PARROT_INTERP,
    ARGIN(const PackFile_Directory *dir),
    PackFile_map_segments_func_t callback,
    ARGIN_NULLOK(void *user_data))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PackFile * PackFile_new(PARROT_INTERP, INTVAL is_mapped)
        __attribute__nonnull__(1);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PackFile * PackFile_new_dummy(PARROT_INTERP, ARGIN(const char *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PackFile_Segment * PackFile_remove_segment_by_name(SHIM_INTERP,
    NOTNULL(PackFile_Directory *dir),
    ARGIN(const char *name))
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
void PackFile_Segment_destroy(PARROT_INTERP,
    NOTNULL(PackFile_Segment *self))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void PackFile_Segment_dump(PARROT_INTERP, ARGIN(PackFile_Segment *self))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PackFile_Segment * PackFile_Segment_new(SHIM_INTERP,
    SHIM(PackFile *pf),
    SHIM(const char *name),
    NULLOK(int add));

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PackFile_Segment * PackFile_Segment_new_seg(PARROT_INTERP,
    ARGINOUT(PackFile_Directory *dir),
    UINTVAL type,
    ARGIN(const char *name),
    int add)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(4);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
opcode_t * PackFile_Segment_pack(PARROT_INTERP,
    ARGIN(PackFile_Segment *self),
    ARGIN(opcode_t *cursor))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
size_t PackFile_Segment_packed_size(PARROT_INTERP,
    ARGIN(const PackFile_Segment *self))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
opcode_t * PackFile_Segment_unpack(PARROT_INTERP,
    NOTNULL(PackFile_Segment *self),
    NOTNULL(opcode_t *cursor))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
PARROT_WARN_UNUSED_RESULT
opcode_t PackFile_unpack(PARROT_INTERP,
    ARGINOUT(PackFile *self),
    ARGIN(opcode_t *packed),
    size_t packed_size)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
void Parrot_debug_add_mapping(PARROT_INTERP,
    NOTNULL(PackFile_Debug *debug),
    opcode_t offset,
    int mapping_type,
    ARGIN(const char *filename),
    int source_seg)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(5);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING * Parrot_debug_pc_to_filename(PARROT_INTERP,
    NOTNULL(PackFile_Debug *debug),
    opcode_t pc)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void Parrot_destroy_constants(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_load_bytecode(PARROT_INTERP, ARGIN(STRING *file_str))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PackFile_Debug * Parrot_new_debug_seg(PARROT_INTERP,
    NOTNULL(PackFile_ByteCode *cs),
    size_t size)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
PackFile_ByteCode * Parrot_switch_to_cs(PARROT_INTERP,
    NOTNULL(PackFile_ByteCode *new_cs),
    int really)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void Parrot_switch_to_cs_by_nr(PARROT_INTERP, opcode_t seg)
        __attribute__nonnull__(1);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PackFile_ByteCode * PF_create_default_segs(PARROT_INTERP,
    ARGIN(const char *file_name),
    int add)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void default_dump_header(PARROT_INTERP, ARGIN(const PackFile_Segment *self))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void mark_const_subs(PARROT_INTERP)
        __attribute__nonnull__(1);

/* HEADERIZER END: src/packfile.c */


/* HEADERIZER BEGIN: src/packdump.c */

PARROT_API
void PackFile_ConstTable_dump(PARROT_INTERP,
    ARGIN(const PackFile_ConstTable *self))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void PackFile_Fixup_dump(PARROT_INTERP,
    ARGIN(const PackFile_FixupTable *ft))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

/* HEADERIZER END: src/packdump.c */

/* HEADERIZER BEGIN: src/fingerprint.c */

PARROT_API
PARROT_PURE_FUNCTION
int PackFile_check_fingerprint(ARGIN(const void *cursor))
        __attribute__nonnull__(1);

PARROT_API
size_t PackFile_write_fingerprint(NOTNULL(void *cursor))
        __attribute__nonnull__(1);

/* HEADERIZER END: src/fingerprint.c */


/* HEADERIZER BEGIN: src/packfile/pf_items.c */

void PackFile_assign_transforms(ARGINOUT(PackFile *pf))
        __attribute__nonnull__(1);

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
char * PF_fetch_cstring(ARGIN(PackFile *pf), ARGIN(opcode_t **cursor))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
INTVAL PF_fetch_integer(
    ARGIN_NULLOK(PackFile *pf),
    ARGIN(opcode_t **stream))
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
FLOATVAL PF_fetch_number(
    ARGIN_NULLOK(PackFile *pf),
    ARGIN(opcode_t **stream))
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
opcode_t PF_fetch_opcode(
    ARGIN_NULLOK(const PackFile *pf),
    ARGIN(opcode_t **stream))
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING * PF_fetch_string(PARROT_INTERP,
    ARGIN_NULLOK(PackFile *pf),
    ARGIN(opcode_t **cursor))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

PARROT_PURE_FUNCTION
size_t PF_size_cstring(ARGIN(const char *s))
        __attribute__nonnull__(1);

PARROT_CONST_FUNCTION
size_t PF_size_integer(void);

PARROT_CONST_FUNCTION
size_t PF_size_number(void);

PARROT_CONST_FUNCTION
size_t PF_size_opcode(void);

PARROT_PURE_FUNCTION
size_t PF_size_string(ARGIN(const STRING *s))
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
opcode_t* PF_store_cstring(ARGOUT(opcode_t *cursor), ARGIN(const char *s))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
opcode_t* PF_store_integer(ARGOUT(opcode_t *cursor), INTVAL val)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
opcode_t* PF_store_number(
    ARGOUT(opcode_t *cursor),
    ARGIN(const FLOATVAL *val))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
opcode_t* PF_store_opcode(ARGOUT(opcode_t *cursor), opcode_t val)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
opcode_t* PF_store_string(ARGOUT(opcode_t *cursor), ARGIN(const STRING *s))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

/* HEADERIZER END: src/packfile/pf_items.c */



/*
** Byte Ordering Functions (byteorder.c)
*/

/* HEADERIZER BEGIN: src/byteorder.c */

void fetch_buf_be_12(
    ARGOUT(unsigned char *rb),
    ARGIN(const unsigned char *b))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void fetch_buf_be_16(
    ARGOUT(unsigned char *rb),
    ARGIN(const unsigned char *b))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void fetch_buf_be_4(
    ARGOUT(unsigned char *rb),
    ARGIN(const unsigned char *b))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void fetch_buf_be_8(
    ARGOUT(unsigned char *rb),
    ARGIN(const unsigned char *b))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void fetch_buf_le_12(
    ARGOUT(unsigned char *rb),
    ARGIN(const unsigned char *b))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void fetch_buf_le_16(
    ARGOUT(unsigned char *rb),
    ARGIN(const unsigned char *b))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void fetch_buf_le_4(
    ARGOUT(unsigned char *rb),
    ARGIN(const unsigned char *b))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void fetch_buf_le_8(
    ARGOUT(unsigned char *rb),
    ARGIN(const unsigned char *b))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CONST_FUNCTION
INTVAL fetch_iv_be(INTVAL w);

PARROT_WARN_UNUSED_RESULT
PARROT_CONST_FUNCTION
INTVAL fetch_iv_le(INTVAL w);

PARROT_WARN_UNUSED_RESULT
PARROT_CONST_FUNCTION
opcode_t fetch_op_be(opcode_t w);

PARROT_WARN_UNUSED_RESULT
PARROT_CONST_FUNCTION
opcode_t fetch_op_le(opcode_t w);

/* HEADERIZER END: src/byteorder.c */

#endif /* PARROT_PACKFILE_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
