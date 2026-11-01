#include "stream_tests.h"

// With BENCH_ASM the kernels are those of asm/stream_tests.S.
#ifndef BENCH_ASM

// Read-only: sum of all elements
int stream_sum(const int * restrict in, int count) {
    int s = 0;
    for (int i = 0; i < count; i++)
        s += in[i];
    return s;
}

// Read + write: out = in * 3 + 1
void stream_scale(const int * restrict in, int * restrict out, int count) {
    for (int i = 0; i < count; i++)
        out[i] = in[i] * 3 + 1;
}

// Read 2 + write 1: y = a * x + y
void stream_saxpy(const float * restrict x, float * restrict y, int count,
                  float a) {
    for (int i = 0; i < count; i++)
        y[i] = a * x[i] + y[i];
}

// The prefetches run past the end of the arrays; PREF does not fault.
#define PREF_AHEAD (STREAM_PREF_LINES * STREAM_LINE_INTS)

// The _pref kernels prefetch once per 64-byte line, in a loop per line.
int stream_sum_pref(const int * restrict in, int count) {
    int s = 0;
    for (int i = 0; i < count; i += STREAM_LINE_INTS) {
        __builtin_prefetch(&in[i + PREF_AHEAD], 0);
        for (int j = 0; j < STREAM_LINE_INTS; j++)
            s += in[i + j];
    }
    return s;
}

void stream_scale_pref(const int * restrict in, int * restrict out, int count) {
    for (int i = 0; i < count; i += STREAM_LINE_INTS) {
        __builtin_prefetch(&in[i + PREF_AHEAD], 0);
        __builtin_prefetch(&out[i + PREF_AHEAD], 1);
        for (int j = 0; j < STREAM_LINE_INTS; j++)
            out[i + j] = in[i + j] * 3 + 1;
    }
}

void stream_saxpy_pref(const float * restrict x, float * restrict y, int count,
                       float a) {
    for (int i = 0; i < count; i += STREAM_LINE_INTS) {
        __builtin_prefetch(&x[i + PREF_AHEAD], 0);
        __builtin_prefetch(&y[i + PREF_AHEAD], 1);
        for (int j = 0; j < STREAM_LINE_INTS; j++)
            y[i + j] = a * x[i + j] + y[i + j];
    }
}

#endif // BENCH_ASM
