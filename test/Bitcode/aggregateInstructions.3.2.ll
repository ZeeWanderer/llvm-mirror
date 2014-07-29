; RUN: llvm-dis < %s.bc| FileCheck %s
; RUN: llvm-uselistorder < %s.bc -preserve-bc-use-list-order -num-shuffles=5

; aggregateOperations.3.2.ll.bc was generated by passing this file to llvm-as-3.2.
; The test checks that LLVM does not misread instructions with aggregate operands
; in older bitcode files.

define void @extractvalue([4 x i8] %x1, [4 x [4 x i8]] %x2, {{i32, float}} %x3){
entry:
; CHECK: %res1 = extractvalue [4 x i8] %x1, 0
  %res1 = extractvalue [4 x i8] %x1, 0

; CHECK-NEXT: %res2 = extractvalue [4 x [4 x i8]] %x2, 1
  %res2 = extractvalue [4 x [4 x i8 ]] %x2, 1

; CHECK-NEXT: %res3 = extractvalue [4 x [4 x i8]] %x2, 0, 1
  %res3 = extractvalue [4 x [4 x i8 ]] %x2, 0, 1

; CHECK-NEXT: %res4 = extractvalue { { i32, float } } %x3, 0, 1
  %res4 = extractvalue {{i32, float}} %x3, 0, 1

  ret void
}

define void @insertvalue([4 x [4 x i8 ]] %x1){
entry:
; CHECK: %res1 = insertvalue [4 x [4 x i8]] %x1, i8 0, 0, 0
  %res1 = insertvalue [4 x [4 x i8 ]] %x1, i8 0, 0, 0

; CHECK-NEXT: %res2 = insertvalue [4 x [4 x i8]] undef, i8 0, 0, 0
  %res2 = insertvalue [4 x [4 x i8 ]] undef, i8 0, 0, 0

  ret void
}
