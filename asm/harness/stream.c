// The STREAM kernels of the bench (stream_tests.c): hand-written variants
// (gen_stream.py, asm/gen/stream.py) and feature clang's, on arrays as in the
// bench (1 MB each, 64-byte aligned, one after another). MB/s per variant.
#include <stdio.h>
#include "streamv.h"
#define N (256 * 1024)
#define PASSES 8
static int in_[N] __attribute__((aligned(64)));
static int out_[N] __attribute__((aligned(64)));
static float x_[N] __attribute__((aligned(64)));
static float y_[N] __attribute__((aligned(64)));
static inline unsigned count(void) {
    unsigned c;
    __asm__ __volatile__("mfc0 %0, $9\n sync.p" : "=r"(c));
    return c;
}
int main(void) {
    int ref_sum = 0;
    for (int i = 0; i < N; i++) {
        in_[i] = i & 255;
        x_[i] = (float)(i & 255);
        ref_sum += in_[i];
    }
    int nv = sizeof(variants) / sizeof(variants[0]), bad = 0;
    for (int rep = 0; rep < 2; rep++)
        for (int k = 0; k < nv; k++) {
            int ok = 1, bytes = 4;
            unsigned t0 = 0, t1 = 0;
            if (variants[k].kind == 0) {
                int (*f)(const int *, int) = variants[k].f;
                int s = 0;
                t0 = count();
                for (int p = 0; p < PASSES; p++)
                    s = f(in_, N);
                t1 = count();
                ok = s == ref_sum;
            } else if (variants[k].kind == 1) {
                void (*f)(const int *, int *, int) = variants[k].f;
                for (int i = 0; i < N; i++)
                    out_[i] = 0;
                t0 = count();
                for (int p = 0; p < PASSES; p++)
                    f(in_, out_, N);
                t1 = count();
                for (int i = 0; i < N; i++)
                    ok &= out_[i] == in_[i] * 3 + 1;
                bytes = 8;
            } else {
                void (*f)(const float *, float *, int, float) = variants[k].f;
                for (int i = 0; i < N; i++)
                    y_[i] = 1.0f;
                t0 = count();
                for (int p = 0; p < PASSES; p++)
                    f(x_, y_, N, 0.5f);
                t1 = count();
                for (int i = 0; i < N; i++)
                    ok &= y_[i] == 1.0f + PASSES * 0.5f * x_[i];
                bytes = 12;
            }
            bad += !ok;
            unsigned cyc = t1 - t0;
            unsigned mbs = (unsigned)((unsigned long long)bytes * N * PASSES * 294912 / 1000 / cyc);
            printf("V %d %s %u%s\n", rep, variants[k].name, mbs, ok ? "" : " WRONG");
        }
    printf("@@PS2TEST_END@@ status=%d\n", bad ? 1 : 0);
    return 0;
}
