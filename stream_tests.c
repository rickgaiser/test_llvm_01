#include "stream_tests.h"

const int stream_dists[STREAM_NUM_DISTS] = {0, 64, 128, 256, 512};

// Each loop iteration handles exactly one 64-byte cache line, so there is one
// prefetch per line per stream. The distance is a compile-time constant in
// every instantiation below.
#define PREFETCH(p, dist)                                \
    do {                                                 \
        if (dist)                                        \
            __builtin_prefetch((const char *)(p) + (dist)); \
    } while (0)

// Read-only: sum of all elements (4 accumulators to avoid one long add chain)
static inline __attribute__((always_inline))
int sum_kernel(const int * restrict in, int count, int dist) {
    int s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    for (int i = 0; i < count; i += STREAM_LINE_INTS) {
        PREFETCH(&in[i], dist);
        for (int j = 0; j < STREAM_LINE_INTS; j += 4) {
            s0 += in[i + j + 0];
            s1 += in[i + j + 1];
            s2 += in[i + j + 2];
            s3 += in[i + j + 3];
        }
    }
    return s0 + s1 + s2 + s3;
}

// Read + write: out = in * 3 + 1
static inline __attribute__((always_inline))
void scale_kernel(const int * restrict in, int * restrict out, int count,
                  int dist) {
    for (int i = 0; i < count; i += STREAM_LINE_INTS) {
        PREFETCH(&in[i], dist);
        PREFETCH(&out[i], dist);
        for (int j = 0; j < STREAM_LINE_INTS; j++)
            out[i + j] = in[i + j] * 3 + 1;
    }
}

// Read 2 + write 1: y = a * x + y
static inline __attribute__((always_inline))
void saxpy_kernel(const float * restrict x, float * restrict y, int count,
                  float a, int dist) {
    for (int i = 0; i < count; i += STREAM_LINE_INTS) {
        PREFETCH(&x[i], dist);
        PREFETCH(&y[i], dist);
        for (int j = 0; j < STREAM_LINE_INTS; j++)
            y[i + j] = a * x[i + j] + y[i + j];
    }
}

#define DEFINE_STREAM(dist)                                                   \
    static int sum_pf##dist(const int *in, int count) {                       \
        return sum_kernel(in, count, dist);                                   \
    }                                                                         \
    static void scale_pf##dist(const int *in, int *out, int count) {          \
        scale_kernel(in, out, count, dist);                                   \
    }                                                                         \
    static void saxpy_pf##dist(const float *x, float *y, int count, float a) { \
        saxpy_kernel(x, y, count, a, dist);                                   \
    }

DEFINE_STREAM(0)
DEFINE_STREAM(64)
DEFINE_STREAM(128)
DEFINE_STREAM(256)
DEFINE_STREAM(512)

const stream_sum_fn stream_sum[STREAM_NUM_DISTS] = {
    sum_pf0, sum_pf64, sum_pf128, sum_pf256, sum_pf512};
const stream_scale_fn stream_scale[STREAM_NUM_DISTS] = {
    scale_pf0, scale_pf64, scale_pf128, scale_pf256, scale_pf512};
const stream_saxpy_fn stream_saxpy[STREAM_NUM_DISTS] = {
    saxpy_pf0, saxpy_pf64, saxpy_pf128, saxpy_pf256, saxpy_pf512};
