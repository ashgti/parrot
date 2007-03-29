/*
Copyright (C) 2001-2005, The Perl Foundation.
This program is free software. It is subject to the same license as
Parrot itself.
$Id$

=head1 NAME

src/packdump.c - Functions for dumping packfile structures

=head1 DESCRIPTION

This is only used by the PBC dumper C<pdump>.

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "parrot/packfile.h"

/*
** FIXME: this should also be segmentized.
** For now just remove some warnings
*/

void PackFile_ConstTable_dump(Interp *,
                                     struct PackFile_ConstTable *);
static void PackFile_Constant_dump(Interp *, struct PackFile_ConstTable *ct,
                                   struct PackFile_Constant *);
void PackFile_Fixup_dump(Interp *,
                         struct PackFile_FixupTable *ft);

/*

=item C<void
PackFile_ConstTable_dump(Interp *interp,
                         struct PackFile_ConstTable *self)>

Dumps the constant table C<self>.

=cut

*/

void
PackFile_ConstTable_dump(Interp *interp,
                         struct PackFile_ConstTable *self)
{
    opcode_t i;

    for (i = 0; i < self->const_count; i++) {
        PIO_printf(interp, "    # %ld:\n", (long)i);
        PackFile_Constant_dump(interp, self, self->constants[i]);
    }
}

/*

=item C<void
PackFile_Constant_dump(Interp *interp, struct PackFile_ConstTable *ct
                       struct PackFile_Constant *self)>

Dumps the constant C<self>.

=cut

*/

void
PackFile_Constant_dump(Interp *interp, struct PackFile_ConstTable *ct,
                       struct PackFile_Constant *self)
{
    struct PMC *key;
    size_t i;
    size_t ct_index;
    opcode_t slice_bits;
    struct PackFile_Constant *detail;

