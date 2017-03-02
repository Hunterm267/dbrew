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
#include <string.h>
#include <llvm-c/Core.h>

#include <instr.h>
#include <printer.h>

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
 * \defgroup LLInstruction Instruction
 * \brief Handling of X86-64 instructions
 *
 * @{
 **/

/**
 * Handling of push, pop and leave instructions.
 *
 * \todo Handle 16 bit integers
 *
 * \private
 *
 * \author Alexis Engelke
 *
 * \param instr The instruction
 * \param state The module state
 **/
static void
ll_generate_instruction_stack(Instr* instr, LLState* state)
{
    LLVMTypeRef i8 = LLVMInt8TypeInContext(state->context);
    LLVMTypeRef i64 = LLVMInt64TypeInContext(state->context);
    LLVMTypeRef pi8 = LLVMPointerType(i8, 0);
    LLVMTypeRef pi64 = LLVMPointerType(i64, 0);

    // In case of a leave instruction, we basically pop the new base pointer
    // from RBP and store the new value as stack pointer.
    RegIndex spRegIndex = instr->type == IT_LEAVE ? RI_BP : RI_SP;
    LLVMValueRef spReg = ll_get_register(getReg(RT_GP64, spRegIndex), FACET_PTR, state);
    LLVMValueRef sp = LLVMBuildPointerCast(state->builder, spReg, pi64, "");
    LLVMValueRef newSp = NULL;

    if (instr->type == IT_PUSH)
    {
        // Decrement Stack Pointer via a GEP instruction
        LLVMValueRef constSub = LLVMConstInt(i64, -1, false);
        newSp = LLVMBuildGEP(state->builder, sp, &constSub, 1, "");

        LLVMValueRef value = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->dst, state);
        value = LLVMBuildSExtOrBitCast(state->builder, value, i64, "");
        LLVMBuildStore(state->builder, value, newSp);
    }
    else if (instr->type == IT_POP || instr->type == IT_LEAVE)
    {
        Operand* operand = instr->type == IT_LEAVE
            ? getRegOp(getReg(RT_GP64, RI_BP))
            : &instr->dst;

        LLVMValueRef value = LLVMBuildLoad(state->builder, sp, "");
        ll_operand_store(OP_SI, ALIGN_MAXIMUM, operand, REG_DEFAULT, value, state);

        // Advance Stack pointer via a GEP
        LLVMValueRef constAdd = LLVMConstInt(i64, 1, false);
        newSp = LLVMBuildGEP(state->builder, sp, &constAdd, 1, "");
    }
    else
        warn_if_reached();

    // Cast back to int for register store
    LLVMValueRef newSpReg = LLVMBuildPointerCast(state->builder, newSp, pi8, "");
    LLVMSetMetadata(newSpReg, LLVMGetMDKindIDInContext(state->context, "asm.reg.rsp", 11), state->emptyMD);

    ll_set_register(getReg(RT_GP64, RI_SP), FACET_PTR, newSpReg, true, state);
}

/**
 * Handling of an instruction.
 *
 * \todo Support other return types than i64, float and double
 *
 * \private
 *
 * \author Alexis Engelke
 *
 * \param instr The push instruction
 * \param state The module state
 **/
