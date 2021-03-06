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

#ifndef LL_FUNCTION_H
#define LL_FUNCTION_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include <llcommon.h>


struct LLFunction;

typedef struct LLFunction LLFunction;

LLFunction* ll_function_declare(uintptr_t, uint64_t, const char*, LLState* state);
LLFunction* ll_function_specialize(LLFunction*, uintptr_t, uintptr_t, size_t, LLState* state);
LLFunction* ll_function_wrap_external(const char*, LLState* state);
void ll_function_dispose(LLFunction*);
void ll_function_dump(LLFunction*);
void* ll_function_get_pointer(LLFunction*, LLState*);

#endif
