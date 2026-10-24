/* Accumulator value checks for the compiler: a sum s = s0 + a0*b0 + a1*b1 ...
   kept in the R5900 FPU or VU0 ACC across the steps (MADDA) must give the
   same bits as a multiply and an add per step, and a value must get into ACC
   and out of it unchanged. The inputs are random and inexact, so every step
   rounds. Prints the mismatches of each variant; the PS2 decides. */

#include <mx/mx.h>

#define TRIALS 4000
#define MAXN 16

static u32 seed = 12345;
static u32 rnd(void) {
  seed ^= seed << 13;
  seed ^= seed >> 17;
  seed ^= seed << 5;
  return seed;
}

/* A random float with an exponent within 2^-8 .. 2^8 and either sign. */
static u32 rndf(void) {
  u32 r = rnd();
  u32 e = 127 + (r >> 23) % 17 - 8;
  return (r & 0x80000000u) | (e << 23) | (rnd() & 0x7fffffu);
}

static float a[MAXN], b[MAXN];
static float va[MAXN][4] MX_ALIGNED(16), vb[MAXN][4] MX_ALIGNED(16);
static float vs0[4] MX_ALIGNED(16), vout[4] MX_ALIGNED(16);

static u32 bits(float f) {
  union {
    float f;
    u32 u;
  } x = {.f = f};
  return x.u;
}

static float fl(u32 u) {
  union {
    u32 u;
    float f;
  } x = {.u = u};
  return x.f;
}

/* s = s0; s = s + a[i] * b[i] with MUL.S and ADD.S */
static float fpu_ref(float s0, int n) {
  float s = s0, x, y;
  const float *pa = a, *pb = b;
  __asm__ __volatile__(".set push\n.set noreorder\n"
                       "1:\n"
                       "lwc1 %1, 0(%3)\n"
                       "lwc1 %2, 0(%4)\n"
                       "mul.s %1, %1, %2\n"
                       "add.s %0, %0, %1\n"
                       "addiu %3, %3, 4\n"
                       "addiu %5, %5, -1\n"
                       "bnez %5, 1b\n"
                       "addiu %4, %4, 4\n"
                       ".set pop\n"
                       : "+f"(s), "=&f"(x), "=&f"(y), "+r"(pa), "+r"(pb), "+r"(n)
                       :
                       : "memory");
  return s;
}

/* ACC = s0 by Init, ACC += a[i] * b[i] with MADDA.S, the result by Read.
   Init 0: MULA.S s0, 1.0; 1: ADDA.S s0, 0.0; 2: MULA.S 0.0, 0.0 (s0 = +0).
   Read 0: MSUB.S d, 0.0, 0.0; 1: MADD.S d, 0.0, 0.0; 2: the last product
   in MADD.S d, a, b. */
static float fpu_acc(float s0, int n, int init, int read) {
  float one = 1.0f, zero, d, x, y;
  const float *pa = a, *pb = b;
  __asm__ __volatile__("mtc1 $zero, %0\n" : "=f"(zero));
  if (init == 0)
    __asm__ __volatile__("mula.s %0, %1\n" ::"f"(s0), "f"(one));
  else if (init == 1)
    __asm__ __volatile__("adda.s %0, %1\n" ::"f"(s0), "f"(zero));
  else
    __asm__ __volatile__("mula.s %0, %0\n" ::"f"(zero));
  if (read == 2)
    n--;
  if (n)
    __asm__ __volatile__(".set push\n.set noreorder\n"
                         "1:\n"
                         "lwc1 %0, 0(%2)\n"
                         "lwc1 %1, 0(%3)\n"
                         "madda.s %0, %1\n"
                         "addiu %2, %2, 4\n"
                         "addiu %4, %4, -1\n"
                         "bnez %4, 1b\n"
                         "addiu %3, %3, 4\n"
                         ".set pop\n"
                         : "=&f"(x), "=&f"(y), "+r"(pa), "+r"(pb), "+r"(n)
                         :
                         : "memory");
  if (read == 0)
    __asm__ __volatile__("msub.s %0, %1, %1\n" : "=f"(d) : "f"(zero));
  else if (read == 1)
    __asm__ __volatile__("madd.s %0, %1, %1\n" : "=f"(d) : "f"(zero));
  else
    __asm__ __volatile__("madd.s %0, %1, %2\n" : "=f"(d) : "f"(*pa), "f"(*pb));
  return d;
}

/* The same with VU0: VMUL and VADD per step. */
static void vu0_ref(int n) {
  const float *pa = va[0], *pb = vb[0];
  __asm__ __volatile__(".set push\n.set noreorder\n"
                       "lqc2 $vf1, 0(%3)\n"
                       "1:\n"
                       "lqc2 $vf2, 0(%0)\n"
                       "lqc2 $vf3, 0(%1)\n"
                       "vmul.xyzw $vf2, $vf2, $vf3\n"
                       "vadd.xyzw $vf1, $vf1, $vf2\n"
                       "addiu %0, %0, 16\n"
                       "addiu %2, %2, -1\n"
                       "bnez %2, 1b\n"
                       "addiu %1, %1, 16\n"
                       "sqc2 $vf1, 0(%4)\n"
                       ".set pop\n"
                       : "+r"(pa), "+r"(pb), "+r"(n)
                       : "r"(vs0), "r"(vout)
                       : "memory");
}

/* ACC = s0 by Init, VMADDA per step, the result by Read, in one asm
   statement so that nothing else uses vf1-vf3 or ACC in between.
   Init: VMULAw ACC, s0, vf0 (s0 * 1.0) or VADDAx ACC, s0, vf0 (+ 0.0).
   Read: VMSUBx d, vf0, vf0 (ACC - 0.0) or VMADDx d, vf0, vf0. */
