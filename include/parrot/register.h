/* register.h
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  CVS Info
 *     $Id$
 *  Overview:
 *     Defines the register api
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#if !defined(PARROT_REGISTER_H_GUARD)
#define PARROT_REGISTER_H_GUARD

#include "parrot/string.h"
#include "parrot/enums.h"

struct IReg {
    INTVAL registers[NUM_REGISTERS];
};

struct IRegFrame {
    INTVAL registers[NUM_REGISTERS/2];
};

struct NReg {
    FLOATVAL registers[NUM_REGISTERS];
};

struct NRegFrame {
    FLOATVAL registers[NUM_REGISTERS/2];
};

struct SReg {
    STRING *registers[NUM_REGISTERS];
};

struct SRegFrame {
    STRING *registers[NUM_REGISTERS/2];
};

struct PReg {
    PMC *registers[NUM_REGISTERS];
};

struct PRegFrame {
    PMC *registers[NUM_REGISTERS/2];
};

struct IRegChunkBuf {
    struct IRegFrame IRegFrame[FRAMES_PER_CHUNK];
};

struct NRegChunkBuf {
    struct NRegFrame NRegFrame[FRAMES_PER_CHUNK];
};

struct SRegChunkBuf {
    struct SRegFrame SRegFrame[FRAMES_PER_CHUNK];
};

struct PRegChunkBuf {
    struct PRegFrame PRegFrame[FRAMES_PER_CHUNK];
};

struct Stack_Chunk;
struct Parrot_Context;

void setup_register_stacks(struct Parrot_Interp*, struct Parrot_Context *);
void mark_pmc_register_stack(struct Parrot_Interp* interpreter,
                             struct Stack_Chunk* stack);
void mark_string_register_stack(struct Parrot_Interp* interpreter,
                                struct Stack_Chunk* stack);
void mark_register_stack(struct Parrot_Interp* interpreter,
                         struct Stack_Chunk* stack);

#endif /* PARROT_REGISTER_H */

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
