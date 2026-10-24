; xyz / |xyz| of each item, w kept (normalize: a sum of squares and 1/sqrt)
@x = external addrspace(4) global [0 x <4 x float>]

define void @norm() #0 {
  %i = call i16 @llvm.vu.item.index()
  %p = getelementptr [0 x <4 x float>], ptr addrspace(4) @x, i16 0, i16 %i
  %v = load <4 x float>, ptr addrspace(4) %p
  %q = fmul fast <4 x float> %v, %v
  %a = extractelement <4 x float> %q, i32 0
  %b = extractelement <4 x float> %q, i32 1
  %c = extractelement <4 x float> %q, i32 2
  %s1 = fadd fast float %a, %b
  %s2 = fadd fast float %s1, %c
  %l = call fast float @llvm.sqrt.f32(float %s2)
  %k = fdiv fast float 1.0, %l
  %ki = insertelement <4 x float> poison, float %k, i32 0
  %ks = shufflevector <4 x float> %ki, <4 x float> poison, <4 x i32> <i32 0, i32 0, i32 0, i32 poison>
  %kw = insertelement <4 x float> %ks, float 1.0, i32 3
  %r = fmul fast <4 x float> %v, %kw
  store <4 x float> %r, ptr addrspace(4) %p
  ret void
}

declare i16 @llvm.vu.item.index()
declare float @llvm.sqrt.f32(float)
attributes #0 = { "vu-entry"="compute" "target-cpu"="vu0" }
