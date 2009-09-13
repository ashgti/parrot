/*
Copyright (C) 2008-2009, Parrot Foundation.
$Id$
*/

/* HEADERIZER HFILE: none */
/* HEADERIZER STOP */

#include <assert.h>
#include "parrot/parrot.h"
#include "parrot/hash.h"
#include "parrot/oplib/ops.h"
#include "pmc/pmc_fixedintegerarray.h"
#include "pmc/pmc_unmanagedstruct.h"
#include "pmc/pmc_pointer.h"
#include "jit.h"
#include "jit_emit.h"

INTVAL
get_nci_I(PARROT_INTERP, ARGMOD(call_state *st), int n)
{
    if (n >= st->src.n)
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
            "too few arguments passed to NCI function");

    Parrot_fetch_arg_nci(interp, st);

    return UVal_int(st->val);
}

FLOATVAL
get_nci_N(PARROT_INTERP, ARGMOD(call_state *st), int n)
{
    if (n >= st->src.n)
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
            "too few arguments passed to NCI function");

    Parrot_fetch_arg_nci(interp, st);

    return UVal_num(st->val);
}

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING*
get_nci_S(PARROT_INTERP, ARGMOD(call_state *st), int n)
{
    /* TODO or act like below? */
    if (n >= st->src.n)
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
            "too few arguments passed to NCI function");

    Parrot_fetch_arg_nci(interp, st);

    return UVal_str(st->val);
}

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC*
get_nci_P(PARROT_INTERP, ARGMOD(call_state *st), int n)
{
    /*
     * excessive args are passed as NULL
     * used by e.g. MMD infix like __add
     */
    if (n < st->src.n)
        Parrot_fetch_arg_nci(interp, st);
    else
        UVal_pmc(st->val) = PMCNULL;

    return UVal_pmc(st->val);
}

/*
 * set return value
 */
void
set_nci_I(PARROT_INTERP, ARGOUT(call_state *st), INTVAL val)
{
    Parrot_init_ret_nci(interp, st, "I");
    if (st->dest.i < st->dest.n) {
        UVal_int(st->val) = val;
        Parrot_convert_arg(interp, st);
        Parrot_store_arg(interp, st);
    }
}

void
set_nci_N(PARROT_INTERP, ARGOUT(call_state *st), FLOATVAL val)
{
    Parrot_init_ret_nci(interp, st, "N");
    if (st->dest.i < st->dest.n) {
        UVal_num(st->val) = val;
        Parrot_convert_arg(interp, st);
        Parrot_store_arg(interp, st);
    }
}

void
set_nci_S(PARROT_INTERP, ARGOUT(call_state *st), STRING *val)
{
    Parrot_init_ret_nci(interp, st, "S");
    if (st->dest.i < st->dest.n) {
        UVal_str(st->val) = val;
        Parrot_convert_arg(interp, st);
        Parrot_store_arg(interp, st);
    }
}

void
set_nci_P(PARROT_INTERP, ARGOUT(call_state *st), PMC* val)
{
    Parrot_init_ret_nci(interp, st, "P");
    if (st->dest.i < st->dest.n) {
        UVal_pmc(st->val) = val;
        Parrot_convert_arg(interp, st);
        Parrot_store_arg(interp, st);
    }
}

int
emit_is8bit(long disp)
{
    return disp >= -128 && disp <= 127;
}

char *
emit_disp8_32(char *pc, int disp)
{
    if (emit_is8bit(disp)) {
        *(pc++) = (char)disp;
        return pc;
    }
    else {
        *(long *)pc = disp;
        return pc + 4;
    }
}

void
emit_sib(PARROT_INTERP, char *pc, int scale, int i, int base)
{
    int scale_byte;

    switch (scale) {
        case 1:
            scale_byte = emit_Scale_1;
            break;
        case 2:
            scale_byte = emit_Scale_2;
            break;
        case 4:
            scale_byte = emit_Scale_4;
            break;
        case 8:
            scale_byte = emit_Scale_8;
            break;
        default:
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_JIT_ERROR,
                "Invalid scale factor %d\n", scale);
            return;
    }

    *pc = (char)(scale_byte | (i == emit_None ? emit_Index_None : emit_reg_Index(i)) |
            emit_reg_Base(base));
}

char *
emit_r_X(PARROT_INTERP, char *pc, int reg_opcode, int base, int i, int scale, long disp)
{
    if (i && !scale)
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_JIT_ERROR,
            "emit_r_X passed invalid scale+index combo\n");

    if (base == emit_EBP) {
    /* modrm disp */
        if (i == emit_None) {
            *(pc++) = (char)((emit_is8bit(disp) ? emit_Mod_b01 : emit_Mod_b10)
                    | reg_opcode | emit_reg_rm(emit_EBP));
            return emit_disp8_32(pc, disp);
        }
        /* modrm sib disp */
        else {
            *(pc++) = (char)((emit_is8bit(disp) ? emit_Mod_b01 : emit_Mod_b10)
                    | reg_opcode | emit_b100);
            emit_sib(interp, pc++, scale, i, base);
            return emit_disp8_32(pc, disp);
        }
    }

    /* modrm sib disp */
    if (base == emit_ESP) {
        *(pc++) = (char)((emit_is8bit(disp) ? emit_Mod_b01 : emit_Mod_b10)
                | reg_opcode | emit_rm_b100);
        emit_sib(interp, pc++, scale, i, emit_ESP);
        return emit_disp8_32(pc, disp);
    }

    /* modrm disp32 */
    if (!base && !(i && scale)) {
        *(pc++) = (char)(emit_Mod_b00 | reg_opcode | emit_rm_b101);
        *(long *)pc = disp;
        return pc + 4;
    }

    /* Ok, everything should be more regular here */
    *(pc++) = (char)((disp == 0 ? emit_Mod_b00 :
              (emit_is8bit(disp) ?
               emit_Mod_b01 : emit_Mod_b10)) |
               reg_opcode |
               (!base || (scale && i) ? emit_rm_b100 : emit_reg_rm(base)));

    if (!base || (scale && i)) {
        emit_sib(interp, pc++, scale, i, base);
    }
    if (disp)
        pc = emit_disp8_32(pc, disp);

    return pc;
}

char *
emit_shift_i_r(PARROT_INTERP, char *pc, int opcode, int imm, int reg)
{
    if (opcode == emit_b000 && imm < 0) {
        opcode = emit_b001;     /* -rol => 32 + ror */
        imm = -imm;
    }

    if (imm == 0) {
        /* noop */
    }
    else if (imm == 1) {
        *(pc++) = (char) 0xd1;
        *(pc++) = (char) emit_alu_X_r(opcode,  reg);
    }
    else if (imm > 1 && imm < 33) {
        *(pc++) = (char) 0xc1;
        *(pc++) = (char) emit_alu_X_r(opcode,  reg);
        *(pc++) = (char)imm;
    }
    else {
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_JIT_ERROR,
            "emit_shift_i_r passed invalid shift\n");
    }

    return pc;
}

char *
emit_shift_i_m(PARROT_INTERP, char *pc, int opcode, int imm,
               int base, int i, int scale, long disp)
{
    if (imm == 0) {
        /* noop */
    }
    else if (imm == 1) {
        *(pc++) = (char) 0xd1;
        pc = emit_r_X(interp, pc, opcode,  base, i, scale, disp);
    }
    else if (imm > 1 && imm < 33) {
        *(pc++) = (char) 0xc1;
        pc = emit_r_X(interp, pc, opcode,  base, i, scale, disp);
        *(pc++) = (char)imm;
    }
    else {
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_JIT_ERROR,
            "emit_shift_i_m passed invalid shift\n");
    }

    return pc;
}

char *
emit_shift_r_r(PARROT_INTERP, char *pc, int opcode, int reg1, int reg2)
{
    if (reg1 != emit_ECX)
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_JIT_ERROR,
            "emit_shift_r_r passed invalid register\n");

    *(pc++) = (char) 0xd3;
    *(pc++) = (char) emit_alu_X_r(opcode,  reg2);

    return pc;
}

char *
emit_shift_r_m(PARROT_INTERP, char *pc, int opcode, int reg,
               int base, int i, int scale, long disp)
{
    if (reg != emit_ECX)
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_JIT_ERROR,
            "emit_shift_r_m passed invalid register\n");

    *(pc++) = (char) 0xd3;
    pc = emit_r_X(interp, pc, opcode,  base, i, scale, disp);

    return pc;
}

char *
emit_pushl_m(PARROT_INTERP, char *pc, int base, int i, int scale, long disp)
{
    *(pc++) = (char) 0xff;
    return emit_r_X(interp, pc, emit_reg(emit_b110), base, i, scale, disp);
}

char *
emit_popl_r(char *pc, int reg)
{
    *(pc++) = (char)(0x58 | (reg - 1));
    return pc;
}

char *
emit_popl_m(PARROT_INTERP, char *pc, int base, int i, int scale, long disp)
{
    *(pc++) = (char) 0x8f;
    return emit_r_X(interp, pc, emit_reg(emit_b000), base, i, scale, disp);
}

char *
emit_movb_r_r(char *pc, int reg1, int reg2)
{
    *(pc++) = (char) 0x88;
    *(pc++) = (char) emit_alu_r_r(reg1, reg2);
    return pc;
}

char *
emit_movb_i_r(char *pc, char imm, int reg)
{
    *(pc++) = (char)(0xb0 | (reg - 1));
    *(pc++) = imm;
    return pc;
}

char *
emit_movb_i_m(PARROT_INTERP, char *pc, char imm, int base, int i, int scale, long disp)
{
    *(pc++) = (char) 0xc6;
    pc = emit_r_X(interp, pc, emit_reg(emit_b000), base, i, scale, disp);
    *(pc++) = imm;
    return pc;
}

char *
opt_mul(PARROT_INTERP, char *pc, int dest, INTVAL imm, int src)
{
    UINTVAL ld2 = ld((UINTVAL) imm);

    if (imm == 0) {
        jit_emit_mov_ri_i(interp, pc, dest, 0);
    }
    else if (imm > 0 && !(imm & (imm - 1))) {
        /* positive power of 2 - do a shift */
        jit_emit_mov_rr_i(pc, dest, src);
        pc = emit_shift_i_r(interp, pc, emit_b100, ld2, dest);
    }
    else {
        /* special small numbers */
        switch (imm) {
            case 3:
                /* LEA dest, base, index, scale, displace
                 * note: src may be dest, so can't be reused
                 *
                 * dest = src + src*2 */
                emitm_lea_m_r(interp, pc, dest, src, src, 2, 0);
                break;
            case 5:      /* dest = src + src*4 */
                emitm_lea_m_r(interp, pc, dest, src, src, 4, 0);
                break;
            case 6:     /* dest = src*3; dest += dest */
                emitm_lea_m_r(interp, pc, dest, src, src, 2, 0);
                jit_emit_add_rr_i(interp, pc, dest, dest);
                break;
            case 9:      /* dest = src + src*8 */
                emitm_lea_m_r(interp, pc, dest, src, src, 8, 0);
                break;
            case 10:      /* dest = src + src*4 ; dest+= dest */
                emitm_lea_m_r(interp, pc, dest, src, src, 4, 0);
                jit_emit_add_rr_i(interp, pc, dest, dest);
                break;
            case 12:     /* dest= src*3; dest <<= 2 */
                emitm_lea_m_r(interp, pc, dest, src, src, 2, 0);
                pc = emit_shift_i_r(interp, pc, emit_b100, 2, dest);
                break;
            case 100:      /* dest = src + src*4 ; dest <<= 2; dest = 5*dest*/
                emitm_lea_m_r(interp, pc, dest, src, src, 4, 0);
                pc = emit_shift_i_r(interp, pc, emit_b100, 2, dest);
                emitm_lea_m_r(interp, pc, dest, dest, dest, 4, 0);
                break;
            default:
                emitm_alul_r_r(pc, 0x69, dest, src);
                *(long *)(pc) = (long)imm;
                pc += 4;
        }
    }
    return pc;
}

