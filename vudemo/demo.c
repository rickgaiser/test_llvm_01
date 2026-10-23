/* p(x) for N_TOTAL quadwords on the EE FPU, VU0 and VU1.

   MODE_MACRO: everything on VU0 in macro mode (COP2 code from clang).
   MODE_MICRO: everything on VU0 in micro mode (poly4.vuk), in batches.
   MODE_ALL:   VU1 micro, VU0 micro and the EE FPU at the same time.
   MODE_BENCH: each unit alone, then all together, for timing on the PS2.

   All runs compute the same array, so they all have the same checksum,
   which the host reference (ref.c) computes as EXPECTED.

   Built on maxsdk (no ps2sdk): VU1 batches go through a VIF1 queue (REF
   tags from the data, UNPACK, MSCAL, FLUSHE), VU0 batches through COP2
   transfers (or a VIF0 queue with VU0_DMA=1), as ps2gl/ps2stuff do. */
#include <mx/mx.h>

#include "demo.h"
#include "expected.h"
#include "expected_vals.h"

typedef float f32x4 __attribute__((vector_size(16)));

void poly_fpu(float *x, const float *c, int n);
void poly_vu0_macro(f32x4 *x, const float *c, int n);

extern const char vu0_poly[], vu0_poly_end[], vu1_poly[], vu1_poly_end[];

static f32x4 x[N_TOTAL] __attribute__((aligned(64)));
static float c[D + 1];

/* -DVUDEMO_DEBUG=<ms>: print each step and wait <ms> before the next one,
   so that the last line on the host shows where the PS2 stopped. */
#ifdef VUDEMO_DEBUG
#define STEP(...)                                                              \
  do {                                                                         \
    mx_printf("dbg: " __VA_ARGS__);                                            \
    mx_printf(" (VPU-STAT %08x, VIF1_STAT %08x)\n", mx_vpu_stat(),             \
              MX_R32(MX_VIF_VIF1_STAT));                                       \
    mx_delay_us(1000 * VUDEMO_DEBUG);                                          \
  } while (0)
#else
#define STEP(...) ((void)0)
#endif

/* The kernels, with the data memory layout from poly.h. */
static struct mx_kernel kern[2];

/* The kernel's parameters: the table (the count is set per batch). */
static f32x4 head[VUK_POLY_SIZE] __attribute__((aligned(16)));

/* VU0's input by VIF0 DMA (1) or through COP2 (0). Both are correct on the
   PS2. With maxsdk VIF0 is faster (PS2, all three units: 31.63 vs 35.56
   cycles/quadword; VU0 alone 45.79 vs 49.66); with the ps2sdk version COP2
   was (36.2 vs 44.4). */
#ifndef VU0_DMA
#define VU0_DMA 1
#endif

/* VU1's results back by DMA through the scratchpad (toSPR from VU1 memory,
   then fromSPR) instead of mapped reads: ~5.5 vs ~26 EE cycles per quadword
   on the PS2 (maxsdk docs/vu1.md). */
#ifndef VU1_DMA_READ
#define VU1_DMA_READ 1
#endif

static struct mx_vuq q[2];
static mx_event ev[2];

/* EE cycles per phase of the micro batches of a run: [vu][send, wait, read] */
static u32 phase[2][3];

static void load_micro(void) {
  kern[0] = MX_VUK_KERNEL(POLY, 0, vu0_poly, vu0_poly_end);
  kern[1] = MX_VUK_KERNEL(POLY, 1, vu1_poly, vu1_poly_end);
  for (int vu = 0; vu < 2; vu++)
    if (mx_vu_load(vu, kern[vu].code, kern[vu].code_bytes, 0) < 0)
      mx_printf("VU%d microcode: upload failed\n", vu);
  STEP("microcode: VU0 %u, VU1 %u bytes", kern[0].code_bytes, kern[1].code_bytes);
}

/* One micro batch: send and start; the caller does other work, then
   finishes it. */
static void batch_start(int vu, f32x4 *part, int n) {
  u32 t0 = mx_count();
  ev[vu] = mx_vuq_batch(&q[vu], &kern[vu], head, part, (u32)n);
  phase[vu][0] += mx_count() - t0;
  STEP("VU%d batch of %d: started", vu, n);
}

