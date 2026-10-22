// Integer vector shuffles of the kinds that the R5900 has MMI instructions
// for: the even and odd elements of two vectors (also of one), the
// interleaved low and high halves, the low and high halves, and word
// permutations of one vector, also of floats. Also splats
// of a register, extensions to wider elements and 16x16->32 multiplies.
#include <stdio.h>
#include "app_tests.h"
#include "shuffle_tests.h"

typedef unsigned int v4w __attribute__((vector_size(16)));
typedef unsigned short v8h __attribute__((vector_size(16)));
typedef unsigned char v16b __attribute__((vector_size(16)));
typedef short v8s __attribute__((vector_size(16)));
typedef signed char v16sb __attribute__((vector_size(16)));
typedef int v8i __attribute__((vector_size(32)));
typedef unsigned short v16uh __attribute__((vector_size(32)));
typedef int v16i __attribute__((vector_size(64)));

static unsigned char in_a[16] __attribute__((aligned(16)));
static unsigned char in_b[16] __attribute__((aligned(16)));
static unsigned char res[8][16] __attribute__((aligned(16)));

static void __attribute__((noinline)) shuffle_v4w(void) {
    v4w a = *(v4w *)in_a, b = *(v4w *)in_b;
    *(v4w *)res[0] = __builtin_shufflevector(a, b, 0, 2, 4, 6);
    *(v4w *)res[1] = __builtin_shufflevector(a, b, 1, 3, 5, 7);
    *(v4w *)res[2] = __builtin_shufflevector(a, b, 0, 4, 1, 5);
    *(v4w *)res[3] = __builtin_shufflevector(a, b, 2, 6, 3, 7);
    *(v4w *)res[4] = __builtin_shufflevector(a, b, 0, 1, 4, 5);
    *(v4w *)res[5] = __builtin_shufflevector(a, b, 2, 3, 6, 7);
    *(v4w *)res[6] = __builtin_shufflevector(a, b, 0, 2, 0, 2);
    *(v4w *)res[7] = __builtin_shufflevector(a, b, 1, 3, 1, 3);
}

static void __attribute__((noinline)) shuffle_v8h(void) {
    v8h a = *(v8h *)in_a, b = *(v8h *)in_b;
    *(v8h *)res[0] = __builtin_shufflevector(a, b, 0, 2, 4, 6, 8, 10, 12, 14);
    *(v8h *)res[1] = __builtin_shufflevector(a, b, 1, 3, 5, 7, 9, 11, 13, 15);
    *(v8h *)res[2] = __builtin_shufflevector(a, b, 0, 8, 1, 9, 2, 10, 3, 11);
    *(v8h *)res[3] = __builtin_shufflevector(a, b, 4, 12, 5, 13, 6, 14, 7, 15);
    *(v8h *)res[4] = __builtin_shufflevector(a, b, 0, 1, 2, 3, 8, 9, 10, 11);
    *(v8h *)res[5] = __builtin_shufflevector(a, b, 4, 5, 6, 7, 12, 13, 14, 15);
    *(v8h *)res[6] = __builtin_shufflevector(a, b, 0, 2, 4, 6, 0, 2, 4, 6);
    *(v8h *)res[7] = __builtin_shufflevector(a, b, 1, 3, 5, 7, 1, 3, 5, 7);
}

static void __attribute__((noinline)) shuffle_v16b(void) {
    v16b a = *(v16b *)in_a, b = *(v16b *)in_b;
    *(v16b *)res[0] = __builtin_shufflevector(a, b, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30);
    *(v16b *)res[1] = __builtin_shufflevector(a, b, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31);
    *(v16b *)res[2] = __builtin_shufflevector(a, b, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23);
    *(v16b *)res[3] = __builtin_shufflevector(a, b, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31);
    *(v16b *)res[4] = __builtin_shufflevector(a, b, 0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23);
    *(v16b *)res[5] = __builtin_shufflevector(a, b, 8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31);
    *(v16b *)res[6] = __builtin_shufflevector(a, b, 0, 2, 4, 6, 8, 10, 12, 14, 0, 2, 4, 6, 8, 10, 12, 14);
    *(v16b *)res[7] = __builtin_shufflevector(a, b, 1, 3, 5, 7, 9, 11, 13, 15, 1, 3, 5, 7, 9, 11, 13, 15);
}

typedef float v4f __attribute__((vector_size(16)));

// Word permutations of one vector (PEXEW, PEXCW, PROT3W, the halves
// swapped), and float shuffles, which are MMI shuffles of the bits with VU0
static void __attribute__((noinline)) shuffle_perm(void) {
    v4w a = *(v4w *)in_a;
    *(v4w *)res[0] = __builtin_shufflevector(a, a, 2, 1, 0, 3);
    *(v4w *)res[1] = __builtin_shufflevector(a, a, 0, 2, 1, 3);
    *(v4w *)res[2] = __builtin_shufflevector(a, a, 1, 2, 0, 3);
    *(v4w *)res[3] = __builtin_shufflevector(a, a, 2, 3, 0, 1);
    v4f fa = *(v4f *)in_a, fb = *(v4f *)in_b;
    *(v4f *)res[4] = __builtin_shufflevector(fa, fb, 0, 4, 1, 5);
    *(v4f *)res[5] = __builtin_shufflevector(fa, fb, 1, 3, 5, 7);
    *(v4f *)res[6] = __builtin_shufflevector(fa, fb, 1, 2, 0, 3);
    *(v4f *)res[7] = __builtin_shufflevector(fa, fb, 2, 3, 6, 7);
}

static volatile unsigned scalar = 0x89abcdefu;
static int wide[64] __attribute__((aligned(64)));

static void __attribute__((noinline)) widen_checks(void) {
    unsigned x = scalar;
    *(v4w *)res[0] = (v4w){x, x, x, x};
    *(v8h *)res[1] = (v8h){x, x, x, x, x, x, x, x};
    *(v16b *)res[2] = (v16b){x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x};
    v8s a = *(v8s *)in_a, b = *(v8s *)in_b;
    v8i sa = __builtin_convertvector(a, v8i), sb = __builtin_convertvector(b, v8i);
    *(v8i *)&wide[0] = sa * sb;
    *(v8i *)&wide[8] = sa * (short)x;
    *(v16uh *)&wide[16] = __builtin_convertvector(*(v16b *)in_b, v16uh);
    *(v16i *)&wide[32] = __builtin_convertvector(*(v16sb *)in_b, v16i);
    *(v16i *)&wide[48] = __builtin_convertvector(*(v16b *)in_b, v16i);
}

void shuffle_checks(void) {
    for (int i = 0; i < 16; i++) {
        in_a[i] = 0x10 + i;
        in_b[i] = 0xa0 + i;
    }
    printf("--- Integer vector shuffles ---\n");
    shuffle_v4w();
    printf("Result: words 0x%08x\n", (unsigned)app_hash(res, sizeof(res)));
    shuffle_v8h();
    printf("Result: halfwords 0x%08x\n", (unsigned)app_hash(res, sizeof(res)));
    shuffle_v16b();
    printf("Result: bytes 0x%08x\n", (unsigned)app_hash(res, sizeof(res)));
    shuffle_perm();
    printf("Result: permutations 0x%08x\n", (unsigned)app_hash(res, sizeof(res)));
    widen_checks();
    printf("Result: splats 0x%08x, extend and multiply 0x%08x\n\n",
           (unsigned)app_hash(res, 48), (unsigned)app_hash(wide, sizeof(wide)));
}