int
intreg_is_used(Parrot_jit_info_t *jit_info, char reg)
{
    int i;
    const jit_arch_regs *reg_info;
    Parrot_jit_register_usage_t *ru = jit_info->optimizer->cur_section->ru;

    reg_info = jit_info->arch_info->regs + jit_info->code_type;

    for (i = 0; i < ru[0].registers_used; ++i) {
        if (reg_info->map_I[i] == reg) {
            return 1;
        }
    }
    return 0;
}

char *
opt_shift_rr(PARROT_INTERP, Parrot_jit_info_t *jit_info, int dest, int count, int op)
{
    char *pc = jit_info->native_ptr;
    if (count == emit_ECX) {
        pc = emit_shift_r_r(interp, pc, op, count, dest);
    }
    else {
        int saved = 0;
        PARROT_ASSERT(count != emit_EAX);
        if (dest == emit_EAX) {
            if (intreg_is_used(jit_info, emit_ECX)) {
                emitm_pushl_r(pc, emit_ECX);
                saved = 1;
            }
            jit_emit_mov_rr_i(pc, emit_ECX, count);
            pc = emit_shift_r_r(interp, pc, op, emit_ECX, dest);
            if (saved) {
                emitm_popl_r(pc, emit_ECX);
            }

        }
        else if (dest == emit_ECX) {
            jit_emit_xchg_rr_i(interp, pc, dest, count);
            pc = emit_shift_r_r(interp, pc, op, dest, count);
            jit_emit_xchg_rr_i(interp, pc, dest, count);
        }
        else {
            jit_emit_mov_rr_i(pc, emit_EAX, emit_ECX);
            jit_emit_mov_rr_i(pc, emit_ECX, count);
            pc = emit_shift_r_r(interp, pc, op, emit_ECX, dest);
            jit_emit_mov_rr_i(pc, emit_ECX, emit_EAX);
        }
    }
    return pc;
}

char *
opt_shift_rm(PARROT_INTERP, Parrot_jit_info_t *jit_info, int dest, int offs, int op)
{
    char *pc = jit_info->native_ptr;
    int saved = 0;
    /* if ECX is mapped, save it */
    if (dest != emit_ECX && intreg_is_used(jit_info, emit_ECX)) {
        emitm_pushl_r(pc, emit_ECX);
        saved = 1;
    }
    if (dest == emit_ECX) {
        /* jit_emit_mov_RM_i(pc, emit_EAX, count); */
        emitm_movl_m_r(interp, pc, emit_EAX, emit_EBX, emit_None, 1, offs);
        jit_emit_xchg_rr_i(interp, pc, dest, emit_EAX);
        pc = emit_shift_r_r(interp, pc, op, emit_ECX, emit_EAX);
        jit_emit_xchg_rr_i(interp, pc, dest, emit_EAX);
    }
    else {
        /* jit_emit_mov_RM_i(pc, emit_ECX, count); */
        emitm_movl_m_r(interp, pc, emit_ECX, emit_EBX, emit_None, 1, offs);
        pc = emit_shift_r_r(interp, pc, op, emit_ECX, dest);
    }
    if (saved) {
        emitm_popl_r(pc, emit_ECX);
    }
    return pc;
}

void call_func(Parrot_jit_info_t *jit_info, void (*addr) (void))
{
    Parrot_jit_newfixup(jit_info);
    jit_info->arena.fixups->type = JIT_X86CALL;
    jit_info->arena.fixups->param.fptr = D2FPTR(addr);
    emitm_calll(jit_info->native_ptr, 0xdeafc0de);
}

void jit_emit_real_exception(Parrot_jit_info_t *jit_info)
{
    call_func(jit_info, (void (*) (void)) & Parrot_ex_throw_from_c_args);
}

unsigned char *lastpc;

/* ST(r1) /= ST(r2) */
char *
div_rr_n(PARROT_INTERP, Parrot_jit_info_t *jit_info, int r1)
{
    char *L1;
    static const char div_by_zero[] = "Divide by zero";
    char *pc = jit_info->native_ptr;

    jit_emit_test_r_n(pc, (char)0);   /* TOS */
    L1 = pc;
    emitm_jxs(pc, emitm_jnz, 0);
    emitm_pushl_i(pc, div_by_zero);
    emitm_pushl_i(pc, EXCEPTION_DIV_BY_ZERO);
    emitm_pushl_i(pc, 0);    /* NULL */
    Parrot_jit_emit_get_INTERP(interp, pc, emit_ECX);
    emitm_pushl_r(pc, emit_ECX);
    jit_info->native_ptr = pc;
    jit_emit_real_exception(jit_info);
    pc = jit_info->native_ptr;
    /* L1: */
    L1[1] = (char)(pc - L1 - 2);
    emitm_fdivp(pc, (r1+1));
    return pc;
}

char *
mod_rr_n(PARROT_INTERP, Parrot_jit_info_t *jit_info, int r)
{
    char *L1;
    static const char div_by_zero[] = "Divide by zero";
    char *pc = jit_info->native_ptr;

    jit_emit_test_r_n(pc, (char)0);   /* TOS */
    L1 = pc;
    emitm_jxs(pc, emitm_jnz, 0);
    emitm_pushl_i(pc, div_by_zero);
    emitm_pushl_i(pc, EXCEPTION_DIV_BY_ZERO);
    emitm_pushl_i(pc, 0);    /* NULL */
    Parrot_jit_emit_get_INTERP(interp, pc, emit_ECX);
    emitm_pushl_r(pc, emit_ECX);
    jit_info->native_ptr = pc;
    jit_emit_real_exception(jit_info);
    pc = jit_info->native_ptr;
    /* L1: */
    L1[1] = (char)(pc - L1 - 2);
    /* L2: */
    emitm_fxch(pc, (char)1);
    emitm_fprem(pc);
    emitm_fstw(pc);
    emitm_sahf(pc);
    emitm_jxs(pc, emitm_jp, -7); /* jo L2 */
    emitm_fstp(pc, (r+1));
    return pc;
}

/* dest /= src
 * edx:eax /= src, quotient => eax, rem => edx
 */
char *
opt_div_rr(PARROT_INTERP, Parrot_jit_info_t *jit_info, int dest, int src, int is_div)
{
    char *pc = jit_info->native_ptr;
    int saved = 0;
    int div_ecx = 0;
    char *L1, *L2, *L3;
    static const char div_by_zero[] = "Divide by zero";

    PARROT_ASSERT(src != emit_EAX);

    if (dest != emit_EAX) {
        jit_emit_mov_rr_i(pc, emit_EAX, dest);
    }
    if (dest == emit_EDX) {
        /* all ok, we can globber it */
    }
    else {
        /* if ECX is not mapped use it */
        if (!intreg_is_used(jit_info, emit_ECX) && src == emit_EDX) {
            jit_emit_mov_rr_i(pc, emit_ECX, emit_EDX);
            div_ecx = 1;
        }
        else
            /* if EDX is mapped, preserve EDX on stack */
            if (intreg_is_used(jit_info, emit_EDX)) {
                emitm_pushl_r(pc, emit_EDX);
                saved = 1;
                /* if EDX is the src, we need another temp register: ECX */
                if (src == emit_EDX) {
                    /* if ECX is mapped save it, but not if it's dest */
                    if (intreg_is_used(jit_info, emit_ECX) &&
                            dest != emit_ECX) {
                        emitm_pushl_r(pc, emit_ECX);
                        saved = 2;
                    }
                    /* else just use it */
                    jit_emit_mov_rr_i(pc, emit_ECX, emit_EDX);
                    div_ecx = 1;
                }
            }
    }
    /* this sequence allows 2 other instructions to run parallel */
    if (dest != emit_EDX) {
        jit_emit_mov_rr_i(pc, emit_EDX, emit_EAX);
    }
    pc = emit_shift_i_r(interp, pc, emit_b111, 31, emit_EDX); /* SAR 31 */
    if (div_ecx) {
        jit_emit_test_r_i(pc, emit_ECX);
        L1 = pc;
        emitm_jxs(pc, emitm_jz, 0);
        emitm_sdivl_r(pc, emit_ECX);
        L3 = pc;
        emitm_jumps(pc, 0);
        /* L1: */
        L1[1] = (char)(pc - L1 - 2);
    }
    else {
        jit_emit_test_r_i(pc, src);
        L2 = pc;
        emitm_jxs(pc, emitm_jz, 0);
        emitm_sdivl_r(pc, src);
        L3 = pc;
        emitm_jumps(pc, 0);
        /* L2: */
        L2[1] = (char)(pc - L2 - 2);
    }
    /* TODO Parrot_ex_throw_from_c_args */
    emitm_pushl_i(pc, div_by_zero);
    emitm_pushl_i(pc, EXCEPTION_DIV_BY_ZERO);
    emitm_pushl_i(pc, 0);    /* NULL */
    Parrot_jit_emit_get_INTERP(interp, pc, emit_ECX);
    emitm_pushl_r(pc, emit_ECX);
    jit_info->native_ptr = pc;
    jit_emit_real_exception(jit_info);
    pc = jit_info->native_ptr;
    /* L3: */
    L3[1] = (char)(pc - L3 - 2);
    if (saved == 2) {
        emitm_popl_r(pc, emit_ECX);
    }
    if (is_div) {
        /* result = quotient in EAX */
        if (saved) {
            emitm_popl_r(pc, emit_EDX);
        }
        if (dest != emit_EAX) {
            jit_emit_mov_rr_i(pc, dest, emit_EAX);
        }
    }
    else {
        /* result = remainder in EDX */
        if (saved) {
            emitm_popl_r(pc, emit_EAX);
            jit_emit_mov_rr_i(pc, dest, emit_EDX);
            jit_emit_mov_rr_i(pc, emit_EDX, emit_EAX);
        }
        else {
            if (dest != emit_EDX)
                jit_emit_mov_rr_i(pc, dest, emit_EDX);
        }
    }
    if (!saved && div_ecx) {
        /* restore EDX */
        jit_emit_mov_rr_i(pc, emit_EDX, emit_ECX);
    }
    return pc;
}

