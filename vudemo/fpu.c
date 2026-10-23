/* EE FPU part: plain scalar code. Built without -mvu0 and without
   vectorization, so it stays on the FPU. Same algorithm as poly4.vuk:
   4 degrees per step, y = y x^4 + (c[k] + c[k+1] x + c[k+2] x^2 + c[k+3] x^3). */
#include "demo.h"

void poly_fpu(float *restrict x, const float *restrict c, int n) {
  for (int i = 0; i < n; i++) {
    float v = x[i], v2 = v * v, v3 = v2 * v, v4 = v2 * v2;
    float y = c[D];
    for (int k = D - 4; k >= 0; k -= 4)
      y = y * v4 + (c[k] + c[k + 1] * v + c[k + 2] * v2 + c[k + 3] * v3);
    x[i] = y;
  }
}
