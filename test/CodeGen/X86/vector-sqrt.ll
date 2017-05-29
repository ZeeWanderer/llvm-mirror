; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+avx  | FileCheck %s

; Function Attrs: nounwind readonly uwtable
define <2 x double> @sqrtd2(double* nocapture readonly %v) local_unnamed_addr #0 {
; CHECK-LABEL: sqrtd2:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    vmovsd {{.*#+}} xmm0 = mem[0],zero
; CHECK-NEXT:    vmovsd {{.*#+}} xmm1 = mem[0],zero
; CHECK-NEXT:    vsqrtsd %xmm0, %xmm0, %xmm0
; CHECK-NEXT:    vsqrtsd %xmm1, %xmm1, %xmm1
; CHECK-NEXT:    vunpcklpd {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; CHECK-NEXT:    retq
entry:
  %0 = load double, double* %v, align 8
  %call = tail call double @sqrt(double %0) #2
  %arrayidx1 = getelementptr inbounds double, double* %v, i64 1
  %1 = load double, double* %arrayidx1, align 8
  %call2 = tail call double @sqrt(double %1) #2
  %vecinit.i = insertelement <2 x double> undef, double %call, i32 0
  %vecinit1.i = insertelement <2 x double> %vecinit.i, double %call2, i32 1
  ret <2 x double> %vecinit1.i
}

; Function Attrs: nounwind readnone
declare double @sqrt(double) local_unnamed_addr #1

; Function Attrs: nounwind readonly uwtable
define <4 x float> @sqrtf4(float* nocapture readonly %v) local_unnamed_addr #0 {
; CHECK-LABEL: sqrtf4:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    vmovss {{.*#+}} xmm0 = mem[0],zero,zero,zero
; CHECK-NEXT:    vmovss {{.*#+}} xmm1 = mem[0],zero,zero,zero
; CHECK-NEXT:    vsqrtss %xmm0, %xmm0, %xmm0
; CHECK-NEXT:    vsqrtss %xmm1, %xmm1, %xmm1
; CHECK-NEXT:    vmovss {{.*#+}} xmm2 = mem[0],zero,zero,zero
; CHECK-NEXT:    vsqrtss %xmm2, %xmm2, %xmm2
; CHECK-NEXT:    vmovss {{.*#+}} xmm3 = mem[0],zero,zero,zero
; CHECK-NEXT:    vsqrtss %xmm3, %xmm3, %xmm3
; CHECK-NEXT:    vinsertps {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[2,3]
; CHECK-NEXT:    vinsertps {{.*#+}} xmm0 = xmm0[0,1],xmm2[0],xmm0[3]
; CHECK-NEXT:    vinsertps {{.*#+}} xmm0 = xmm0[0,1,2],xmm3[0]
; CHECK-NEXT:    retq
entry:
  %0 = load float, float* %v, align 4
  %call = tail call float @sqrtf(float %0) #2
  %arrayidx1 = getelementptr inbounds float, float* %v, i64 1
  %1 = load float, float* %arrayidx1, align 4
  %call2 = tail call float @sqrtf(float %1) #2
  %arrayidx3 = getelementptr inbounds float, float* %v, i64 2
  %2 = load float, float* %arrayidx3, align 4
  %call4 = tail call float @sqrtf(float %2) #2
  %arrayidx5 = getelementptr inbounds float, float* %v, i64 3
  %3 = load float, float* %arrayidx5, align 4
  %call6 = tail call float @sqrtf(float %3) #2
  %vecinit.i = insertelement <4 x float> undef, float %call, i32 0
  %vecinit1.i = insertelement <4 x float> %vecinit.i, float %call2, i32 1
  %vecinit2.i = insertelement <4 x float> %vecinit1.i, float %call4, i32 2
  %vecinit3.i = insertelement <4 x float> %vecinit2.i, float %call6, i32 3
  ret <4 x float> %vecinit3.i
}

; Function Attrs: nounwind readnone
declare float @sqrtf(float) local_unnamed_addr #1

attributes #0 = { nounwind readonly uwtable "target-features"="+avx" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone  "target-features"="+avx2" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind readnone }
