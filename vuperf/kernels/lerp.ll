; x + (b - x) * t of each item, b and t uniforms (one product)
@b = external addrspace(1) global <4 x float>, !vu.uniform !0
@t = external addrspace(1) global <4 x float>, !vu.uniform !1
@x = external addrspace(4) global [0 x <4 x float>]

define void @lerp() #0 {
  %i = call i16 @llvm.vu.item.index()
  %p = getelementptr [0 x <4 x float>], ptr addrspace(4) @x, i16 0, i16 %i
  %v = load <4 x float>, ptr addrspace(4) %p
  %bv = load <4 x float>, ptr addrspace(1) @b
  %tv = load <4 x float>, ptr addrspace(1) @t
  %d = fsub fast <4 x float> %bv, %v
  %m = fmul fast <4 x float> %d, %tv
  %r = fadd fast <4 x float> %v, %m
  store <4 x float> %r, ptr addrspace(4) %p
  ret void
}

declare i16 @llvm.vu.item.index()
attributes #0 = { "vu-entry"="compute" "target-cpu"="vu0" }
!0 = !{i32 0}
!1 = !{i32 16}
