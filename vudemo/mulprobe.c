/* Is the R5900 multiplier exact for exact products, in both operand orders?

   For pairs (p, q) whose product is exact, compute p * q and q * p:
     FPU:  mul.s fd, fs, ft
     VU0 (macro mode):  vmul.xyzw fd, fs, ft
                        vmula.xyzw ACC, fs, ft, then vmaddw ACC + 0 * 1
   The inputs are bit patterns (no floating point operation makes them), and
   the expected product is computed with integers, so every difference is
   the hardware. Pairs: a power of two times an integer, and the products of
   the polynomial in vudemo for x = 0.5. */
#include <stdint.h>
#include <stdio.h>

typedef float f32x4 __attribute__((vector_size(16)));
typedef union { float f; uint32_t u; } fu;

static uint32_t fpu_mul(uint32_t s, uint32_t t) {
  fu a = {.u = s}, b = {.u = t}, d;
  __asm__ volatile("mul.s %0, %1, %2" : "=f"(d.f) : "f"(a.f), "f"(b.f));
  return d.u;
}

static f32x4 vs __attribute__((aligned(16))), vt __attribute__((aligned(16))),
    vd __attribute__((aligned(16)));

static void vu_set(uint32_t s, uint32_t t) {
  for (int i = 0; i < 4; i++) {
    ((uint32_t *)&vs)[i] = s;
    ((uint32_t *)&vt)[i] = t;
  }
}

static uint32_t vu0_mul(uint32_t s, uint32_t t) {
  vu_set(s, t);
  __asm__ volatile("lqc2 $vf1, 0(%1)\n\tlqc2 $vf2, 0(%2)\n\t"
                   "vmul.xyzw $vf3, $vf1, $vf2\n\tsqc2 $vf3, 0(%0)"
                   : : "r"(&vd), "r"(&vs), "r"(&vt) : "memory");
  return ((uint32_t *)&vd)[0];
}

/* ACC = s * t; d = ACC + vf0.x * vf0.w (= ACC + 0). */
static uint32_t vu0_mula(uint32_t s, uint32_t t) {
  vu_set(s, t);
  __asm__ volatile("lqc2 $vf1, 0(%1)\n\tlqc2 $vf2, 0(%2)\n\t"
                   "vmula.xyzw $ACC, $vf1, $vf2\n\t"
                   "vmaddw.xyzw $vf3, $vf0, $vf0\n\tsqc2 $vf3, 0(%0)"
                   : : "r"(&vd), "r"(&vs), "r"(&vt) : "memory");
  return ((uint32_t *)&vd)[0];
}

/* The float with value m * 2^e, for an integer m with |m| < 2^24. */
static uint32_t make(int64_t m, int e) {
  if (m == 0)
    return 0;
  uint32_t sign = m < 0 ? 0x80000000u : 0;
  uint64_t a = m < 0 ? -m : m;
  while (a >= (1u << 24)) { /* only exact inputs are used */
    a >>= 1;
    e++;
  }
  while (a < (1u << 23)) {
    a <<= 1;
    e--;
  }
  return sign | (uint32_t)(e + 23 + 127) << 23 | (uint32_t)(a & 0x7fffff);
}

static int tests, wrong;

static void check(int64_t pm, int pe, int64_t qm, int qe, const char *what) {
  uint32_t p = make(pm, pe), q = make(qm, qe), want = make(pm * qm, pe + qe);
  uint32_t r[6] = {fpu_mul(p, q), fpu_mul(q, p), vu0_mul(p, q),
                   vu0_mul(q, p), vu0_mula(p, q), vu0_mula(q, p)};
  static const char *form[6] = {"mul.s p,q", "mul.s q,p", "vmul p,q",
                                "vmul q,p", "vmula p,q", "vmula q,p"};
  tests++;
  for (int k = 0; k < 6; k++)
    if (r[k] != want) {
      if (wrong < 60)
        printf("%-9s %s p=%08x q=%08x: got %08x want %08x\n", form[k], what, p, q,
               r[k], want);
      wrong++;
    }
}

int main(void) {
  static const int ints[] = {1, 3, 5, 7, 9, 13, 17, 68, 4097, 0x7fffff, 0xffffff};
  char what[32];
  for (int e = -16; e <= 4; e++)
    for (unsigned j = 0; j < sizeof(ints) / sizeof(ints[0]); j++)
      for (int sign = 1; sign >= -1; sign -= 2) {
        snprintf(what, sizeof(what), "2^%d*%d", e, sign * ints[j]);
        check(1, e, sign * ints[j], 0, what);
      }
  /* The polynomial for x = 0.5: c * 2^-k (c in -4..4) and y * 2^-4 for the
     values y the 4-degree steps produce (multiples of 2^-12 up to 68). */
  for (int c = -4; c <= 4; c++)
    for (int k = 1; k <= 4; k++) {
      snprintf(what, sizeof(what), "c=%d*2^-%d", c, k);
      check(c, 0, 1, -k, what);
    }
  for (int m = -68 * 4096; m <= 68 * 4096; m += 4099) {
    snprintf(what, sizeof(what), "y=%d*2^-12*2^-4", m);
    check(m, -12, 1, -4, what);
  }
  printf("mulprobe: %d products x 6 forms, %d not exact\n", tests, wrong);
  printf("@@PS2TEST_END@@ status=%d\n", wrong != 0);
  return 0;
}
