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
#include "expected_vals.h"
#include "vu.h"

typedef float f32x4 __attribute__((vector_size(16)));

void poly_fpu(float *x, const float *c, int n);
void poly_vu0_macro(f32x4 *x, const float *c, int n);

extern const char vu0_poly[], vu0_poly_end[], vu1_poly[], vu1_poly_end[];

static f32x4 x[N_TOTAL];
static float c[D + 1];

/* -DVUDEMO_DEBUG=<ms>: print each step, flush, and wait <ms> before the next
   one, so that the last line on the host shows where the PS2 stopped. */
#ifdef VUDEMO_DEBUG
#define STEP(...)                                                              \
  do {                                                                         \
    printf("dbg: " __VA_ARGS__);                                               \
    printf(" (VPU-STAT %08x, VIF1_STAT %08x)\n", vpu_stat(), VIF1_STAT);      \
    fflush(stdout);                                                            \
    delay_ms(VUDEMO_DEBUG);                                                    \
  } while (0)
#else
#define STEP(...) ((void)0)
#endif

/* The kernel's parameters, in the layout from poly.h: n, then the table. */
static f32x4 head[VUK_POLY_X] __attribute__((aligned(16)));

static void set_params(int n) {
  memset(head, 0, sizeof(head));
  ((uint32_t *)&head[VUK_POLY_N])[0] = n;
  memcpy(&head[VUK_POLY_C], c, sizeof(c));
}

/* The VIF1 packet: parameters and data of one VU1 batch, or the microcode. */
static uint32_t vif_buf[4 * (VUK_POLY_X + N_VU1) + 64] __attribute__((aligned(64)));

static void load_micro(void) {
  int quads0 = (int)(vu0_poly_end - vu0_poly + 15) / 16;
  vu0_load_micro(vu0_poly, quads0);
  STEP("VU0 microcode: %d quadwords, first word %08x (want %08x)", quads0,
       *(volatile uint32_t *)0x11000000, *(const uint32_t *)vu0_poly);
  if (vu1_load_micro(vif_buf, vu1_poly, (int)(vu1_poly_end - vu1_poly)))
    printf("VU1 microcode: MPG did not finish\n");
  STEP("VU1 microcode: %d bytes by MPG", (int)(vu1_poly_end - vu1_poly));
}

/* One micro batch: send, start; the caller does other work; finish. */
static void batch_start(int vu, f32x4 *part, int n) {
  set_params(n);
  if (vu) {
    struct vif_packet p = {vif_buf, 0};
    vif_put(&p, VIF_STCYCL(1, 1));
    vif_unpack(&p, 0, head, VUK_POLY_X);
    vif_unpack(&p, VUK_POLY_X, part, n);
    vif_put(&p, VIF_MSCAL(0));
    vif_put(&p, VIF_FLUSHE);
    STEP("VU1 batch of %d: send %d quadwords", n, (p.n + 3) / 4);
    vif_send(&p);
  } else {
    vu0_write(0, head, VUK_POLY_X);
    vu0_write(VUK_POLY_X, part, n);
    STEP("VU0 batch of %d: written, start", n);
    vu0_start();
  }
  STEP("VU%d batch: started", vu);
}

static void batch_finish(int vu, f32x4 *part, int n) {
  if (vu) {
    if (vif_wait())
      printf("VU1 did not stop within a second (VPU-STAT %08x, VIF1_STAT %08x)\n",
             vpu_stat(), VIF1_STAT);
    STEP("VU1 batch: stopped, read results");
    vu1_read(part, VUK_POLY_X, n);
  } else {
    vu0_wait();
    STEP("VU0 batch: stopped, read results");
    vu0_read(part, VUK_POLY_X, n);
  }
  STEP("VU%d batch: done", vu);
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

/* The R5900 multiplier is not commutative: with a power of two as the first
   operand (fs) and low mantissa bits in the second, the product is 1 ULP low
   (mulprobe.c, measured on the PS2). The compiler chooses the operand order,
   so the results are compared within MAX_ULP of the exact ones, not bit for
   bit. */
#define MAX_ULP 4

/* Floats as integers in the order of their values, for ULP distances. */
static int32_t ordered(uint32_t u) {
  return u & 0x80000000u ? -(int32_t)(u & 0x7fffffffu) : (int32_t)u;
}

/* Returns the largest distance in ULP from the exact results, and prints the
   first results that differ, with their inputs (demo_init gives them). */
static int32_t check_results(int verbose) {
  static float in[4 * N_TOTAL], cc[D + 1];
  const uint32_t *got = (const uint32_t *)x;
  int32_t max_ulp = 0;
  int differ = 0;
  demo_init(in, cc);
  for (int i = 0; i < 4 * N_TOTAL; i++) {
    int32_t d = ordered(got[i]) - ordered(expected_vals[i]);
    if (d < 0)
      d = -d;
    if (d == 0)
      continue;
    if (verbose && differ < 4) {
      union { uint32_t u; float f; } w = {expected_vals[i]};
      printf("  [%d] x=%f got %08x %f want %08x %f (%d ULP)\n", i, in[i],
             got[i], ((const float *)x)[i], w.u, w.f, (int)d);
    }
    differ++;
    if (d > max_ulp)
      max_ulp = d;
  }
  printf("  %d of %d floats differ from the exact result, by at most %d ULP\n",
         differ, 4 * N_TOTAL, (int)max_ulp);
  return max_ulp;
}

int main(void) {
  int failed = 0;
  printf("vudemo: D=%d, %d quadwords (4 floats each)\n", D, N_TOTAL);
  STEP("COP0 Status %08x (CU2 = COP2 usable: %d)", cop0_status(),
       (cop0_status() >> 30) & 1);
  vu_init();
  STEP("VU0, VU1 and VIF1 reset, VIF1 DMA channel set up");
  load_micro();

  for (unsigned r = 0; r < sizeof(runs) / sizeof(runs[0]); r++) {
    demo_init((float *)x, c);
    STEP("run %s", runs[r].name);
    uint32_t t0 = ee_cycles();
    runs[r].run();
    uint32_t t1 = ee_cycles();

    uint32_t sum = demo_checksum((const float *)x, 4 * N_TOTAL);
    printf("%s:\n", runs[r].name);
    printf("  First: %f %f %f %f\n", x[0][0], x[0][1], x[0][2], x[0][3]);
    printf("  Checksum: %08x (exact results: %08x)\n", sum, EXPECTED);
    int32_t ulp = check_results(sum != EXPECTED);
    failed |= ulp > MAX_ULP;
    printf("  Result: %s\n", ulp <= MAX_ULP ? "OK" : "FAIL");
    /* EE cycles; PCSX2 timings are not performance data */
    printf("  Cycles: %u (%u.%02u per quadword)\n", t1 - t0, (t1 - t0) / N_TOTAL,
           (t1 - t0) % N_TOTAL * 100 / N_TOTAL);
  }
  printf("@@PS2TEST_END@@ status=%d\n", failed);
  return failed;
}
