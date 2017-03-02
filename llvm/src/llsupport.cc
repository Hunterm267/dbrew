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

#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm-c/Transforms/PassManagerBuilder.h>

#include <llsupport-internal.h>

/**
 * \defgroup LLSupport Support
 * \brief Support functions for the LLVM API
 *
 * @{
 **/

/**
 * Get the declaration of an LLVM intrinsic with the given types.
 *
 * \private
 *
 * \author Alexis Engelke
 *
 * \param module The LLVM module
 * \param intrinsic The intrinsic
 * \param types The types of the instantiation, or NULL
 * \param typeCount The number of types
 * \returns A declaration of the requested intrinsic
 **/
extern "C"
LLVMValueRef
ll_support_get_intrinsic(LLVMModuleRef module, LLSupportIntrinsics intrinsic, LLVMTypeRef* types, unsigned typeCount)
{
    llvm::ArrayRef<llvm::Type*> Tys(llvm::unwrap(types), typeCount);
    llvm::Intrinsic::ID intrinsicId;

    switch (intrinsic)
    {
        case LL_INTRINSIC_DO_NOTHING: intrinsicId = llvm::Intrinsic::donothing; break;
        case LL_INTRINSIC_CTPOP: intrinsicId = llvm::Intrinsic::ctpop; break;
        case LL_INTRINSIC_SADD_WITH_OVERFLOW: intrinsicId = llvm::Intrinsic::sadd_with_overflow; break;
        case LL_INTRINSIC_SSUB_WITH_OVERFLOW: intrinsicId = llvm::Intrinsic::ssub_with_overflow; break;
        default: intrinsicId = llvm::Intrinsic::not_intrinsic; break;
    }

    return llvm::wrap(llvm::Intrinsic::getDeclaration(llvm::unwrap(module), intrinsicId, Tys));
}

/**
 * Enable vectorization on a pass manager builder.
 *
 * \private
 *
 * \author Alexis Engelke
 *
 * \param PMB The pass manager builder
 * \param value Whether to enable vectorization
 **/
extern "C"
void
ll_support_pass_manager_builder_set_enable_vectorize(LLVMPassManagerBuilderRef PMB, LLVMBool value)
{
    llvm::PassManagerBuilder* Builder = reinterpret_cast<llvm::PassManagerBuilder*>(PMB);
    Builder->BBVectorize = value;
    Builder->SLPVectorize = value;
    Builder->LoopVectorize = value;
}

/**
 * Enable unsafe algebra on the result of a floating-point instruction.
 *
 * \private
 *
 * \author Alexis Engelke
 *
 * \param value The result of a supported floating-point instruction
 **/
extern "C"
void
ll_support_enable_fast_math(LLVMValueRef value)
{
    llvm::unwrap<llvm::Instruction>(value)->setHasUnsafeAlgebra(true);
}

/**
 * Whether a value is a constant integer
 *
 * \private
 *
 * \author Alexis Engelke
 *
 * \param value The value to check
 * \returns Whether the value is a constant integer
 **/
extern "C"
LLVMBool
ll_support_is_constant_int(LLVMValueRef value)
{
    return llvm::isa<llvm::ConstantInt>(llvm::unwrap(value));
}

/**
 * Construct a metadata node to force full loop unrolling.
 *
 * \private
 *
 * \author Alexis Engelke
 *
 * \param context The LLVM context
 * \returns A metadata node
 **/
extern "C"
LLVMValueRef
ll_support_metadata_loop_unroll(LLVMContextRef context)
{
    llvm::SmallVector<llvm::Metadata *, 1> unrollElts;
    llvm::SmallVector<llvm::Metadata *, 2> loopElts;

    llvm::LLVMContext* C = llvm::unwrap(context);
    llvm::MDString* unrollString = llvm::MDString::get(*C, "llvm.loop.unroll.full");
    unrollElts.push_back(unrollString);

    llvm::MDNode* unrollNode = llvm::MDTuple::get(*C, unrollElts);

    llvm::TempMDNode temp = llvm::MDNode::getTemporary(*C, unrollElts);
    loopElts.push_back(temp.get());
    loopElts.push_back(unrollNode);
    llvm::MDNode* loopNode = llvm::MDTuple::get(*C, loopElts);

    temp->replaceAllUsesWith(loopNode);

    return llvm::wrap(llvm::MetadataAsValue::get(*C, loopNode));
}

/**
 * @}
 **/
