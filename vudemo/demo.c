/* p(x) for N_TOTAL quadwords on the EE FPU, VU0 and VU1.

   MODE_MACRO: everything on VU0 in macro mode (COP2 code from clang).
   MODE_MICRO: everything on VU0 in micro mode (poly4.vuk), in batches.
   MODE_ALL:   VU1 micro, VU0 micro and the EE FPU at the same time.
   MODE_BENCH: each unit alone, then all together, for timing on the PS2.

   All runs compute the same array, so they all have the same checksum,
   which the host reference (ref.c) computes as EXPECTED. */
#include <stdio.h>
#include <string.h>

#include "demo.h"
#include "expected.h"
#include "vu.h"

typedef float f32x4 __attribute__((vector_size(16)));

void poly_fpu(float *x, const float *c, int n);
void poly_vu0_macro(f32x4 *x, const float *c, int n);

extern const char vu0_poly[], vu0_poly_end[], vu1_poly[], vu1_poly_end[];

static f32x4 x[N_TOTAL];
static float c[D + 1];

/* The kernel's parameters, in the layout from poly.h: n, then the table. */
static void vu_params(volatile uint32_t *data, int n) {
  f32x4 head[VUK_POLY_X] __attribute__((aligned(16)));
  memset(head, 0, sizeof(head));
  ((uint32_t *)&head[VUK_POLY_N])[0] = n;
  memcpy(&head[VUK_POLY_C], c, sizeof(c));
  vu_copy_to(data, head, VUK_POLY_X);
}

static void load_micro(volatile uint32_t *micro, const char *start, const char *end) {
  vu_copy_to(micro, start, (int)(end - start + 15) / 16);
}

/* One micro batch: copy in, start; the caller does other work; finish. */
static void batch_start(int vu, f32x4 *part, int n) {
  volatile uint32_t *data = vu ? VU1_DATA : VU0_DATA;
  vu_params(data, n);
  vu_copy_to(data + 4 * VUK_POLY_X, part, n);
  if (vu)
    vu1_start();
  else
    vu0_start();
}

static void batch_finish(int vu, f32x4 *part, int n) {
  vu_wait(vu ? VPU_STAT_VBS1 : VPU_STAT_VBS0);
  vu_copy_from(part, (vu ? VU1_DATA : VU0_DATA) + 4 * VUK_POLY_X, n);
}

/* All quadwords on one VU, in batches that fit its data memory. */
static void run_micro_on(int vu) {
  int size = vu ? N_VU1 : N_VU0;
  for (int done = 0; done < N_TOTAL; done += size) {
    int n = N_TOTAL - done < size ? N_TOTAL - done : size;
    batch_start(vu, &x[done], n);
    batch_finish(vu, &x[done], n);
  }
}

static __attribute__((unused)) void run_fpu(void) { poly_fpu((float *)x, c, 4 * N_TOTAL); }
static __attribute__((unused)) void run_vu1(void) { run_micro_on(1); }
static __attribute__((unused)) void run_macro(void) { poly_vu0_macro(x, c, N_TOTAL); }
static __attribute__((unused)) void run_micro(void) { run_micro_on(0); }

static __attribute__((unused)) void run_all(void) {
  f32x4 *vu1_part = x;
  f32x4 *vu0_part = x + N_VU1;
  f32x4 *fpu_part = vu0_part + VU0_BATCHES * N_VU0;

  /* VU1 gets one large batch and runs for the whole time. */
  batch_start(1, vu1_part, N_VU1);

  /* The EE keeps VU0 busy with batches and runs the FPU in between. */
  for (int b = 0; b < VU0_BATCHES; b++) {
    batch_start(0, vu0_part + b * N_VU0, N_VU0);
    poly_fpu((float *)(fpu_part + b * N_FPU_BATCH), c, 4 * N_FPU_BATCH);
    batch_finish(0, vu0_part + b * N_VU0, N_VU0);
  }

  batch_finish(1, vu1_part, N_VU1);
}

static const struct {
  const char *name;
  void (*run)(void);
} runs[] = {
#if defined(MODE_MACRO)
    {"VU0 macro", run_macro},
#elif defined(MODE_MICRO)
    {"VU0 micro", run_micro},
#elif defined(MODE_ALL)
    {"FPU + VU0 micro + VU1 micro", run_all},
#else
    {"FPU", run_fpu},
    {"VU1 micro", run_vu1},
    {"VU0 macro", run_macro},
    {"VU0 micro", run_micro},
    {"FPU + VU0 micro + VU1 micro", run_all},
#endif
};

int main(void) {
  int failed = 0;
  load_micro(VU0_MICRO, vu0_poly, vu0_poly_end);
  load_micro(VU1_MICRO, vu1_poly, vu1_poly_end);
  printf("vudemo: D=%d, %d quadwords (4 floats each)\n", D, N_TOTAL);

  for (unsigned r = 0; r < sizeof(runs) / sizeof(runs[0]); r++) {
    demo_init((float *)x, c);
    uint32_t t0 = ee_cycles();
    runs[r].run();
    uint32_t t1 = ee_cycles();

    uint32_t sum = demo_checksum((const float *)x, 4 * N_TOTAL);
    failed |= sum != EXPECTED;
    printf("%s:\n", runs[r].name);
    printf("  First: %f %f %f %f\n", x[0][0], x[0][1], x[0][2], x[0][3]);
    printf("  Result: %08x (expected %08x) %s\n", sum, EXPECTED,
           sum == EXPECTED ? "OK" : "FAIL");
    /* EE cycles; PCSX2 timings are not performance data */
    printf("  Cycles: %u (%u.%02u per quadword)\n", t1 - t0, (t1 - t0) / N_TOTAL,
           (t1 - t0) % N_TOTAL * 100 / N_TOTAL);
  }
  printf("@@PS2TEST_END@@ status=%d\n", failed);
  return failed;
}
