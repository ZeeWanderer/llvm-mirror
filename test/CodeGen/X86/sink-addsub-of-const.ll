; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=i686-unknown-unknown -mattr=+slow-lea,+slow-3ops-lea,+sse,+sse2   | FileCheck %s --check-prefixes=ALL,X32
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+slow-lea,+slow-3ops-lea,+sse,+sse2 | FileCheck %s --check-prefixes=ALL,X64

; Scalar tests. Trying to avoid LEA here, so the output is actually readable..

; add (add %x, C), %y
; Outer 'add' is commutative - 2 variants.

define i32 @sink_add_of_const_to_add0(i32 %a, i32 %b, i32 %c) {
; X32-LABEL: sink_add_of_const_to_add0:
; X32:       # %bb.0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    addl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    addl %ecx, %eax
; X32-NEXT:    addl $32, %eax
; X32-NEXT:    retl
;
; X64-LABEL: sink_add_of_const_to_add0:
; X64:       # %bb.0:
; X64-NEXT:    # kill: def $edx killed $edx def $rdx
; X64-NEXT:    # kill: def $edi killed $edi def $rdi
; X64-NEXT:    addl %esi, %edi
; X64-NEXT:    leal 32(%rdx,%rdi), %eax
; X64-NEXT:    retq
  %t0 = add i32 %a, %b
  %t1 = add i32 %t0, 32 ; constant always on RHS
  %r = add i32 %t1, %c
  ret i32 %r
}
define i32 @sink_add_of_const_to_add1(i32 %a, i32 %b, i32 %c) {
; X32-LABEL: sink_add_of_const_to_add1:
; X32:       # %bb.0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    addl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    addl %ecx, %eax
; X32-NEXT:    addl $32, %eax
; X32-NEXT:    retl
;
; X64-LABEL: sink_add_of_const_to_add1:
; X64:       # %bb.0:
; X64-NEXT:    # kill: def $edx killed $edx def $rdx
; X64-NEXT:    # kill: def $edi killed $edi def $rdi
; X64-NEXT:    addl %esi, %edi
; X64-NEXT:    leal 32(%rdx,%rdi), %eax
; X64-NEXT:    retq
  %t0 = add i32 %a, %b
  %t1 = add i32 %t0, 32 ; constant always on RHS
  %r = add i32 %c, %t1
  ret i32 %r
}

; add (sub %x, C), %y
; Outer 'add' is commutative - 2 variants.

define i32 @sink_sub_of_const_to_add0(i32 %a, i32 %b, i32 %c) {
; X32-LABEL: sink_sub_of_const_to_add0:
; X32:       # %bb.0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    addl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    addl %ecx, %eax
; X32-NEXT:    addl $-32, %eax
; X32-NEXT:    retl
;
; X64-LABEL: sink_sub_of_const_to_add0:
; X64:       # %bb.0:
; X64-NEXT:    # kill: def $edx killed $edx def $rdx
; X64-NEXT:    # kill: def $edi killed $edi def $rdi
; X64-NEXT:    addl %esi, %edi
; X64-NEXT:    leal -32(%rdx,%rdi), %eax
; X64-NEXT:    retq
  %t0 = add i32 %a, %b
  %t1 = sub i32 %t0, 32
  %r = add i32 %t1, %c
  ret i32 %r
}
define i32 @sink_sub_of_const_to_add1(i32 %a, i32 %b, i32 %c) {
; X32-LABEL: sink_sub_of_const_to_add1:
; X32:       # %bb.0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    addl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    addl %ecx, %eax
; X32-NEXT:    addl $-32, %eax
; X32-NEXT:    retl
;
; X64-LABEL: sink_sub_of_const_to_add1:
; X64:       # %bb.0:
; X64-NEXT:    # kill: def $edx killed $edx def $rdx
; X64-NEXT:    # kill: def $edi killed $edi def $rdi
; X64-NEXT:    addl %esi, %edi
; X64-NEXT:    leal -32(%rdx,%rdi), %eax
; X64-NEXT:    retq
  %t0 = add i32 %a, %b
  %t1 = sub i32 %t0, 32
  %r = add i32 %c, %t1
  ret i32 %r
}