char *
opt_div_ri(PARROT_INTERP, Parrot_jit_info_t *jit_info, int dest, INTVAL imm, int is_div)
{
    char *pc = jit_info->native_ptr;

    UINTVAL ld2 = ld((UINTVAL) imm);
    if (is_div && imm > 1 && !(imm & (imm - 1))) {
        /* positive power of 2 - do a shift */
        pc = emit_shift_i_r(interp, pc, emit_b101, ld2, dest);
    }
    else {
        if (dest != emit_EBX) {
            emitm_pushl_r(pc, emit_EBX);
            jit_emit_mov_ri_i(interp, pc, emit_EBX, imm);
            jit_info->native_ptr = pc;
            pc = opt_div_rr(interp, jit_info, dest, emit_EBX, is_div);
            pc = emit_popl_r(pc, emit_EBX);
        }
        else {
            emitm_pushl_r(pc, emit_EDI);
            jit_emit_mov_ri_i(interp, pc, emit_EDI, imm);
            jit_info->native_ptr = pc;
            pc = opt_div_rr(interp, jit_info, dest, emit_EDI, is_div);
            pc = emit_popl_r(pc, emit_EDI);
        }
    }
    return pc;
}

char *
opt_div_RM(PARROT_INTERP, Parrot_jit_info_t *jit_info, int dest, int offs, int is_div)
{
    char *pc = jit_info->native_ptr;
    int saved = 0;

    if (dest != emit_EAX) {
        jit_emit_mov_rr_i(pc, emit_EAX, dest);
    }
    if (dest == emit_EDX) {
        /* all ok, we can globber it */
    }
    else {
        /* if ECX is mapped, push EDX on stack */
        if (intreg_is_used(jit_info, emit_ECX)) {
            emitm_pushl_r(pc, emit_EDX);
            saved = 2;
        }
        /* if EDX is mapped, save it in ECX */
        else if (intreg_is_used(jit_info, emit_EDX)) {
            saved = 1;
            jit_emit_mov_rr_i(pc, emit_ECX, emit_EDX);
        }
    }
    /* this sequence allows 2 other instructions to run parallel */
    jit_emit_mov_rr_i(pc, emit_EDX, emit_EAX);
    pc = emit_shift_i_r(interp, pc, emit_b111, 31, emit_EDX); /* SAR 31 */

    emitm_sdivl_m(pc, emit_EBX, 0, 1, offs);

    if (is_div) {
        /* result = quotient in EAX */
        if (saved == 1) {
            jit_emit_mov_rr_i(pc, emit_EDX, emit_ECX);
        }
        if (dest != emit_EAX) {
            jit_emit_mov_rr_i(pc, dest, emit_EAX);
        }
        if (saved == 2) {
            emitm_popl_r(pc, emit_EDX);
        }
    }
    else {
        /* result = remainder in EDX */
        if (dest != emit_EDX) {
            jit_emit_mov_rr_i(pc, dest, emit_EDX);
            if (saved == 1) {
                jit_emit_mov_rr_i(pc, emit_EDX, emit_ECX);
            }
            else if (saved == 2)
                emitm_popl_r(pc, emit_EDX);
        }
    }
    return pc;
}

void
jit_emit_jcc(Parrot_jit_info_t *jit_info, int code, opcode_t disp)
{
    long offset;
    opcode_t opcode;

    opcode = jit_info->op_i + disp;

    if (opcode <= jit_info->op_i) {
        offset = jit_info->arena.op_map[opcode].offset -
                (jit_info->native_ptr - jit_info->arena.start);

        /* If we are here, the current section must have a branch_target
           section, I think. */
        if (jit_info->optimizer->cur_section->branch_target ==
            jit_info->optimizer->cur_section)
                offset +=
                    jit_info->optimizer->cur_section->branch_target->load_size;

        if (emit_is8bit(offset - 2)) {
            emitm_jxs(jit_info->native_ptr, code, offset - 2);
        }
        else {
            emitm_jxl(jit_info->native_ptr, code, offset - 6);
        }

        return;
    }

    Parrot_jit_newfixup(jit_info);
    jit_info->arena.fixups->type = JIT_X86BRANCH;
    jit_info->arena.fixups->param.opcode = opcode;
    /* If the branch is to the current section, skip the load instructions. */
    if (jit_info->optimizer->cur_section->branch_target ==
        jit_info->optimizer->cur_section)
            jit_info->arena.fixups->skip =
                (char)jit_info->optimizer->cur_section->branch_target->load_size;

    emitm_jxl(jit_info->native_ptr, code, 0xc0def00d);
}

void
emit_jump(Parrot_jit_info_t *jit_info, opcode_t disp)
{
    long offset;
    opcode_t opcode;

    opcode = jit_info->op_i + disp;

    if (opcode <= jit_info->op_i) {
        offset = jit_info->arena.op_map[opcode].offset -
                                (jit_info->native_ptr - jit_info->arena.start);
        if (emit_is8bit(offset - 2)) {
            emitm_jumps(jit_info->native_ptr, (char)(offset - 2));
        }
        else {
            emitm_jumpl(jit_info->native_ptr, offset - 5);
        }
        return;
    }

    Parrot_jit_newfixup(jit_info);
    jit_info->arena.fixups->type = JIT_X86JUMP;
    jit_info->arena.fixups->param.opcode = opcode;
    /* If the branch is to the current section, skip the load instructions. */
    if (jit_info->optimizer->cur_section->branch_target ==
        jit_info->optimizer->cur_section)
            jit_info->arena.fixups->skip =
                (char)jit_info->optimizer->cur_section->branch_target->load_size;
    emitm_jumpl(jit_info->native_ptr, 0xc0def00d);
}

void
Parrot_emit_jump_to_eax(Parrot_jit_info_t *jit_info,
                   PARROT_INTERP)
{
    /* we have to get the code pointer, which might change
     * due to intersegment branches
     */

    /* get interpreter
     */
    emitm_movl_m_r(interp, jit_info->native_ptr,
            emit_EBX, emit_EBP, emit_None, 1, INTERP_BP_OFFS);
    if (!jit_info->objfile) {
        /*
         * emit interp->code->base.data
         */
        emitm_movl_m_r(interp, jit_info->native_ptr, emit_ECX, emit_EBX, 0, 1,
                offsetof(Interp, code));
        emitm_movl_m_r(interp, jit_info->native_ptr, emit_EDX, emit_ECX, 0, 1,
                offsetof(PackFile_Segment, data));
        /* calc code offset */
        jit_emit_sub_rr_i(interp, jit_info->native_ptr, emit_EAX, emit_EDX);
        /*
         * now we have the offset of the ins in EAX
         *
         * interp->code->jit_info->arena->op_map
         *
         * TODO interleave these 2 calculations
         */
        emitm_movl_m_r(interp, jit_info->native_ptr, emit_EDX, emit_ECX, 0, 1,
                offsetof(PackFile_ByteCode, jit_info));
        emitm_lea_m_r(interp, jit_info->native_ptr, emit_EDX, emit_EDX, 0, 1,
                offsetof(Parrot_jit_info_t, arena));
        emitm_movl_m_r(interp, jit_info->native_ptr, emit_EDX, emit_EDX, 0, 1,
                offsetof(Parrot_jit_arena_t, op_map));

    }
#if EXEC_CAPABLE
    else {
        int *reg;
        emitm_subl_i_r(jit_info->native_ptr,
            jit_info->objfile->bytecode_header_size, emit_EAX);
        Parrot_exec_add_text_rellocation(jit_info->objfile,
            jit_info->native_ptr, RTYPE_DATA, "program_code", -4);
        reg = Parrot_exec_add_text_rellocation_reg(jit_info->objfile,
                jit_info->native_ptr, "opcode_map", 0, 0);
        jit_emit_mov_ri_i(interp, jit_info->native_ptr, emit_EDX, (long) reg);
    }
#endif
    /* get base pointer */
    emitm_movl_m_r(interp, jit_info->native_ptr, emit_EBX, emit_EBX, 0, 1,
            offsetof(Interp, ctx));
    emitm_movl_m_r(interp, jit_info->native_ptr, emit_EBX, emit_EBX, 0, 1,
            offsetof(PMC, data));
    emitm_movl_m_r(interp, jit_info->native_ptr, emit_EBX, emit_EBX, 0, 1,
            offsetof(Parrot_Context, bp));

    /* This jumps to the address in op_map[EDX + sizeof (void *) * INDEX] */
    emitm_jumpm(jit_info->native_ptr, emit_EDX, emit_EAX,
            sizeof (*jit_info->arena.op_map) / 4, 0);
}

#  define CONST(i) (int *)(jit_info->cur_op[i] * \
       sizeof (PackFile_Constant) + \
       offsetof(PackFile_Constant, u))

/* for vtable calls registers are already saved back */
void
Parrot_jit_vtable_n_op(Parrot_jit_info_t *jit_info,
                PARROT_INTERP, int n, int *args)
{
    int        nvtable = op_jit[*jit_info->cur_op].extcall;
    size_t     offset;
    op_info_t *op_info = &interp->op_info_table[*jit_info->cur_op];
    int        pi;
    int        idx, i, op;
    int        st      = 0;         /* stack pop correction */
    char      *L4      = NULL;

    /* get the offset of the first vtable func */
    offset  = offsetof(VTABLE, absolute);
    offset += nvtable * sizeof (void *);
    op      = *jit_info->cur_op;

    /* get params $i, 0 is opcode */
    for (idx = n; idx > 0; idx--) {
        i  = args[idx-1];
        pi = *(jit_info->cur_op + i);

        switch (op_info->types[i - 1]) {
            case PARROT_ARG_S:
                emitm_movl_m_r(interp, jit_info->native_ptr,
                        emit_EAX, emit_EBX, emit_None, 1, REG_OFFS_STR(pi));
                emitm_pushl_r(jit_info->native_ptr, emit_EAX);
                break;
            case PARROT_ARG_K:
            case PARROT_ARG_P:
                emitm_movl_m_r(interp, jit_info->native_ptr,
                        emit_EAX, emit_EBX, emit_None, 1, REG_OFFS_PMC(pi));
                /* push $i, the left most Pi stays in eax, which is used
                 * below, to call the vtable method
                 */
                emitm_pushl_r(jit_info->native_ptr, emit_EAX);
                break;
            case PARROT_ARG_KI:
            case PARROT_ARG_I:
                if (MAP(i))
                    emitm_pushl_r(jit_info->native_ptr, MAP(i));
                else {
                    emitm_movl_m_r(interp, jit_info->native_ptr,
                            emit_EAX, emit_EBX, emit_None, 1, REG_OFFS_INT(pi));
                    emitm_pushl_r(jit_info->native_ptr, emit_EAX);
                }
                break;
                break;
            case PARROT_ARG_KIC:
            case PARROT_ARG_IC:
                /* XXX INTVAL_SIZE */
                /* push value */
                emitm_pushl_i(jit_info->native_ptr, pi);
                break;
            case PARROT_ARG_N:
                /* push num on st(0) */
                if (MAP(i))
                    emitm_fld(jit_info->native_ptr, MAP(i));
                else
                    jit_emit_fload_mb_n(interp, jit_info->native_ptr,
                            emit_EBX, REG_OFFS_NUM(pi));
                goto store;
            case PARROT_ARG_NC:
                /*
                 * TODO not all constants are shared between interpreters
                 */
#if EXEC_CAPABLE
                if (jit_info->objfile) {
                    jit_emit_fload_m_n(interp, jit_info->native_ptr, CONST(i));
                    Parrot_exec_add_text_rellocation(jit_info->objfile,
                            jit_info->native_ptr, RTYPE_DATA, "const_table", -4);
                }
                else
#endif
                    jit_emit_fload_m_n(interp, jit_info->native_ptr,
                            &interp->code->const_table->
                            constants[pi]->u.number);
store:
#if NUMVAL_SIZE == 8
                /* make room for double */
                emitm_addb_i_r(jit_info->native_ptr, -8, emit_ESP);
                emitm_fstpl(interp, jit_info->native_ptr, emit_ESP, emit_None, 1, 0);
                /* additional stack adjustment */
                st += 4;
#else
                emitm_addb_i_r(jit_info->native_ptr, -12, emit_ESP);
                emitm_fstpt(jit_info->native_ptr, emit_ESP, emit_None, 1, 0);
                st += 8;
#endif
                break;

            case PARROT_ARG_SC:
#if EXEC_CAPABLE
                if (jit_info->objfile) {
                    emitm_pushl_m(jit_info->native_ptr, CONST(i));
                    Parrot_exec_add_text_rellocation(jit_info->objfile,
                            jit_info->native_ptr, RTYPE_DATA, "const_table", -4);
                }
                else
#endif
                    emitm_pushl_i(jit_info->native_ptr,
                            interp->code->const_table->
                            constants[pi]->u.string);
                break;

            case PARROT_ARG_KC:
            case PARROT_ARG_PC:
#if EXEC_CAPABLE
                if (jit_info->objfile) {
                    emitm_pushl_m(jit_info->native_ptr, CONST(i));
                    Parrot_exec_add_text_rellocation(jit_info->objfile,
                            jit_info->native_ptr, RTYPE_DATA,
                            "const_table", -4);
                }
                else
#endif
                    emitm_pushl_i(jit_info->native_ptr,
                            interp->code->const_table->
                            constants[pi]->u.key);
                break;

            default:
                Parrot_ex_throw_from_c_args(interp, NULL, 1,
                    "jit_vtable_n_op: unimp type %d, arg %d vtable %d",
                    op_info->types[i - 1], i, nvtable);
                break;
        }
    }

    /* push interpreter */
    Parrot_jit_emit_get_INTERP(interp, jit_info->native_ptr, emit_ECX);
    emitm_pushl_r(jit_info->native_ptr, emit_ECX);

    if (L4) {
        emitm_callr(jit_info->native_ptr, emit_ESI);
    }
    else {
        /* mov (offs)%eax, %eax i.e. $1->vtable */
        emitm_movl_m_r(interp, jit_info->native_ptr, emit_EAX, emit_EAX, emit_None, 1,
                offsetof(struct PMC, vtable));
        /* call *(offset)eax */
        emitm_callm(jit_info->native_ptr, emit_EAX, emit_None, emit_None, offset);
    }

    emitm_addb_i_r(jit_info->native_ptr,
            st + sizeof (void *) * (n + 1), emit_ESP);

    /* L4: */
    if (L4)
        L4[1] = (char)(jit_info->native_ptr - L4 - 2);
}

