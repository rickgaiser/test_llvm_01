/* Host reference: prints the expected checksum as a C define, and with -v
   the expected values as a table, for finding mismatches. It also checks
   that the inputs keep every result exact: Horner's method in float, step by
   step, must give the same result as in double. */
#include <stdio.h>

#include "demo.h"

static float x[4 * N_TOTAL], c[D + 1];

int main(int argc, char **argv) {
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
  if (argc > 1) {
    printf("static const uint32_t expected_vals[4 * N_TOTAL] = {\n");
    for (int i = 0; i < 4 * N_TOTAL; i++) {
      union { float f; uint32_t u; } v = {x[i]};
      printf("0x%08x,%s", v.u, i % 8 == 7 ? "\n" : " ");
    }
    printf("};\n");
    return 0;
  }
  printf("#define EXPECTED 0x%08xu\n", demo_checksum(x, 4 * N_TOTAL));
  return 0;
}
