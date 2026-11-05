/* The IOP bench: every kernel in every variant, on the same data. The output of
   each is compared with the first variant's; times are in cycles, the best of
   several runs with interrupts off. Prints "b <kernel> <variant> <cycles>". */

#include "irx_imports.h"
#include "kernels.h"

IRX_ID("iop_bench", 1, 0);

#define DECL(n)                                                                            \
  extern kernel gos_##n, go2_##n, cos_##n, co2_##n, cor_##n, asm_##n;
KERNELS(DECL)

static const char *const variants[] = {"gcc-Os", "gcc-O2", "clang-Os", "clang-O2", "clang-O2r", "asm"};
#define NV 6

struct bench {
  const char *name;
  kernel *f[NV];
};
#define ROW(n) {#n, {gos_##n, go2_##n, cos_##n, co2_##n, cor_##n, asm_##n}},
static const struct bench benches[] = {KERNELS(ROW)};
#define NB (sizeof benches / sizeof *benches)

#define BUF 16384
static u32 src_w[BUF / 4 + 16] __attribute__((aligned(64)));
static u32 dst_w[BUF / 4 + 16] __attribute__((aligned(64)));
static u32 fifo_word __attribute__((aligned(64)));
#define SRC ((u8 *)src_w)
#define DST ((u8 *)dst_w)

static int timer;

#define P(...)                                                                             \
  do {                                                                                     \
    printf(__VA_ARGS__);                                                                   \
    DelayThread(2000);                                                                     \
  } while (0)

struct args {
  const void *s;
  u32 n, x;
};

/* the same as prep() in sim.py */
static void fill(void) {
  u32 v = 12345;
  for (u32 i = 0; i < BUF / 4 + 16; i++) {
    v = v * 1664525 + 1013904223;
    src_w[i] = v;
  }
}

static int streq(const char *a, const char *b) {
  while (*a && *a == *b)
    a++, b++;
  return *a == *b;
}

static void prep(const char *name, struct args *a) {
  fill();
  a->s = SRC;
  a->n = 4096;
  a->x = 0;
  if (streq(name, "cpy_unal")) {
    a->s = SRC + 2;
  } else if (streq(name, "set_word")) {
    a->x = 0x5a5aa5a5;
  } else if (streq(name, "str_len")) {
    for (u32 i = 0; i < 4096; i++)
      SRC[i] |= 1;
    SRC[4093] = 0;
  } else if (streq(name, "bit_rev") || streq(name, "mix_vol")) {
    a->x = 0x2000;
  } else if (streq(name, "gain_q16")) {
    a->n = 2048;
    a->x = 0x00412000;
  } else if (streq(name, "fifo_rd")) {
    fifo_word = 0x12345678;
    a->x = ((u32)&fifo_word & 0x1fffffff) | 0xa0000000;
  } else if (streq(name, "ring_put")) {
    a->n = 1024;
    a->x = 1536;
  } else if (streq(name, "list_walk")) {
    for (u32 i = 0; i < 256; i++) {
      src_w[i * 4] = i == 255 ? 0 : (u32)&src_w[(i + 1) * 4];
    }
  } else if (streq(name, "fat_scan")) {
    for (u32 i = 0; i < 1024; i++)
      src_w[i] |= 1;
    src_w[1021] = 0;
  } else if (streq(name, "itoa10")) {
    a->n = 512;
  } else if (streq(name, "mul64")) {
    a->n = 1024;
  }
}

static u32 hash(u32 ret) {
  u32 h = ret;
  for (u32 i = 0; i < 12288 / 4; i++)
    h = h * 31 + dst_w[i];
  return h;
}

static void clear_dst(const char *name) {
  /* in place kernels start from the source */
  int copy = streq(name, "bit_rev") || streq(name, "mix_vol");
  for (u32 i = 0; i < BUF / 4 + 16; i++)
    dst_w[i] = copy ? src_w[i] : 0;
}

int _start(int argc, char **argv) {
  int fails = 0;
  timer = AllocHardTimer(1, 32, 1);
  if (timer < 0) {
    P("iop_bench: no timer\n");
    return 1 << 2 | MODULE_NO_RESIDENT_END;
  }
  SetTimerMode(timer, 0);
  P("iop_bench: %u kernels, %d variants\n", (u32)NB, NV);

  for (u32 b = 0; b < NB; b++) {
    const struct bench *k = &benches[b];
    struct args a;
    u32 want = 0;
    prep(k->name, &a);
    for (int v = 0; v < NV; v++) {
      u32 best = ~0u, h = 0;
      for (int r = 0; r < 5; r++) {
        int st;
        clear_dst(k->name);
        CpuSuspendIntr(&st);
        u32 t0 = GetTimerCounter(timer);
        u32 ret = k->f[v](DST, a.s, a.n, a.x);
        u32 t1 = GetTimerCounter(timer);
        CpuResumeIntr(st);
        if (t1 - t0 < best)
          best = t1 - t0;
        h = hash(ret);
      }
      if (v == 0)
        want = h;
      P("b %-10s %-9s %7u %s\n", k->name, variants[v], best, h == want ? "ok" : "WRONG");
      if (h != want)
        fails++;
    }
  }
  FreeHardTimer(timer);
  P("iop_bench: end, %d wrong\n", fails);
  return fails << 2 | MODULE_NO_RESIDENT_END;
}
