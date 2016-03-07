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

#include "printer.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "instr.h"
#include "common.h"


const char* regName(Reg r, OpType t)
{
    switch(t) {
    case OT_Reg32:
        switch(r) {
        case Reg_AX: return "eax";
        case Reg_BX: return "ebx";
        case Reg_CX: return "ecx";
        case Reg_DX: return "edx";
        case Reg_DI: return "edi";
        case Reg_SI: return "esi";
        case Reg_BP: return "ebp";
        case Reg_SP: return "esp";
        case Reg_8:  return "r8";
        case Reg_9:  return "r9";
        case Reg_10: return "r10";
        case Reg_11: return "r11";
        case Reg_12: return "r12";
        case Reg_13: return "r13";
        case Reg_14: return "r14";
        case Reg_15: return "r15";
        case Reg_IP: return "eip";
        }
        break;

    case OT_Reg64:
        switch(r) {
        case Reg_AX: return "rax";
        case Reg_BX: return "rbx";
        case Reg_CX: return "rcx";
        case Reg_DX: return "rdx";
        case Reg_DI: return "rdi";
        case Reg_SI: return "rsi";
        case Reg_BP: return "rbp";
        case Reg_SP: return "rsp";
        case Reg_8:  return "r8";
        case Reg_9:  return "r9";
        case Reg_10: return "r10";
        case Reg_11: return "r11";
        case Reg_12: return "r12";
        case Reg_13: return "r13";
        case Reg_14: return "r14";
        case Reg_15: return "r15";
        case Reg_IP: return "rip";

        case Reg_X0:  return "mm0";
        case Reg_X1:  return "mm1";
        case Reg_X2:  return "mm2";
        case Reg_X3:  return "mm3";
        case Reg_X4:  return "mm4";
        case Reg_X5:  return "mm5";
        case Reg_X6:  return "mm6";
        case Reg_X7:  return "mm7";
        case Reg_X8:  return "mm8";
        case Reg_X9:  return "mm9";
        case Reg_X10: return "mm10";
        case Reg_X11: return "mm11";
        case Reg_X12: return "mm12";
        case Reg_X13: return "mm13";
        case Reg_X14: return "mm14";
        case Reg_X15: return "mm15";
        }
        break;

    case OT_Reg128:
        switch(r) {
        case Reg_X0:  return "xmm0";
        case Reg_X1:  return "xmm1";
        case Reg_X2:  return "xmm2";
        case Reg_X3:  return "xmm3";
        case Reg_X4:  return "xmm4";
        case Reg_X5:  return "xmm5";
        case Reg_X6:  return "xmm6";
        case Reg_X7:  return "xmm7";
        case Reg_X8:  return "xmm8";
        case Reg_X9:  return "xmm9";
        case Reg_X10: return "xmm10";
        case Reg_X11: return "xmm11";
        case Reg_X12: return "xmm12";
        case Reg_X13: return "xmm13";
        case Reg_X14: return "xmm14";
        case Reg_X15: return "xmm15";
        }
        break;

    case OT_Reg256:
        switch(r) {
        case Reg_X0:  return "ymm0";
        case Reg_X1:  return "ymm1";
        case Reg_X2:  return "ymm2";
        case Reg_X3:  return "ymm3";
        case Reg_X4:  return "ymm4";
        case Reg_X5:  return "ymm5";
        case Reg_X6:  return "ymm6";
        case Reg_X7:  return "ymm7";
        case Reg_X8:  return "ymm8";
        case Reg_X9:  return "ymm9";
        case Reg_X10: return "ymm10";
        case Reg_X11: return "ymm11";
        case Reg_X12: return "ymm12";
        case Reg_X13: return "ymm13";
        case Reg_X14: return "ymm14";
        case Reg_X15: return "ymm15";
        }
        break;

    }
    assert(0);
}

