
// Compile and run with:
//   make -C ../.. LLVM_CONFIG=llvm-config-4.0
//   make LLVM_CONFIG=llvm-config-4.0
//   ./lifter

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <dbrew.h>
#include <dbrew-llvm.h>
#include "lifter.h"

int
main(void)
{
    Rewriter* r = dbrew_new();
    LLState* state = ll_engine_init();

    // Eventually configure state

    LLConfig config = {
        .name = "compoundAdd",
        .stackSize = 128,
        .signature = 0221, // float (i64, i8*)
    };

    // Decod function and lift to LLVM IR
    LLFunction* fn = ll_decode_function((uintptr_t) compoundAdd, (DecodeFunc) dbrew_decode, r, &config, state);

    // Run optimization passes
    ll_engine_optimize(state, 3);

    // Dump module to stderr
    ll_engine_dump(state);

    return 0;
}

/*

Sample output:

; ModuleID = '<llengine>'
source_filename = "<llengine>"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: norecurse nounwind readonly
define float @sample_func(i64, i8*) local_unnamed_addr #0 {
  %3 = ptrtoint i8* %1 to i64, !asm.reg.rsi !0
  %4 = icmp eq i64 %0, 0, !asm.flag.zf !0
  br i1 %4, label %9, label %5

; <label>:5:                                      ; preds = %2
  %6 = shl i64 %0, 2
  %7 = add i64 %3, %6, !asm.reg.rax !0
  br label %11

; <label>:8:                                      ; preds = %11
  br label %9

; <label>:9:                                      ; preds = %8, %2
  %10 = phi float [ 0.000000e+00, %2 ], [ %17, %8 ]
  ret float %10

; <label>:11:                                     ; preds = %11, %5
  %12 = phi i8* [ %1, %5 ], [ %19, %11 ], !asm.reg.rsi !0
  %13 = phi i64 [ %3, %5 ], [ %18, %11 ], !asm.reg.rsi !0
  %14 = phi float [ 0.000000e+00, %5 ], [ %17, %11 ], !asm.reg.xmm0 !0
  %15 = bitcast i8* %12 to float*
  %16 = load float, float* %15, align 4
  %17 = fadd float %14, %16
  %18 = add i64 %13, 4, !asm.reg.rsi !0
  %19 = getelementptr i8, i8* %12, i64 4, !asm.reg.rsi !0
  %20 = icmp eq i64 %7, %18
  br i1 %20, label %8, label %11
}

attributes #0 = { norecurse nounwind readonly }

!0 = !{}


*/