static void batch_finish(int vu, f32x4 *part, int n) {
  u32 t0 = mx_count();
  if (mx_vuq_wait(&q[vu], ev[vu]) < 0)
    mx_printf("VU%d did not stop within a second (VPU-STAT %08x)\n", vu, mx_vpu_stat());
  u32 t1 = mx_count();
  /* the results go to memory, not into the data cache */
  mx_vuq_read(&q[vu], &kern[vu], MX_UCAB(part), (u32)n);
  phase[vu][1] += t1 - t0;
  phase[vu][2] += mx_count() - t1;
  STEP("VU%d batch: done", vu);
#ifdef VUDEMO_DEBUG
  if (!vu) {
    u32 m[8] __attribute__((aligned(16)));
    mx_vu0_read(m, 0, 1);
    mx_vu0_read(m + 4, VUK_POLY_X, 1);
    mx_printf("dbg: VU0 mem: n %u, x0 %08x; part[0] %08x; VIF0_STAT %08x MARK %x\n", m[0], m[4],
              *(u32 *)MX_UNCACHED(part), MX_R32(MX_VIF_VIF0_STAT), MX_R32(MX_VIF_VIF0_MARK));
  }
#endif
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
      mx_printf("  [%d] x=%f got %08x %f want %08x %f (%d ULP)\n", i, in[i],
                (unsigned)got[i], ((const float *)x)[i], (unsigned)w.u, w.f, (int)d);
    }
    differ++;
    if (d > max_ulp)
      max_ulp = d;
  }
  mx_printf("  %d of %d floats differ from the exact result, by at most %d ULP\n", differ,
            4 * N_TOTAL, (int)max_ulp);
  return max_ulp;
}

int main(void) {
  int failed = 0;
  mx_printf("vudemo: D=%d, %d quadwords (4 floats each), maxsdk\n", D, N_TOTAL);
  mx_dma_init();
  mx_vu_reset(3);
  load_micro();
  mx_vuq_init(&q[0], 0, VU0_DMA);
  mx_vuq_init(&q[1], 1, 1);
  if (VU1_DMA_READ)
    mx_vuq_readback_spr(&q[1], (void *)0x70000000, 0x4000);

  for (unsigned r = 0; r < sizeof(runs) / sizeof(runs[0]); r++) {
    demo_init((float *)x, c);
    memset(head, 0, sizeof(head));
    memcpy(&head[VUK_POLY_C], c, sizeof(c));
    STEP("run %s", runs[r].name);
    memset(phase, 0, sizeof(phase));
    u32 t0 = mx_count();
    /* The VUs get x by DMA from memory: write it back once (and invalidate
       it), then no VU batch needs the data cache written back. */
    mx_dcache_flush_all();
    runs[r].run();
    u32 t1 = mx_count();

    uint32_t sum = demo_checksum((const float *)x, 4 * N_TOTAL);
    mx_printf("%s:\n", runs[r].name);
    mx_printf("  First: %f %f %f %f\n", x[0][0], x[0][1], x[0][2], x[0][3]);
    mx_printf("  Checksum: %08x (exact results: %08x)\n", (unsigned)sum, EXPECTED);
    int32_t ulp = check_results(sum != EXPECTED);
    failed |= ulp > MAX_ULP;
    mx_printf("  Result: %s\n", ulp <= MAX_ULP ? "OK" : "FAIL");
    /* EE cycles; PCSX2 timings are not performance data */
    mx_printf("  Cycles: %u (%u.%02u per quadword)\n", t1 - t0, (t1 - t0) / N_TOTAL,
              (t1 - t0) % N_TOTAL * 100 / N_TOTAL);
    for (int vu = 0; vu < 2; vu++)
      if (phase[vu][0] | phase[vu][1] | phase[vu][2])
        mx_printf("  VU%d batches: send %u, wait %u, read %u cycles\n", vu, phase[vu][0],
                  phase[vu][1], phase[vu][2]);
  }
  MX_TEST_END(failed);
  return failed;
}
