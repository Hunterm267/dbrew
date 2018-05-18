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

#ifndef LL_INSTRUCTION_H
#define LL_INSTRUCTION_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include <instr.h>

#include <llcommon.h>
#include <llcommon-internal.h>
#include <llinstr-internal.h>

void ll_instruction_movgp(LLInstr*, LLState*);
void ll_instruction_add(LLInstr*, LLState*);
void ll_instruction_sub(LLInstr*, LLState*);
void ll_instruction_cmp(LLInstr*, LLState*);
void ll_instruction_test(LLInstr*, LLState*);
void ll_instruction_notneg(LLInstr*, LLState*);
void ll_instruction_incdec(LLInstr*, LLState*);
void ll_instruction_mul(LLInstr*, LLState*);
void ll_instruction_rotate(LLInstr*, LLState*);
void ll_instruction_lea(LLInstr*, LLState*);
void ll_instruction_cmov(LLInstr*, LLState*);
void ll_instruction_setcc(LLInstr*, LLState*);
void ll_instruction_cdqe(LLInstr*, LLState*);

void ll_instruction_call(LLInstr*, LLState*);
void ll_instruction_ret(LLInstr*, LLState*);

void ll_instruction_stack(LLInstr*, LLState*);

void ll_instruction_movq(LLInstr* instr, LLState* state);
void ll_instruction_movs(LLInstr* instr, LLState* state);
void ll_instruction_movp(LLInstr* instr, LLState* state);
void ll_instruction_movdq(LLInstr* instr, LLState* state);
void ll_instruction_movlp(LLInstr* instr, LLState* state);
void ll_instruction_movhps(LLInstr* instr, LLState* state);
void ll_instruction_movhpd(LLInstr* instr, LLState* state);
void ll_instruction_unpckl(LLInstr* instr, LLState* state);

void ll_generate_instruction(LLInstr*, LLState*);

#endif
