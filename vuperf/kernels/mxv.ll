; M * v of each item, M a uniform 4x4 matrix by columns
@m = external addrspace(1) global [4 x <4 x float>], !vu.uniform !0
@x = external addrspace(4) global [0 x <4 x float>]

define void @mxv() #0 {
  %i = call i16 @llvm.vu.item.index()
  %p = getelementptr [0 x <4 x float>], ptr addrspace(4) @x, i16 0, i16 %i
  %v = load <4 x float>, ptr addrspace(4) %p
  %c0 = load <4 x float>, ptr addrspace(1) @m
  %p1 = getelementptr <4 x float>, ptr addrspace(1) @m, i16 1
  %c1 = load <4 x float>, ptr addrspace(1) %p1
  %p2 = getelementptr <4 x float>, ptr addrspace(1) @m, i16 2
  %c2 = load <4 x float>, ptr addrspace(1) %p2
  %p3 = getelementptr <4 x float>, ptr addrspace(1) @m, i16 3
  %c3 = load <4 x float>, ptr addrspace(1) %p3
  %vx = shufflevector <4 x float> %v, <4 x float> poison, <4 x i32> zeroinitializer
  %vy = shufflevector <4 x float> %v, <4 x float> poison, <4 x i32> <i32 1, i32 1, i32 1, i32 1>
  %vz = shufflevector <4 x float> %v, <4 x float> poison, <4 x i32> <i32 2, i32 2, i32 2, i32 2>
  %vw = shufflevector <4 x float> %v, <4 x float> poison, <4 x i32> <i32 3, i32 3, i32 3, i32 3>
  %t0 = fmul fast <4 x float> %c0, %vx
  %t1 = fmul fast <4 x float> %c1, %vy
  %t2 = fmul fast <4 x float> %c2, %vz
  %t3 = fmul fast <4 x float> %c3, %vw
  %s1 = fadd fast <4 x float> %t0, %t1
  %s2 = fadd fast <4 x float> %s1, %t2
  %s3 = fadd fast <4 x float> %s2, %t3
  store <4 x float> %s3, ptr addrspace(4) %p
  ret void
}

declare i16 @llvm.vu.item.index()
attributes #0 = { "vu-entry"="compute" "target-cpu"="vu0" }
!0 = !{i32 0}
