; NOTE: Assertions have been autogenerated by utils/update_test_checks.py UTC_ARGS: --function-signature --scrub-attributes
; RUN: opt -attributor --attributor-disable=false -attributor-annotate-decl-cs -S < %s | FileCheck %s
; TODO: Add max-iteration check

; Disable update test checks and enable it where required.
; UTC_ARGS: --turn off

; ModuleID = 'value-simplify.ll'
source_filename = "value-simplify.ll"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
declare void @f(i32)

; Test1: Replace argument with constant
define internal void @test1(i32 %a) {
; CHECK: tail call void @f(i32 1)
  tail call void @f(i32 %a)
  ret void
}

define void @test1_helper() {
  tail call void @test1(i32 1)
  ret void
}

; TEST 2 : Simplify return value
define i32 @return0() {
  ret i32 0
}

define i32 @return1() {
  ret i32 1
}

; CHECK: define i32 @test2_1(i1 %c)
define i32 @test2_1(i1 %c) {
  br i1 %c, label %if.true, label %if.false
if.true:
  %call = tail call i32 @return0()

; FIXME: %ret0 should be replaced with i32 1.
; CHECK: %ret0 = add i32 0, 1
  %ret0 = add i32 %call, 1
  br label %end
if.false:
  %ret1 = tail call i32 @return1()
  br label %end
end:

; FIXME: %ret should be replaced with i32 1.
; CHECK: %ret = phi i32 [ %ret0, %if.true ], [ 1, %if.false ]
  %ret = phi i32 [ %ret0, %if.true ], [ %ret1, %if.false ]

; FIXME: ret i32 1
; CHECK: ret i32 %ret
  ret i32 %ret
}



; CHECK: define i32 @test2_2(i1 %c)
define i32 @test2_2(i1 %c) {
; FIXME: %ret should be replaced with i32 1.
  %ret = tail call i32 @test2_1(i1 %c)
; FIXME: ret i32 1
; CHECK: ret i32 %ret
  ret i32 %ret
}

declare void @use(i32)
; CHECK: define void @test3(i1 %c)
define void @test3(i1 %c) {
  br i1 %c, label %if.true, label %if.false
if.true:
  br label %end
if.false:
  %ret1 = tail call i32 @return1()
  br label %end
end:

; CHECK: %r = phi i32 [ 1, %if.true ], [ 1, %if.false ]
  %r = phi i32 [ 1, %if.true ], [ %ret1, %if.false ]

; CHECK: tail call void @use(i32 1)
  tail call void @use(i32 %r)
  ret void
}

define void @test-select-phi(i1 %c) {
  %select-same = select i1 %c, i32 1, i32 1
  ; CHECK: tail call void @use(i32 1)
  tail call void @use(i32 %select-same)

  %select-not-same = select i1 %c, i32 1, i32 0
  ; CHECK: tail call void @use(i32 %select-not-same)
  tail call void @use(i32 %select-not-same)
  br i1 %c, label %if-true, label %if-false
if-true:
  br label %end
if-false:
  br label %end
end:
  %phi-same = phi i32 [ 1, %if-true ], [ 1, %if-false ]
  %phi-not-same = phi i32 [ 0, %if-true ], [ 1, %if-false ]
  %phi-same-prop = phi i32 [ 1, %if-true ], [ %select-same, %if-false ]
  %phi-same-undef = phi i32 [ 1, %if-true ], [ undef, %if-false ]
  %select-not-same-undef = select i1 %c, i32 %phi-not-same, i32 undef


  ; CHECK: tail call void @use(i32 1)
  tail call void @use(i32 %phi-same)

  ; CHECK: tail call void @use(i32 %phi-not-same)
  tail call void @use(i32 %phi-not-same)

  ; CHECK: tail call void @use(i32 1)
  tail call void @use(i32 %phi-same-prop)

  ; CHECK: tail call void @use(i32 1)
  tail call void @use(i32 %phi-same-undef)

  ; CHECK: tail call void @use(i32 %select-not-same-undef)
  tail call void @use(i32 %select-not-same-undef)

  ret void

}

define i32 @ipccp1(i32 %a) {
; CHECK-LABEL: define {{[^@]+}}@ipccp1
; CHECK-SAME: (i32 returned [[A:%.*]]) #0
; CHECK-NEXT:    br i1 true, label [[T:%.*]], label [[F:%.*]]
; CHECK:       t:
; CHECK-NEXT:    ret i32 [[A:%.*]]
; CHECK:       f:
; CHECK-NEXT:    unreachable
;
  br i1 true, label %t, label %f
t:
  ret i32 %a
f:
  %r = call i32 @ipccp1(i32 5)
  ret i32 %r
}

define internal i1 @ipccp2i(i1 %a) {
; CHECK-LABEL: define {{[^@]+}}@ipccp2i
; CHECK-SAME: (i1 returned [[A:%.*]]) #0
; CHECK-NEXT:    br label %t
; CHECK:       t:
; CHECK-NEXT:    ret i1 true
; CHECK:       f:
; CHECK-NEXT:    unreachable
;
  br i1 %a, label %t, label %f
t:
  ret i1 %a
f:
  %r = call i1 @ipccp2i(i1 false)
  ret i1 %r
}