    switch (self->type) {

    case PFC_NUMBER:
        PIO_printf(interp, "    [ 'PFC_NUMBER', %g ],\n", self->u.number);
        break;

    case PFC_STRING:
        PIO_printf(interp, "    [ 'PFC_STRING', {\n");
        PIO_printf(interp, "        FLAGS    => 0x%04lx,\n",
                   (long)PObj_get_FLAGS(self->u.string));
        PIO_printf(interp, "        CHARSET  => %ld,\n",
                   self->u.string->charset);
        PIO_printf(interp, "        SIZE     => %ld,\n",
                   (long)self->u.string->bufused);
        /* TODO: Won't do anything reasonable for most encodings */
        PIO_printf(interp, "        DATA     => '%.*s'\n",
                   (int)self->u.string->bufused,
                   (char *)self->u.string->strstart);
        PIO_printf(interp, "    } ],\n");
        break;

    case PFC_KEY:
        PIO_printf(interp, "    [ 'PFC_KEY");
        for (i = 0, key = self->u.key; key; key = PMC_data(key), i++)
            ;
        /* number of key components */
        PIO_printf(interp, " %ld items\n", i);
        /* and now type / value per component */
        for (key = self->u.key; key; key = PMC_data(key)) {
            opcode_t type = PObj_get_FLAGS(key);
            PIO_printf(interp, "       {\n");
            slice_bits = 0;
            if ((type & (KEY_start_slice_FLAG|KEY_inf_slice_FLAG)) ==
                (KEY_start_slice_FLAG|KEY_inf_slice_FLAG))
                PIO_printf(interp, "        SLICE_BITS  => PF_VT_END_INF\n");
            if ((type & (KEY_end_slice_FLAG|KEY_inf_slice_FLAG)) ==
                (KEY_end_slice_FLAG|KEY_inf_slice_FLAG))
                slice_bits |= PF_VT_START_ZERO;
                PIO_printf(interp, "        SLICE_BITS  => PF_VT_START_ZERO\n");
            if (type & KEY_start_slice_FLAG)
                slice_bits |= PF_VT_START_SLICE;
                PIO_printf(interp, "        SLICE_BITS  => PF_VT_START_SLICE\n");
            if (type & KEY_end_slice_FLAG)
                slice_bits |= PF_VT_END_SLICE;
                PIO_printf(interp, "        SLICE_BITS  => PF_VT_END_SLICE\n");

            type &= KEY_type_FLAGS;
            PIO_printf(interp, "        FLAGS       => 0x%04lx,\n", (long)PObj_get_FLAGS(key));
            switch (type) {
                case KEY_integer_FLAG:
                    PIO_printf(interp, "        TYPE        => INTEGER\n");
                    PIO_printf(interp, "        DATA        => %ld\n", PMC_int_val(key));
                    PIO_printf(interp, "       },\n");
                    break;
                case KEY_number_FLAG:
                    PIO_printf(interp, "        TYPE        => NUMBER\n");
                    ct_index = PackFile_find_in_const(interp, ct, key, PFC_NUMBER);
                    PIO_printf(interp, "        PFC_OFFSET  => %ld\n", ct_index);
                    detail = ct->constants[ct_index];
                    PIO_printf(interp, "        DATA        => %ld\n", detail->u.number);
                    PIO_printf(interp, "       },\n");
                    break;
                case KEY_string_FLAG:
                    PIO_printf(interp, "        TYPE        => STRING\n");
                    ct_index = PackFile_find_in_const(interp, ct, key, PFC_STRING);
                    PIO_printf(interp, "        PFC_OFFSET  => %ld\n", ct_index);
                    detail = ct->constants[ct_index];
                    PIO_printf(interp, "        DATA        => '%.*s'\n",
                              (int)detail->u.string->bufused,
                              (char *)detail->u.string->strstart);
                    PIO_printf(interp, "       },\n");
                    break;
                case KEY_integer_FLAG | KEY_register_FLAG:
                    PIO_printf(interp, "        TYPE        => I REGISTER\n");
                    PIO_printf(interp, "        DATA        => %ld\n", PMC_int_val(key));
                    PIO_printf(interp, "       },\n");
                    break;
                case KEY_number_FLAG | KEY_register_FLAG:
                    PIO_printf(interp, "        TYPE        => N REGISTER\n");
                    PIO_printf(interp, "        DATA        => %ld\n", PMC_int_val(key));
                    PIO_printf(interp, "       },\n");
                    break;
                case KEY_string_FLAG | KEY_register_FLAG:
                    PIO_printf(interp, "        TYPE        => S REGISTER\n");
                    PIO_printf(interp, "        DATA        => %ld\n", PMC_int_val(key));
                    PIO_printf(interp, "       },\n");
                    break;
                case KEY_pmc_FLAG | KEY_register_FLAG:
                    PIO_printf(interp, "        TYPE        => P REGISTER\n");
                    PIO_printf(interp, "        DATA        => %ld\n", PMC_int_val(key));
                    PIO_printf(interp, "       },\n");
                    break;
                default:
                    PIO_eprintf(NULL, "PackFile_Constant_pack: "
                            "unsupported constant type\n");
                    Parrot_exit(interp, 1);
            }
        }
        PIO_printf(interp, "    ],\n");
        break;
    case PFC_PMC:
        PIO_printf(interp, "    [ 'PFC_PMC', {\n");
        {
            PMC *pmc = self->u.key;
            parrot_sub_t sub;
            STRING *a_key = const_string(interp, "(keyed)");
            STRING *null = const_string(interp, "(null)");
            STRING *namespace_description;
            opcode_t *code_start =
                interp->code->base.data;
            switch (pmc->vtable->base_type) {
                case enum_class_FixedBooleanArray:
                case enum_class_FixedFloatArray:
                case enum_class_FixedPMCArray:
                case enum_class_FixedStringArray:
                case enum_class_ResizableBooleanArray:
                case enum_class_ResizableIntegerArray:
                case enum_class_ResizableFloatArray:
                case enum_class_ResizablePMCArray:
                case enum_class_ResizableStringArray:
                    {
                    int n = VTABLE_get_integer(interp, pmc);
                    STRING* out_buffer = VTABLE_get_repr(interp, pmc);
                    PIO_printf(interp,
                            "\tclass => %Ss,\n"
                            "\telement count => %d,\n"
                            "\telements => %Ss,\n",
                            pmc->vtable->whoami,
                            n,
                            out_buffer
                            );
                    }
                    break;
                case enum_class_Sub:
                case enum_class_Coroutine:
                    sub = PMC_sub(pmc);
                    if (sub->namespace_name) {
                        switch (sub->namespace_name->vtable->base_type) {
                            case enum_class_String:
                                namespace_description = string_from_cstring(interp, "'", 1);
                                namespace_description = string_append(interp,
                                        namespace_description,
                                        PMC_str_val(sub->namespace_name));
                                namespace_description = string_append(interp,
                                        namespace_description,
                                        string_from_cstring(interp, "'", 1));
                                break;
                            case enum_class_Key:
                                namespace_description =
                                    key_set_to_string(interp, sub->namespace_name);
                                break;
                            default:
                                namespace_description = sub->namespace_name->vtable->whoami;
                        }
                    }
                    else {
                        namespace_description = null;
                    }
                    PIO_printf(interp,
                            "\tclass => %Ss,\n"
                            "\tstart_offs => %d,\n"
                            "\tend_offs => %d,\n"
                            "\tname => '%Ss',\n"
                            "\tnamespace => %Ss\n"
                            "\tHLL_id => %d,\n",
                            pmc->vtable->whoami,
                            sub->start_offs,
                            sub->end_offs,
                            sub->name,
                            namespace_description,
                            sub->HLL_id
                            );
                    break;
                case enum_class_FixedIntegerArray:
                    PIO_printf(interp,
                            "\tclass => %Ss,\n"
                            "\trepr => '%Ss'\n",
                            pmc->vtable->whoami,
                            VTABLE_get_repr(interp, pmc)
                            );
                    break;
                default:
                    PIO_printf(interp, "\tno dump info for PMC %ld %Ss\n",
                            pmc->vtable->base_type, pmc->vtable->whoami);
                    PIO_printf(interp, "\tclass => %Ss,\n", pmc->vtable->whoami);
            }
        }
        PIO_printf(interp, "    } ],\n");
        break;
    default:
        PIO_printf(interp, "    [ 'PFC_\?\?\?', type '0x%x' ],\n",
                self->type);
        break;
    }
}

/*

=item C<void
PackFile_Fixup_dump(Interp *interp,
                    struct PackFile_FixupTable *ft)>

Dumps the fix-up table C<ft>.

=cut

*/

void
PackFile_Fixup_dump(Interp *interp,
                    struct PackFile_FixupTable *ft)
{
    opcode_t i;

    for (i = 0; i < ft->fixup_count; i++) {
        PIO_printf(interp,"\t#%d\n", (int) i);
        switch (ft->fixups[i]->type) {
            case enum_fixup_label:
            case enum_fixup_sub:
                PIO_printf(interp,
                        "\ttype => %d offs => %8d name => '%s',\n",
                        (int)ft->fixups[i]->type,
                        (int)ft->fixups[i]->offset,
                        ft->fixups[i]->name);
                    break;
            default:
                PIO_printf(interp,"\ttype => %d ???,\n",
                        (int) ft->fixups[i]->type);
                break;
        }
    }
}

/*

=back

=head1 SEE ALSO

F<src/pdump.c>.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