void
Parrot_jit_store_retval(Parrot_jit_info_t *jit_info,
                     PARROT_INTERP)
{
    op_info_t *op_info = &interp->op_info_table[*jit_info->cur_op];
    int p1 = *(jit_info->cur_op + 1);

    /* return result is in EAX or ST(0) */
    switch (op_info->types[0]) {
        case PARROT_ARG_I:
            emitm_movl_r_m(interp, jit_info->native_ptr,
                    emit_EAX, emit_EBX, emit_None, 1, REG_OFFS_INT(p1));
            break;
        case PARROT_ARG_S:
            emitm_movl_r_m(interp, jit_info->native_ptr,
                    emit_EAX, emit_EBX, emit_None, 1, REG_OFFS_STR(p1));
            break;
        case PARROT_ARG_P:
            emitm_movl_r_m(interp, jit_info->native_ptr,
                    emit_EAX, emit_EBX, emit_None, 1, REG_OFFS_PMC(p1));
            break;
        case PARROT_ARG_N:
            jit_emit_fstore_mb_n(interp, jit_info->native_ptr, emit_EBX,
                    REG_OFFS_NUM(p1));
            break;
        default:
            Parrot_ex_throw_from_c_args(interp, NULL, 1, "jit_vtable1r: ill LHS");
            break;
    }
}

/* emit a call to a vtable func
 * $1->vtable(interp, $1)
 */
void
Parrot_jit_vtable1_op(Parrot_jit_info_t *jit_info,
                     PARROT_INTERP)
{
    int a[] = { 1 };
    Parrot_jit_vtable_n_op(jit_info, interp, 1, a);
}

/* emit a call to a vtable func
 * $1 = $2->vtable(interp, $2)
 */
void
Parrot_jit_vtable1r_op(Parrot_jit_info_t *jit_info,
                     PARROT_INTERP)
{
    int a[] = { 2 };
    Parrot_jit_vtable_n_op(jit_info, interp, 1, a);
    Parrot_jit_store_retval(jit_info, interp);
}


/* emit a call to a vtable func
 * $1 = $2->vtable(interp, $2, $3)
 */
void
Parrot_jit_vtable_1r223_op(Parrot_jit_info_t *jit_info,
                     PARROT_INTERP)
{
    int a[] = { 2 , 3};
    Parrot_jit_vtable_n_op(jit_info, interp, 2, a);
    Parrot_jit_store_retval(jit_info, interp);
}

/* emit a call to a vtable func
 * $1 = $3->vtable(interp, $3, $2)
 */
void
Parrot_jit_vtable_1r332_op(Parrot_jit_info_t *jit_info,
                     PARROT_INTERP)
{
    int a[] = { 3 , 2};
    Parrot_jit_vtable_n_op(jit_info, interp, 2, a);
    Parrot_jit_store_retval(jit_info, interp);
}
/* emit a call to a vtable func
 * $1->vtable(interp, $1, $2)
 */
void
Parrot_jit_vtable_112_op(Parrot_jit_info_t *jit_info,
                     PARROT_INTERP)
{
    int a[] = { 1, 2 };
    Parrot_jit_vtable_n_op(jit_info, interp, 2, a);
}

/* emit a call to a vtable func
 * $1->vtable(interp, $1, $1)
 */
void
Parrot_jit_vtable_111_op(Parrot_jit_info_t *jit_info,
                     PARROT_INTERP)
{
    int a[] = { 1, 1 };
    Parrot_jit_vtable_n_op(jit_info, interp, 2, a);
}
/* emit a call to a vtable func
 * $2->vtable(interp, $2, $1)
 */
void
Parrot_jit_vtable_221_op(Parrot_jit_info_t *jit_info,
                     PARROT_INTERP)
{
    int a[] = { 2, 1 };
    Parrot_jit_vtable_n_op(jit_info, interp, 2, a);
}

/* emit a call to a vtable func
 * $2->vtable(interp, $2, $3, $1)
 */
void
Parrot_jit_vtable_2231_op(Parrot_jit_info_t *jit_info,
                     PARROT_INTERP)
{
    int a[] = { 2, 3, 1 };
    Parrot_jit_vtable_n_op(jit_info, interp, 3, a);
}

/* emit a call to a vtable func
 * $1->vtable(interp, $1, $2, $3)
 */
void
Parrot_jit_vtable_1123_op(Parrot_jit_info_t *jit_info,
                     PARROT_INTERP)
{
    int a[] = { 1, 2, 3 };
    Parrot_jit_vtable_n_op(jit_info, interp, 3, a);
}

/* emit a call to a vtable func
 * $1->vtable(interp, $1, $2, $1)
 */
void
Parrot_jit_vtable_1121_op(Parrot_jit_info_t *jit_info,
                     PARROT_INTERP)
{
    int a[] = { 1, 2, 1 };
    Parrot_jit_vtable_n_op(jit_info, interp, 3, a);
}


/* if_p_ic, unless_p_ic */
void
Parrot_jit_vtable_if_unless_op(Parrot_jit_info_t *jit_info,
                     PARROT_INTERP, int unless)
{
    int ic = *(jit_info->cur_op + 2);   /* branch offset */

    /* emit call  vtable function i.e. get_bool, result eax */
    Parrot_jit_vtable1_op(jit_info, interp);
    /* test result */
    jit_emit_test_r_i(jit_info->native_ptr, emit_EAX);
    jit_emit_jcc(jit_info, unless ? emitm_jz : emitm_jnz, ic);
}

/* unless_p_ic */
void
Parrot_jit_vtable_unlessp_op(Parrot_jit_info_t *jit_info,
                     PARROT_INTERP)
{
    Parrot_jit_vtable_if_unless_op(jit_info, interp, 1);
}

/* if_p_ic */
void
Parrot_jit_vtable_ifp_op(Parrot_jit_info_t *jit_info,
                     PARROT_INTERP)
{
    Parrot_jit_vtable_if_unless_op(jit_info, interp, 0);
}

#  define CALL(f) Parrot_exec_add_text_rellocation_func(jit_info->objfile, \
       jit_info->native_ptr, (f)); \
       emitm_calll(jit_info->native_ptr, EXEC_CALLDISP);

#  undef IREG
#  undef NREG
#  undef SREG
#  undef PREG
#  undef CONST
#  undef CALL

#  define NATIVECODE jit_info->native_ptr
#  define CUR_OPCODE jit_info->cur_op
#  define CONST(i) PCONST(jit_info->cur_op[(i)])

void
jit_get_params_pc(Parrot_jit_info_t *jit_info, PARROT_INTERP)
{
    PMC *sig_pmc;
    INTVAL *sig_bits, i, n;

    sig_pmc = Parrot_pcc_get_pmc_constant(interp, CURRENT_CONTEXT(interp), CUR_OPCODE[1]);
    GETATTR_FixedIntegerArray_int_array(interp, sig_pmc, sig_bits);
    n = VTABLE_elements(interp, sig_pmc);
    jit_info->n_args = n;
    emitm_movl_m_r(interp, NATIVECODE, emit_EAX, emit_EBP, emit_None, 1, 16);
    for (i = 0; i < n; ++i) {
        switch (sig_bits[i] & PARROT_ARG_TYPE_MASK) {
            case PARROT_ARG_INTVAL:
                emitm_movl_m_r(interp, NATIVECODE, MAP(2+i), emit_EAX, emit_None,
                        1, 4 + i*4);
                break;
            case PARROT_ARG_FLOATVAL:
                emitm_movl_m_r(interp, NATIVECODE, emit_EAX, emit_EAX, emit_None,
                        1, 4+ i*4);
                jit_emit_fload_mb_n(interp, NATIVECODE, emit_EAX, 0);
                emitm_fstp(NATIVECODE, (MAP(2+i) + 1));
                if (i < n - 1)
                    emitm_movl_m_r(interp, NATIVECODE, emit_EAX, emit_EBP,
                            emit_None, 1, 16);
                break;
            default:
                break;
        }
    }
}

/*
 * preserve registers
 * a) all callee saved on function entry
 */
void
jit_save_regs(Parrot_jit_info_t *jit_info, PARROT_INTERP)
{
    int i, used_i, save_i;
    const jit_arch_regs *reg_info;

    used_i = Parrot_pcc_get_regs_used(interp, CURRENT_CONTEXT(interp), REGNO_INT);
    reg_info = &jit_info->arch_info->regs[jit_info->code_type];
    save_i = reg_info->n_preserved_I;
    for (i = save_i; i < used_i; ++i) {
        emitm_pushl_r(jit_info->native_ptr, reg_info->map_I[i]);
    }
}

