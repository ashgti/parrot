/*
 * exec_dep.h
 *
 * CVS Info
 *    $Id$
 * Overview:
 *    ARM dependent functions to emit an executable.
 * History:
 *      Initial version by Daniel Grunblatt on 2003.8.12
 * Notes:
 * References:
 */

#ifndef EXEC_DEP_H_GUARD
#  define EXEC_DEP_H_GUARD

#  ifdef JIT_CGP

void
Parrot_exec_normal_op(Parrot_jit_info_t *jit_info,
                     struct Parrot_Interp * interpreter)
{
}

#  else /* JIT_CGP */

void
Parrot_exec_normal_op(Parrot_jit_info_t *jit_info,
                     struct Parrot_Interp * interpreter)
{
    jit_info->native_ptr = emit_mov (jit_info->native_ptr, r1, r4);
#   ifndef ARM_K_BUG
    jit_info->native_ptr = emit_mov (jit_info->native_ptr, REG14_lr, REG15_pc);
    jit_info->native_ptr = emit_ldmstm (jit_info->native_ptr,
                                        cond_AL, is_load, dir_IA,
                                        is_writeback,
                                        REG14_lr,
                                        reg2mask(0) | reg2mask(REG15_pc));
#   else
    jit_info->native_ptr = emit_arith_immediate (jit_info->native_ptr, cond_AL,
                                                 ADD, 0, REG14_lr, REG15_pc,
                                                 4, 0);
    jit_info->native_ptr = emit_ldmstm (jit_info->native_ptr,
                                        cond_AL, is_load, dir_IA,
                                        is_writeback,
                                        REG14_lr,
                                        reg2mask(0) | reg2mask(REG12_ip));
    jit_info->native_ptr = emit_mov (jit_info->native_ptr, REG15_pc, REG12_ip);
#   endif
    Parrot_exec_add_text_rellocation(jit_info->objfile,
        jit_info->native_ptr, RTYPE_DATA, "program_code", 0);
    jit_info->native_ptr
        = emit_word (jit_info->native_ptr, ((int)jit_info->cur_op) -
            ((int)interpreter->code->byte_code) +
                (jit_info->objfile->bytecode_header_size));
    Parrot_exec_add_text_rellocation(jit_info->objfile,
        jit_info->native_ptr, RTYPE_FUNC,
            interpreter->op_info_table[*jit_info->cur_op].func_name, 0);
    jit_info->native_ptr
        = emit_word (jit_info->native_ptr, 0);
}

#  endif /* JIT_CGP */

void
Parrot_exec_cpcf_op(Parrot_jit_info_t *jit_info,
                   struct Parrot_Interp * interpreter)
{
    Parrot_exec_normal_op(jit_info, interpreter);
    Parrot_jump_to_op_in_reg(jit_info, interpreter, r0);
}

void
Parrot_exec_restart_op(Parrot_jit_info_t *jit_info,
                       struct Parrot_Interp * interpreter)
{
}

void
Parrot_exec_emit_mov_mr(struct Parrot_Interp * interpreter, char *mem, int reg)
{
    Parrot_jit_emit_mov_mr(interpreter, mem, reg);
}

void
Parrot_exec_emit_mov_rm(struct Parrot_Interp * interpreter, int reg, char *mem)
{
    Parrot_jit_emit_mov_rm(interpreter, reg, mem);
}

void
Parrot_exec_emit_mov_mr_n(struct Parrot_Interp * interpreter, char *mem,int reg)
{
    Parrot_jit_emit_mov_mr_n(interpreter, mem, reg);
}

void
Parrot_exec_emit_mov_rm_n(struct Parrot_Interp * interpreter, int reg,char *mem)
{
    Parrot_jit_emit_mov_rm_n(interpreter, reg, mem);
}

/* Assign the offset of the progra_code */
static void
offset_fixup(Parrot_exec_objfile_t *obj)
{
    int i,j;

    for (i = 0; i < obj->data_count; i++) {
        for (j = 0; j < i; j++) 
            obj->symbol_table[i].value += obj->data_size[j];
    }
}

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
