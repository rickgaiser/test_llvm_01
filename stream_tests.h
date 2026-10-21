#ifndef STREAM_TESTS_H
#define STREAM_TESTS_H

// Memory streaming tests: the arrays are much larger than the 8KB data cache,
// so every cache line comes from main memory.

#define STREAM_INTS (256 * 1024)  // 1MB per array
#define STREAM_LINE_INTS 16       // 64-byte cache line
#define STREAM_PASSES 16

int stream_sum(const int *in, int count);
void stream_scale(const int *in, int *out, int count);
void stream_saxpy(const float *x, float *y, int count, float a);

#endif // STREAM_TESTS_H
