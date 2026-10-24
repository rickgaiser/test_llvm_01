; x = x + y + z + w of each item (a horizontal sum)
@x = external addrspace(4) global [0 x <4 x float>]

define void @hsum() #0 {
  %i = call i16 @llvm.vu.item.index()
  %p = getelementptr [0 x <4 x float>], ptr addrspace(4) @x, i16 0, i16 %i
  %v = load <4 x float>, ptr addrspace(4) %p
  %a = extractelement <4 x float> %v, i32 0
  %b = extractelement <4 x float> %v, i32 1
  %c = extractelement <4 x float> %v, i32 2
  %d = extractelement <4 x float> %v, i32 3
  %s1 = fadd fast float %a, %b
  %s2 = fadd fast float %s1, %c
  %s3 = fadd fast float %s2, %d
  %r = insertelement <4 x float> %v, float %s3, i32 0
  store <4 x float> %r, ptr addrspace(4) %p
  ret void
}

declare i16 @llvm.vu.item.index()
attributes #0 = { "vu-entry"="compute" "target-cpu"="vu0" }
