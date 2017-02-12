/**
 * This file is part of DBrew, the dynamic binary rewriting library.
 *
 * (c) 2015-2016, Josef Weidendorfer <josef.weidendorfer@gmx.de>
 *
 * DBrew is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License (LGPL)
 * as published by the Free Software Foundation, either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * DBrew is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with DBrew.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 **/

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <llvm-c/Core.h>

#include <instr.h>

#include <llinstruction-internal.h>

#include <llbasicblock-internal.h>
#include <llcommon.h>
#include <llcommon-internal.h>
#include <llflags-internal.h>
#include <llfunction.h>
#include <llfunction-internal.h>
#include <lloperand-internal.h>
#include <llsupport-internal.h>

/**
 * \defgroup LLInstructionSSE SSE Instructions
 * \ingroup LLInstruction
 *
 * @{
 **/

void
ll_instruction_movq(Instr* instr, LLState* state)
{
    LLVMValueRef operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src, state);
    if (opIsVReg(&instr->dst))
        ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_ZERO_UPPER_SSE, operand1, state);
    else
        ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, operand1, state);
}

void
ll_instruction_movs(Instr* instr, LLState* state)
{
    OperandDataType type = instr->type == IT_MOVSS ? OP_SF32 : OP_SF64;
    LLVMValueRef operand1 = ll_operand_load(type, ALIGN_MAXIMUM, &instr->src, state);

    if (opIsInd(&instr->src))
        ll_operand_store(type, ALIGN_MAXIMUM, &instr->dst, REG_ZERO_UPPER_SSE, operand1, state);
    else
        ll_operand_store(type, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, operand1, state);
}

void
ll_instruction_movp(Instr* instr, LLState* state)
{
    Alignment alignment = instr->type == IT_MOVAPS || instr->type == IT_MOVAPD ? ALIGN_MAXIMUM : ALIGN_8;
    OperandDataType type = instr->type == IT_MOVAPS || instr->type == IT_MOVUPS ? OP_VF32 : OP_VF64;

    LLVMValueRef operand1 = ll_operand_load(type, alignment, &instr->src, state);
    ll_operand_store(type, alignment, &instr->dst, REG_KEEP_UPPER, operand1, state);
}

void
ll_instruction_movdq(Instr* instr, LLState* state)
{
    Alignment alignment = instr->type == IT_MOVDQA ? ALIGN_MAXIMUM : ALIGN_8;

    LLVMValueRef operand1 = ll_operand_load(OP_VI64, alignment, &instr->src, state);
    ll_operand_store(OP_VI64, alignment, &instr->dst, REG_KEEP_UPPER, operand1, state);
}

void
ll_instruction_movlp(Instr* instr, LLState* state)
{
    OperandDataType type = instr->type == IT_MOVLPS ? OP_VF32 : OP_SF64;
    LLVMValueRef operand1 = ll_operand_load(type, ALIGN_MAXIMUM, &instr->src, state);
    ll_operand_store(type, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, operand1, state);
}

void
ll_instruction_movhps(Instr* instr, LLState* state)
{
    LLVMTypeRef i32 = LLVMInt32TypeInContext(state->context);

    if (opIsVReg(&instr->dst))
    {
        LLVMValueRef maskElements[4];
        maskElements[0] = LLVMConstInt(i32, 0, false);
        maskElements[1] = LLVMConstInt(i32, 1, false);
        maskElements[2] = LLVMConstInt(i32, 4, false);
        maskElements[3] = LLVMConstInt(i32, 5, false);
        LLVMValueRef mask = LLVMConstVector(maskElements, 4);

        LLVMValueRef operand1 = ll_operand_load(OP_VF32, ALIGN_MAXIMUM, &instr->dst, state);
        LLVMValueRef operand2 = ll_operand_load(OP_VF32, ALIGN_MAXIMUM, &instr->src, state);
        LLVMValueRef result = LLVMBuildShuffleVector(state->builder, operand1, operand2, mask, "");
        ll_operand_store(OP_VF32, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
    }
    else
    {
        // Hack to ensure that the destination receives a <2 x float>.
        opOverwriteType(&instr->dst, VT_64);

        LLVMValueRef maskElements[2];
        maskElements[0] = LLVMConstInt(i32, 2, false);
        maskElements[1] = LLVMConstInt(i32, 3, false);
        LLVMValueRef mask = LLVMConstVector(maskElements, 2);

        LLVMValueRef operand1 = ll_operand_load(OP_VF32, ALIGN_MAXIMUM, &instr->src, state);
        LLVMValueRef result = LLVMBuildShuffleVector(state->builder, operand1, LLVMGetUndef(LLVMTypeOf(operand1)), mask, "");
        ll_operand_store(OP_VF32, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
    }
}

void
ll_instruction_movhpd(Instr* instr, LLState* state)
{
    LLVMTypeRef i32 = LLVMInt32TypeInContext(state->context);

    if (opIsVReg(&instr->dst))
    {
        LLVMValueRef operand1 = ll_operand_load(OP_VF64, ALIGN_MAXIMUM, &instr->dst, state);
        LLVMValueRef operand2 = ll_operand_load(OP_SF64, ALIGN_MAXIMUM, &instr->src, state);
        LLVMValueRef result = LLVMBuildInsertElement(state->builder, operand1, operand2, LLVMConstInt(i32, 1, false), "");
        ll_operand_store(OP_VF64, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
    }
    else
    {
        LLVMValueRef operand1 = ll_operand_load(OP_VF64, ALIGN_MAXIMUM, &instr->src, state);
        LLVMValueRef result = LLVMBuildExtractElement(state->builder, operand1, LLVMConstInt(i32, 1, false), "");
        ll_operand_store(OP_SF64, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
    }
}

void
ll_instruction_unpckl(Instr* instr, LLState* state)
{
    LLVMTypeRef i32 = LLVMInt32TypeInContext(state->context);
    OperandDataType type = instr->type == IT_UNPCKLPS ? OP_VF32 : OP_VF64;

    LLVMValueRef maskElements[4];
    LLVMValueRef mask;

    if (instr->type == IT_UNPCKLPS)
    {
        maskElements[0] = LLVMConstInt(i32, 0, false);
        maskElements[1] = LLVMConstInt(i32, 4, false);
        maskElements[2] = LLVMConstInt(i32, 1, false);
        maskElements[3] = LLVMConstInt(i32, 5, false);
        mask = LLVMConstVector(maskElements, 4);
    }
    else // IT_UNPCKLPD
    {
        maskElements[0] = LLVMConstInt(i32, 0, false);
        maskElements[1] = LLVMConstInt(i32, 2, false);
        mask = LLVMConstVector(maskElements, 2);
    }

    LLVMValueRef operand1 = ll_operand_load(type, ALIGN_MAXIMUM, &instr->dst, state);
    LLVMValueRef operand2 = ll_operand_load(type, ALIGN_MAXIMUM, &instr->src, state);
    LLVMValueRef result = LLVMBuildShuffleVector(state->builder, operand1, operand2, mask, "");
    ll_operand_store(type, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
}

/**
 * @}
 **/