#define VU0_ACC(name, init, read)                                              \
  static void name(int n) {                                                    \
    const float *pa = va[0], *pb = vb[0];                                      \
    __asm__ __volatile__(".set push\n.set noreorder\n"                         \
                         "lqc2 $vf1, 0(%3)\n" init "\n"                        \
                         "1:\n"                                                \
                         "lqc2 $vf2, 0(%0)\n"                                  \
                         "lqc2 $vf3, 0(%1)\n"                                  \
                         "vmadda.xyzw $ACC, $vf2, $vf3\n"                      \
                         "addiu %0, %0, 16\n"                                  \
                         "addiu %2, %2, -1\n"                                  \
                         "bnez %2, 1b\n"                                       \
                         "addiu %1, %1, 16\n" read "\n"                        \
                         "sqc2 $vf1, 0(%4)\n"                                  \
                         ".set pop\n"                                          \
                         : "+r"(pa), "+r"(pb), "+r"(n)                         \
                         : "r"(vs0), "r"(vout)                                 \
                         : "memory");                                          \
  }
VU0_ACC(vu0_acc_mulaw_msub, "vmulaw.xyzw $ACC, $vf1, $vf0",
        "vmsubx.xyzw $vf1, $vf0, $vf0")
VU0_ACC(vu0_acc_addax_msub, "vaddax.xyzw $ACC, $vf1, $vf0",
        "vmsubx.xyzw $vf1, $vf0, $vf0")
VU0_ACC(vu0_acc_mulaw_madd, "vmulaw.xyzw $ACC, $vf1, $vf0",
        "vmaddx.xyzw $vf1, $vf0, $vf0")

static void vu0_acc(int n, int var) {
  if (var == 0)
    vu0_acc_mulaw_msub(n);
  else if (var == 1)
    vu0_acc_addax_msub(n);
  else
    vu0_acc_mulaw_madd(n);
}

struct count {
  const char *name;
  int bad, total;
  u32 got, want;
};

static void check(struct count *c, u32 got, u32 want) {
  c->total++;
  if (got != want && c->bad++ == 0) {
    c->got = got;
    c->want = want;
  }
}

static void report(const struct count *c) {
  mx_printf("Check: %-34s %4d of %4d differ", c->name, c->bad, c->total);
  if (c->bad)
    mx_printf("  (first: %08x, want %08x)", (unsigned)c->got, (unsigned)c->want);
  mx_printf("\n");
}

int main(void) {
  struct count f[] = {
      {"fpu mula s0,1 / madda / msub 0,0"}, {"fpu adda s0,0 / madda / msub 0,0"},
      {"fpu mula s0,1 / madda / madd 0,0"}, {"fpu mula s0,1 / madda / madd a,b"},
      {"fpu mula 0,0 / madda / msub 0,0 (s0=+0)"},
      {"fpu s0 = -0: mula s0,1 / msub 0,0"},
  };
  struct count v[] = {
      {"vu0 mulaw / vmadda / vmsubx"},
      {"vu0 addax / vmadda / vmsubx"},
      {"vu0 mulaw / vmadda / vmaddx"},
      {"vu0 s0 = -0: mulaw / vmsubx"},
  };
  for (int t = 0; t < TRIALS; t++) {
    int n = 1 + rnd() % MAXN;
    for (int i = 0; i < MAXN; i++) {
      a[i] = fl(rndf());
      b[i] = fl(rndf());
      for (int k = 0; k < 4; k++) {
        va[i][k] = fl(rndf());
        vb[i][k] = fl(rndf());
      }
    }
    float s0 = fl(rndf());
    u32 want = bits(fpu_ref(s0, n));
    check(&f[0], bits(fpu_acc(s0, n, 0, 0)), want);
    check(&f[1], bits(fpu_acc(s0, n, 1, 0)), want);
    check(&f[2], bits(fpu_acc(s0, n, 0, 1)), want);
    check(&f[3], bits(fpu_acc(s0, n, 0, 2)), want);
    check(&f[4], bits(fpu_acc(0.0f, n, 2, 0)), bits(fpu_ref(0.0f, n)));

    for (int k = 0; k < 4; k++)
      vs0[k] = fl(rndf());
    vu0_ref(n);
    u32 vw[4];
    for (int k = 0; k < 4; k++)
      vw[k] = bits(vout[k]);
    for (int var = 0; var < 3; var++) {
      vu0_acc(n, var);
      for (int k = 0; k < 4; k++)
        check(&v[var], bits(vout[k]), vw[k]);
    }
  }
  /* -0.0 in and out, with no products: a = 0. */
  for (int i = 0; i < MAXN; i++) {
    a[i] = 0.0f;
    for (int k = 0; k < 4; k++)
      va[i][k] = 0.0f;
  }
  check(&f[5], bits(fpu_acc(-0.0f, 1, 0, 0)), bits(fpu_ref(-0.0f, 1)));
  for (int k = 0; k < 4; k++)
    vs0[k] = -0.0f;
  vu0_ref(1);
  u32 vw[4];
  for (int k = 0; k < 4; k++)
    vw[k] = bits(vout[k]);
  vu0_acc(1, 0);
  for (int k = 0; k < 4; k++)
    check(&v[3], bits(vout[k]), vw[k]);

  mx_printf("ACC value checks, %d sums of 1-%d random inexact products\n", TRIALS, MAXN);
  for (unsigned i = 0; i < sizeof f / sizeof f[0]; i++)
    report(&f[i]);
  for (unsigned i = 0; i < sizeof v / sizeof v[0]; i++)
    report(&v[i]);
  MX_TEST_END(0);
  return 0;
}