/* restore saved regs, see above */
void
jit_restore_regs(Parrot_jit_info_t *jit_info, PARROT_INTERP)
{

    int i, used_i, save_i;
    const jit_arch_regs *reg_info;

    used_i = Parrot_pcc_get_regs_used(interp, CURRENT_CONTEXT(interp), REGNO_INT);
    reg_info = &jit_info->arch_info->regs[jit_info->code_type];
    save_i = reg_info->n_preserved_I;
    /* note - reversed order of jit_save_regs  */
    for (i = used_i - 1; i >= save_i; --i) {
        emitm_popl_r(jit_info->native_ptr, reg_info->map_I[i]);
    }
}

/*
 * preserve registers around a functioncall
 *
 * all used register around a call (skip >= 0 := return result
 *
 * TODO factor out common code
 *      use jit_emit_mov_RM_{in} functions (load/store base indexed)
 *      and a macro to retrieve sp
 */
int
jit_save_regs_call(Parrot_jit_info_t *jit_info, PARROT_INTERP, int skip)
{
    int i, used_i, used_n;
    const jit_arch_regs *reg_info;

    used_i = Parrot_pcc_get_regs_used(interp, CURRENT_CONTEXT(interp), REGNO_INT);
    used_n = Parrot_pcc_get_regs_used(interp, CURRENT_CONTEXT(interp), REGNO_NUM);
    jit_emit_sub_ri_i(interp, jit_info->native_ptr, emit_ESP,
            (used_i * sizeof (INTVAL) + used_n * sizeof (FLOATVAL)));
    reg_info = &jit_info->arch_info->regs[jit_info->code_type];
    for (i = 0; i < used_i; ++i) {
        /* XXX need 2 skip vars */
        if (reg_info->map_I[i] == skip)
            continue;
        emitm_movl_r_m(interp, NATIVECODE, reg_info->map_I[i], emit_ESP,
                            emit_None, 1,
                            (used_n * sizeof (FLOATVAL) +
                             i      * sizeof (INTVAL)));
    }
    for (i = 0; i < used_n; ++i) {
        if (reg_info->map_F[i] == skip)
            continue;
        emitm_fld(NATIVECODE, reg_info->map_F[i]);
        jit_emit_fstore_mb_n(interp, NATIVECODE, emit_ESP, (i * sizeof (FLOATVAL)));
    }
    return used_n;
}

void
jit_restore_regs_call(Parrot_jit_info_t *jit_info, PARROT_INTERP,
        int skip)
{

    int i, used_i, used_n;
    const jit_arch_regs *reg_info;

    used_i = Parrot_pcc_get_regs_used(interp, CURRENT_CONTEXT(interp), REGNO_INT);
    used_n = Parrot_pcc_get_regs_used(interp, CURRENT_CONTEXT(interp), REGNO_NUM);
    reg_info = &jit_info->arch_info->regs[jit_info->code_type];

    for (i = 0; i < used_i; ++i) {
        if (reg_info->map_I[i] == skip)
            continue;
        emitm_movl_m_r(interp, NATIVECODE, reg_info->map_I[i], emit_ESP,
                            emit_None, 1,
                            (used_n * sizeof (FLOATVAL) +
                             i      * sizeof (INTVAL)));
    }
    for (i = 0; i < used_n; ++i) {
        if (reg_info->map_F[i] == skip)
            continue;
        jit_emit_fload_mb_n(interp, NATIVECODE, emit_ESP, (i * sizeof (FLOATVAL)));
        emitm_fstp(NATIVECODE, (1+reg_info->map_F[i]));
    }

    jit_emit_add_ri_i(interp, jit_info->native_ptr, emit_ESP,
            (used_i * sizeof (INTVAL) + used_n * sizeof (FLOATVAL)));
}

void
jit_set_returns_pc(Parrot_jit_info_t *jit_info, PARROT_INTERP,
        int recursive)
{
    PMC *sig_pmc;
    INTVAL *sig_bits, sig;

    sig_pmc = Parrot_pcc_get_pmc_constant(interp, CURRENT_CONTEXT(interp), CUR_OPCODE[1]);
    if (!VTABLE_elements(interp, sig_pmc))
        return;
    GETATTR_FixedIntegerArray_int_array(interp, sig_pmc, sig_bits);
    sig = sig_bits[0];
    if (!recursive) {
        /* mov 16(%ebp), %eax - fetch args ptr */
        emitm_movl_m_r(interp, NATIVECODE, emit_EAX, emit_EBP, emit_None, 1, 16);
        emitm_movl_m_r(interp, NATIVECODE, emit_EAX, emit_EAX, emit_None, 1, 0);
    }
    /*
     * recursive returns according to ABI */
    switch (sig & (PARROT_ARG_TYPE_MASK|PARROT_ARG_CONSTANT)) {
        case PARROT_ARG_INTVAL:
            if (recursive) {
                jit_emit_mov_rr_i(NATIVECODE, emit_EAX, MAP(2));
            }
            else {
                emitm_movl_r_m(interp, NATIVECODE, MAP(2), emit_EAX, 0, 1, 0);
            }
            break;
        case PARROT_ARG_INTVAL|PARROT_ARG_CONSTANT:
            if (recursive) {
                jit_emit_mov_ri_i(interp, NATIVECODE, emit_EAX, CUR_OPCODE[2]);
            }
            else {
                emitm_movl_i_m(NATIVECODE, CUR_OPCODE[2], emit_EAX, 0, 1, 0);
            }
            break;
        case PARROT_ARG_FLOATVAL:
            if (recursive) {
                jit_emit_mov_rr_n(NATIVECODE, FSR1, MAP(2));
            }
            else {
                emitm_fld(NATIVECODE, MAP(2));
                jit_emit_fstore_mb_n(interp, NATIVECODE, emit_EAX, 0);
            }
            break;
        case PARROT_ARG_FLOATVAL|PARROT_ARG_CONSTANT:
            if (recursive) {
                jit_emit_mov_ri_n(interp, NATIVECODE, FSR1, &CONST(2)->u.number);
            }
            else {
                jit_emit_mov_ri_n(interp, NATIVECODE, FSR1, &CONST(2)->u.number);
                jit_emit_fstore_mb_n(interp, NATIVECODE, emit_EAX, 0);
            }
            break;
        default:
            Parrot_ex_throw_from_c_args(interp, NULL, 1,
                "set_returns_jit - unknown typ");
            break;
    }
}

void
jit_set_args_pc(Parrot_jit_info_t *jit_info, PARROT_INTERP,
        int recursive)
{
    PMC *sig_args, *sig_params, *sig_result;
    INTVAL *sig_bits, sig, i, n;
    opcode_t *params, *result;
    char params_map;
    int skip, used_n;
    char collision[16];
    const jit_arch_regs *reg_info;

    reg_info = &jit_info->arch_info->regs[jit_info->code_type];

    /* create args array */
    if (!recursive)
        Parrot_ex_throw_from_c_args(interp, NULL, 1,
            "set_args_jit - can't do that yet ");

    sig_args  = Parrot_pcc_get_pmc_constant(interp, CURRENT_CONTEXT(interp), CUR_OPCODE[1]);

    if (!VTABLE_elements(interp, sig_args))
        return;
    params = jit_info->optimizer->sections->begin;
    sig_params = Parrot_pcc_get_pmc_constant(interp, CURRENT_CONTEXT(interp), params[1]);
    ASSERT_SIG_PMC(sig_params);
    GETATTR_FixedIntegerArray_int_array(interp, sig_args, sig_bits);
    n = VTABLE_elements(interp, sig_args);
    /*
     * preserve registers - need get_results, because we skip the
     * return value
     */
    result = CUR_OPCODE + 2 + n + 3; /* set_args, set_p_pc */
    PARROT_ASSERT(*result == PARROT_OP_get_results_pc);
    sig_result = Parrot_pcc_get_pmc_constant(interp, CURRENT_CONTEXT(interp), result[1]);
    ASSERT_SIG_PMC(sig_result);

    if (!VTABLE_elements(interp, sig_result))
        skip = -1;
    else
        skip = MAP(2 + n + 3 + 2);
    used_n = jit_save_regs_call(jit_info, interp, skip);
    memset(collision, 0, 16);
    for (i = 0; i < n; ++i) {
        sig = sig_bits[i];
        /* move args to params regs */
        params_map = jit_info->optimizer->map_branch[2 + i];
        switch (sig & (PARROT_ARG_TYPE_MASK|PARROT_ARG_CONSTANT)) {
            case PARROT_ARG_INTVAL:
                /* if the arg was already set,
                 * we can't use the src again - fetch from stack
                 *
                 * XXX skip
                 *
                 * TODO write a general reg_move
                 */
                if (collision[(int)MAP(2+i)]) {
                    int j;
                    for (j = 0; j < reg_info->n_mapped_I; ++j) {
                        if (reg_info->map_I[j] == MAP(2+i)) {
                            emitm_movl_m_r(interp, NATIVECODE, params_map, emit_ESP,
                                    emit_None, 1,
                                    (used_n * sizeof (FLOATVAL) +
                                     j  * sizeof (INTVAL)));
                            break;
                        }
                    }
                }
                else {
                    if (params_map != MAP(2+i)) {
                        jit_emit_mov_rr_i(NATIVECODE, params_map, MAP(2 + i));
                    }
                }
                collision[(int)params_map] = 1;
                break;
            case PARROT_ARG_INTVAL|PARROT_ARG_CONSTANT:
                /* TODO move constants last */
                jit_emit_mov_ri_i(interp, NATIVECODE, params_map, CUR_OPCODE[2 + i]);
                break;
            case PARROT_ARG_FLOATVAL:
                if (collision[(int)MAP(2+i)]) {
                    int j;
                    for (j = 0; j < reg_info->n_mapped_F; ++j) {
                        if (reg_info->map_F[j] == MAP(2+i)) {
                            jit_emit_fload_mb_n(interp, NATIVECODE, emit_ESP,
                                                (j * sizeof (FLOATVAL)));
                            emitm_fstp(NATIVECODE, (1+params_map));
                            break;
                        }
                    }
                }
                else {
                    jit_emit_mov_rr_n(NATIVECODE, params_map, MAP(2 + i));
                }
                collision[(int)params_map] = 1;
                break;
            case PARROT_ARG_FLOATVAL|PARROT_ARG_CONSTANT:
                jit_emit_mov_ri_n(interp, NATIVECODE, params_map,
                        &CONST(2 + i)->u.number);
                break;
            default:
                Parrot_ex_throw_from_c_args(interp, NULL, 1,
                    "set_args_jit - unknown type");
                break;
        }
    }
}

#  undef CONST