define i1 @ipccp2() {
; CHECK-LABEL: define {{[^@]+}}@ipccp2() #1
; CHECK-NEXT:    [[R:%.*]] = call i1 @ipccp2i(i1 true) #0
; CHECK-NEXT:    ret i1 [[R]]
;
  %r = call i1 @ipccp2i(i1 true)
  ret i1 %r
}

define internal i32 @ipccp3i(i32 %a) {
; CHECK-LABEL: define {{[^@]+}}@ipccp3i
; CHECK-SAME: (i32 [[A:%.*]]) #1
; CHECK-NEXT:    [[C:%.*]] = icmp eq i32 [[A:%.*]], 7
; CHECK-NEXT:    br i1 [[C]], label [[T:%.*]], label [[F:%.*]]
; CHECK:       t:
; CHECK-NEXT:    ret i32 [[A]]
; CHECK:       f:
; CHECK-NEXT:    [[R:%.*]] = call i32 @ipccp3i(i32 5) #1
; CHECK-NEXT:    ret i32 [[R]]
;
  %c = icmp eq i32 %a, 7
  br i1 %c, label %t, label %f
t:
  ret i32 %a
f:
  %r = call i32 @ipccp3i(i32 5)
  ret i32 %r
}

define i32 @ipccp3() {
; CHECK-LABEL: define {{[^@]+}}@ipccp3() #1
; CHECK-NEXT:    [[R:%.*]] = call i32 @ipccp3i(i32 7) #1
; CHECK-NEXT:    ret i32 [[R]]
;
  %r = call i32 @ipccp3i(i32 7)
  ret i32 %r
}

; UTC_ARGS: --turn on

; Do not touch complicated arguments (for now)
%struct.X = type { i8* }
define internal i32* @test_inalloca(i32* inalloca %a) {
; CHECK-LABEL: define {{[^@]+}}@test_inalloca
; CHECK-SAME: (i32* inalloca noalias nofree returned writeonly [[A:%.*]])
; CHECK-NEXT:    ret i32* [[A]]
;
  ret i32* %a
}
define i32* @complicated_args_inalloca() {
; CHECK-LABEL: define {{[^@]+}}@complicated_args_inalloca()
; CHECK-NEXT:    [[CALL:%.*]] = call i32* @test_inalloca(i32* noalias nofree null)
; CHECK-NEXT:    ret i32* [[CALL]]
;
  %call = call i32* @test_inalloca(i32* null)
  ret i32* %call
}

define internal void @test_sret(%struct.X* sret %a, %struct.X** %b) {
; CHECK-LABEL: define {{[^@]+}}@test_sret
; CHECK-SAME: (%struct.X* nofree sret writeonly [[A:%.*]], %struct.X** nocapture nofree nonnull writeonly dereferenceable(8) [[B:%.*]])
; CHECK-NEXT:    store %struct.X* [[A]], %struct.X** [[B]]
; CHECK-NEXT:    ret void
;
  store %struct.X* %a, %struct.X** %b
  ret void
}
define void @complicated_args_sret(%struct.X** %b) {
; CHECK-LABEL: define {{[^@]+}}@complicated_args_sret
; CHECK-SAME: (%struct.X** nocapture nofree writeonly [[B:%.*]])
; CHECK-NEXT:    call void @test_sret(%struct.X* nofree null, %struct.X** nocapture nofree writeonly [[B]])
; CHECK-NEXT:    ret void
;
  call void @test_sret(%struct.X* null, %struct.X** %b)
  ret void
}

define internal %struct.X* @test_nest(%struct.X* nest %a) {
; CHECK-LABEL: define {{[^@]+}}@test_nest
; CHECK-SAME: (%struct.X* nest noalias nofree readnone returned [[A:%.*]])
; CHECK-NEXT:    ret %struct.X* [[A]]
;
  ret %struct.X* %a
}
define %struct.X* @complicated_args_nest() {
; CHECK-LABEL: define {{[^@]+}}@complicated_args_nest()
; CHECK-NEXT:    [[CALL:%.*]] = call %struct.X* @test_nest(%struct.X* noalias nofree null)
; CHECK-NEXT:    ret %struct.X* [[CALL]]
;
  %call = call %struct.X* @test_nest(%struct.X* null)
  ret %struct.X* %call
}

@S = external global %struct.X
define internal void @test_byval(%struct.X* byval %a) {
; CHECK-LABEL: define {{[^@]+}}@test_byval
; CHECK-SAME: (%struct.X* nocapture nofree nonnull writeonly byval align 8 dereferenceable(8) [[A:%.*]])
; CHECK-NEXT:    [[G0:%.*]] = getelementptr [[STRUCT_X:%.*]], %struct.X* [[A]], i32 0, i32 0
; CHECK-NEXT:    store i8* null, i8** [[G0]], align 8
; CHECK-NEXT:    ret void
;
  %g0 = getelementptr %struct.X, %struct.X* %a, i32 0, i32 0
  store i8* null, i8** %g0
  ret void
}
define void @complicated_args_byval() {
; CHECK-LABEL: define {{[^@]+}}@complicated_args_byval()
; CHECK-NEXT:    call void @test_byval(%struct.X* nofree nonnull align 8 dereferenceable(8) @S)
; CHECK-NEXT:    ret void
;
  call void @test_byval(%struct.X* @S)
  ret void
}

; UTC_ARGS: --turn off
