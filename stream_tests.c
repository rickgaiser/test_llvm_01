#include "stream_tests.h"

// Read-only: sum of all elements (4 accumulators to avoid one long add chain)
int stream_sum(const int * restrict in, int count) {
    int s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    for (int i = 0; i < count; i += STREAM_LINE_INTS) {
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
void stream_scale(const int * restrict in, int * restrict out, int count) {
    for (int i = 0; i < count; i += STREAM_LINE_INTS) {
        for (int j = 0; j < STREAM_LINE_INTS; j++)
            out[i + j] = in[i + j] * 3 + 1;
    }
}

// Read 2 + write 1: y = a * x + y
void stream_saxpy(const float * restrict x, float * restrict y, int count,
                  float a) {
    for (int i = 0; i < count; i += STREAM_LINE_INTS) {
        for (int j = 0; j < STREAM_LINE_INTS; j++)
            y[i + j] = a * x[i + j] + y[i + j];
    }
}