void
Parrot_jit_dofixup(Parrot_jit_info_t *jit_info, PARROT_INTERP)
{
    Parrot_jit_fixup_t *fixup, *next;
    char *fixup_ptr;

    fixup = jit_info->arena.fixups;

    while (fixup) {
        switch (fixup->type) {
        /* This fixes-up a branch to a known opcode offset -
           32-bit displacement only */
            case JIT_X86BRANCH:
                fixup_ptr = Parrot_jit_fixup_target(jit_info, fixup) + 2;
                *(long *)(fixup_ptr) =
                    jit_info->arena.op_map[fixup->param.opcode].offset
                        - (fixup->native_offset + 6) + fixup->skip;
                break;
            case JIT_X86JUMP:
                fixup_ptr = Parrot_jit_fixup_target(jit_info, fixup) + 1;
                *(long *)(fixup_ptr) =
                    jit_info->arena.op_map[fixup->param.opcode].offset
                        - (fixup->native_offset + 5) + fixup->skip;
                break;
            case JIT_X86CALL:
                fixup_ptr = jit_info->arena.start + fixup->native_offset + 1;
                *(long *)(fixup_ptr) = (long)fixup->param.fptr -
                    (long)fixup_ptr - 4;
                break;
            default:
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_JIT_ERROR,
                    "Unknown fixup type:%d\n", fixup->type);
            break;
        }
        next = fixup->next;
        free(fixup);
        fixup = next;
    }
    jit_info->arena.fixups = NULL;
}

int control_word = 0x27f;

#ifdef JIT_CGP

/*
 * This is the somewhat complicated program flow
 *
 * JIT code                     prederef code
 * 1) jit_begin
 *    stack_enter
 *    call cgp_core  -->        set stack frame
 *                              jump to retaddr
 *    test EAX, 0    <--        also from HALT
 *    jnz code_start
 *    stack_leave
 *    ret
 * code_start: of JIT code
 *    jit code
 *    ....
 *
 * 2) normal_op
 *    mov prederef_code_ptr, esi
 *    call *(esi)    ---->      prederefed (non JITted code)
 *                              ....
 *    ....           <----      ret
 *    jit_code
 *    ....
 * 3) HALT == jit_end
 *    mov prederefed_op_func[0], esi
 *    jump *esi      ----->     cleanup prederef stack frame
 *                              xor eax,eax ; return 0
 *                              ret (--> after call cgp_core in 1)
 *
 */

void
Parrot_jit_begin(Parrot_jit_info_t *jit_info,
                 PARROT_INTERP)
{
    jit_emit_stack_frame_enter(jit_info->native_ptr);
    emitm_fldcw(interp, jit_info->native_ptr, &control_word);
    emitm_pushl_r(jit_info->native_ptr, emit_EBX);
    /* get the pc from stack:  mov 12(%ebp), %ebx */
    emitm_movl_m_r(interp, jit_info->native_ptr, emit_EBX, emit_EBP, emit_None, 1, 12);
    /* emit cgp_core(1, interp) */
    /* get the interpreter from stack:  mov 8(%ebp), %eax */
    emitm_movl_m_r(interp, jit_info->native_ptr, emit_EAX, emit_EBP, emit_None, 1, 8);
    emitm_pushl_r(jit_info->native_ptr, emit_EAX);
    /*
     * TODO define the offset of the interpreter on the stack
     *      relative to %ebp
     */
    emitm_pushl_i(jit_info->native_ptr, 1);
    /* use EAX as flag, when jumping back on init, EAX==1 */
    jit_emit_mov_ri_i(interp, jit_info->native_ptr, emit_EAX, 1);
    if (!jit_info->objfile)
        call_func(jit_info, (void (*)(void))cgp_core);
#  if EXEC_CAPABLE
    else {
        Parrot_exec_add_text_rellocation_func(jit_info->objfile,
            jit_info->native_ptr, "cgp_core");
        emitm_calll(jit_info->native_ptr, EXEC_CALLDISP);
    }
#  endif
    /* when cur_opcode == 1, cgp_core jumps back here
     * when EAX == 0, the official return from HALT was called */
    jit_emit_test_r_i(jit_info->native_ptr, emit_EAX);
    emitm_jxs(jit_info->native_ptr, emitm_jnz, 5);
    emitm_popl_r(jit_info->native_ptr, emit_EBX);
    jit_emit_stack_frame_leave(jit_info->native_ptr);
    emitm_ret(jit_info->native_ptr);
    /* get PC = ebx to eax, jump there */
    jit_emit_mov_rr_i(jit_info->native_ptr, emit_EAX, emit_EBX);
    Parrot_emit_jump_to_eax(jit_info, interp);

/* code_start: */
}

#else /* JIT_CGP */

void
Parrot_jit_begin(Parrot_jit_info_t *jit_info,
                 PARROT_INTERP)
{
    /* the generated code gets called as:
     * (jit_code)(interp, pc)
     * jumping to pc is the same code as used in Parrot_jit_cpcf_op()
     */

    /* Maintain the stack frame pointer for the sake of gdb */
    jit_emit_stack_frame_enter(jit_info->native_ptr);
    emitm_fldcw(interp, jit_info->native_ptr, &control_word);
    /* stack:
     *  12   pc
     *   8   interpreter
     *   4   retaddr
     *   0   ebp <----- ebp
     *  -4   ebx .. preserved regs
     *  -8   esi ..
     * -12   edi ..
     * -16   interpreter
     */

    /* Save all callee-saved registers (cdecl)
     */
    emitm_pushl_r(jit_info->native_ptr, emit_EBX);
    emitm_pushl_r(jit_info->native_ptr, emit_ESI);
    emitm_pushl_r(jit_info->native_ptr, emit_EDI);

    /* Cheat on op function calls by writing the interpreter arg on the stack
     * just once. If an op function ever modifies the interpreter argument on
     * the stack this will stop working !!! */

    /* get the interpreter from stack:  mov 8(%ebp), %eax */
    emitm_movl_m_r(interp, jit_info->native_ptr, emit_EAX, emit_EBP, emit_None, 1, 8);
    emitm_pushl_r(jit_info->native_ptr, emit_EAX);

    /* get the pc from stack:  mov 12(%ebp), %eax */
    emitm_movl_m_r(interp, jit_info->native_ptr, emit_EAX, emit_EBP, emit_None, 1, 12);

    /* jump to restart pos or first op */
    Parrot_emit_jump_to_eax(jit_info, interp);
}

#endif /* JIT_CGP */

/*
 * create a JITted version of a PIR sub, where everything
 * resided in registers
 *
 * The sub is called as
 *
 *   opcode_t * func(Interp *i, INTVAL *sig_bits, void **args);
 *
 *   args[0] ...    NULL / return value address
 *   args[1..n] ... addresses of n arguments
 *   args[n + 1] .. opcode_t* next - usually just returned
 */

void
Parrot_jit_begin_sub_regs(Parrot_jit_info_t *jit_info,
                 PARROT_INTERP)
{
    jit_emit_stack_frame_enter(jit_info->native_ptr);
    /* stack:
     *  16   args
     *  12   sig_bits
     *   8   interpreter
     *   4   retaddr
     *   0   ebp <----- ebp
     * [ -4   ebx .. preserved regs ]
     * [ -8   edi .. preserved regs ]
     */
    /*
     * check register usage of the subroutine
     * how many we have to preserve
     */
    jit_save_regs(jit_info, interp);
    /* when it's a recursive sub, we fetch params to registers
     * and all a inner helper sub, which run with registers only
     */
    if (jit_info->flags & JIT_CODE_RECURSIVE) {
        char * L1;
        PMC *sig_result;
        opcode_t *result;

        jit_get_params_pc(jit_info, interp);
        /* remember fixup position - call sub */
        L1 = NATIVECODE;
        emitm_calll(NATIVECODE, 0);
        /* check type of return value */
        result      = Parrot_pcc_get_results(interp, CURRENT_CONTEXT(interp));
        sig_result  = Parrot_pcc_get_pmc_constant(interp, CURRENT_CONTEXT(interp), result[1]);
        if (!VTABLE_elements(interp, sig_result))
            goto no_result;
        /* fetch args to %edx */
        emitm_movl_m_r(interp, NATIVECODE, emit_EDX, emit_EBP, emit_None, 1, 16);
        emitm_movl_m_r(interp, NATIVECODE, emit_ECX, emit_EDX, emit_None, 1, 0);
        if (VTABLE_get_integer_keyed_int(interp, sig_result, 0) ==
                PARROT_ARG_FLOATVAL) {
            jit_emit_fst_mb_n(interp, jit_info->native_ptr, emit_ECX, 0);
        }
        else {
            emitm_movl_r_m(interp, NATIVECODE, emit_EAX, emit_ECX, 0, 1, 0);
        }
no_result:
        /* return 0 - no branch */
        jit_emit_bxor_rr_i(interp, NATIVECODE, emit_EAX, emit_EAX);
        /* restore pushed callee saved */
        jit_restore_regs(jit_info, interp);
        jit_emit_stack_frame_leave(NATIVECODE);
        emitm_ret(NATIVECODE);
        /* align the inner sub */
#if SUB_ALIGN
        while ((long)jit_info->native_ptr & ((1<<SUB_ALIGN) - 1)) {
            jit_emit_noop(jit_info->native_ptr);
        }
#endif
        /* fixup call statement */
        L1[1] = NATIVECODE - L1 - 5;
    }
    /* TODO be sure we got a label here in map_branch */
}

void
Parrot_jit_begin_sub(Parrot_jit_info_t *jit_info,
                 PARROT_INTERP)
{
}

void
jit_mov_mr_n_offs(PARROT_INTERP, Parrot_jit_info_t *jit_info,
        int base_reg, INTVAL offs, int src_reg)
{
    emitm_fld(jit_info->native_ptr, src_reg);
    jit_emit_fstore_mb_n(interp, jit_info->native_ptr, base_reg, offs);
}

void
jit_mov_mr_offs(PARROT_INTERP, Parrot_jit_info_t *jit_info,
        int base_reg, INTVAL offs, int src_reg)
{
    emitm_movl_r_m(interp, jit_info->native_ptr,
            src_reg, base_reg, emit_None, 1, offs);
}

void
jit_mov_rm_n_offs(PARROT_INTERP, Parrot_jit_info_t *jit_info,
        int dst_reg, int base_reg, INTVAL offs)
{
    jit_emit_fload_mb_n(interp, jit_info->native_ptr, base_reg,  offs);
    emitm_fstp(jit_info->native_ptr, (dst_reg+1));
}

void
jit_mov_rm_offs(PARROT_INTERP, Parrot_jit_info_t *jit_info,
        int dst_reg, int base_reg, INTVAL offs)
{
    emitm_movl_m_r(interp, jit_info->native_ptr,
            dst_reg, base_reg, emit_None, 1, offs);
}

void
Parrot_jit_emit_finit(Parrot_jit_info_t *jit_info)
{
    jit_emit_finit(jit_info->native_ptr);
}

#ifdef JIT_CGP
/*
 * XXX needs some fixing
 * s. t/sub/pmc_{8,9}.t: the 2 print in tail call without that 'end'
 *    are recognized as one non JITted block
 */
