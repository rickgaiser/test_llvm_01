; (y, z, w, x) of each item
@x = external addrspace(4) global [0 x <4 x float>]

define void @rot() #0 {
  %i = call i16 @llvm.vu.item.index()
  %p = getelementptr [0 x <4 x float>], ptr addrspace(4) @x, i16 0, i16 %i
  %v = load <4 x float>, ptr addrspace(4) %p
  %r = shufflevector <4 x float> %v, <4 x float> poison, <4 x i32> <i32 1, i32 2, i32 3, i32 0>
  store <4 x float> %r, ptr addrspace(4) %p
  ret void
}

declare i16 @llvm.vu.item.index()
attributes #0 = { "vu-entry"="compute" "target-cpu"="vu0" }