; add (sub C, %x), %y
; Outer 'add' is commutative - 2 variants.

define i32 @sink_sub_from_const_to_add0(i32 %a, i32 %b, i32 %c) {
; X32-LABEL: sink_sub_from_const_to_add0:
; X32:       # %bb.0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    addl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    movl $32, %eax
; X32-NEXT:    subl %ecx, %eax
; X32-NEXT:    addl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    retl
;
; X64-LABEL: sink_sub_from_const_to_add0:
; X64:       # %bb.0:
; X64-NEXT:    addl %esi, %edi
; X64-NEXT:    movl $32, %eax
; X64-NEXT:    subl %edi, %eax
; X64-NEXT:    addl %edx, %eax
; X64-NEXT:    retq
  %t0 = add i32 %a, %b
  %t1 = sub i32 32, %t0
  %r = add i32 %t1, %c
  ret i32 %r
}
define i32 @sink_sub_from_const_to_add1(i32 %a, i32 %b, i32 %c) {
; X32-LABEL: sink_sub_from_const_to_add1:
; X32:       # %bb.0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    addl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    movl $32, %eax
; X32-NEXT:    subl %ecx, %eax
; X32-NEXT:    addl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    retl
;
; X64-LABEL: sink_sub_from_const_to_add1:
; X64:       # %bb.0:
; X64-NEXT:    addl %esi, %edi
; X64-NEXT:    movl $32, %eax
; X64-NEXT:    subl %edi, %eax
; X64-NEXT:    addl %edx, %eax
; X64-NEXT:    retq
  %t0 = add i32 %a, %b
  %t1 = sub i32 32, %t0
  %r = add i32 %c, %t1
  ret i32 %r
}

; sub (add %x, C), %y
; sub %y, (add %x, C)

define i32 @sink_add_of_const_to_sub(i32 %a, i32 %b, i32 %c) {
; X32-LABEL: sink_add_of_const_to_sub:
; X32:       # %bb.0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    subl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    subl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    addl $32, %eax
; X32-NEXT:    retl
;
; X64-LABEL: sink_add_of_const_to_sub:
; X64:       # %bb.0:
; X64-NEXT:    # kill: def $edi killed $edi def $rdi
; X64-NEXT:    subl %esi, %edi
; X64-NEXT:    subl %edx, %edi
; X64-NEXT:    leal 32(%rdi), %eax
; X64-NEXT:    retq
  %t0 = sub i32 %a, %b
  %t1 = add i32 %t0, 32 ; constant always on RHS
  %r = sub i32 %t1, %c
  ret i32 %r
}
define i32 @sink_add_of_const_to_sub2(i32 %a, i32 %b, i32 %c) {
; X32-LABEL: sink_add_of_const_to_sub2:
; X32:       # %bb.0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    subl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    addl $32, %ecx
; X32-NEXT:    subl %ecx, %eax
; X32-NEXT:    retl
;
; X64-LABEL: sink_add_of_const_to_sub2:
; X64:       # %bb.0:
; X64-NEXT:    movl %edx, %eax
; X64-NEXT:    subl %esi, %edi
; X64-NEXT:    addl $32, %edi
; X64-NEXT:    subl %edi, %eax
; X64-NEXT:    retq
  %t0 = sub i32 %a, %b
  %t1 = add i32 %t0, 32 ; constant always on RHS
  %r = sub i32 %c, %t1
  ret i32 %r
}

; sub (sub %x, C), %y
; sub %y, (sub %x, C)

