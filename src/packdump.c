/*
** packdump.c
**
** Functions for dumping packfile structures
**
** Copyright (C) 2001-2002 Gregor N. Purdy. All rights reserved.
** This program is free software. It is subject to the same
** license as Parrot itself.
**
** $Id$
*
* Note: more dump functions are in packfile.c and pdump.c
*/

#include "parrot/parrot.h"
#include "parrot/packfile.h"

/*
** FIXME: this should also be segmentized.
** For now just remove some warnings
*/

void PackFile_ConstTable_dump(struct Parrot_Interp *,
                                     struct PackFile_ConstTable *);
static void PackFile_Constant_dump(struct Parrot_Interp *,
                                   struct PackFile_Constant *);
void PackFile_Fixup_dump(Parrot_Interp , struct PackFile_FixupTable *ft);

void
PackFile_ConstTable_dump(struct Parrot_Interp *interpreter,
                         struct PackFile_ConstTable *self)
{
    opcode_t i;

    for (i = 0; i < self->const_count; i++) {
        PIO_printf(interpreter, "    # %ld:\n", (long)i);
        PackFile_Constant_dump(interpreter, self->constants[i]);
    }
}

void
PackFile_Constant_dump(struct Parrot_Interp *interpreter,
                       struct PackFile_Constant *self)
{
    switch (self->type) {

    case PFC_NUMBER:
        PIO_printf(interpreter, "    [ 'PFC_NUMBER', %g ],\n", self->u.number);
        break;

    case PFC_STRING:
        PIO_printf(interpreter, "    [ 'PFC_STRING', {\n");
        PIO_printf(interpreter, "        FLAGS    => 0x%04lx,\n",
                   (long)PObj_get_FLAGS(self->u.string));
        PIO_printf(interpreter, "        ENCODING => %s,\n",
                   self->u.string->encoding->name);
        PIO_printf(interpreter, "        TYPE     => %s,\n",
                   self->u.string->type->name);
        PIO_printf(interpreter, "        SIZE     => %ld,\n",
                   (long)self->u.string->bufused);
        /* TODO: Won't do anything reasonable for most encodings */
        PIO_printf(interpreter, "        DATA     => '%.*s'\n",
                   (int)self->u.string->bufused,
                   (char *)self->u.string->strstart);
        PIO_printf(interpreter, "    } ],\n");
        break;

    case PFC_KEY:
        PIO_printf(interpreter, "    [ 'PFC_KEY', {\n");
        PIO_printf(interpreter, "    ??? TODO \n");
        PIO_printf(interpreter, "    } ],\n");
        break;
    case PFC_PMC:
        PIO_printf(interpreter, "    [ 'PFC_PMC', {\n");
        PIO_printf(interpreter, "    ??? TODO \n");
        PIO_printf(interpreter, "    } ],\n");
        break;
    default:
        PIO_printf(interpreter, "    [ 'PFC_????', type '0x%x' ],\n",
                self->type);
        break;
    }
}

void
PackFile_Fixup_dump(Parrot_Interp interpreter, struct PackFile_FixupTable *ft)
{
    opcode_t i;

    for (i = 0; i < ft->fixup_count; i++) {
        PIO_printf(interpreter,"\t#%d\n", (int) i);
        switch (ft->fixups[i]->type) {
            case enum_fixup_label:
            case enum_fixup_sub:
                PIO_printf(interpreter,
                        "\ttype => %d offs => %8d name => '%s',\n",
                        (int)ft->fixups[i]->type,
                        (int)ft->fixups[i]->offset,
                        ft->fixups[i]->name);
                    break;
            default:
                PIO_printf(interpreter,"\ttype => %d ???,\n",
                        (int) ft->fixups[i]->type);
                break;
        }
    }
}
/*
* Local variables:
* c-indentation-style: bsd
* c-basic-offset: 4
* indent-tabs-mode: nil
* End:
*
* vim: expandtab shiftwidth=4:
*/