void
Parrot_jit_normal_op(Parrot_jit_info_t *jit_info,
                     PARROT_INTERP)
{
    Parrot_jit_optimizer_section_ptr cur_section =
        jit_info->optimizer->cur_section;
    int last_is_branch = 0;
    void ** offset;

    PARROT_ASSERT(op_jit[*jit_info->cur_op].extcall == 1);
    if (cur_section->done == 1)
        return;
    else if (cur_section->done == -1 && --cur_section->ins_count > 0)
        return;
    /* check, where section ends
     */
    if (interp->op_info_table[*cur_section->end].jump)
        last_is_branch = 1;
    else if (cur_section->next && !cur_section->next->isjit)
        last_is_branch = 1;
    /* if more then 1 op, then jump to CGP, branches are never
     * executed in CGP, they are handled below */
    if (cur_section->done >= 0 &&
            (INTVAL)cur_section->op_count >= 2 + last_is_branch) {
        int saved = 0;
        offset = (jit_info->cur_op - interp->code->base.data) +
            interp->code->prederef.code;

        jit_emit_mov_ri_i(interp, jit_info->native_ptr, emit_ESI, offset);
        emitm_callm(jit_info->native_ptr, emit_ESI, 0, 0, 0);
        /* now patch a B<cpu_ret> opcode after the end of the
         * prederefed (non JIT) section */
        if (last_is_branch) {
            offset = (cur_section->end - interp->code->base.data) +
                interp->code->prederef.code;
            cur_section->done = -1;
            /* ins to skip */
            cur_section->ins_count = cur_section->op_count - 1;
        }
        else {
            /* There must be a next section: either we have a B<end>
             * or a JITed branch,
             * when the branch is non JIT, we are in the above case
             */
            offset = (cur_section->next->begin - interp->code->base.data)
                + interp->code->prederef.code;
            cur_section->done = 1;
        }
        *offset = ((op_func_t*)interp->op_lib->op_func_table)[PARROT_OP_cpu_ret];
    }
    else {
        /* else call normal funtion */
        emitm_pushl_i(jit_info->native_ptr, interp);
        emitm_pushl_i(jit_info->native_ptr, jit_info->cur_op);
        call_func(jit_info,
            (void (*)(void))interp->op_func_table[*(jit_info->cur_op)]);
        emitm_addb_i_r(jit_info->native_ptr, 8, emit_ESP);
        /* when this was a branch, then EAX is now the offset
         * in the byte_code
         */
    }
}

#else /* JIT_CGP */
extern int jit_op_count(void);

void
Parrot_jit_normal_op(Parrot_jit_info_t *jit_info,
                     PARROT_INTERP)
{
    int cur_op = *jit_info->cur_op;
    static int check;

    if (cur_op >= jit_op_count()) {
        cur_op = CORE_OPS_wrapper__;
    }

    if ((++check & 0x7) == 0) {
        /*
         * every 8 ??? normal ops, we emit a check for event processing
         */

/*
 * There is an optimization to reuse arguments on the stack.  Compilers may
 * decide to reuse the argument space though.  If you are *absolutely sure*
 * this does not happen define PARROT_JIT_STACK_REUSE_INTERP.
 */
#  ifdef PARROT_JIT_STACK_REUSE_INTERP
        /*
        * op functions have the signature (cur_op, interp)
        * we use the interpreter already on stack and only push the
        * cur_op
        */
#  else
        /* push interpreter */
        Parrot_jit_emit_get_INTERP(interp, jit_info->native_ptr, emit_ECX);
        emitm_pushl_r(jit_info->native_ptr, emit_ECX);
#  endif

        emitm_pushl_i(jit_info->native_ptr, CORE_OPS_check_events);

        call_func(jit_info,
            (void (*) (void)) (interp->op_func_table[CORE_OPS_check_events]));
#  ifdef PARROT_JIT_STACK_REUSE_INTERP
        emitm_addb_i_r(jit_info->native_ptr, 4, emit_ESP);
#  else
        emitm_addb_i_r(jit_info->native_ptr, 8, emit_ESP);
#  endif
    }

#  ifdef PARROT_JIT_STACK_REUSE_INTERP
    /*
    * op functions have the signature (cur_op, interp)
    * we use the interpreter already on stack and only push the
    * cur_op
    */
#  else
    Parrot_jit_emit_get_INTERP(interp, jit_info->native_ptr, emit_ECX);
    emitm_pushl_r(jit_info->native_ptr, emit_ECX);
#  endif

    emitm_pushl_i(jit_info->native_ptr, jit_info->cur_op);

    call_func(jit_info,
            (void (*) (void))(interp->op_func_table[cur_op]));
#  ifdef PARROT_JIT_STACK_REUSE_INTERP
    emitm_addb_i_r(jit_info->native_ptr, 4, emit_ESP);
#  else
    emitm_addb_i_r(jit_info->native_ptr, 8, emit_ESP);
#  endif
}

#endif /* JIT_CGP */

void
Parrot_jit_cpcf_op(Parrot_jit_info_t *jit_info,
                   PARROT_INTERP)
{
    Parrot_jit_normal_op(jit_info, interp);
    Parrot_emit_jump_to_eax(jit_info, interp);
}

void
Parrot_jit_restart_op(Parrot_jit_info_t *jit_info,
                   PARROT_INTERP)
{
    char *jmp_ptr, *sav_ptr;

    Parrot_jit_normal_op(jit_info, interp);
    /* test eax, if zero (e.g after trace), return from JIT */
    jit_emit_test_r_i(jit_info->native_ptr, emit_EAX);
    /* remember PC */
    jmp_ptr = jit_info->native_ptr;
    /* emit jump past exit code, dummy offset
     * this assumes exit code is not longer then a short jump (126 bytes) */
    emitm_jxs(jit_info->native_ptr, emitm_jnz, 0);
    jit_emit_end(jit_info->native_ptr);
    /* fixup above jump */
    sav_ptr = jit_info->native_ptr;
    jit_info->native_ptr = jmp_ptr;
    emitm_jxs(jit_info->native_ptr, emitm_jnz, (long)(sav_ptr - jmp_ptr) - 2);
    /* restore PC */
    jit_info->native_ptr = sav_ptr;
    Parrot_emit_jump_to_eax(jit_info, interp);
}

/*
 * params are put rigth to left on the stack
 * parrot registers are counted left to right
 * so this function returns for a given register type
 * the needed register number
 * TODO handel overflow params
 */

int
count_regs(PARROT_INTERP, char *sig, char *sig_start)
{
    const char *typs[] = {
        "Ilisc",         /* I */
        "StbB",          /* S */
        "pP234",         /* P */
        "Nfd"            /* N */
    };
    int first_reg = 0;
    int i, found;

    /* char at sig is the type to look at */
    for (found = -1, i = 0; i < 4; i++) {
        if (strchr(typs[i], *sig)) {
            found = i;
            break;
        }
    }

    if (found == -1)
        Parrot_ex_throw_from_c_args(interp, NULL, 1,
            "Parrot_jit_build_call_func: sig char not found\n");

    for (--sig; sig > sig_start; --sig) {
        if (strchr(typs[found], *sig)) {
            ++first_reg;
        }
    }
    return first_reg;
}

size_t
calc_signature_needs(const char *sig, int *strings)
{
    size_t stack_size = 0;
    while (*sig) {
        switch (*sig) {
            case 't':
                (*strings)++;
                stack_size +=4;
                break;
            case 'd':
                stack_size +=8;
                break;
            default:
                stack_size +=4;
                break;
        }
        sig++;
    }
    return stack_size;

}

/*
 * The function generated here is called as func(interp, nci_info)
 * interp   ...  8(%ebp)
 * nci_info ... 12(%ebp)
 *
 * The generate function for a specific signature looks quite similar to
 * an optimized compile of src/nci.c:pcf_x_yy(). In case of any troubles
 * just compare the disassembly.
 *
 * If a non-NULL sizeptr is passed, the integer it points to will be written
 * with the size of the allocated execmem buffer.
 */

void *
Parrot_jit_build_call_func(PARROT_INTERP, PMC *pmc_nci, STRING *signature, int *sizeptr)
{
    Parrot_jit_info_t jit_info;
    char     *pc;
    int       i                    = 0;
    int       arg_count            = 0;
    int       string_buffer_count  = 0;
    const int ST_SIZE_OF           = 124;
    const int JIT_ALLOC_SIZE       = 1024;

    char      *signature_str      = Parrot_str_to_cstring(interp, signature);
    /* skip over the result */
    char      *sig                = signature_str + 1;
    size_t     stack_space_needed = calc_signature_needs(sig,
                                        &string_buffer_count);

    int base_offset        = 0;
    int strings_offset     = base_offset - (sizeof (char *) * string_buffer_count);
    int st_offset          = strings_offset - ST_SIZE_OF;
    int args_offset        = st_offset - stack_space_needed;
    int temp_calls_offset  = args_offset - 16;
    int total_stack_needed = -temp_calls_offset;

    /*
     * ESP
     * 0-15, 16 bytes for utility calls
     * stack_space_needed for actual NCI call
     * st
     * STRINGS -> char * holding space
     * EBP
     */

    /* this ought to be enough - the caller of this function
     * should free the function pointer returned here
     */
    pc = jit_info.native_ptr = jit_info.arena.start = (char *)mem_alloc_executable(JIT_ALLOC_SIZE);
    if (! pc)
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_JIT_ERROR,
                "Cannot allocate executable memory");


    /* this generated jit function will be called as (INTERP (EBP 8), func_ptr
    * (ESP 12), args signature (ESP 16)) */

    /* make stack frame, preserve %ebx */
    jit_emit_stack_frame_enter(pc);

    emitm_subl_i_r(pc, total_stack_needed, emit_ESP);

    /* Parrot_init_arg_nci(interp, &st, "S"); */
    /* args signature "S" */
    emitm_movl_m_r(interp, pc, emit_EAX, emit_EBP, 0, 1, 16);
    emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 8);

    /*&st*/
    emitm_lea_m_r(interp,  pc, emit_EAX, emit_EBP, 0, 1, st_offset);
    emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 4);

    /*interpreter*/
    emitm_movl_m_r(interp, pc, emit_EAX, emit_EBP, 0, 1, 8);
    emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 0);

    if (sig && *sig)
      emitm_call_cfunc(pc, Parrot_init_arg_nci);

    while (*sig) {
        emitm_movl_i_m(pc, arg_count, emit_EBP, 0, 1, temp_calls_offset + 8);

        switch (*sig) {
            case '0':    /* null ptr or such - doesn't consume a reg */
                jit_emit_bxor_rr_i(interp, pc, emit_EAX, emit_EAX);
                emitm_movl_m_r(interp, pc, emit_EAX, emit_EBP, 0, 1, args_offset);
                break;
            case 'f':
                emitm_call_cfunc(pc, get_nci_N);
                emitm_fstps(interp, pc, emit_EBP, 0, 1, args_offset);
                break;
            case 'N':
            case 'd':
                emitm_call_cfunc(pc, get_nci_N);
                emitm_fstpl(interp, pc, emit_EBP, 0, 1, args_offset);
                args_offset += 4;
                break;
            case 'I':   /* INTVAL */
            case 'l':   /* long */
            case 'i':   /* int */
                emitm_call_cfunc(pc, get_nci_I);
                emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, args_offset);
                break;
            case 't':   /* string, pass a cstring */
                emitm_call_cfunc(pc, get_nci_S);
                emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 4);
                emitm_call_cfunc(pc, string_to_cstring_nullable);

                emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, args_offset);
                /* save off temporary allocation address */
                emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, strings_offset);
                strings_offset += 4;

                /* reset ESP(4) */
                emitm_lea_m_r(interp, pc, emit_EAX, emit_EBP, 0, 1, st_offset);
                emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 4);
                break;
            case 's':   /* short: movswl intreg_o(base), %eax */
                emitm_call_cfunc(pc, get_nci_I);
                emitm_movswl_r_r(pc, emit_EAX, emit_EAX);
                emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, args_offset);
                break;
            case 'c':   /* char: movsbl intreg_o(base), %eax */
                emitm_call_cfunc(pc, get_nci_I);
                emitm_movsbl_r_r(pc, emit_EAX, emit_EAX);
                emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, args_offset);
                break;
            case 'J':   /* interpreter */
                emitm_movl_m_r(interp, pc, emit_EAX, emit_EBP, 0, 1, 8);
                emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, args_offset);
                arg_count--;
                break;
            case 'p':   /* push pmc->data */
                emitm_call_cfunc(pc, get_nci_P);
                /* save off PMC* */
                emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 4);
                /* lookup get_pointer in VTABLE */
                emitm_movl_m_r(interp, pc, emit_EAX, emit_EAX, 0, 1, offsetof(PMC, vtable));
                emitm_movl_m_r(interp, pc, emit_EAX, emit_EAX, 0, 1, offsetof(VTABLE, get_pointer));
                emitm_callr(pc, emit_EAX);
                emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, args_offset);
                /* reset ESP(4) */
                emitm_lea_m_r(interp, pc, emit_EAX, emit_EBP, 0, 1, st_offset);
                emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 4);

                break;
            case 'O':   /* push PMC * object in P2 */
            case 'P':   /* push PMC * */
            case '@':
                emitm_call_cfunc(pc, get_nci_P);