define i32 @sink_sub_of_const_to_sub(i32 %a, i32 %b, i32 %c) {
; X32-LABEL: sink_sub_of_const_to_sub:
; X32:       # %bb.0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    subl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    subl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    addl $-32, %eax
; X32-NEXT:    retl
;
; X64-LABEL: sink_sub_of_const_to_sub:
; X64:       # %bb.0:
; X64-NEXT:    # kill: def $edi killed $edi def $rdi
; X64-NEXT:    subl %esi, %edi
; X64-NEXT:    subl %edx, %edi
; X64-NEXT:    leal -32(%rdi), %eax
; X64-NEXT:    retq
  %t0 = sub i32 %a, %b
  %t1 = sub i32 %t0, 32
  %r = sub i32 %t1, %c
  ret i32 %r
}
define i32 @sink_sub_of_const_to_sub2(i32 %a, i32 %b, i32 %c) {
; X32-LABEL: sink_sub_of_const_to_sub2:
; X32:       # %bb.0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    subl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    addl %ecx, %eax
; X32-NEXT:    addl $32, %eax
; X32-NEXT:    retl
;
; X64-LABEL: sink_sub_of_const_to_sub2:
; X64:       # %bb.0:
; X64-NEXT:    # kill: def $edx killed $edx def $rdx
; X64-NEXT:    # kill: def $esi killed $esi def $rsi
; X64-NEXT:    subl %edi, %esi
; X64-NEXT:    leal 32(%rsi,%rdx), %eax
; X64-NEXT:    retq
  %t0 = sub i32 %a, %b
  %t1 = sub i32 %t0, 32
  %r = sub i32 %c, %t1
  ret i32 %r
}

; sub (sub C, %x), %y
; sub %y, (sub C, %x)

define i32 @sink_sub_from_const_to_sub(i32 %a, i32 %b, i32 %c) {
; X32-LABEL: sink_sub_from_const_to_sub:
; X32:       # %bb.0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    subl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    subl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    addl $32, %eax
; X32-NEXT:    retl
;
; X64-LABEL: sink_sub_from_const_to_sub:
; X64:       # %bb.0:
; X64-NEXT:    # kill: def $esi killed $esi def $rsi
; X64-NEXT:    subl %edi, %esi
; X64-NEXT:    subl %edx, %esi
; X64-NEXT:    leal 32(%rsi), %eax
; X64-NEXT:    retq
  %t0 = sub i32 %a, %b
  %t1 = sub i32 32, %t0
  %r = sub i32 %t1, %c
  ret i32 %r
}
define i32 @sink_sub_from_const_to_sub2(i32 %a, i32 %b, i32 %c) {
; X32-LABEL: sink_sub_from_const_to_sub2:
; X32:       # %bb.0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    subl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    addl %ecx, %eax
; X32-NEXT:    addl $-32, %eax
; X32-NEXT:    retl
;
; X64-LABEL: sink_sub_from_const_to_sub2:
; X64:       # %bb.0:
; X64-NEXT:    # kill: def $edx killed $edx def $rdx
; X64-NEXT:    # kill: def $edi killed $edi def $rdi
; X64-NEXT:    subl %esi, %edi
; X64-NEXT:    leal -32(%rdi,%rdx), %eax
; X64-NEXT:    retq
  %t0 = sub i32 %a, %b
  %t1 = sub i32 32, %t0
  %r = sub i32 %c, %t1
  ret i32 %r
}

;------------------------------------------------------------------------------;
; Basic vector tests. Here it is easier to see where the constant operand is.
;------------------------------------------------------------------------------;

; add (add %x, C), %y
; Outer 'add' is commutative - 2 variants.

