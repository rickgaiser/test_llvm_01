; x = max(x, y, z, w) of each item
@x = external addrspace(4) global [0 x <4 x float>]

define void @hmax() #0 {
  %i = call i16 @llvm.vu.item.index()
  %p = getelementptr [0 x <4 x float>], ptr addrspace(4) @x, i16 0, i16 %i
  %v = load <4 x float>, ptr addrspace(4) %p
  %m = call fast float @llvm.vector.reduce.fmax.v4f32(<4 x float> %v)
  %r = insertelement <4 x float> %v, float %m, i32 0
  store <4 x float> %r, ptr addrspace(4) %p
  ret void
}

declare i16 @llvm.vu.item.index()
declare float @llvm.vector.reduce.fmax.v4f32(<4 x float>)
attributes #0 = { "vu-entry"="compute" "target-cpu"="vu0" }