char* op2string(Operand* o, ValType t)
{
    static char buf[30];
    int off = 0;
    uint64_t val;

    switch(o->type) {
    case OT_Reg32:
    case OT_Reg64:
    case OT_Reg128:
    case OT_Reg256:
        sprintf(buf, "%%%s", regName(o->reg, o->type));
        break;

    case OT_Imm8:
        val = o->val;
        assert(val < (1l<<8));
        switch(t) {
        case VT_None:
        case VT_8:
            break;
        case VT_32:
            if (val > 0x7F) val += 0xFFFFFF00;
            break;
        case VT_64:
            if (val > 0x7F) val += 0xFFFFFFFFFFFFFF00;
            break;
        default: assert(0);
        }
        sprintf(buf, "$0x%lx", val);
        break;

    case OT_Imm16:
        val = o->val;
        assert(val < (1l<<16));
        switch(t) {
        case VT_32:
            if (val > 0x7FFF) val += 0xFFFF0000;
            break;
        case VT_64:
            if (val > 0x7FFF) val += 0xFFFFFFFFFFFF0000;
            break;
        case VT_None:
            break;
        default: assert(0);
        }
        sprintf(buf, "$0x%lx", val);
        break;

    case OT_Imm32:
        val = o->val;
        assert(val < (1l<<32));
        switch(t) {
        case VT_None:
        case VT_32:
            break;
        case VT_64:
            if (val > 0x7FFFFFFF) val += 0xFFFFFFFF00000000;
            break;
        default: assert(0);
        }
        sprintf(buf, "$0x%lx", val);
        break;

    case OT_Imm64:
        sprintf(buf, "$0x%lx", o->val);
        break;

    case OT_Ind8:
    case OT_Ind16:
    case OT_Ind32:
    case OT_Ind64:
    case OT_Ind128:
    case OT_Ind256:
        off = 0;
        switch(o->seg) {
        case OSO_None: break;
        case OSO_UseFS: off += sprintf(buf+off, "fs:"); break;
        case OSO_UseGS: off += sprintf(buf+off, "gs:"); break;
        }
        if (o->val != 0) {
            if (o->val & (1l<<63))
                off += sprintf(buf+off, "-0x%lx", (~ o->val)+1);
            else
                off += sprintf(buf+off, "0x%lx", o->val);
        }
        if ((o->scale == 0) || (o->ireg == Reg_None)) {
            if (o->reg != Reg_None)
                sprintf(buf+off,"(%%%s)", regName(o->reg, OT_Reg64));
        }
        else {
            const char* ri = regName(o->ireg, OT_Reg64);
            if (o->reg == Reg_None) {
                sprintf(buf+off,"(,%%%s,%d)", ri, o->scale);
            }
            else
                sprintf(buf+off,"(%%%s,%%%s,%d)",
                        regName(o->reg, OT_Reg64), ri, o->scale);
        }
        break;
    default: assert(0);
    }
    return buf;
}

const char* instrName(InstrType it, int* pOpCount)
{
    const char* n;
    int opCount = 0;

    switch(it) {
    case IT_HINT_CALL: n = "H-call"; break;
    case IT_HINT_RET:  n = "H-ret"; break;

    case IT_NOP:     n = "nop"; break;
    case IT_RET:     n = "ret"; break;
    case IT_LEAVE:   n = "leave"; break;
    case IT_CLTQ:    n = "cltq"; break;
    case IT_CQTO:    n = "cqto"; break;
    case IT_PUSH:    n = "push";    opCount = 1; break;
    case IT_POP:     n = "pop";     opCount = 1; break;
    case IT_CALL:    n = "call";    opCount = 1; break;
    case IT_JMP:     n = "jmp";     opCount = 1; break;
    case IT_JMPI:    n = "jmp*";    opCount = 1; break;
    case IT_JE:      n = "je";      opCount = 1; break;
    case IT_JNE:     n = "jne";     opCount = 1; break;
    case IT_JLE:     n = "jle";     opCount = 1; break;
    case IT_JG:      n = "jg";      opCount = 1; break;
    case IT_JL:      n = "jl";      opCount = 1; break;
    case IT_JGE:     n = "jge";     opCount = 1; break;
    case IT_JP:      n = "jp";      opCount = 1; break;
    case IT_MOV:     n = "mov";     opCount = 2; break;
    case IT_MOVSX:   n = "movsx";   opCount = 2; break;
    case IT_MOVZBL:  n = "movzbl";  opCount = 2; break;
    case IT_NEG:     n = "neg";     opCount = 1; break;
    case IT_INC:     n = "inc";     opCount = 1; break;
    case IT_DEC:     n = "dec";     opCount = 1; break;
    case IT_ADD:     n = "add";     opCount = 2; break;
    case IT_ADC:     n = "adc";     opCount = 2; break;
    case IT_SUB:     n = "sub";     opCount = 2; break;
    case IT_SBB:     n = "sbb";     opCount = 2; break;
    case IT_IMUL:    n = "imul";    opCount = 2; break;
    case IT_IDIV1:   n = "idiv";    opCount = 1; break;
    case IT_AND:     n = "and";     opCount = 2; break;
    case IT_OR:      n = "or";      opCount = 2; break;
    case IT_XOR:     n = "xor";     opCount = 2; break;
    case IT_SHL:     n = "shl";     opCount = 2; break;
    case IT_SHR:     n = "shr";     opCount = 2; break;
    case IT_SAR:     n = "sar";     opCount = 2; break;
    case IT_LEA:     n = "lea";     opCount = 2; break;
    case IT_CMP:     n = "cmp";     opCount = 2; break;
    case IT_TEST:    n = "test";    opCount = 2; break;
    case IT_BSF:     n = "bsf";     opCount = 2; break;
    case IT_PXOR:    n = "pxor";    opCount = 2; break;
    case IT_MOVSD:   n = "movsd";   opCount = 2; break;
    case IT_UCOMISD: n = "ucomisd"; opCount = 2; break;
    case IT_MULSD:   n = "mulsd";   opCount = 2; break;
    case IT_ADDSD:   n = "addsd";   opCount = 2; break;
    case IT_SUBSD:   n = "subsd";   opCount = 2; break;
    case IT_MOVDQU:  n = "movdqu";  opCount = 2; break;
    case IT_PCMPEQB: n = "pcmpeqb"; opCount = 2; break;
    case IT_PMINUB:  n = "pminub";  opCount = 2; break;
    case IT_PMOVMSKB:n = "pmovmskb";opCount = 2; break;
    case IT_CMOVO :  n = "cmovo";   opCount = 2; break;
    case IT_CMOVNO:  n = "cmovno";  opCount = 2; break;
    case IT_CMOVC :  n = "cmovc";   opCount = 2; break;
    case IT_CMOVNC:  n = "cmovnc";  opCount = 2; break;
    case IT_CMOVZ :  n = "cmovz";   opCount = 2; break;
    case IT_CMOVNZ:  n = "cmovnz";  opCount = 2; break;
    case IT_CMOVS :  n = "cmovs";   opCount = 2; break;
    case IT_CMOVNS:  n = "cmovns";  opCount = 2; break;

    default: n = "<Invalid>"; break;
    }

    if (pOpCount) *pOpCount = opCount;
    return n;
}