define <4 x i32> @vec_sink_add_of_const_to_add0(<4 x i32> %a, <4 x i32> %b, <4 x i32> %c) {
; X32-LABEL: vec_sink_add_of_const_to_add0:
; X32:       # %bb.0:
; X32-NEXT:    paddd %xmm2, %xmm1
; X32-NEXT:    paddd %xmm1, %xmm0
; X32-NEXT:    paddd {{\.LCPI.*}}, %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: vec_sink_add_of_const_to_add0:
; X64:       # %bb.0:
; X64-NEXT:    paddd %xmm2, %xmm1
; X64-NEXT:    paddd %xmm1, %xmm0
; X64-NEXT:    paddd {{.*}}(%rip), %xmm0
; X64-NEXT:    retq
  %t0 = add <4 x i32> %a, %b
  %t1 = add <4 x i32> %t0, <i32 42, i32 24, i32 undef, i32 46> ; constant always on RHS
  %r = add <4 x i32> %t1, %c
  ret <4 x i32> %r
}
define <4 x i32> @vec_sink_add_of_const_to_add1(<4 x i32> %a, <4 x i32> %b, <4 x i32> %c) {
; X32-LABEL: vec_sink_add_of_const_to_add1:
; X32:       # %bb.0:
; X32-NEXT:    paddd %xmm2, %xmm1
; X32-NEXT:    paddd %xmm1, %xmm0
; X32-NEXT:    paddd {{\.LCPI.*}}, %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: vec_sink_add_of_const_to_add1:
; X64:       # %bb.0:
; X64-NEXT:    paddd %xmm2, %xmm1
; X64-NEXT:    paddd %xmm1, %xmm0
; X64-NEXT:    paddd {{.*}}(%rip), %xmm0
; X64-NEXT:    retq
  %t0 = add <4 x i32> %a, %b
  %t1 = add <4 x i32> %t0, <i32 42, i32 24, i32 undef, i32 46> ; constant always on RHS
  %r = add <4 x i32> %c, %t1
  ret <4 x i32> %r
}

; add (sub %x, C), %y
; Outer 'add' is commutative - 2 variants.

define <4 x i32> @vec_sink_sub_of_const_to_add0(<4 x i32> %a, <4 x i32> %b, <4 x i32> %c) {
; X32-LABEL: vec_sink_sub_of_const_to_add0:
; X32:       # %bb.0:
; X32-NEXT:    paddd %xmm1, %xmm0
; X32-NEXT:    psubd {{\.LCPI.*}}, %xmm0
; X32-NEXT:    paddd %xmm2, %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: vec_sink_sub_of_const_to_add0:
; X64:       # %bb.0:
; X64-NEXT:    paddd %xmm1, %xmm0
; X64-NEXT:    psubd {{.*}}(%rip), %xmm0
; X64-NEXT:    paddd %xmm2, %xmm0
; X64-NEXT:    retq
  %t0 = add <4 x i32> %a, %b
  %t1 = sub <4 x i32> %t0, <i32 42, i32 24, i32 undef, i32 46>
  %r = add <4 x i32> %t1, %c
  ret <4 x i32> %r
}
define <4 x i32> @vec_sink_sub_of_const_to_add1(<4 x i32> %a, <4 x i32> %b, <4 x i32> %c) {
; X32-LABEL: vec_sink_sub_of_const_to_add1:
; X32:       # %bb.0:
; X32-NEXT:    paddd %xmm1, %xmm0
; X32-NEXT:    psubd {{\.LCPI.*}}, %xmm0
; X32-NEXT:    paddd %xmm2, %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: vec_sink_sub_of_const_to_add1:
; X64:       # %bb.0:
; X64-NEXT:    paddd %xmm1, %xmm0
; X64-NEXT:    psubd {{.*}}(%rip), %xmm0
; X64-NEXT:    paddd %xmm2, %xmm0
; X64-NEXT:    retq
  %t0 = add <4 x i32> %a, %b
  %t1 = sub <4 x i32> %t0, <i32 42, i32 24, i32 undef, i32 46>
  %r = add <4 x i32> %c, %t1
  ret <4 x i32> %r
}

; add (sub C, %x), %y
; Outer 'add' is commutative - 2 variants.

