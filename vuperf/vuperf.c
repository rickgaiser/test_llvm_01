/* VU micro-mode code generation benchmarks (kernels/ *.ll): VU0 cycles per
   item of each compiled kernel. The items stay in VU0 memory; the timed
   region is only the start (VCALLMS) and the wait for the end, for N1 and N2
   items, so (T(N2) - T(N1)) / (N2 - N1) is the VU time per item without the
   start, the end and the transfers. Run on the PS2 only. */

#include <mx/mx.h>

struct kernel {
  const char *name;
  const unsigned char *code, *end;
  u32 entry, size, n_qw, item_qw;
};

#include "kernels.h"

#define RUNS 9
#define VU0_QW 256

static u32 median(u32 *v, int n) {
  for (int i = 1; i < n; i++)
    for (int j = i; j > 0 && v[j] < v[j - 1]; j--) {
      u32 t = v[j];
      v[j] = v[j - 1];
      v[j - 1] = t;
    }
  return v[n / 2];
}

static float buf[4 * VU0_QW] MX_ALIGNED(16);

/* the header (uniforms 0.5, the count n) and the items (1.0 .. 1.75, w 1.0) */
static void setup(const struct kernel *k, u32 n) {
  for (u32 i = 0; i < 4 * VU0_QW; i++)
    buf[i] = 0.5f;
  static const float val[7] = {1.0f, 1.125f, 1.25f, 1.375f, 1.5f, 1.625f, 1.75f};
  for (u32 i = 4 * k->size; i < 4 * VU0_QW; i++)
    buf[i] = (i & 3) == 3 ? 1.0f : val[i % 7];
  ((u32 *)buf)[4 * k->n_qw] = n;
  mx_vu0_write(0, buf, VU0_QW);
}

static u32 time_n(const struct kernel *k, u32 n) {
  u32 v[RUNS];
  for (int r = 0; r < RUNS; r++) {
    setup(k, n);
    u32 t0 = mx_count();
    mx_vu0_call(k->entry);
    mx_vu0_wait();
    v[r] = mx_count() - t0;
  }
  return median(v, RUNS);
}

int main(void) {
  int n = (int)(sizeof kernels / sizeof kernels[0]);
  mx_printf("VU0 micro kernels: cycles per item, median of %d\n", RUNS);
  for (int i = 0; i < n; i++) {
    const struct kernel *k = &kernels[i];
    if (mx_vu_load(0, k->code, (u32)(k->end - k->code), 0) < 0) {
      mx_printf("%s: upload failed\n", k->name);
      continue;
    }
    u32 n2 = (VU0_QW - k->size) / k->item_qw, n1 = n2 / 4;
    u32 t1 = time_n(k, n1), t2 = time_n(k, n2);
    u32 per = (t2 - t1) * 100 / (n2 - n1);
    /* results of one run over n2 items: a hash of all words and item 0 */
    setup(k, n2);
    mx_vu0_call(k->entry);
    mx_vu0_wait();
    mx_vu0_read(buf, 0, VU0_QW);
    const u32 *w = (const u32 *)buf + 4 * k->size;
    u32 h = 2166136261u;
    for (u32 j = 0; j < 4 * n2 * k->item_qw; j++)
      h = (h ^ w[j]) * 16777619u;
    mx_printf("kernel %-8s %4u.%02u cycles/item (%u words, items %u/%u: %u/%u cycles)\n",
              k->name, per / 100, per % 100, (u32)(k->end - k->code) / 8, n1, n2, t1, t2);
    mx_printf("Result: %-8s hash %08x item0 %08x %08x %08x %08x\n", k->name, h, w[0], w[1],
              w[2], w[3]);
  }
  mx_printf("@@PS2TEST_END@@ status=0\n");
  return 0;
}