#if PARROT_CATCH_NULL
                /* PMCNULL is a global */
                jit_emit_cmp_rm_i(pc, emit_EAX, &PMCNULL);
                emitm_jxs(pc, emitm_jne, 2); /* skip the xor */
                jit_emit_bxor_rr_i(interp, pc, emit_EAX, emit_EAX);
#endif
                emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, args_offset);
                break;
            case 'v':
                break;
            case 'b':   /* buffer (void*) pass Buffer_bufstart(SReg) */
                emitm_call_cfunc(pc, get_nci_S);
                emitm_movl_m_r(interp, pc, emit_EAX, emit_EAX, 0, 1,
                               (size_t) &Buffer_bufstart((STRING *) NULL));
                emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, args_offset);
                break;
            case 'B':   /* buffer (void**) pass &Buffer_bufstart(SReg) */
                emitm_call_cfunc(pc, get_nci_S);
                emitm_lea_m_r(interp, pc, emit_EAX, emit_EAX, 0, 1,
                              (size_t) &Buffer_bufstart((STRING *) NULL));
                emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, args_offset);
                break;
            case 'S':
                emitm_call_cfunc(pc, get_nci_S);
                emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, args_offset);
                break;


            /* I have no idea how to handle these */
            case '2':
            case '3':
            case '4':
            case 'V':
                mem_free_executable(jit_info.native_ptr, JIT_ALLOC_SIZE);
                Parrot_str_free_cstring(signature_str);
                return NULL;
                break;
            default:
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_JIT_ERROR,
                    "Unknown arg Signature %c\n", *sig);
                /*
                 * oops unknown signature:
                 * cleanup and try nci.c
                 */
                mem_free_executable(jit_info.native_ptr, JIT_ALLOC_SIZE);
                Parrot_str_free_cstring(signature_str);
                return NULL;
        }
        args_offset +=4;
        arg_count++;
        sig++;
    }

    /* prepare to call VTABLE_get_pointer, set up args */
    /* interpreter - movl 8(%ebp), %eax */
    emitm_movl_m_r(interp, pc, emit_EAX, emit_EBP, 0, 1, 8);
    emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 0);

    /* pmc - movl 12(%ebp), %eax */
    emitm_movl_m_r(interp, pc, emit_EAX, emit_EBP, 0, 1, 12);
    emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 4);

    /* get the get_pointer() pointer from the pmc's vtable */
    emitm_movl_m_r(interp, pc, emit_EAX, emit_EAX, 0, 1, offsetof(PMC, vtable));
    emitm_movl_m_r(interp, pc, emit_EAX, emit_EAX, 0, 1, offsetof(VTABLE, get_pointer));

    /* call get_pointer(), result goes into eax */
    emitm_callr(pc, emit_EAX);
    emitm_addl_i_r(pc, 16, emit_ESP);

    /* call the resulting function pointer */
    emitm_callr(pc, emit_EAX);
    emitm_subl_i_r(pc, 16, emit_ESP);

    /* SAVE OFF EAX */
    emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 8);

    /*&st*/
    emitm_lea_m_r(interp, pc, emit_EAX, emit_EBP, 0, 1, st_offset);
    emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 4);

    /*interpreter*/
    emitm_movl_m_r(interp, pc, emit_EAX, emit_EBP, 0, 1, 8);
    emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 0);

    /* RESTORE BACK EAX */
    emitm_movl_m_r(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 8);

    /* now place return value in registers */
    /* first in signature is the return value */
    sig = signature_str; /* the result */
    switch (*sig) {
        /* I have no idea how to handle these */
        case '2':
        case '3':
        case '4':
            /* get integer from pointer - untested */
            emitm_movl_m_r(interp, pc, emit_EAX, emit_EAX, 0, 1, 0);
            if (*sig == 2)      /* short */
                emitm_movswl_r_r(pc, emit_EAX, emit_EAX);
            emitm_call_cfunc(pc, set_nci_I);
            break;
        case 'f':
        case 'd':
            jit_emit_fstore_mb_n(interp, pc, emit_EBP, temp_calls_offset + 8);
            emitm_call_cfunc(pc, set_nci_N);
            /* pop num from st(0) and mov to reg */
            break;
        case 's':
            /* movswl %ax, %eax */
            emitm_movswl_r_r(pc, emit_EAX, emit_EAX);
            emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 8);
            emitm_call_cfunc(pc, set_nci_I);
            break;
        case 'c':
            /* movsbl %al, %eax */
            emitm_movsbl_r_r(pc, emit_EAX, emit_EAX);
            emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 8);
            emitm_call_cfunc(pc, set_nci_I);
            break;
        case 'I':   /* INTVAL */
        case 'l':
        case 'i':
            emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 8);
            emitm_call_cfunc(pc, set_nci_I);
            break;
        case 'v': /* void - do nothing */
            break;
        case 'P':
            emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 8);
            emitm_call_cfunc(pc, set_nci_P);
            break;
        case 'p':   /* make a new unmanaged struct */
            /* save return value on stack */

            /* save pointer p */
            emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 12);

            /* make new pmc */
            emitm_movl_i_m(pc, enum_class_UnManagedStruct, emit_EBP, 0, 1, temp_calls_offset + 4);
            emitm_call_cfunc(pc, pmc_new);

            /* restore pointer p to EDX */
            emitm_movl_m_r(interp, pc, emit_EDX, emit_EBP, 0, 1, temp_calls_offset + 12);

            /* copy UnManagedStruct to stack for set_nci_P call */
            emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 8);

            /* eax = PMC, get return value into edx */
            /* mov data(%eax), %eax
               mov %edx, ptr(%eax) */
            emitm_movl_m_r(interp, pc, emit_EAX, emit_EAX, 0, 1, offsetof(struct PMC, data));
            emitm_movl_r_m(interp, pc, emit_EDX, emit_EAX, 0, 1,
                           offsetof(struct Parrot_UnManagedStruct_attributes, ptr));

            /* reset EBP(4) */
            emitm_lea_m_r(interp, pc, emit_EAX, emit_EBP, 0, 1, st_offset);
            emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 4);

            emitm_call_cfunc(pc, set_nci_P);
            break;
        case 'S':
            emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 8);
            emitm_call_cfunc(pc, set_nci_S);
            break;
        case 't':   /* string */
            /* EAX is char* */
            emitm_movl_i_m(pc, 0, emit_EBP, 0, 1, temp_calls_offset + 8); /* len */

            /* overwrites address of st in EBP(4) */
            emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 4);

            emitm_call_cfunc(pc, Parrot_str_new);

            emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 8);

            /* reset EBP(4) */
            emitm_lea_m_r(interp, pc, emit_EAX, emit_EBP, 0, 1, st_offset);
            emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 4);

            emitm_call_cfunc(pc, set_nci_S);
            break;
        default:
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_JIT_ERROR,
                "Unknown return Signature %c\n", *sig);
            /*
             * oops unknown signature:
             * cleanup and try nci.c
             */
            Parrot_str_free_cstring(signature_str);
            mem_free_executable(jit_info.native_ptr, JIT_ALLOC_SIZE);
            return NULL;
    }

    /* free temporary strings */
    strings_offset = st_offset + ST_SIZE_OF;
    for (i=0; i<string_buffer_count; i++) {
        emitm_movl_m_r(interp, pc, emit_EAX, emit_EBP, 0, 1, strings_offset);
        emitm_movl_r_m(interp, pc, emit_EAX, emit_EBP, 0, 1, temp_calls_offset + 0);
        emitm_call_cfunc(pc, Parrot_str_free_cstring);
        strings_offset += 4;
    }

    jit_emit_stack_frame_leave(pc);
    emitm_ret(pc);
    PARROT_ASSERT(pc - jit_info.arena.start <= JIT_ALLOC_SIZE);

    /* could shrink arena.start here to used size */

    if (sizeptr)
        *sizeptr = JIT_ALLOC_SIZE;
    Parrot_str_free_cstring(signature_str);
    return (void *)D2FPTR(jit_info.arena.start);
}

const char i_map[] =
    { emit_EDI, emit_ESI, emit_EDX, emit_ECX };
const char floatval_map[] =
    { 1, 2, 3, 4, 5 };         /* ST(1) .. (ST(4) */

const char i_map_sub[] =
    { emit_EDX, emit_ECX, emit_EBX, emit_EDI, emit_ESI };

const jit_arch_info arch_info = {
    jit_mov_rm_offs,
    jit_mov_rm_n_offs,
    jit_mov_mr_offs,
    jit_mov_mr_n_offs,
    Parrot_jit_dofixup,
    (jit_arch_f)0,              /* no cache flush needed */
    {
        {
            Parrot_jit_begin,   /* emit code prologue */
            4,                  /* 4 mapped ints */
            2,                  /* first 2 are preserved */
            i_map,
            4,                  /* 4 mapped float regs */
            0,                  /* ABI sez it's not preserved */
            floatval_map
        },
        /* unused */
        {
            Parrot_jit_begin_sub,   /* emit code prologue */
            4,                  /* 4 mapped ints */
            2,                  /* first 2 are *non*preserved */
            i_map_sub,
            4,                  /* 4 mapped float regs */
            0,                  /* ABI sez it's not preserved */
            floatval_map
        },
        /*
         * compile a sub to registers only
         * if a mapped count is 0, code containing this register kind
         * will not be created
         *
         * TODO implement FLOATVAL arg passing and set mapped
         * register count.
         */
        {
            Parrot_jit_begin_sub_regs,   /* emit code prologue */
            5,                  /* 5 mapped ints */
            2,                  /* first 2 are *non*preserved */
            i_map_sub,
            5,                  /* TODO 0 mapped float regs */
            5,                  /* ABI sez it's not preserved */
            floatval_map
        }
    }
};


PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
const jit_arch_info *
Parrot_jit_init(PARROT_INTERP)
{
    return &arch_info;
}

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
