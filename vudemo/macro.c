/* VU0 macro mode: vector code, built with -mvu0 so clang lowers it to COP2.
   Same algorithm as poly4.vuk. */
#include "demo.h"

typedef float f32x4 __attribute__((vector_size(16)));

void poly_vu0_macro(f32x4 *restrict x, const float *restrict c, int n) {
  f32x4 cv[D + 1];
  for (int k = 0; k <= D; k++)
    cv[k] = (f32x4){c[k], c[k], c[k], c[k]};
  for (int i = 0; i < n; i++) {
    f32x4 v = x[i], v2 = v * v, v3 = v2 * v, v4 = v2 * v2;
    f32x4 y = cv[D];
    for (int k = D - 4; k >= 0; k -= 4)
      y = y * v4 + (cv[k] + cv[k + 1] * v + cv[k + 2] * v2 + cv[k + 3] * v3);
    x[i] = y;
  }
}
