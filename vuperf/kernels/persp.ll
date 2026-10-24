; xyz / w of each item, w kept (perspective divide)
@x = external addrspace(4) global [0 x <4 x float>]

define void @persp() #0 {
  %i = call i16 @llvm.vu.item.index()
  %p = getelementptr [0 x <4 x float>], ptr addrspace(4) @x, i16 0, i16 %i
  %v = load <4 x float>, ptr addrspace(4) %p
  %w = extractelement <4 x float> %v, i32 3
  %k = fdiv fast float 1.0, %w
  %ki = insertelement <4 x float> poison, float %k, i32 0
  %ks = shufflevector <4 x float> %ki, <4 x float> poison, <4 x i32> <i32 0, i32 0, i32 0, i32 poison>
  %kw = insertelement <4 x float> %ks, float 1.0, i32 3
  %r = fmul fast <4 x float> %v, %kw
  store <4 x float> %r, ptr addrspace(4) %p
  ret void
}

declare i16 @llvm.vu.item.index()
attributes #0 = { "vu-entry"="compute" "target-cpu"="vu0" }