char* instr2string(Instr* instr, int align)
{
    static char buf[100];
    const char* n;
    int oc = 0, off = 0;

    n = instrName(instr->type, &oc);

    if (align)
        off += sprintf(buf, "%-7s", n);
    else
        off += sprintf(buf, "%s", n);

    // add value type if given
    Bool appendVType = (instr->vtype != VT_None);
    if ((instr->form == OF_2) &&
        (opIsGPReg(&(instr->dst)) ||
         opIsGPReg(&(instr->src)))) appendVType = False;
    if (appendVType) {
        char vt = ' ';
        switch(instr->vtype) {
        case VT_8:  vt = 'b'; break;
        case VT_16: vt = 'w'; break;
        case VT_32: vt = 'l'; break;
        case VT_64: vt = 'q'; break;
        }
        if (vt != ' ') {
            int nlen = strlen(n);
            if (buf[nlen] == ' ') buf[nlen] = vt;
            else {
                buf[nlen] = vt;
                buf[nlen+1] = 0;
                off++;
            }
        }
    }

    switch(instr->form) {
    case OF_0:
        assert(instr->dst.type == OT_None);
        assert(instr->src.type == OT_None);
        assert(instr->src2.type == OT_None);
        break;

    case OF_1:
        assert(instr->dst.type != OT_None);
        assert(instr->src.type == OT_None);
        assert(instr->src2.type == OT_None);
        off += sprintf(buf+off, " %s", op2string(&(instr->dst), instr->vtype));
        break;

    case OF_2:
        assert(instr->dst.type != OT_None);
        assert(instr->src.type != OT_None);
        assert(instr->src2.type == OT_None);
        off += sprintf(buf+off, " %s", op2string(&(instr->src), instr->vtype));
        off += sprintf(buf+off, ",%s", op2string(&(instr->dst), instr->vtype));
        break;

    case OF_3:
        assert(instr->dst.type != OT_None);
        assert(instr->src.type != OT_None);
        assert(instr->src2.type != OT_None);
        off += sprintf(buf+off, " %s", op2string(&(instr->src2), instr->vtype));
        off += sprintf(buf+off, ",%s", op2string(&(instr->src), instr->vtype));
        off += sprintf(buf+off, ",%s", op2string(&(instr->dst), instr->vtype));
        break;

    default: assert(0);
    }

    return buf;
}

char* bytes2string(Instr* instr, int start, int count)
{
    static char buf[100];
    int off = 0, i, j;
    for(i = start, j=0; (i < instr->len) && (j<count); i++, j++) {
        uint8_t b = ((uint8_t*) instr->addr)[i];
        off += sprintf(buf+off, " %02x", b);
    }
    for(;j<count;j++)
        off += sprintf(buf+off, "   ");
    return (off == 0) ? "" : buf;
}

void dbrew_print_decoded(DBB* bb)
{
    int i;
    for(i = 0; i < bb->count; i++) {
        Instr* instr = bb->instr + i;
        printf("  %p: %s  %s\n", (void*) instr->addr,
               bytes2string(instr, 0, 7), instr2string(instr, 1));
        if (instr->len > 7)
            printf("  %p: %s\n", (void*) instr->addr + 7,
                   bytes2string(instr, 7, 7));
        if (instr->len > 14)
            printf("  %p: %s\n", (void*) instr->addr + 14,
                   bytes2string(instr, 14, 7));
    }
}

void printDecodedBBs(Rewriter* c)
{
    int i;
    for(i=0; i< c->decBBCount; i++) {
        printf("BB %lx (%d instructions):\n", c->decBB[i].addr, c->decBB[i].count);
        dbrew_print_decoded(c->decBB + i);
    }
}