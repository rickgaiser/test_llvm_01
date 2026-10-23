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

static f32x4 x[N_TOTAL] __attribute__((aligned(64)));
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

static uint64_t chain_buf[2][16][2] __attribute__((aligned(64)));

/* EE cycles per phase of the micro batches of a run: [vu][send, wait, read] */
static uint32_t phase[2][3];

/* One micro batch: send by DMA and start; the caller does other work, then
   finishes it. head is written by set_params and sent before it changes
   again (the other VU's batch is sent after this DMA has read it). */
static void batch_start(int vu, f32x4 *part, int n) {
  uint32_t t0 = ee_cycles();
  static f32x4 heads[2][VUK_POLY_X] __attribute__((aligned(64)));
  set_params(n);
  if (vu) {
    memcpy(UNCACHED(&heads[vu][0]), head, sizeof(head));
    struct vif_chain ch = {UNCACHED(&chain_buf[vu][0]), 0};
    chain_unpack(&ch, 0, heads[vu], VUK_POLY_X, VIF_STCYCL(1, 1));
    chain_unpack(&ch, VUK_POLY_X, part, n, VIF_NOP);
    STEP("VU1 batch of %d: chain of %d tags", n, ch.n + 1);
    chain_send(DMA_CHANNEL_VIF1, &ch, VIF_MSCAL(0), VIF_FLUSHE);
  } else {
    vu0_write(0, head, VUK_POLY_X);
    vu0_write(VUK_POLY_X, part, n);
    STEP("VU0 batch of %d: written, start", n);
    vu0_start();
  }
  phase[vu][0] += ee_cycles() - t0;
  STEP("VU%d batch: started", vu);
}

static void batch_finish(int vu, f32x4 *part, int n) {
  uint32_t t0 = ee_cycles();
  if (!vu)
    vu0_wait();
  else if (vif_wait(1))
    printf("VU1 did not stop within a second (VPU-STAT %08x, VIF1_STAT %08x)\n",
           vpu_stat(), VIF1_STAT);
  uint32_t t1 = ee_cycles();
  STEP("VU%d batch: stopped, read results", vu);
  /* the results go to memory, not into the data cache */
  if (vu)
    vu1_read(UCAB(part), VUK_POLY_X, n);
  else
    vu0_read(UCAB(part), VUK_POLY_X, n);
  phase[vu][1] += t1 - t0;
  phase[vu][2] += ee_cycles() - t1;
  STEP("VU%d batch: done; VU memory (mapped): n %08x, first input/result %08x; "
       "stored %08x",
       vu, ((volatile uint32_t *)(vu ? 0x1100c000 : 0x11004000))[4 * VUK_POLY_N],
       ((volatile uint32_t *)(vu ? 0x1100c000 : 0x11004000))[4 * VUK_POLY_X],
       *UNCACHED((uint32_t *)part));
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
  for (int r = 0; r < ROUNDS; r++) {
    f32x4 *vu0_part = x + r * N_ROUND;
    f32x4 *vu1_part = vu0_part + N_VU0;
    f32x4 *fpu_part = vu1_part + N_VU1_ROUND;

    batch_start(0, vu0_part, N_VU0);
    if (r > 0)
      batch_finish(1, vu1_part - N_ROUND, N_VU1_ROUND);
    batch_start(1, vu1_part, N_VU1_ROUND);
    poly_fpu((float *)fpu_part, c, 4 * N_FPU_ROUND);
    batch_finish(0, vu0_part, N_VU0);
  }
  batch_finish(1, x + (ROUNDS - 1) * N_ROUND + N_VU0, N_VU1_ROUND);
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
    memset(phase, 0, sizeof(phase));
    uint32_t t0 = ee_cycles();
    /* The VUs get x by DMA from memory: write it back once (and invalidate
       it), then no VU batch needs the data cache written back. */
    FlushCache(0);
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
    for (int vu = 0; vu < 2; vu++)
      if (phase[vu][0] | phase[vu][1] | phase[vu][2])
        printf("  VU%d batches: send %u, wait %u, read %u cycles\n", vu,
               phase[vu][0], phase[vu][1], phase[vu][2]);
  }
  printf("@@PS2TEST_END@@ status=%d\n", failed);
  return failed;
}