void
ll_generate_instruction(Instr* instr, LLState* state)
{
    LLVMValueRef cond;
    LLVMValueRef operand1;
    LLVMValueRef operand2;
    LLVMValueRef result;

    LLVMTypeRef i8 = LLVMInt8TypeInContext(state->context);
    LLVMTypeRef i32 = LLVMInt32TypeInContext(state->context);
    LLVMTypeRef i64 = LLVMInt64TypeInContext(state->context);

    // Set new instruction pointer register
    uintptr_t rip = instr->addr + instr->len;
    LLVMValueRef ripValue = LLVMConstInt(LLVMInt64TypeInContext(state->context), rip, false);
    ll_set_register(getReg(RT_IP, 0), FACET_I64, ripValue, true, state);

    // Add Metadata for debugging.
    LLVMValueRef intrinsicDoNothing = ll_support_get_intrinsic(state->module, LL_INTRINSIC_DO_NOTHING, NULL, 0);
    char* instructionName = instr2string(instr, 0, NULL);
    LLVMValueRef mdCall = LLVMBuildCall(state->builder, intrinsicDoNothing, NULL, 0, "");
    LLVMValueRef mdNode = LLVMMDStringInContext(state->context, instructionName, strlen(instructionName));
    LLVMSetMetadata(mdCall, LLVMGetMDKindIDInContext(state->context, "asm.instr", 9), mdNode);

    // TODO: Flags!
    switch (instr->type)
    {
        case IT_NOP:
            break;

        ////////////////////////////////////////////////////////////////////////
        //// Move Instructions
        ////////////////////////////////////////////////////////////////////////

        case IT_MOV:
        case IT_MOVSX:
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src, state);
            ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, operand1, state);
            break;
        case IT_MOVD:
        case IT_MOVQ:
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src, state);
            if (opIsVReg(&instr->dst))
                ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_ZERO_UPPER, operand1, state);
            else
                ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, operand1, state);
            break;
        case IT_MOVZX:
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src, state);
            result = LLVMBuildZExtOrBitCast(state->builder, operand1, LLVMIntTypeInContext(state->context, opTypeWidth(&instr->dst)), "");
            ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, result, state);
            break;
        case IT_CMOVO:
        case IT_CMOVNO:
        case IT_CMOVC:
        case IT_CMOVNC:
        case IT_CMOVZ:
        case IT_CMOVNZ:
        case IT_CMOVBE:
        case IT_CMOVA:
        case IT_CMOVS:
        case IT_CMOVNS:
        case IT_CMOVP:
        case IT_CMOVNP:
        case IT_CMOVL:
        case IT_CMOVGE:
        case IT_CMOVLE:
        case IT_CMOVG:
            cond = ll_flags_condition(instr->type, IT_CMOVO, state);
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src, state);
            operand2 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->dst, state);
            result = LLVMBuildSelect(state->builder, cond, operand1, operand2, "");
            ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, result, state);
            break;
        case IT_SETO:
        case IT_SETNO:
        case IT_SETC:
        case IT_SETNC:
        case IT_SETZ:
        case IT_SETNZ:
        case IT_SETBE:
        case IT_SETA:
        case IT_SETS:
        case IT_SETNS:
        case IT_SETP:
        case IT_SETNP:
        case IT_SETL:
        case IT_SETGE:
        case IT_SETLE:
        case IT_SETG:
            cond = ll_flags_condition(instr->type, IT_SETO, state);
            result = LLVMBuildZExtOrBitCast(state->builder, cond, i8, "");
            ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, result, state);
            break;

        ////////////////////////////////////////////////////////////////////////
        //// Control Flow Instructions
        ////////////////////////////////////////////////////////////////////////

        case IT_CALL:
            {
                if (instr->dst.type != OT_Imm64)
                    warn_if_reached();

                uintptr_t address = instr->dst.val;

                // Find function with corresponding address.
                LLFunction* function = NULL;

                for (size_t i = 0; i < state->functionCount; i++)
                    if (state->functions[i]->address == address)
                        function = state->functions[i];

                if (function == NULL)
                    warn_if_reached();

                LLVMValueRef llvmFunction = function->llvmFunction;
                LLVMAddFunctionAttr(llvmFunction, LLVMInlineHintAttribute);

                // Construct arguments.
                LLVMTypeRef fnType = LLVMGetElementType(LLVMTypeOf(llvmFunction));
                size_t argCount = LLVMCountParamTypes(fnType);

                LLVMValueRef args[argCount];
                ll_operand_construct_args(fnType, args, state);

                result = LLVMBuildCall(state->builder, llvmFunction, args, argCount, "");

                if (LLVMGetTypeKind(LLVMTypeOf(result)) == LLVMPointerTypeKind)
                    result = LLVMBuildPtrToInt(state->builder, result, i64, "");
                if (LLVMTypeOf(result) != i64)
                    warn_if_reached();

                // TODO: Handle return values except for i64!
                ll_set_register(getReg(RT_GP64, RI_A), FACET_I64, result, true, state);

                // Clobber registers.
                ll_clear_register(getReg(RT_GP64, RI_C), state);
                ll_clear_register(getReg(RT_GP64, RI_D), state);
                ll_clear_register(getReg(RT_GP64, RI_SI), state);
                ll_clear_register(getReg(RT_GP64, RI_DI), state);
                ll_clear_register(getReg(RT_GP64, RI_8), state);
                ll_clear_register(getReg(RT_GP64, RI_9), state);
                ll_clear_register(getReg(RT_GP64, RI_10), state);
                ll_clear_register(getReg(RT_GP64, RI_11), state);
            }
            break;
        case IT_RET:
            {
                LLVMTypeRef fnType = LLVMGetElementType(LLVMTypeOf(state->currentFunction->llvmFunction));
                LLVMTypeRef retType = LLVMGetReturnType(fnType);
                LLVMTypeKind retTypeKind = LLVMGetTypeKind(retType);

                if (retTypeKind == LLVMPointerTypeKind)
                {
                    LLVMValueRef value = ll_operand_load(OP_SI, ALIGN_MAXIMUM, getRegOp(getReg(RT_GP64, RI_A)), state);
                    result = LLVMBuildIntToPtr(state->builder, value, retType, "");
                }
                else if (retTypeKind == LLVMIntegerTypeKind)
                    // TODO: Non 64-bit integers!
                    result = ll_operand_load(OP_SI, ALIGN_MAXIMUM, getRegOp(getReg(RT_GP64, RI_A)), state);
                else if (retTypeKind == LLVMFloatTypeKind)
                    result = ll_operand_load(OP_SF32, ALIGN_MAXIMUM, getRegOp(getReg(RT_XMM, RI_XMM0)), state);
                else if (retTypeKind == LLVMDoubleTypeKind)
                    result = ll_operand_load(OP_SF64, ALIGN_MAXIMUM, getRegOp(getReg(RT_XMM, RI_XMM0)), state);
                else if (retTypeKind == LLVMVoidTypeKind)
                    result = NULL;
                else
                {
                    result = NULL;
                    warn_if_reached();
                }

                if (result != NULL)
                    LLVMBuildRet(state->builder, result);
                else
                    LLVMBuildRetVoid(state->builder);
            }
            break;

        ////////////////////////////////////////////////////////////////////////
        //// Stack Instructions
        ////////////////////////////////////////////////////////////////////////

        case IT_LEAVE:
        case IT_PUSH:
        case IT_POP:
            ll_generate_instruction_stack(instr, state);
            break;

        ////////////////////////////////////////////////////////////////////////
        //// Integer Arithmetic Instructions
        ////////////////////////////////////////////////////////////////////////

        case IT_NOT:
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->dst, state);
            result = LLVMBuildNot(state->builder, operand1, "");
            ll_flags_invalidate(state);
            // ll_flags_set_not(result, operand1, state);
            ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, result, state);
            break;
        case IT_NEG:
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->dst, state);
            result = LLVMBuildNeg(state->builder, operand1, "");
            ll_flags_invalidate(state);
            // ll_flags_set_neg(result, operand1, state);
            ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, result, state);
            break;
        case IT_INC:
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = LLVMConstInt(LLVMTypeOf(operand1), 1, false);
            result = LLVMBuildAdd(state->builder, operand1, operand2, "");
            ll_flags_set_inc(result, operand1, state);
            ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, result, state);
            break;
        case IT_DEC:
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = LLVMConstInt(LLVMTypeOf(operand1), 1, false);
            result = LLVMBuildSub(state->builder, operand1, operand2, "");
            ll_flags_set_dec(result, operand1, state);
            ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, result, state);
            break;
        case IT_ADD:
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src, state);
            operand2 = LLVMBuildSExtOrBitCast(state->builder, operand2, LLVMTypeOf(operand1), "");

            result = LLVMBuildAdd(state->builder, operand1, operand2, "");

            if (LLVMGetIntTypeWidth(LLVMTypeOf(operand1)) == 64 && opIsReg(&instr->dst))
            {
                LLVMValueRef ptr = ll_get_register(instr->dst.reg, FACET_PTR, state);
                LLVMValueRef gep = LLVMBuildGEP(state->builder, ptr, &operand2, 1, "");

                ll_set_register(instr->dst.reg, FACET_I64, result, true, state);
                ll_set_register(instr->dst.reg, FACET_PTR, gep, false, state);
            }
            else
            {
                ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, result, state);
            }

            ll_flags_set_add(result, operand1, operand2, state);
            break;
        case IT_ADC:
            // TODO: Test this!!
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src, state);
            operand2 = LLVMBuildSExtOrBitCast(state->builder, operand2, LLVMTypeOf(operand1), "");

            operand1 = LLVMBuildAdd(state->builder, operand1, operand2, "");
            operand2 = LLVMBuildSExtOrBitCast(state->builder, ll_get_flag(RFLAG_CF, state), LLVMTypeOf(operand1), "");
            result = LLVMBuildAdd(state->builder, operand1, operand2, "");
            ll_flags_invalidate(state);
            // ll_flags_set_adc(result, operand1, operand2, state);
            ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, result, state);
            break;
        case IT_SUB:
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src, state);
            operand2 = LLVMBuildSExtOrBitCast(state->builder, operand2, LLVMTypeOf(operand1), "");

            result = LLVMBuildSub(state->builder, operand1, operand2, "");

            if (LLVMGetIntTypeWidth(LLVMTypeOf(operand1)) == 64 && opIsReg(&instr->dst))
            {
                LLVMValueRef sub = LLVMBuildNeg(state->builder, operand2, "");
                LLVMValueRef ptr = ll_get_register(instr->dst.reg, FACET_PTR, state);
                LLVMValueRef gep = LLVMBuildGEP(state->builder, ptr, &sub, 1, "");

                ll_set_register(instr->dst.reg, FACET_I64, result, true, state);
                ll_set_register(instr->dst.reg, FACET_PTR, gep, false, state);
            }
            else
            {
                ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, result, state);
            }

            ll_flags_set_sub(result, operand1, operand2, state);
            break;
        case IT_IMUL:
            // TODO: handle variant with one operand
            if (instr->form == OF_2)
            {
                operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->dst, state);
                operand2 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src, state);
                operand2 = LLVMBuildSExtOrBitCast(state->builder, operand2, LLVMTypeOf(operand1), "");
                result = LLVMBuildMul(state->builder, operand1, operand2, "");
            }
            else if (instr->form == OF_3)
            {
                operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src, state);
                operand2 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src2, state);
                operand2 = LLVMBuildSExtOrBitCast(state->builder, operand2, LLVMTypeOf(operand1), "");
                result = LLVMBuildMul(state->builder, operand1, operand2, "");
            }
            else
            {
                result = LLVMGetUndef(LLVMInt64TypeInContext(state->context));
                warn_if_reached();
            }
            ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, result, state);
            break;
        case IT_AND:
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src, state);
            operand2 = LLVMBuildSExtOrBitCast(state->builder, operand2, LLVMTypeOf(operand1), "");
            result = LLVMBuildAnd(state->builder, operand1, operand2, "");
            ll_flags_set_bit(result, state);
            ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, result, state);
            break;
        case IT_OR:
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src, state);
            operand2 = LLVMBuildSExtOrBitCast(state->builder, operand2, LLVMTypeOf(operand1), "");
            result = LLVMBuildOr(state->builder, operand1, operand2, "");
            ll_flags_set_bit(result, state);
            ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, result, state);
            break;
        case IT_XOR:
            if (opIsEqual(&instr->dst, &instr->src))
            {
                int width = opTypeWidth(&instr->dst);
                result = LLVMConstInt(LLVMIntTypeInContext(state->context, width), 0, false);
            }
            else
            {
                operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->dst, state);
                operand2 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src, state);
                operand2 = LLVMBuildSExtOrBitCast(state->builder, operand2, LLVMTypeOf(operand1), "");
                result = LLVMBuildXor(state->builder, operand1, operand2, "");
            }
            ll_flags_set_bit(result, state);
            ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, result, state);
            break;
        case IT_SHL:
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src, state);
            operand2 = LLVMBuildSExtOrBitCast(state->builder, operand2, LLVMTypeOf(operand1), "");
            result = LLVMBuildShl(state->builder, operand1, operand2, "");
            ll_flags_invalidate(state);
            // ll_flags_set_shift(result, operand1, operand2, state);
            ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, result, state);
            break;
        case IT_SHR:
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src, state);
            operand2 = LLVMBuildSExtOrBitCast(state->builder, operand2, LLVMTypeOf(operand1), "");
            result = LLVMBuildLShr(state->builder, operand1, operand2, "");
            ll_flags_invalidate(state);
            // ll_flags_set_shift(result, operand1, operand2, state);
            ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, result, state);
            break;
        case IT_SAR:
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src, state);
            operand2 = LLVMBuildSExtOrBitCast(state->builder, operand2, LLVMTypeOf(operand1), "");
            result = LLVMBuildAShr(state->builder, operand1, operand2, "");
            ll_flags_invalidate(state);
            // ll_flags_set_shift(result, operand1, operand2, state);
            ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, result, state);
            break;
        case IT_LEA:
            // assert(opIsInd(&(instr->src)));
            operand1 = ll_operand_get_address(OP_SI, &instr->src, state);
            result = LLVMBuildPtrToInt(state->builder, operand1, LLVMInt64TypeInContext(state->context), "");
            ll_operand_store(OP_SI, ALIGN_MAXIMUM, &instr->dst, REG_DEFAULT, result, state);
            break;
        case IT_TEST:
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src, state);
            operand2 = LLVMBuildSExtOrBitCast(state->builder, operand2, LLVMTypeOf(operand1), "");
            result = LLVMBuildAnd(state->builder, operand1, operand2, "");
            ll_flags_set_bit(result, state);
            break;
        case IT_CMP:
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, &instr->src, state);
            operand2 = LLVMBuildSExtOrBitCast(state->builder, operand2, LLVMTypeOf(operand1), "");
            result = LLVMBuildSub(state->builder, operand1, operand2, "");
            ll_flags_set_sub(result, operand1, operand2, state);
            break;
        case IT_CLTQ:
            operand1 = ll_operand_load(OP_SI, ALIGN_MAXIMUM, getRegOp(getReg(RT_GP32, RI_A)), state);
            ll_operand_store(OP_SI, ALIGN_MAXIMUM, getRegOp(getReg(RT_GP64, RI_A)), REG_DEFAULT, operand1, state);
            break;

        ////////////////////////////////////////////////////////////////////////
        //// SSE + AVX Instructions
        ////////////////////////////////////////////////////////////////////////

        case IT_MOVSS:
            operand1 = ll_operand_load(OP_SF32, ALIGN_MAXIMUM, &instr->src, state);
            if (opIsInd(&instr->src))
            {
                LLVMValueRef zero = LLVMConstNull(LLVMVectorType(LLVMFloatTypeInContext(state->context), 4));

                result = LLVMBuildInsertElement(state->builder, zero, operand1, LLVMConstInt(i64, 0, false), "");
                opOverwriteType(&instr->dst, VT_128);
                ll_operand_store(OP_VF32, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            }
            else
                ll_operand_store(OP_SF32, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, operand1, state);
            break;
        case IT_MOVSD:
            operand1 = ll_operand_load(OP_SF64, ALIGN_MAXIMUM, &instr->src, state);
            if (opIsInd(&instr->src))
            {
                LLVMValueRef zero = LLVMConstNull(LLVMVectorType(LLVMDoubleTypeInContext(state->context), 2));

                result = LLVMBuildInsertElement(state->builder, zero, operand1, LLVMConstInt(i64, 0, false), "");
                opOverwriteType(&instr->dst, VT_128);
                ll_operand_store(OP_VF64, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            }
            else
                ll_operand_store(OP_SF64, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, operand1, state);
            break;
        case IT_MOVUPS:
            operand1 = ll_operand_load(OP_VF32, ALIGN_8, &instr->src, state);
            ll_operand_store(OP_VF32, ALIGN_8, &instr->dst, REG_KEEP_UPPER, operand1, state);
            break;
        case IT_MOVUPD:
            operand1 = ll_operand_load(OP_VF64, ALIGN_8, &instr->src, state);
            ll_operand_store(OP_VF64, ALIGN_8, &instr->dst, REG_KEEP_UPPER, operand1, state);
            break;
        case IT_MOVAPS:
            operand1 = ll_operand_load(OP_VF32, ALIGN_MAXIMUM, &instr->src, state);
            ll_operand_store(OP_VF32, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, operand1, state);
            break;
        case IT_MOVAPD:
            operand1 = ll_operand_load(OP_VF64, ALIGN_MAXIMUM, &instr->src, state);
            ll_operand_store(OP_VF64, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, operand1, state);
            break;
        case IT_MOVLPS:
            operand1 = ll_operand_load(OP_VF32, ALIGN_MAXIMUM, &instr->src, state);
            ll_operand_store(OP_VF32, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, operand1, state);
            break;
        case IT_MOVLPD:
            operand1 = ll_operand_load(OP_SF64, ALIGN_MAXIMUM, &instr->src, state);
            ll_operand_store(OP_SF64, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, operand1, state);
            break;
        case IT_MOVHPS:
            if (opIsVReg(&instr->dst))
            {
                LLVMValueRef maskElements[4];
                maskElements[0] = LLVMConstInt(i32, 0, false);
                maskElements[1] = LLVMConstInt(i32, 1, false);
                maskElements[2] = LLVMConstInt(i32, 4, false);
                maskElements[3] = LLVMConstInt(i32, 5, false);
                LLVMValueRef mask = LLVMConstVector(maskElements, 4);

                operand1 = ll_operand_load(OP_VF32, ALIGN_MAXIMUM, &instr->dst, state);
                operand2 = ll_operand_load(OP_VF32, ALIGN_MAXIMUM, &instr->src, state);
                result = LLVMBuildShuffleVector(state->builder, operand1, operand2, mask, "");
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

                operand1 = ll_operand_load(OP_VF32, ALIGN_MAXIMUM, &instr->src, state);
                result = LLVMBuildShuffleVector(state->builder, operand1, LLVMGetUndef(LLVMTypeOf(operand1)), mask, "");
                ll_operand_store(OP_VF32, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            }
            break;
        case IT_MOVHPD:
            if (opIsVReg(&instr->dst))
            {
                operand1 = ll_operand_load(OP_VF64, ALIGN_MAXIMUM, &instr->dst, state);
                operand2 = ll_operand_load(OP_SF64, ALIGN_MAXIMUM, &instr->src, state);
                result = LLVMBuildInsertElement(state->builder, operand1, operand2, LLVMConstInt(i64, 1, false), "");
                ll_operand_store(OP_VF64, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            }
            else
            {
                operand1 = ll_operand_load(OP_VF64, ALIGN_MAXIMUM, &instr->src, state);
                result = LLVMBuildExtractElement(state->builder, operand1, LLVMConstInt(i64, 1, false), "");
                ll_operand_store(OP_SF64, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            }
            break;
        case IT_ADDSS:
            operand1 = ll_operand_load(OP_SF32, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_SF32, ALIGN_MAXIMUM, &instr->src, state);
            result = LLVMBuildFAdd(state->builder, operand1, operand2, "");
            if (state->enableFastMath)
                ll_support_enable_fast_math(result);
            ll_operand_store(OP_SF32, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            break;
        case IT_ADDSD:
            operand1 = ll_operand_load(OP_SF64, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_SF64, ALIGN_MAXIMUM, &instr->src, state);
            result = LLVMBuildFAdd(state->builder, operand1, operand2, "");
            if (state->enableFastMath)
                ll_support_enable_fast_math(result);
            ll_operand_store(OP_SF64, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            break;
        case IT_ADDPS:
            operand1 = ll_operand_load(OP_VF32, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_VF32, ALIGN_MAXIMUM, &instr->src, state);
            result = LLVMBuildFAdd(state->builder, operand1, operand2, "");
            if (state->enableFastMath)
                ll_support_enable_fast_math(result);
            ll_operand_store(OP_VF32, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            break;
        case IT_ADDPD:
            operand1 = ll_operand_load(OP_VF64, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_VF64, ALIGN_MAXIMUM, &instr->src, state);
            result = LLVMBuildFAdd(state->builder, operand1, operand2, "");
            if (state->enableFastMath)
                ll_support_enable_fast_math(result);
            ll_operand_store(OP_VF64, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            break;
        case IT_SUBSS:
            operand1 = ll_operand_load(OP_SF32, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_SF32, ALIGN_MAXIMUM, &instr->src, state);
            result = LLVMBuildFSub(state->builder, operand1, operand2, "");
            if (state->enableFastMath)
                ll_support_enable_fast_math(result);
            ll_operand_store(OP_SF32, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            break;
        case IT_SUBSD:
            operand1 = ll_operand_load(OP_SF64, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_SF64, ALIGN_MAXIMUM, &instr->src, state);
            result = LLVMBuildFSub(state->builder, operand1, operand2, "");
            if (state->enableFastMath)
                ll_support_enable_fast_math(result);
            ll_operand_store(OP_SF64, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            break;
        case IT_SUBPS:
            operand1 = ll_operand_load(OP_VF32, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_VF32, ALIGN_MAXIMUM, &instr->src, state);
            result = LLVMBuildFSub(state->builder, operand1, operand2, "");
            if (state->enableFastMath)
                ll_support_enable_fast_math(result);
            ll_operand_store(OP_VF32, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            break;
        case IT_SUBPD:
            operand1 = ll_operand_load(OP_VF64, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_VF64, ALIGN_MAXIMUM, &instr->src, state);
            result = LLVMBuildFSub(state->builder, operand1, operand2, "");
            if (state->enableFastMath)
                ll_support_enable_fast_math(result);
            ll_operand_store(OP_VF64, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            break;
        case IT_MULSS:
            operand1 = ll_operand_load(OP_SF32, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_SF32, ALIGN_MAXIMUM, &instr->src, state);
            result = LLVMBuildFMul(state->builder, operand1, operand2, "");
            if (state->enableFastMath)
                ll_support_enable_fast_math(result);
            ll_operand_store(OP_SF32, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            break;
        case IT_MULSD:
            operand1 = ll_operand_load(OP_SF64, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_SF64, ALIGN_MAXIMUM, &instr->src, state);
            result = LLVMBuildFMul(state->builder, operand1, operand2, "");
            if (state->enableFastMath)
                ll_support_enable_fast_math(result);
            ll_operand_store(OP_SF64, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            break;
        case IT_MULPS:
            operand1 = ll_operand_load(OP_VF32, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_VF32, ALIGN_MAXIMUM, &instr->src, state);
            result = LLVMBuildFMul(state->builder, operand1, operand2, "");
            if (state->enableFastMath)
                ll_support_enable_fast_math(result);
            ll_operand_store(OP_VF32, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            break;
        case IT_MULPD:
            operand1 = ll_operand_load(OP_VF64, ALIGN_MAXIMUM, &instr->dst, state);
            operand2 = ll_operand_load(OP_VF64, ALIGN_MAXIMUM, &instr->src, state);
            result = LLVMBuildFMul(state->builder, operand1, operand2, "");
            if (state->enableFastMath)
                ll_support_enable_fast_math(result);
            ll_operand_store(OP_VF64, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            break;
        case IT_XORPS:
            if (opIsEqual(&instr->dst, &instr->src))
                result = LLVMConstNull(LLVMVectorType(LLVMFloatTypeInContext(state->context), 4));
            else
            {
                operand1 = ll_operand_load(OP_VI32, ALIGN_MAXIMUM, &instr->dst, state);
                operand2 = ll_operand_load(OP_VI32, ALIGN_MAXIMUM, &instr->src, state);
                result = LLVMBuildXor(state->builder, operand1, operand2, "");
            }
            ll_operand_store(OP_VF32, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            break;
        case IT_XORPD:
            if (opIsEqual(&instr->dst, &instr->src))
                result = LLVMConstNull(LLVMVectorType(LLVMDoubleTypeInContext(state->context), 2));
            else
            {
                operand1 = ll_operand_load(OP_VI64, ALIGN_MAXIMUM, &instr->dst, state);
                operand2 = ll_operand_load(OP_VI64, ALIGN_MAXIMUM, &instr->src, state);
                result = LLVMBuildXor(state->builder, operand1, operand2, "");
            }
            ll_operand_store(OP_VF64, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            break;
        case IT_PXOR:
            if (opIsEqual(&instr->dst, &instr->src))
                result = LLVMConstInt(LLVMIntTypeInContext(state->context, opTypeWidth(&instr->dst)), 0, false);
            else
            {
                operand1 = ll_operand_load(OP_VI64, ALIGN_MAXIMUM, &instr->dst, state);
                operand2 = ll_operand_load(OP_VI64, ALIGN_MAXIMUM, &instr->src, state);
                result = LLVMBuildXor(state->builder, operand1, operand2, "");
            }
            ll_operand_store(OP_VI64, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            break;
        case IT_UNPCKLPS:
            {
                LLVMValueRef maskElements[4];
                maskElements[0] = LLVMConstInt(i32, 0, false);
                maskElements[1] = LLVMConstInt(i32, 4, false);
                maskElements[2] = LLVMConstInt(i32, 1, false);
                maskElements[3] = LLVMConstInt(i32, 5, false);
                LLVMValueRef mask = LLVMConstVector(maskElements, 4);

                operand1 = ll_operand_load(OP_VF32, ALIGN_MAXIMUM, &instr->dst, state);
                operand2 = ll_operand_load(OP_VF32, ALIGN_MAXIMUM, &instr->src, state);
                result = LLVMBuildShuffleVector(state->builder, operand1, operand2, mask, "");
                ll_operand_store(OP_VF32, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            }
            break;
        case IT_UNPCKLPD:
            {
                LLVMValueRef maskElements[2];
                maskElements[0] = LLVMConstInt(i32, 0, false);
                maskElements[1] = LLVMConstInt(i32, 2, false);
                LLVMValueRef mask = LLVMConstVector(maskElements, 2);

                operand1 = ll_operand_load(OP_VF64, ALIGN_MAXIMUM, &instr->dst, state);
                operand2 = ll_operand_load(OP_VF64, ALIGN_MAXIMUM, &instr->src, state);
                result = LLVMBuildShuffleVector(state->builder, operand1, operand2, mask, "");
                ll_operand_store(OP_VF64, ALIGN_MAXIMUM, &instr->dst, REG_KEEP_UPPER, result, state);
            }
            break;


        ////////////////////////////////////////////////////////////////////////
        //// Unhandled Instructions
        ////////////////////////////////////////////////////////////////////////

        // These are no instructions
        case IT_HINT_CALL:
        case IT_HINT_RET:
            break;

        // These are handled by the basic block generation code.
        case IT_JMP:
        case IT_JO:
        case IT_JNO:
        case IT_JC:
        case IT_JNC:
        case IT_JZ:
        case IT_JNZ:
        case IT_JBE:
        case IT_JA:
        case IT_JS:
        case IT_JNS:
        case IT_JP:
        case IT_JNP:
        case IT_JL:
        case IT_JGE:
        case IT_JLE:
        case IT_JG:
            break;

        case IT_Invalid:
            LLVMBuildUnreachable(state->builder);
            break;

        case IT_DIVSS:
        case IT_DIVSD:
        case IT_DIVPS:
        case IT_DIVPD:
        case IT_ORPS:
        case IT_ORPD:
        case IT_ANDPS:
        case IT_ANDPD:
        case IT_ANDNPS:
        case IT_ANDNPD:
        case IT_MAXSS:
        case IT_MAXSD:
        case IT_MAXPS:
        case IT_MAXPD:
        case IT_MINSS:
        case IT_MINSD:
        case IT_MINPS:
        case IT_MINPD:
        case IT_SQRTSS:
        case IT_SQRTSD:
        case IT_SQRTPS:
        case IT_SQRTPD:
        case IT_COMISS:
        case IT_COMISD:
        case IT_UCOMISS:
        case IT_ADDSUBPS:
        case IT_ADDSUBPD:
        case IT_HADDPS:
        case IT_HADDPD:
        case IT_HSUBPS:
        case IT_HSUBPD:
        case IT_RCPSS:
        case IT_RCPPS:
        case IT_RSQRTSS:
        case IT_RSQRTPS:
        case IT_PCMPEQW:
        case IT_PCMPEQD:
        case IT_CWTL:
        case IT_CQTO:
        case IT_SBB:
        case IT_IDIV1:
        case IT_JMPI:
        case IT_BSF:
        case IT_MUL:
        case IT_DIV:
        case IT_UCOMISD:
        case IT_MOVDQU:
        case IT_UNPCKHPS:
        case IT_UNPCKHPD:
        case IT_PMINUB:
        case IT_PADDQ:
        case IT_MOVDQA:
        case IT_PCMPEQB:
        case IT_PMOVMSKB:
        case IT_VMOVSS:
        case IT_VMOVSD:
        case IT_VMOVUPS:
        case IT_VMOVUPD:
        case IT_VMOVAPS:
        case IT_VMOVAPD:
        case IT_VMOVDQU:
        case IT_VMOVDQA:
        case IT_VMOVNTDQ:
        case IT_VADDSS:
        case IT_VADDSD:
        case IT_VADDPS:
        case IT_VADDPD:
        case IT_VMULSS:
        case IT_VMULSD:
        case IT_VMULPS:
        case IT_VMULPD:
        case IT_VXORPS:
        case IT_VXORPD:
        case IT_VZEROUPPER:
        case IT_VZEROALL:
        case IT_Max:
        case IT_None:
        default:
            printf("%s\n", instr2string(instr, 0, NULL));
            warn_if_reached();
            break;
    }
}

/**
 * @}
 **/