define <4 x i32> @vec_sink_sub_from_const_to_add0(<4 x i32> %a, <4 x i32> %b, <4 x i32> %c) {
; ALL-LABEL: vec_sink_sub_from_const_to_add0:
; ALL:       # %bb.0:
; ALL-NEXT:    paddd %xmm1, %xmm0
; ALL-NEXT:    movdqa {{.*#+}} xmm1 = <42,24,u,46>
; ALL-NEXT:    psubd %xmm0, %xmm1
; ALL-NEXT:    paddd %xmm2, %xmm1
; ALL-NEXT:    movdqa %xmm1, %xmm0
; ALL-NEXT:    ret{{[l|q]}}
  %t0 = add <4 x i32> %a, %b
  %t1 = sub <4 x i32> <i32 42, i32 24, i32 undef, i32 46>, %t0
  %r = add <4 x i32> %t1, %c
  ret <4 x i32> %r
}
define <4 x i32> @vec_sink_sub_from_const_to_add1(<4 x i32> %a, <4 x i32> %b, <4 x i32> %c) {
; ALL-LABEL: vec_sink_sub_from_const_to_add1:
; ALL:       # %bb.0:
; ALL-NEXT:    paddd %xmm1, %xmm0
; ALL-NEXT:    movdqa {{.*#+}} xmm1 = <42,24,u,46>
; ALL-NEXT:    psubd %xmm0, %xmm1
; ALL-NEXT:    paddd %xmm2, %xmm1
; ALL-NEXT:    movdqa %xmm1, %xmm0
; ALL-NEXT:    ret{{[l|q]}}
  %t0 = add <4 x i32> %a, %b
  %t1 = sub <4 x i32> <i32 42, i32 24, i32 undef, i32 46>, %t0
  %r = add <4 x i32> %c, %t1
  ret <4 x i32> %r
}

; sub (add %x, C), %y
; sub %y, (add %x, C)

define <4 x i32> @vec_sink_add_of_const_to_sub(<4 x i32> %a, <4 x i32> %b, <4 x i32> %c) {
; X32-LABEL: vec_sink_add_of_const_to_sub:
; X32:       # %bb.0:
; X32-NEXT:    psubd %xmm1, %xmm0
; X32-NEXT:    psubd %xmm2, %xmm0
; X32-NEXT:    paddd {{\.LCPI.*}}, %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: vec_sink_add_of_const_to_sub:
; X64:       # %bb.0:
; X64-NEXT:    psubd %xmm1, %xmm0
; X64-NEXT:    psubd %xmm2, %xmm0
; X64-NEXT:    paddd {{.*}}(%rip), %xmm0
; X64-NEXT:    retq
  %t0 = sub <4 x i32> %a, %b
  %t1 = add <4 x i32> %t0, <i32 42, i32 24, i32 undef, i32 46> ; constant always on RHS
  %r = sub <4 x i32> %t1, %c
  ret <4 x i32> %r
}
define <4 x i32> @vec_sink_add_of_const_to_sub2(<4 x i32> %a, <4 x i32> %b, <4 x i32> %c) {
; X32-LABEL: vec_sink_add_of_const_to_sub2:
; X32:       # %bb.0:
; X32-NEXT:    psubd %xmm1, %xmm0
; X32-NEXT:    paddd {{\.LCPI.*}}, %xmm0
; X32-NEXT:    psubd %xmm0, %xmm2
; X32-NEXT:    movdqa %xmm2, %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: vec_sink_add_of_const_to_sub2:
; X64:       # %bb.0:
; X64-NEXT:    psubd %xmm1, %xmm0
; X64-NEXT:    paddd {{.*}}(%rip), %xmm0
; X64-NEXT:    psubd %xmm0, %xmm2
; X64-NEXT:    movdqa %xmm2, %xmm0
; X64-NEXT:    retq
  %t0 = sub <4 x i32> %a, %b
  %t1 = add <4 x i32> %t0, <i32 42, i32 24, i32 undef, i32 46> ; constant always on RHS
  %r = sub <4 x i32> %c, %t1
  ret <4 x i32> %r
}

; sub (sub %x, C), %y
; sub %y, (sub %x, C)

define <4 x i32> @vec_sink_sub_of_const_to_sub(<4 x i32> %a, <4 x i32> %b, <4 x i32> %c) {
; X32-LABEL: vec_sink_sub_of_const_to_sub:
; X32:       # %bb.0:
; X32-NEXT:    psubd %xmm1, %xmm0
; X32-NEXT:    psubd {{\.LCPI.*}}, %xmm0
; X32-NEXT:    psubd %xmm2, %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: vec_sink_sub_of_const_to_sub:
; X64:       # %bb.0:
; X64-NEXT:    psubd %xmm1, %xmm0
; X64-NEXT:    psubd {{.*}}(%rip), %xmm0
; X64-NEXT:    psubd %xmm2, %xmm0
; X64-NEXT:    retq
  %t0 = sub <4 x i32> %a, %b
  %t1 = sub <4 x i32> %t0, <i32 42, i32 24, i32 undef, i32 46>
  %r = sub <4 x i32> %t1, %c
  ret <4 x i32> %r
}
define <4 x i32> @vec_sink_sub_of_const_to_sub2(<4 x i32> %a, <4 x i32> %b, <4 x i32> %c) {
; X32-LABEL: vec_sink_sub_of_const_to_sub2:
; X32:       # %bb.0:
; X32-NEXT:    psubd %xmm0, %xmm1
; X32-NEXT:    paddd %xmm2, %xmm1
; X32-NEXT:    paddd {{\.LCPI.*}}, %xmm1
; X32-NEXT:    movdqa %xmm1, %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: vec_sink_sub_of_const_to_sub2:
; X64:       # %bb.0:
; X64-NEXT:    psubd %xmm0, %xmm1
; X64-NEXT:    paddd %xmm2, %xmm1
; X64-NEXT:    paddd {{.*}}(%rip), %xmm1
; X64-NEXT:    movdqa %xmm1, %xmm0
; X64-NEXT:    retq
  %t0 = sub <4 x i32> %a, %b
  %t1 = sub <4 x i32> %t0, <i32 42, i32 24, i32 undef, i32 46>
  %r = sub <4 x i32> %c, %t1
  ret <4 x i32> %r
}

; sub (sub C, %x), %y
; sub %y, (sub C, %x)

define <4 x i32> @vec_sink_sub_from_const_to_sub(<4 x i32> %a, <4 x i32> %b, <4 x i32> %c) {
; X32-LABEL: vec_sink_sub_from_const_to_sub:
; X32:       # %bb.0:
; X32-NEXT:    psubd %xmm0, %xmm1
; X32-NEXT:    psubd %xmm2, %xmm1
; X32-NEXT:    paddd {{\.LCPI.*}}, %xmm1
; X32-NEXT:    movdqa %xmm1, %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: vec_sink_sub_from_const_to_sub:
; X64:       # %bb.0:
; X64-NEXT:    psubd %xmm0, %xmm1
; X64-NEXT:    psubd %xmm2, %xmm1
; X64-NEXT:    paddd {{.*}}(%rip), %xmm1
; X64-NEXT:    movdqa %xmm1, %xmm0
; X64-NEXT:    retq
  %t0 = sub <4 x i32> %a, %b
  %t1 = sub <4 x i32> <i32 42, i32 24, i32 undef, i32 46>, %t0
  %r = sub <4 x i32> %t1, %c
  ret <4 x i32> %r
}
define <4 x i32> @vec_sink_sub_from_const_to_sub2(<4 x i32> %a, <4 x i32> %b, <4 x i32> %c) {
; X32-LABEL: vec_sink_sub_from_const_to_sub2:
; X32:       # %bb.0:
; X32-NEXT:    psubd %xmm1, %xmm0
; X32-NEXT:    psubd {{\.LCPI.*}}, %xmm0
; X32-NEXT:    paddd %xmm2, %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: vec_sink_sub_from_const_to_sub2:
; X64:       # %bb.0:
; X64-NEXT:    psubd %xmm1, %xmm0
; X64-NEXT:    psubd {{.*}}(%rip), %xmm0
; X64-NEXT:    paddd %xmm2, %xmm0
; X64-NEXT:    retq
  %t0 = sub <4 x i32> %a, %b
  %t1 = sub <4 x i32> <i32 42, i32 24, i32 undef, i32 46>, %t0
  %r = sub <4 x i32> %c, %t1
  ret <4 x i32> %r
}
