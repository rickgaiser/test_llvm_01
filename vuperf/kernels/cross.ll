; a = cross(a, b) of each item pair (xyz; w = 0)
@x = external addrspace(4) global [0 x [2 x <4 x float>]]

define void @cross() #0 {
  %i = call i16 @llvm.vu.item.index()
  %pa = getelementptr [0 x [2 x <4 x float>]], ptr addrspace(4) @x, i16 0, i16 %i, i16 0
  %pb = getelementptr [0 x [2 x <4 x float>]], ptr addrspace(4) @x, i16 0, i16 %i, i16 1
  %a = load <4 x float>, ptr addrspace(4) %pa
  %b = load <4 x float>, ptr addrspace(4) %pb
  %a1 = shufflevector <4 x float> %a, <4 x float> poison, <4 x i32> <i32 1, i32 2, i32 0, i32 3>
  %b2 = shufflevector <4 x float> %b, <4 x float> poison, <4 x i32> <i32 2, i32 0, i32 1, i32 3>
  %a2 = shufflevector <4 x float> %a, <4 x float> poison, <4 x i32> <i32 2, i32 0, i32 1, i32 3>
  %b1 = shufflevector <4 x float> %b, <4 x float> poison, <4 x i32> <i32 1, i32 2, i32 0, i32 3>
  %m1 = fmul fast <4 x float> %a1, %b2
  %m2 = fmul fast <4 x float> %a2, %b1
  %c = fsub fast <4 x float> %m1, %m2
  %r = insertelement <4 x float> %c, float 0.0, i32 3
  store <4 x float> %r, ptr addrspace(4) %pa
  ret void
}

declare i16 @llvm.vu.item.index()
attributes #0 = { "vu-entry"="compute" "target-cpu"="vu0" }
