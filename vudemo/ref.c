/* Host reference: prints the expected checksum as a C define. It also checks
   that the inputs keep every result exact: Horner's method in float, step by
   step, must give the same result as in double. */
#include <stdio.h>

#include "demo.h"

static float x[4 * N_TOTAL], c[D + 1];

int main(void) {
  demo_init(x, c);
  for (int i = 0; i < 4 * N_TOTAL; i++) {
    double y = c[D];
    volatile float yf = c[D];
    for (int k = D - 1; k >= 0; k--) {
      y = y * x[i] + c[k];
      volatile float p = yf * x[i];
      yf = p + c[k];
      if ((double)yf != y) {
        fprintf(stderr, "x[%d] = %g: not exact at step %d\n", i, x[i], k);
        return 1;
      }
    }
    x[i] = (float)y;
  }
  printf("#define EXPECTED 0x%08xu\n", demo_checksum(x, 4 * N_TOTAL));
  return 0;
}
